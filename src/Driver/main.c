
#include <ntifs.h>
#include <ntddk.h>
#include <wdm.h>
#include <ntimage.h>
#include <windef.h>
#include <ntstrsafe.h>
#include "define.h"
#include "util.h"
#include "undocumented.h"
#include "listentry.h"
#include "engine.h"
#include "hidedriver.h"


PDRIVER_OBJECT g_pDriverObject;


#define DEVICE_NAME	 L"\\Device\\WKernelV"
#define DEVICE_LINK	 L"\\DosDevices\\KernelV"

#define IOCTL_KERNELV_LOAD_DRIVER		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS) 
#define IOCTL_KERNELV_HIDE_MYSELF		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS) 
#define IOCTL_KERNELV_HIDE_DRIVER		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS) 
#define IOCTL_KERNELV_SCAN_PATTERN		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS) 



VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	kprintf(("DriverUnload()\n"));
	FreeDriverListEntry(0);
	FreeDriverListEntry(1);
	FreeDriverListEntry(2);
	FreeDriverListEntry(3);
	FreeDetectListEntry();

	UNICODE_STRING DeviceLink = RTL_CONSTANT_STRING(DEVICE_LINK);
//	RtlInitUnicodeString(&DeviceLink, DEVICE_LINK);
	IoDeleteSymbolicLink(&DeviceLink);
	IoDeleteDevice(pDriverObject->DeviceObject);

}


