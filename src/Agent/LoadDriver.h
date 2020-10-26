#pragma once

#define DRIVER_NAME		L"KernelV"

class CLoadDriver
{
public:
	CLoadDriver();
	~CLoadDriver();
	BOOL Load();
	BOOL Unload();	
};

