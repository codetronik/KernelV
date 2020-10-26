#include <ntifs.h>
#include <ntddk.h>
#include <wdm.h>
#include <ntimage.h>
#include <windef.h>
#include <ntstrsafe.h>
#include "engine.h"
#include "define.h"
#include "undocumented.h"
#include "listentry.h"

// Build 19041
#define EPROCESS_ACTIVE_PROCESS_LINKS	0x448 
// Build 18362
//#define EPROCESS_ACTIVE_PROCESS_LINKS	0x2F0 

#define EPROCESS_IMAGE_FILE_NAME		0x5A8		
#define EPROCESS_UNIQUE_PROCESS_ID		0x440 // PVOID64


extern PDRIVER_OBJECT g_pDriverObject;
extern POBJECT_TYPE* IoDriverObjectType;
PLDR_DATA_TABLE_ENTRY PsLoadedModuleList;

VOID EnumerateEPROCESS(PUCHAR pTargetFileName)
{
	// system(pid:4)의 EPROCESS 를 반환함
	PEPROCESS pCurrentProcess = PsInitialSystemProcess; /* PsGetCurrentProcess() 와 같음 */
	PLIST_ENTRY pCurrentListEntry = NULL;

	while (TRUE)
	{
		// next EPROCESS
		pCurrentListEntry = (PLIST_ENTRY)((uintptr_t)pCurrentProcess + EPROCESS_ACTIVE_PROCESS_LINKS);
		PEPROCESS pNextProcess = (PEPROCESS)((uintptr_t)pCurrentListEntry->Flink - EPROCESS_ACTIVE_PROCESS_LINKS);

		/* 구조체에서 구하기
		size_t dwPid = *((uintptr_t*)((uintptr_t)pCurrentProcess + EPROCESS_UNIQUE_PROCESS_ID));
		char* pImageFileName = (char*)((uintptr_t)pCurrentProcess + EPROCESS_IMAGE_FILE_NAME);
		*/

		// build 버전마다 오프셋이 다르기때문에 가급적 api를 사용
		size_t dwPid = (size_t)PsGetProcessId(pCurrentProcess);
		dwPid = 0;
		UCHAR* pImageFileName = PsGetProcessImageFileName(pCurrentProcess); // 제대로 못가져옴.

		kprintf("%p %s %lld", pCurrentProcess, PsGetProcessImageFileName(pCurrentProcess), dwPid);

		// process 숨기기
		// 수 분 이내로 patch guard에 의한 BSOD 발생 (CRITICAL STRUCTURE CORRUPTION)
		// 참조 : https://www.sysnet.pe.kr/2/0/12110
		if (0 == strcmp((char*)pTargetFileName, (char*)pImageFileName))
		{
			kprintf("hide process\n");
			PLIST_ENTRY pPrevListEntry = pCurrentListEntry->Blink;
			PLIST_ENTRY pNextListEntry = pCurrentListEntry->Flink;
			pPrevListEntry->Flink = pCurrentListEntry->Flink;  // 이전 노드의 다음을 내 다음으로 바꿈
			pNextListEntry->Blink = pCurrentListEntry->Blink; // 다음 노드의 이전을 내 이전로 바꿈 

			pCurrentListEntry->Flink = pCurrentListEntry;
			pCurrentListEntry->Blink = pCurrentListEntry;
		}

		/* ring3 프로세스인지 검사하는 루틴 필요
		if (PsGetProcessPeb(pCurrentProcess))
		{
			PPEB ppp = PsGetProcessPeb(pCurrentProcess);

			kprintf("have peb\n");
		}
		*/

		// 원형 구조라 break하지 않으면 무한으로 돌아감
		if (pNextProcess == PsInitialSystemProcess)
		{
			break;
		}
		pCurrentProcess = pNextProcess;
	}

}

