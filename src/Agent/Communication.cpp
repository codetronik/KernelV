#include "pch.h"
#include "Communication.h"
#include "LoadDriver.h"
#include <devioctl.h>

#define IOCTL_KERNELV_LOAD_DRIVER		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS) 
#define IOCTL_KERNELV_HIDE_MYSELF		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS) 
#define IOCTL_KERNELV_HIDE_DRIVER		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS) 
#define IOCTL_KERNELV_SCAN_PATTERN		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS) 


CCommunication::CCommunication()
{
	m_hDevice = NULL;
}
CCommunication::~CCommunication()
{

}

BOOL CCommunication::Initialize()
{
	TCHAR szPath[MAX_PATH] = { 0, };
	TCHAR szCurrentDir[MAX_PATH] = { 0, };
	CString strErrorMsg;
	
	GetCurrentDirectory(MAX_PATH, szCurrentDir);
	swprintf_s(szPath, MAX_PATH, _T("%s\\%s"), szCurrentDir, DRIVER_NAME);

	TCHAR* szDeviceLink = _T("\\\\.\\KernelV");

	m_hDevice = CreateFile(szDeviceLink, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM, NULL);
	if (INVALID_HANDLE_VALUE == m_hDevice)
	{
		strErrorMsg.Format(L"[I/O driver] CreateFile Error : %d", GetLastError());
		AfxMessageBox(strErrorMsg);
		return FALSE;
	}
	return TRUE;

}
void CCommunication::Finalize()
{
	if (m_hDevice)
	{
		CloseHandle(m_hDevice);
		m_hDevice = NULL;
	}
}
BOOL CCommunication::HideMyself()
{
	DWORD dwRetBytes = 0;	

	BOOL bSuccess = DeviceIoControl(m_hDevice, IOCTL_KERNELV_HIDE_MYSELF, NULL, 0, NULL, 0, &dwRetBytes, NULL);
	if (FALSE == bSuccess)
	{
		return FALSE;
	}

	return TRUE;	
}
BOOL CCommunication::HideDriver(LPCWSTR pDriverPath, LPCWSTR pServiceName, PBOOL pbHide)
{
	DWORD dwRetBytes = 0;
	WCHAR szYes[4] = { 0, };
	// null스트링 포함
	CString SendData;	
	SendData.Format(L"%s::%s", pDriverPath, pServiceName);
	BOOL bSuccess = DeviceIoControl(m_hDevice, IOCTL_KERNELV_HIDE_DRIVER, (LPVOID)SendData.GetBuffer(), (DWORD)(wcslen(SendData.GetBuffer())*2 + 2), szYes, (DWORD)sizeof(szYes), &dwRetBytes, NULL);
	if (FALSE == bSuccess)
	{
		return FALSE;
	}
	if (wcscmp(L"YES", szYes) == 0)
	{
		*pbHide = TRUE;
	}
	else
	{
		*pbHide = FALSE;
	}
	return TRUE;
}

BOOL CCommunication::GetDrivers()
{
	m_DriverListEntry.clear();

	DWORD dwRetBytes = 0;
	// 필요한 버퍼 사이즈를 묻는다.
	BOOL bSuccess = DeviceIoControl(m_hDevice, IOCTL_KERNELV_LOAD_DRIVER, NULL, 0, NULL, 0, &dwRetBytes, NULL);
	printf("1. dwret : %d\n", dwRetBytes);

	// 정확한 사이즈를 가지고 도전
	m_DriverListEntry.resize(dwRetBytes / sizeof(DRIVER_LIST_ENTRY));
	bSuccess = DeviceIoControl(m_hDevice, IOCTL_KERNELV_LOAD_DRIVER, NULL, 0, &m_DriverListEntry.front(), dwRetBytes, &dwRetBytes, NULL);
	if (FALSE == bSuccess || dwRetBytes == 0)
	{		
		return FALSE;
	}
	printf("2. dwret : %d\n", dwRetBytes);
	return TRUE;
}

BOOL CCommunication::ScanPattern(PBYTE pPattern, size_t nPatternSize)
{
	m_DetectListEntry.clear();

	DWORD dwRetBytes = 0;

	// 필요한 버퍼 사이즈를 묻는다.
	BOOL bSuccess = DeviceIoControl(m_hDevice, IOCTL_KERNELV_SCAN_PATTERN, pPattern, (DWORD)nPatternSize, NULL, 0, &dwRetBytes, NULL);
	printf("1. dwret : %d\n", dwRetBytes);
	if (FALSE == bSuccess)
	{		
		return FALSE;
	}
	if (dwRetBytes == 0)
	{
		return TRUE;
	}
	
	// 정확한 사이즈를 가지고 도전
	m_DetectListEntry.resize(dwRetBytes / sizeof(DETECT_LIST_ENTRY));

	bSuccess = DeviceIoControl(m_hDevice, IOCTL_KERNELV_SCAN_PATTERN, NULL, 0, &m_DetectListEntry.front(), dwRetBytes, &dwRetBytes, NULL);
	if (FALSE == bSuccess)
	{		
		return FALSE;
	}
	printf("2. dwret : %d\n", dwRetBytes);

	
	return TRUE;
}