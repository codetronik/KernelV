// https://github.com/swatkat/arkitlib/tree/master/ARKitDrv 참조해서 만듦

#include "listentry.h"
#include "define.h"
#include <wdm.h>
#include <windef.h>
#include <ntstrsafe.h>

PLIST_ENTRY pListDriverEntry[4];
PLIST_ENTRY pListDetectEntry;


void FreeDriverListEntry(int type)
{
	if (!MmIsAddressValid(pListDriverEntry[type]))
	{
		return;
	}

	while (!IsListEmpty(pListDriverEntry[type]))
	{
		// 리스트에서 하나 꺼냄
		PLIST_ENTRY pEntry = RemoveHeadList(pListDriverEntry[type]);
		PDRIVER_LIST_ENTRY pDriverEntry = CONTAINING_RECORD(pEntry, DRIVER_LIST_ENTRY, Entry);
		if (MmIsAddressValid(pDriverEntry))
		{
			RtlZeroMemory(pDriverEntry, sizeof(DRIVER_LIST_ENTRY)); // wipe
		//	kprintf("free: %p", pDriverEntry);
			ExFreePool(pDriverEntry);
		}

	}
	kprintf("free : %p (type : %d)", pListDriverEntry[type], type);
	ExFreePool(pListDriverEntry[type]);
	pListDriverEntry[type] = NULL;
}


void InitDriverListEntry(int type)
{		
	// 이미 존재하면 지우고 만듦 : IOCTL_KERNELV_SCAN_PATTERN 요청시 기존 pListDriverEntry가 필요하므로 free를 수행하면 안됨. 그래서 reinit이 존재함.
	if (MmIsAddressValid(pListDriverEntry[type]))
	{
		kprintf("reinit (type : %d)", type);
		FreeDriverListEntry(type);
		
	}
	pListDriverEntry[type] = ExAllocatePoolWithTag(NonPagedPool, sizeof(LIST_ENTRY), POOL_TAG);
	if (NULL == pListDriverEntry[type])
	{
		kprintf("AllocatePool error!");
		return;
	}
	RtlZeroMemory(pListDriverEntry[type], sizeof(LIST_ENTRY));
	InitializeListHead(pListDriverEntry[type]);
	kprintf("init : %p (type : %d)", pListDriverEntry[type], type);
}

void AddDriverListEntry(int type, PDRIVER_LIST_ENTRY pDriverEntry)
{
	// 새롭게 붙이는 entry 생성
	PDRIVER_LIST_ENTRY pNewEntry = NULL;
	pNewEntry = ExAllocatePoolWithTag(NonPagedPool, sizeof(DRIVER_LIST_ENTRY), POOL_TAG);	
	if (NULL == pNewEntry)
	{
		kprintf("AllocatePool error!");
		return;
	}
	RtlZeroMemory(pNewEntry, sizeof(DRIVER_LIST_ENTRY));

	pNewEntry->DriverObject = pDriverEntry->DriverObject;
	pNewEntry->modBaseAddress = pDriverEntry->modBaseAddress;
	pNewEntry->modSize = pDriverEntry->modSize;
	RtlStringCchCopyW(pNewEntry->ServiceName, MAX_PATH, pDriverEntry->ServiceName);
	RtlStringCchCopyW(pNewEntry->FilePath, MAX_PATH, pDriverEntry->FilePath);
	pNewEntry->isHidden = pDriverEntry->isHidden;
//	kprintf("alloc : %p", pNewEntry);
	InsertHeadList(pListDriverEntry[type], &(pNewEntry->Entry));
}

UINT GetDriverListEntryCount(int type)
{
	int nIndex = 0;
	while (TRUE)
	{
		PDRIVER_LIST_ENTRY pDriverListEntry = NULL;
		GetDriverListEntry(type, nIndex, &pDriverListEntry);
		if (NULL == pDriverListEntry)
		{
			break;
		}
		nIndex++;
	}
	return nIndex;
}