VOID ScanMemory(UINT PatternNo, uintptr_t StartAddr, uintptr_t EndAddr, PBYTE Pattern, UINT PatternSize)
{

//	kprintf("pattern size : %d %llx %llx", PatternSize, StartAddr , EndAddr);

	if (Pattern == NULL || PatternSize < 4)
	{
		kprintf("pattern is null or size is too small.");
		return;
	}
	__try
	{
		// page 사이즈 단위로 검색
		for (uintptr_t Addr = StartAddr; Addr < EndAddr; Addr = Addr + PAGE_SIZE)
		{
			// 특정 section은 메모리 접근이 불가능하기 때문에 skip함
			BOOLEAN bValid = MmIsAddressValid((PVOID)Addr);
			if (bValid == FALSE)
			{
				//	kprintf("skipAddr %p \n", Addr);
				continue;
			}

			for (int i = 0; i <= PAGE_SIZE - (int)PatternSize; i++)
			{
				size_t j = 0;

				for (j = 0; j < PatternSize; j++)
				{
					PUCHAR pAddr = (PUCHAR)Addr;
					if (pAddr[i + j] != Pattern[j])
					{
						// 일치하지 않으면 j의 증가를 멈춤
						break;
					}
				}
				// 패턴 검출
				if (j == PatternSize)
				{
					DETECT_LIST_ENTRY DetectListEntry = { 0, };
					DetectListEntry.BaseAddress = StartAddr;		
					uintptr_t Offset = Addr + i - StartAddr;
					DetectListEntry.Offset = (UINT)Offset;
					DetectListEntry.PatternNo = PatternNo;
					AddDetectListEntry(&DetectListEntry);
					kprintf("byte pattern detect : %llx %llx\n", Addr + i, Offset);
				}
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{		
		kprintf("Exception caught in ScanMemory()");
	}
}


VOID EnumerateDriversViaPsLoadedModuleList(int type, uintptr_t* OutStartAddr, uintptr_t* OutEndAddr)
{
	kprintf("===EnumerateDriversViaPsLoadedModuleList (%d)===", type);
	if (!MmIsAddressValid(PsLoadedModuleList))
	{
		return;
	}
	PLDR_DATA_TABLE_ENTRY Entry = (PLDR_DATA_TABLE_ENTRY)PsLoadedModuleList->InLoadOrderLinks.Flink;
	
	int modCount = 0;
	uintptr_t EntryArray[500] = { 0, };

	while (Entry != PsLoadedModuleList)
	{		
		kprintf("[%d] %p %ws %p %llx ", modCount, Entry, Entry->FullDllName.Buffer, Entry->DllBase, Entry->SizeOfImage);	
		EntryArray[modCount] = (uintptr_t)Entry;
		DRIVER_LIST_ENTRY DriverListEntry = { 0, };
		DriverListEntry.DriverObject = 0;
		DriverListEntry.modBaseAddress = (uintptr_t)Entry->DllBase;
		DriverListEntry.modSize = (ULONG)Entry->SizeOfImage;
		RtlStringCchCopyW(DriverListEntry.ServiceName, MAX_PATH, L"");
		RtlStringCchCopyW(DriverListEntry.FilePath, MAX_PATH, (WCHAR*)Entry->FullDllName.Buffer);
		DriverListEntry.isHidden = FALSE;
		AddDriverListEntry(type, &DriverListEntry);

		Entry = (PLDR_DATA_TABLE_ENTRY)Entry->InLoadOrderLinks.Flink;
		modCount++;
	}
	// 정렬
	uintptr_t temp = 0;
	for (int i = 0; i < modCount - 1; i++) 
	{
		for (int j = i + 1; j < modCount; j++)
		{
			if (EntryArray[i] < EntryArray[j]) 
			{
				temp = EntryArray[j];
				EntryArray[j] = EntryArray[i];
				EntryArray[i] = temp;
			}
		}
	}
	kprintf("min max %p %p", EntryArray[modCount - 1], EntryArray[0]);
	
	// PAGE_SIZE 단위로 재조정
	// 숨겨진 entry를 찾기 위해 범위를 늘린다.
	// 만약 시작 위치가 0xFFFFDC8A51E50C40라면
	// 0x1000000을 align해서 0xffffdc8a51000000로 만든다.
	*OutStartAddr = (EntryArray[modCount-1] & ~(0x1000000 - 1));
	*OutEndAddr = (EntryArray[0] & ~(0x1000000 - 1))+ 0x1000000;
	kprintf("min max %p %p", *OutStartAddr, *OutEndAddr);

}

/* 약점 : 메모리 풀스캔이 아니라 LDR 엔트리의 가장 작은 주소 ~ 가장 큰 주소 사이를 검색하기 때문에
	가장 작은 주소나 가장 큰 주소를 숨기면 찾을 수 없게 된다.
	
	숨겨진 모듈만 찾는다.
*/
VOID EnumerateDriversViaMmLd(int type, uintptr_t StartAddr, uintptr_t EndAddr)
{
	kprintf("===EnumerateDriversViaMmLd (%d)===", type);
	InitDetectListEntry();

	// "MmLd"
	BYTE Pattern[4] = { 0x4d, 0x6d, 0x4c, 0x64 };
	ScanMemory(0x1938, StartAddr, EndAddr, Pattern, 4); // 0x1938은 로깅을 위해서.. 0x0으로 해도 무방
	
	UINT nIndex = 0;
//	int entryCnt = 0;
	
	while (TRUE)
	{
		PDETECT_LIST_ENTRY pDetectListEntry;
		GetDetectListEntry(nIndex, &pDetectListEntry);
		if (NULL == pDetectListEntry)
		{
			break;
		}
		
		PLDR_DATA_TABLE_ENTRY pEntry = (PLDR_DATA_TABLE_ENTRY)((uintptr_t)pDetectListEntry->BaseAddress + pDetectListEntry->Offset + 0xC);
//		kprintf("PLDR_DATA_TABLE_ENTRY %p", pEntry);
		__try
		{		
			/*
				숨겨진 모듈만 찾는다.
			*/
			if (pEntry->InLoadOrderLinks.Blink == (PLIST_ENTRY)pEntry // chain이 끊어진 경우
				&& pEntry->InLoadOrderLinks.Flink == (PLIST_ENTRY)pEntry // chain이 끊어진 경우
				&& pEntry->InInitializationOrderLinks.Flink == NULL 	// 유효한 모듈은 Blink는 값이 일부 있지만 Flink는 죄다 null임
				&& pEntry->ObsoleteLoadCount != 0) // 언로드된 모듈은 ObsoleteLoadCount가 0
			{
				DRIVER_LIST_ENTRY DriverListEntry = { 0, };
				DriverListEntry.DriverObject = 0;
				DriverListEntry.modBaseAddress = (uintptr_t)pEntry->DllBase;
				DriverListEntry.modSize = (ULONG)pEntry->SizeOfImage;
				RtlStringCchCopyW(DriverListEntry.ServiceName, MAX_PATH, L"");
				RtlStringCchCopyW(DriverListEntry.FilePath, MAX_PATH, (WCHAR*)pEntry->FullDllName.Buffer);
				DriverListEntry.isHidden = FALSE;
				AddDriverListEntry(type, &DriverListEntry);
		//		kprintf("[%d] %ws", entryCnt++, DriverListEntry.FilePath);
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{			
			kprintf("Exception caught in EnumerateDriversViaMmLd()");
		}
//	NEXT:
		nIndex++;
	}
	
	FreeDetectListEntry();
}

BOOLEAN ScanKernelDriver(UINT PatternNo, PBYTE Pattern, UINT PatternSize)
{
	kprintf("----- ScanKernelDriver() %d-----", PatternSize);

	int modCount = 0;
	
	PDRIVER_LIST_ENTRY pDriverListEntry = NULL;
		
	__try
	{
		while (TRUE)
		{
			GetDriverListEntry(0, modCount, &pDriverListEntry);
			if (NULL == pDriverListEntry)
			{
				break;
			}
		//	kprintf("[%d] %p %ws %llx %x ", modCount, pDriverListEntry, pDriverListEntry->FilePath, pDriverListEntry->modBaseAddress, pDriverListEntry->modSize);
			BOOLEAN bSuccess = MmIsAddressValid((PVOID)pDriverListEntry->modBaseAddress);
			if (bSuccess == FALSE)
			{
				kprintf("cannot access memory\n");
				goto NEXT;
			}
			ScanMemory(PatternNo, pDriverListEntry->modBaseAddress, pDriverListEntry->modBaseAddress + pDriverListEntry->modSize, Pattern, PatternSize);
		NEXT:
			modCount++;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{	
		kprintf("Exception caught in ScanKernelDriver()");
	}
	if (0 >= modCount)
	{
		kprintf("first init");
		goto EXIT_ERROR;
	}

	
	UINT nIndex = 0;	
	
	while (TRUE)
	{
		PDETECT_LIST_ENTRY pDetectListEntry;
		GetDetectListEntry(nIndex, &pDetectListEntry);
		if (NULL == pDetectListEntry)
		{
			break;
		}
		kprintf("[%d] %llx %llx ", nIndex, pDetectListEntry->Offset , pDetectListEntry->BaseAddress);

		nIndex++;
	}
	
EXIT_ERROR:

	return TRUE;
}

BOOLEAN ScanKernelDriverViaAPI()
{
	NTSTATUS ntSuccess = STATUS_SUCCESS;
	BOOLEAN bSuccess = FALSE;

	// 정확한 사이즈를 얻기 위해 사용
	ULONG neededSize = 0;
	ZwQuerySystemInformation(SystemModuleInformation, &neededSize, 0, &neededSize);

	PSYSTEM_MODULE_INFORMATION pModuleList = (PSYSTEM_MODULE_INFORMATION)ExAllocatePoolWithTag(PagedPool, neededSize, POOL_TAG);
	if (!pModuleList)
	{
		kprintf("error.1\n");
		goto EXIT_ERROR;
	}
	// 커널 드라이버 리스트 버퍼 획득
	ntSuccess = ZwQuerySystemInformation(SystemModuleInformation, pModuleList, neededSize, 0);
	if (!NT_SUCCESS(ntSuccess))
	{
		kprintf("error.2\n");
		goto EXIT_ERROR;
	}

	uintptr_t modBaseAddress = 0;
	ULONG modSize = 0;
	kprintf("mod Count : %d", (int)pModuleList->ulModuleCount);
	// 커널 드라이버 열거
	for (int i = 0; i < (int)pModuleList->ulModuleCount; i++)
	{
		SYSTEM_MODULE mod = pModuleList->Modules[i];

		modBaseAddress = (uintptr_t)(pModuleList->Modules[i].Base);
		modSize = (ULONG)(pModuleList->Modules[i].Size);
		kprintf("Path : %s Base : %llx Size : %x\n", mod.ImageName, modBaseAddress, modSize);

		// system32\drivers\volmgrx.sys 같은 아예 접근이 불가능한 모듈은 걸러냄
		bSuccess = MmIsAddressValid((PVOID)modBaseAddress);
		if (bSuccess == FALSE)
		{
			//			kprintf("Skip.1\n");
			continue;
		}
		
		PIMAGE_NT_HEADERS pHdr = (PIMAGE_NT_HEADERS)RtlImageNtHeader((PVOID)mod.Base);
		if (!pHdr)
		{
			kprintf("Skip.2\n");
			continue;
		}

		// 섹션 체크 (스캔에는 필요하지는 않으나 확인용도)
		PIMAGE_SECTION_HEADER pFirstSection = (PIMAGE_SECTION_HEADER)((uintptr_t)&pHdr->FileHeader + pHdr->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER));
		for (PIMAGE_SECTION_HEADER pSection = pFirstSection; pSection < pFirstSection + pHdr->FileHeader.NumberOfSections; pSection++)
		{
			kprintf("%-10s\t%-8x\t%-6x\n", pSection->Name, pSection->VirtualAddress, pSection->SizeOfRawData);
		}
		

	//	ScanMemory(modBaseAddress, modBaseAddress + modSize);

	}
EXIT_ERROR:
	if (pModuleList) ExFreePoolWithTag(pModuleList, POOL_TAG);

	return TRUE;
}


VOID GetPsLoadedModuleList()
{
	UNICODE_STRING name = RTL_CONSTANT_STRING(L"PsLoadedModuleList");
	PsLoadedModuleList = MmGetSystemRoutineAddress(&name);

}

/*
* 오브젝트 이름을 enumerate하고 해당 이름으로 DriverObject를 구한다. 
* 
* 참조 사이트
* https://github.com/swatkat/arkitlib/blob/master/ARKitDrv/Drv.c 
* https://github.com/ApexLegendsUC/anti-cheat-emulator/blob/master/Source.cpp
* 
*/
VOID EnumerateDriversViaObjectNameInside(int type, IN PUNICODE_STRING pObjectName)
{
	kprintf("===EnumerateDriversViaObjectNameInside (%wZ)===", pObjectName);
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	OBJECT_ATTRIBUTES objectAttributes;
	HANDLE hDirectoryHandle = NULL;
	ULONG Context = 0;
	ULONG ReturnedLength;
	POBJECT_DIRECTORY_INFORMATION pObjectDirectoryInformation = NULL;
		
	InitializeObjectAttributes(&objectAttributes, pObjectName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	status = ZwOpenDirectoryObject(&hDirectoryHandle, DIRECTORY_QUERY /*GENERIC_READ | SYNCHRONIZE*/, &objectAttributes);
	if (!NT_SUCCESS(status))
	{
		kprintf("ZwOpenDirectoryObject occurs an error. %x\n", status);
		return;
	}

	// 정확한 구조체 사이즈를 모르니까 넉넉하게 페이지 크기만큼 할당한다.
	pObjectDirectoryInformation = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, POOL_TAG);

	RtlZeroMemory(pObjectDirectoryInformation, PAGE_SIZE);
	//int count = 0;
	while(TRUE)
	{			
		status = ZwQueryDirectoryObject(hDirectoryHandle, pObjectDirectoryInformation, PAGE_SIZE, TRUE, FALSE, &Context, &ReturnedLength);
		if (status == STATUS_NO_MORE_ENTRIES)
		{
			// success
			break;
		}
		else if (status != STATUS_SUCCESS)
		{
			kprintf("ZwQueryDirectoryObject occurs an error. %x", status);
			break;
		}
				
//		kprintf("[%d] %wZ %wZ ", count++, pObjectDirectoryInformation->Name, pObjectDirectoryInformation->TypeName);
		
		// Driver 유형만 구함. 나머지는 skip. 
		UNICODE_STRING Driver = RTL_CONSTANT_STRING(L"Driver");
		if (!RtlEqualUnicodeString(&Driver, &pObjectDirectoryInformation->TypeName, TRUE))
		{
//			kprintf("non driver skip");
			continue;
		}

		UNICODE_STRING FullName;
		FullName.MaximumLength = MAX_PATH * 2; // 문자열 버퍼 최대 사이즈
		WCHAR szFullName[MAX_PATH] = { 0, };
		FullName.Buffer = szFullName;
			
		RtlCopyUnicodeString(&FullName, pObjectName);
		RtlAppendUnicodeToString(&FullName, L"\\");
		RtlAppendUnicodeStringToString(&FullName, &pObjectDirectoryInformation->Name);
		
	//	kprintf("Name %wZ\n", &FullName);
		
		//PVOID pObject = NULL;
		PDRIVER_OBJECT pDriverObject = NULL;
			
		// 이름으로 DriverObject 구하기
		NTSTATUS retVal = ObReferenceObjectByName(&FullName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, 0, *IoDriverObjectType, KernelMode, NULL, &pDriverObject);
		if (STATUS_SUCCESS != retVal)
		{
			kprintf("ObReferenceObjectByName occurs an error. %x", retVal);
			continue;

		}
	//	kprintf("pDriverObject : %p", pDriverObject);

		if (MmIsAddressValid(pDriverObject))
		{
			PLDR_DATA_TABLE_ENTRY pModEntry = (PLDR_DATA_TABLE_ENTRY)pDriverObject->DriverSection;
	//		kprintf("DriverSection %p", pModEntry);
			if (MmIsAddressValid(pModEntry))
			{
				DRIVER_LIST_ENTRY DriverListEntry = { 0, };
				DriverListEntry.DriverObject = (uintptr_t)pDriverObject;
				DriverListEntry.modBaseAddress = (uintptr_t)pModEntry->DllBase;
				DriverListEntry.modSize = (ULONG)pModEntry->SizeOfImage;
				RtlStringCchCopyW(DriverListEntry.ServiceName, MAX_PATH, pObjectDirectoryInformation->Name.Buffer);
				RtlStringCchCopyW(DriverListEntry.FilePath, MAX_PATH, (WCHAR*)pModEntry->FullDllName.Buffer);
				DriverListEntry.isHidden = FALSE;
				AddDriverListEntry(type, &DriverListEntry);

	//			kprintf("final get : %ws %p", pModEntry->BaseDllName.Buffer, pModEntry->DllBase);
			}
			else
			{
	//			kprintf("DriverSection is not valid.");
			}
		}
		else
		{
	//		kprintf("pDriverObject is not vaild.");
		}
	
		ObDereferenceObject(pDriverObject);	
	}

	if (pObjectDirectoryInformation)
	{
		ExFreePoolWithTag(pObjectDirectoryInformation, POOL_TAG);
	}

	if (hDirectoryHandle) ZwClose(hDirectoryHandle);

}


VOID EnumerateDriversViaObjectName(int type)
{	
	UNICODE_STRING directory = RTL_CONSTANT_STRING(L"\\Driver");
	UNICODE_STRING FileSystem = RTL_CONSTANT_STRING(L"\\FileSystem");

	EnumerateDriversViaObjectNameInside(type, &directory);
	EnumerateDriversViaObjectNameInside(type, &FileSystem);
}


void EnumerateDrivers()
{
	UINT nIndex0 = 0;
	UINT nIndex1 = 0;
	UINT nIndex2 = 0;
	InitDriverListEntry(0);
	InitDriverListEntry(1);
	InitDriverListEntry(2);
	uintptr_t ldrEntryStartAddr = 0;
	uintptr_t ldrEntryEndAddr = 0;
	EnumerateDriversViaPsLoadedModuleList(0, &ldrEntryStartAddr, &ldrEntryEndAddr);
	EnumerateDriversViaObjectName(1);
	EnumerateDriversViaMmLd(2, ldrEntryStartAddr, ldrEntryEndAddr);
	
	PDRIVER_LIST_ENTRY pDriverListEntryObjectName = NULL;
	PDRIVER_LIST_ENTRY pDriverListEntry = NULL;
	PDRIVER_LIST_ENTRY pDriverListEntryMmLd = NULL;

	BOOLEAN isHidden = FALSE;

	
	pDriverListEntry = NULL;
	nIndex0 = 0;
	while (TRUE)
	{
		GetDriverListEntry(2, nIndex2, &pDriverListEntryMmLd);
		if (NULL == pDriverListEntryMmLd)
		{
			break;
		}

		pDriverListEntryMmLd->isHidden = TRUE;
		AddDriverListEntry(0, pDriverListEntryMmLd);


		nIndex2++;
	}
	
	pDriverListEntry = NULL;
	nIndex0 = 0;
	while (TRUE)
	{
		GetDriverListEntry(1, nIndex1, &pDriverListEntryObjectName);
		if (NULL == pDriverListEntryObjectName)
		{
			break;
		}
	
		nIndex0 = 0;
		pDriverListEntry = NULL;
		isHidden = TRUE;
		while (TRUE)
		{			
			
			GetDriverListEntry(0, nIndex0, &pDriverListEntry);
			if (NULL == pDriverListEntry)
			{
				break;
			}
			if (0 == wcscmp(pDriverListEntryObjectName->FilePath, pDriverListEntry->FilePath))
			{				
				isHidden = FALSE;
				// 서비스 이름 채워넣기
				RtlStringCchCopyW(pDriverListEntry->ServiceName, MAX_PATH, pDriverListEntryObjectName->ServiceName);
				// 드라이버 오브젝트 채워넣기
				pDriverListEntry->DriverObject = pDriverListEntryObjectName->DriverObject;				
			}	
	
			nIndex0++;
		}

		if (isHidden == TRUE) 
		{
		//	kprintf("Hidden Driver:");		
			pDriverListEntryObjectName->isHidden = TRUE;
			AddDriverListEntry(0, pDriverListEntryObjectName);			
		}
		
		nIndex1++;
	}


	/*

	pDriverListEntry = NULL;
	nIndex0 = 0;
	while (TRUE)
	{
		GetDriverListEntry(0, nIndex0, &pDriverListEntry);
		if (NULL == pDriverListEntry)
		{
			break;
		}
	//	kprintf("[%d] %ws Base Address : %llx", nIndex0, pDriverListEntry->FilePath, pDriverListEntry->modBaseAddress);
	//	kprintf("Size : %x Service : %ws DriverObject : %llx", pDriverListEntry->modSize, pDriverListEntry->ServiceName, pDriverListEntry->DriverObject);
	//	kprintf("hidden : %x", pDriverListEntry->isHidden);
		nIndex0++;
	}
	*/

	FreeDriverListEntry(1);
	FreeDriverListEntry(2);
}