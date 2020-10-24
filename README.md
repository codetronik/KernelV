# KernelV
Built in Visual Studio 2019 + wdk 10.0.19041.0

Tested on Windows 10 Build 19041 64-bit version.

The built binary is not signed. Run in test mode.

Features
- Scan binary patterns in kernel drivers
- Hide the driver
  + Breaking the chain of LDR_DATA_TABLE_ENTRY(PsLoadedModuleList)
  + Breaking the chain of OBJECT_DIRECTORY_ENTRY
- Detect hidden drivers
  + Enumerating Object Name 
  + Scanning "MmLd"(LDR_DATA_TABLE_ENTRY) in Kernel-memory