void GetDriverListEntry(int type, UINT nPosition, DRIVER_LIST_ENTRY** ppDriverListEntry)
{
	if (!MmIsAddressValid(pListDriverEntry[type]))
	{
		kprintf("pListDriverEntry is not valid. %p", pListDriverEntry[type]);
		return;
	}
	*ppDriverListEntry = NULL;
	PLIST_ENTRY pEntry = pListDriverEntry[type]->Blink;
	
	UINT nIndex = 0;
	while (TRUE)
	{		
		if (!MmIsAddressValid(pEntry))
		{
			kprintf("not valid");
			break;
		}		
		
		// 한바퀴 다 돌면 멈춤
		if (pEntry == pListDriverEntry[type])
		{		
			break;
		}
		
		if (nIndex == nPosition)
		{
			*ppDriverListEntry = CONTAINING_RECORD(pEntry, DRIVER_LIST_ENTRY, Entry);
		}
		nIndex++;
		pEntry = pEntry->Blink;		
	}
}





void InitDetectListEntry()
{
	// pListDetectEntry는 계속 메모리에 남길 필요가 없으므로 reinit이 존재할 필요가 없음
	if (MmIsAddressValid(pListDetectEntry))
	{
		kprintf("reinit pListDetectEntry");
		FreeDetectListEntry();

	}

	pListDetectEntry = ExAllocatePoolWithTag(NonPagedPool, sizeof(LIST_ENTRY), POOL_TAG);
	if (NULL == pListDetectEntry)
	{
		kprintf("AllocatePool error!");
		return;
	}
	RtlZeroMemory(pListDetectEntry, sizeof(LIST_ENTRY));
	InitializeListHead(pListDetectEntry);
	kprintf("init : %p", pListDetectEntry);
}

void FreeDetectListEntry()
{
	if (!MmIsAddressValid(pListDetectEntry))
	{
		return;
	}

	while (!IsListEmpty(pListDetectEntry))
	{
		// 리스트에서 하나 꺼냄
		PLIST_ENTRY pEntry = RemoveHeadList(pListDetectEntry);
		PDETECT_LIST_ENTRY pDetectEntry = CONTAINING_RECORD(pEntry, DETECT_LIST_ENTRY, Entry);
		if (MmIsAddressValid(pDetectEntry))
		{
			RtlZeroMemory(pDetectEntry, sizeof(DETECT_LIST_ENTRY)); // wipe
		//	kprintf("free: %p", pDriverEntry);
			ExFreePool(pDetectEntry);
		}

	}
	kprintf("free : %p", pListDetectEntry);
	ExFreePool(pListDetectEntry);
	pListDetectEntry = NULL;
}
void AddDetectListEntry(PDETECT_LIST_ENTRY pDetectEntry)
{
	if (!MmIsAddressValid(pListDetectEntry))
	{
		kprintf("no pListDetectEntry");
		return;
	}

	// 새롭게 붙이는 entry 생성
	PDETECT_LIST_ENTRY pNewEntry = NULL;
	pNewEntry = ExAllocatePoolWithTag(NonPagedPool, sizeof(DETECT_LIST_ENTRY), POOL_TAG);
	if (NULL == pNewEntry)
	{
		kprintf("AllocatePool error!");
		return;
	}
	RtlZeroMemory(pNewEntry, sizeof(DETECT_LIST_ENTRY));
	__try
	{
		pNewEntry->PatternNo = pDetectEntry->PatternNo;
		pNewEntry->BaseAddress = pDetectEntry->BaseAddress;
		pNewEntry->Offset = pDetectEntry->Offset;

		InsertHeadList(pListDetectEntry, &pNewEntry->Entry);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{	
		kprintf("Exception caught in AddDetectListEntry()");
	}
}
void GetDetectListEntry(UINT nPosition, DETECT_LIST_ENTRY** ppDetectListEntry)
{
	*ppDetectListEntry = NULL;
	PLIST_ENTRY pEntry = pListDetectEntry->Blink;

	UINT nIndex = 0;
	while (TRUE)
	{
		if (!MmIsAddressValid(pEntry))
		{
			kprintf("not valid");
			break;
		}

		// 한바퀴 다 돌면 멈춤
		if (pEntry == pListDetectEntry)
		{
			break;
		}

		if (nIndex == nPosition)
		{
			*ppDetectListEntry = CONTAINING_RECORD(pEntry, DETECT_LIST_ENTRY, Entry);
		}
		nIndex++;
		pEntry = pEntry->Blink;
	}
}
UINT GetDetectListEntryCount()
{
	int nIndex = 0;
	while (TRUE)
	{
		PDETECT_LIST_ENTRY pDetectListEntry = NULL;
		GetDetectListEntry(nIndex, &pDetectListEntry);
		if (NULL == pDetectListEntry)
		{
			break;
		}
		nIndex++;
	}
	return nIndex;
}