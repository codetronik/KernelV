# KernelV
Built in Visual Studio 2019 + wdk 10.0.19041.0

Tested on Windows 10 Build 19041 64-bit version.

The built binary is not signed. Run in test mode.

Features
- Scan binary patterns in kernel drivers
- Hide the driver
  + Traversing PsLoadedModuleList
  + Traversing ObjectDirectory
- Detect hidden drivers
  + Enumerating ObjectName 
  + Scanning "MmLd" in Kernel-memory
