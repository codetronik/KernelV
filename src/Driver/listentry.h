#pragma once
#include <ntddk.h>
#include <windef.h>

typedef struct _DRIVER_LIST_ENTRY {	
	uintptr_t modBaseAddress;
	ULONG modSize;	
	WCHAR FilePath[MAX_PATH];
	WCHAR ServiceName[MAX_PATH];
	uintptr_t DriverObject;
	BOOLEAN isHidden;	
	LIST_ENTRY Entry; // list chain
} DRIVER_LIST_ENTRY, *PDRIVER_LIST_ENTRY;


typedef struct _DETECT_LIST_ENTRY {
	LIST_ENTRY Entry; // list chain
	UINT PatternNo;
	uintptr_t BaseAddress;
	UINT Offset; // detect offset
} DETECT_LIST_ENTRY, *PDETECT_LIST_ENTRY;

void FreeDriverListEntry(int type);
void InitDriverListEntry(int type);
void AddDriverListEntry(int type, PDRIVER_LIST_ENTRY pDriverEntry);
void GetDriverListEntry(int type, UINT nPosition, DRIVER_LIST_ENTRY** ppDriverListEntry);
UINT GetDriverListEntryCount(int type);

void FreeDetectListEntry();
void InitDetectListEntry();
void AddDetectListEntry(PDETECT_LIST_ENTRY pDetectEntry);
void GetDetectListEntry(UINT nPosition, DETECT_LIST_ENTRY** ppDetectListEntry);
UINT GetDetectListEntryCount();