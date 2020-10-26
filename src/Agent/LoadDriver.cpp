#include "pch.h"
#include "LoadDriver.h"
#include <winsvc.h>

CLoadDriver::CLoadDriver()
{

}

CLoadDriver::~CLoadDriver(void)
{

}

BOOL CLoadDriver::Load()
{
	BOOL bRet = FALSE;
	SC_HANDLE hSCManager = NULL;
	SC_HANDLE hService = NULL;
	SC_HANDLE hCService = NULL;
	TCHAR szCurrentDirectory[MAX_PATH] = { 0, };
	TCHAR szDriverPath[MAX_PATH] = { 0, };
	
	DWORD dwError = 0;
	CString str;

	GetCurrentDirectory(MAX_PATH, szCurrentDirectory);
	swprintf_s(szDriverPath, MAX_PATH, _T("%s\\%s.sys"), szCurrentDirectory, DRIVER_NAME);

	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (NULL == hSCManager)
	{
		dwError = GetLastError();
		str.Format(L"[Load driver] OpenSCManager() Error : %d", dwError);
		AfxMessageBox(str);
		goto EXIT_ERROR;
	}

	hCService = CreateService(
		hSCManager,
		DRIVER_NAME,
		DRIVER_NAME,
		SERVICE_ALL_ACCESS,
		SERVICE_KERNEL_DRIVER,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL,
		szDriverPath,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL);
	if (NULL == hCService)
	{		
		dwError = GetLastError();
		if (ERROR_SERVICE_EXISTS == dwError)
		{
			str.Format(L"[Load driver] CreateService() Error : %d", dwError);
			bRet = TRUE;
			goto EXIT;
		}		
		str.Format(L"[Load driver] CreateService() Error : %d", dwError);
		AfxMessageBox(str);	
		goto EXIT_ERROR;
	}

	hService = OpenService(hSCManager, DRIVER_NAME, SERVICE_ALL_ACCESS);
	if (NULL == hService)
	{
		dwError = GetLastError();
		str.Format(L"[Load driver] OpenService() Error : %d", dwError);
		AfxMessageBox(str);
		goto EXIT_ERROR;
	}

	if (FALSE == StartService(hService, 0, NULL))
	{
		dwError = GetLastError();
		str.Format(L"[Load driver] StartService() Error : %d", dwError);
		AfxMessageBox(str);
		goto EXIT_ERROR;
	}

	bRet = TRUE;
	goto EXIT;

EXIT_ERROR:
	bRet = FALSE;

EXIT:
	if (hCService)
	{
		CloseServiceHandle(hCService);
	}
	if (hService)
	{
		CloseServiceHandle(hService);
	}
	if (hSCManager)
	{
		CloseServiceHandle(hSCManager);
	}
	return bRet;
}



BOOL CLoadDriver::Unload()
{
	BOOL bRet = FALSE;
	SC_HANDLE hSCManager = NULL;
	SC_HANDLE hService = NULL;
	SERVICE_STATUS ss;
	DWORD dwError = 0;
	CString str;

	_tprintf(_T("Stoping and unloading Driver.."));
	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (NULL == hSCManager)
	{
		dwError = GetLastError();
		str.Format(L"[Unload driver] OpenSCManager() Error : %d", dwError);
		AfxMessageBox(str);
		goto EXIT_ERROR;
	}

	hService = OpenService(hSCManager, DRIVER_NAME, SERVICE_ALL_ACCESS);
	if (NULL == hService)
	{
		dwError = GetLastError();
		str.Format(L"[Unload driver] OpenService() Error : %d", dwError);
		AfxMessageBox(str);
		goto EXIT_ERROR;
	}

	if (!ControlService(hService, SERVICE_CONTROL_STOP, &ss))
	{
		dwError = GetLastError();
		str.Format(L"[Unload driver] ControlService() Error : %d", dwError);
		AfxMessageBox(str);
	//	goto EXIT_ERROR;
	}

	if (!DeleteService(hService))
	{
		dwError = GetLastError();
		str.Format(L"[Unload driver] DeleteService() Error : %d", dwError);
		AfxMessageBox(str);
		goto EXIT_ERROR;
	}
	
	bRet = TRUE;
	goto EXIT;

EXIT_ERROR:
	bRet = FALSE;

EXIT:
	if (hService)
	{
		CloseServiceHandle(hService);
	}
	if (hSCManager)
	{
		CloseServiceHandle(hSCManager);
	}
	return bRet;
}