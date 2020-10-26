# KernelV
![image](https://raw.githubusercontent.com/codetronik/KernelV/master/screenshots/mainmenu.png)

Download : https://github.com/codetronik/KernelV/releases

Windows 10 x64 only

Built in Visual Studio 2019 + wdk 10.0.19041.0

Tested on Windows 10 Build 19041 64-bit version.

## Features
- Scan binary patterns in kernel drivers
- Hide the driver
  + Breaking the chain of LDR_DATA_TABLE_ENTRY(PsLoadedModuleList)
  + Breaking the chain of OBJECT_DIRECTORY_ENTRY
- Detect hidden drivers
  + Enumerating Object Name 
  + Scanning "MmLd"(LDR_DATA_TABLE_ENTRY) in Kernel-memory

## Pattern Scan 
![image](https://raw.githubusercontent.com/codetronik/KernelV/master/screenshots/scanpattern.png)

Enter one binary pattern per line. Patterns cannot be entered in more than two lines.

The delimiter("\r\n") of the pattern is the Enter key.

## Precautions
If you hide ntoskrnl.exe or myself, the next run may fail.

Hiding a driver can cause BSOD. (19 : Loaded module list modification)


## Codesign
The built driver I provide is signed with the "HT Srl" certificate. It is highly likely to be detected in the anti-virus.
