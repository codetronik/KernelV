#pragma once
#include "listentry.h"

VOID GetPsLoadedModuleList();
BOOLEAN ScanKernelDriver(UINT PatternNo, PBYTE Pattern, UINT PatternSize);
//BOOLEAN ScanKernelDriverViaAPI();
VOID EnumerateDriversViaPsLoadedModuleList(int type, uintptr_t* OutStartAddr, uintptr_t* OutEndAddr);
VOID EnumerateDriversViaObjectName(int type);
VOID EnumerateDriversViaMmLd(int type, uintptr_t StartAddr, uintptr_t EndAddr);
void EnumerateDrivers();
VOID ScanMemory(UINT PatternNo, uintptr_t StartAddr, uintptr_t EndAddr, PBYTE Pattern, UINT PatternSize);
