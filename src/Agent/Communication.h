#pragma once

typedef struct _DRIVER_LIST_ENTRY {
	uintptr_t modBaseAddress;
	ULONG modSize;
	WCHAR FilePath[MAX_PATH];
	WCHAR ServiceName[MAX_PATH];
	uintptr_t DriverObject;
	BOOLEAN isHidden;
	LIST_ENTRY Entry; // list chain
} DRIVER_LIST_ENTRY, * PDRIVER_LIST_ENTRY;

typedef struct _DETECT_LIST_ENTRY {
	LIST_ENTRY Entry; // list chain
	UINT PatternNo;
	uintptr_t BaseAddress;
	UINT Offset;	
} DETECT_LIST_ENTRY, * PDETECT_LIST_ENTRY;


class CCommunication
{
public:
	CCommunication();
	~CCommunication();
public:
	BOOL Initialize();
	void Finalize();
	BOOL GetDrivers();
	BOOL HideMyself();
	BOOL ScanPattern(PBYTE pPattern, int nPatternSize);
	BOOL HideDriver(LPCWSTR pDriverPath, LPCWSTR pServiceName, PBOOL pbHide);
	PDRIVER_LIST_ENTRY m_pDriverEntry;
	int m_nDriverListEntryCount;
	PDETECT_LIST_ENTRY m_pDetectEntry;
	int m_nDetectListEntryCount;
private:
	HANDLE m_hDevice;
};