NTSTATUS CreateCloseDispatch(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{

	UNREFERENCED_PARAMETER(pDeviceObject);
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

UINT IOCTLScan(PBYTE pUserInput, ULONG InputBufferSize)
{
	int nPatternCnt = 0;
	BYTE pStart[] = { 0x53, 0x54, 0x61, 0x72, 0x74 };
	BYTE pEnd[] = { 0x65, 0x6E, 0x44 };
	int nRemainSize = InputBufferSize;

	PBYTE pPattern = pUserInput;

	kprintf("%x %x %x %x %x %x %x %x", pPattern[0], pPattern[1], pPattern[2], pPattern[3], pPattern[4], pPattern[5], pPattern[6], pPattern[7]);
	
	while (TRUE)
	{
		BYTE Pattern[512] = { 0, };

		PBYTE pPosStart = MemSearch(pPattern, nRemainSize, pStart, 5);
		PBYTE pPosEnd = MemSearch(pPattern, nRemainSize, pEnd, 3);
		if (pPosStart == NULL || pPosEnd == NULL)
		{
			kprintf("fuck.."); // 발생하면 안됨
			break;
		}

		nPatternCnt++;
		uintptr_t PatternSize = (uintptr_t)pPosEnd - ((uintptr_t)pPosStart + 5);
		RtlCopyMemory(Pattern, pPosStart + 5, PatternSize);
		kprintf("%x %x %x %x size : %lld", Pattern[0], Pattern[1], Pattern[2], Pattern[3], PatternSize);
		ScanKernelDriver(nPatternCnt, Pattern, (UINT)PatternSize);
		uintptr_t gap = (uintptr_t)pPosEnd + 3 - (uintptr_t)pPosStart;
		nRemainSize = nRemainSize - (int)gap;
		kprintf("nRemainSize : %d", nRemainSize);
		if (nRemainSize == 0)
		{
			kprintf("There are no more buffers left.");
			break;
		}
		pPattern = (PBYTE)pPosEnd + 3;
	}

	return nPatternCnt;
}

NTSTATUS IoControlDispatch(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDeviceObject);
	PIO_STACK_LOCATION pIoStackIrp;
	pIrp->IoStatus.Information = 0;
	pIoStackIrp = IoGetCurrentIrpStackLocation(pIrp);
	ULONG InputBufferSize = 0;
	ULONG OutputBufferSize = 0;

	InputBufferSize = pIoStackIrp->Parameters.DeviceIoControl.InputBufferLength; // ring3에서 인자로 가져옴
	OutputBufferSize = pIoStackIrp->Parameters.DeviceIoControl.OutputBufferLength; // ring3에 리턴할 예정
	kprintf("InputBufferSize : %lu / OutputBufferSize: %lu", InputBufferSize, OutputBufferSize);

	if (IOCTL_KERNELV_SCAN_PATTERN == pIoStackIrp->Parameters.DeviceIoControl.IoControlCode)
	{
		__try
		{
			kprintf("===IOCTL_KERNELV_SCAN_PATTERN===");
			int nPatternCnt = 0;
			PBYTE pRecvBuffer = pIrp->AssociatedIrp.SystemBuffer;

			UINT NeedSize = 0;		
						
			// output길이가 궁금한 경우		
			if (OutputBufferSize == 0)
			{
				InitDetectListEntry();
				nPatternCnt = IOCTLScan(pRecvBuffer, InputBufferSize);
				if (nPatternCnt == 0) // 발생하면 안됨
				{
					kprintf("No pattern was found.");
					pIrp->IoStatus.Information = 0;
					pIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;
					FreeDetectListEntry();
					goto EXIT;
				}
				NeedSize = sizeof(DETECT_LIST_ENTRY) * GetDetectListEntryCount();
				kprintf("OutputBuffer is too small.");
				kprintf("need Size : %d", NeedSize);
				pIrp->IoStatus.Information = NeedSize; // need size
				pIrp->IoStatus.Status = STATUS_SUCCESS; // success로 하지 않으면 NeedSize가 리턴되지 않는다.

				goto EXIT;
			}
			/*
				2번째 IOCTL_KERNELV_SCAN_PATTERN 요청 처리
			*/
			// 버퍼에 복사
			PDETECT_LIST_ENTRY pSendMem = pIrp->AssociatedIrp.SystemBuffer;
			// user mode에서 넘어온 포인터	
			if (!MmIsAddressValid(pSendMem))
			{
				kprintf("input buffer is wrong");
				pIrp->IoStatus.Information = 0;
				pIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;
				goto EXIT;
			}

			PDETECT_LIST_ENTRY pDetectListEntry = NULL;
			int nIndex = 0;

			while (TRUE)
			{
				GetDetectListEntry(nIndex, &pDetectListEntry);
				if (NULL == pDetectListEntry)
				{
					break;
				}
				kprintf("detect : %llx", pDetectListEntry->BaseAddress);
				RtlCopyMemory(&pSendMem[nIndex], pDetectListEntry, sizeof(DETECT_LIST_ENTRY));
				nIndex++;
			}
			if (nIndex > 0)
			{
				kprintf("sent %d", sizeof(DETECT_LIST_ENTRY) * nIndex);
				pIrp->IoStatus.Information = sizeof(DETECT_LIST_ENTRY) * nIndex;
			}

			FreeDetectListEntry();
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{			
			kprintf("Exception caught in IOCTL_KERNELV_SCAN_PATTERN");
		}

	}
	if (IOCTL_KERNELV_HIDE_DRIVER == pIoStackIrp->Parameters.DeviceIoControl.IoControlCode)
	{	
		kprintf("===IOCTL_KERNELV_HIDE_DRIVER===");
		//	L"%s::%s" 형태
		LPWSTR pReceiveString = pIrp->AssociatedIrp.SystemBuffer;
		kprintf("pDriverPath : %ws InputBufferSize : %d", pReceiveString, InputBufferSize);		
		LPWSTR pPos = wcsstr(pReceiveString, L"::");
		LPWSTR pServiceName = pPos + 2;
		RtlCopyMemory(pPos, "\x00\x00", 2);
		LPWSTR pDriverPath = pReceiveString;
		kprintf("parse : %ws %ws", pDriverPath, pServiceName);
		BOOLEAN bHide = FALSE;		
		bHide = HideDriver(pDriverPath, pServiceName);
		if (TRUE == bHide)
		{
			RtlStringCchCopyW(pDriverPath, InputBufferSize, L"YES");
		}
		else
		{
			RtlStringCchCopyW(pDriverPath, InputBufferSize, L"NO");
		}
		pIrp->IoStatus.Information = OutputBufferSize;

	}
	if (IOCTL_KERNELV_HIDE_MYSELF == pIoStackIrp->Parameters.DeviceIoControl.IoControlCode)
	{
		kprintf("===IOCTL_KERNELV_HIDE_MYSELF===");
		HideMyself();
		pIrp->IoStatus.Information = InputBufferSize;
	}
	if (IOCTL_KERNELV_LOAD_DRIVER == pIoStackIrp->Parameters.DeviceIoControl.IoControlCode)
	{
		kprintf("===IOCTL_KERNELV_LOAD_DRIVER===");
		
		PDRIVER_LIST_ENTRY pSendMem = pIrp->AssociatedIrp.SystemBuffer;
		
		UINT NeedSize = 0;

		// output길이가 궁금한 경우		
		if (OutputBufferSize == 0)
		{
			EnumerateDrivers();
			NeedSize = sizeof(DRIVER_LIST_ENTRY) *  GetDriverListEntryCount(0);
			kprintf("OutputBuffer is too small.");
			kprintf("need Size : %d", NeedSize);
			pIrp->IoStatus.Information = NeedSize; // need size
			pIrp->IoStatus.Status = STATUS_SUCCESS; // success로 하지 않으면 NeedSize가 리턴되지 않는다.
			
			goto EXIT;
		}
		
		/*
			2번째 IOCTL_KERNELV_LOAD_DRIVER 요청 처리
		*/

		// user mode에서 넘어온 포인터	
		if (!MmIsAddressValid(pSendMem))
		{
			kprintf("input buffer is wrong");
			pIrp->IoStatus.Information = 0;
			pIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;
			goto EXIT;
		}
		
		// output 버퍼에 복사
		PDRIVER_LIST_ENTRY pDriverListEntry = NULL;
		int nIndex = 0;
		while (TRUE)
		{
			GetDriverListEntry(0, nIndex, &pDriverListEntry);
			if (NULL == pDriverListEntry)
			{
				break;
			}
			RtlCopyMemory(&pSendMem[nIndex], pDriverListEntry, sizeof(DRIVER_LIST_ENTRY));
			nIndex++;
			NeedSize += sizeof(DRIVER_LIST_ENTRY);
		}
				
		if (nIndex >= 0)
		{
			pIrp->IoStatus.Information = NeedSize;

		}	

	}

	pIrp->IoStatus.Status = STATUS_SUCCESS;
EXIT:
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

BOOLEAN InitComm(PDRIVER_OBJECT pDriverObject)
{
	BOOLEAN bRet = FALSE;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(DEVICE_NAME);
	UNICODE_STRING DeviceLink = RTL_CONSTANT_STRING(DEVICE_LINK);
	PDEVICE_OBJECT pDeviceObject = NULL;
		
	__try
	{

		status = IoCreateDevice(pDriverObject, 0, &DeviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &pDeviceObject);

		if (!NT_SUCCESS(status))
		{
			kprintf("IoCreateDevice occurs an error. %x\n", status);
			bRet = FALSE;
			goto EXIT;

		}
		status = IoCreateSymbolicLink(&DeviceLink, &DeviceName);
		if (!NT_SUCCESS(status))
		{
			kprintf("IoCreateSymbolicLink occurs an error. %x\n", status);
			if (pDeviceObject)
			{
				IoDeleteDevice(pDeviceObject);
				pDeviceObject = NULL;
			}
			bRet = FALSE;
			goto EXIT;
		}

		pDeviceObject->Flags |= DO_BUFFERED_IO;
		pDriverObject->MajorFunction[IRP_MJ_CREATE] = CreateCloseDispatch;
		pDriverObject->MajorFunction[IRP_MJ_CLOSE] = CreateCloseDispatch;
		pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IoControlDispatch;
		pDriverObject->DriverUnload = DriverUnload;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{		
		bRet = FALSE;
		kprintf("Exception caught in InitComm()");
	}
	bRet = TRUE;
EXIT:

	return bRet;
}


void Test()
{	
//	ScanKernelDriver();
//	EnumerateEPROCESS("notepad.exe");

}
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	UNREFERENCED_PARAMETER(pRegistryPath);
	g_pDriverObject = pDriverObject;
	kprintf("===== KernelV start =====");
	if (FALSE == InitComm(pDriverObject))
	{
		return STATUS_SUCCESS;
	}
	kprintf("pDriverObject %p", pDriverObject);
	GetPsLoadedModuleList(); // init;


	return STATUS_SUCCESS;
}
