# sysinfofetch

A fast Windows system information fetcher written in C++. Outputs system info in the same format as `systeminfo`. Compiled with LLVM/Clang (MinGW target).

## Requirements

- **LLVM/Clang** (`C:\Program Files\LLVM\bin\clang++.exe`) or **g++** (MinGW-w64)
- Windows 10 or later
- No external dependencies (uses only Win32 APIs)

## Building

Run the build script:

```
build.bat
```

Or compile manually:

```
g++ -std=c++17 -O2 -o sysinfofetch.exe sysinfofetch.cpp -lws2_32 -liphlpapi -lpsapi -ladvapi32 -luser32
```

To build the noconsole (GUI) version:

```
g++ -std=c++17 -O2 -DNOCONSOLE -o sysinfofetch_noconsole.exe sysinfofetch.cpp -lws2_32 -liphlpapi -lpsapi -ladvapi32 -luser32 -mwindows
```

## Usage

```
sysinfofetch.exe [options]
```

### Options

| Flag | Description |
|---|---|
| `--json` | Output in JSON format |
| `--txt` | Output in plain text format (default, matches `systeminfo`) |
| `--filesavepath PATH` | Save output to a file at the specified path |
| `--noconsole` | Use the noconsole build (`sysinfofetch_noconsole.exe`) to hide the console window |
| `--help` | Show help message |

### Examples

**Print system info to console (default):**
```
sysinfofetch.exe
```

**Output JSON to console:**
```
sysinfofetch.exe --json
```

**Save JSON to file:**
```
sysinfofetch.exe --json --filesavepath C:\report.json
```

**Save plain text to file:**
```
sysinfofetch.exe --txt --filesavepath report.txt
```

**Run silently (no console window) and save JSON:**
Use `sysinfofetch_noconsole.exe` (built with `-DNOCONSOLE`):
```
sysinfofetch_noconsole.exe --json --filesavepath C:\report.json
```

**Combine all flags:**
```
sysinfofetch.exe --noconsole --json --filesavepath C:\Users\Admin\Desktop\sysinfo.json
```

## Information Collected

- **OS:** Name, version, build, manufacturer, configuration, build type
- **System:** Manufacturer, model, type, BIOS version
- **CPU:** Model name, core count, clock speed
- **RAM:** Total, used, available (physical memory)
- **Virtual Memory:** Max, available, in use
- **GPU:** Primary display adapter (via registry)
- **Network:** IP addresses, adapter names, DHCP status
- **User:** Username, computer name, domain, logon server
- **Other:** Install date, boot time, uptime, hotfixes, timezone, Hyper-V requirements

## Text Output Format

```
Host Name:                 DESKTOP-HDRURUH
OS Name:                   Windows 10 Pro
OS Version:                22H2 N/A Build 19045
OS Manufacturer:           Microsoft Corporation
OS Configuration:          Standalone Workstation
OS Build Type:             Multiprocessor Free
Registered Owner:          Windows User
Registered Organization:   No organization name provided.
Product ID:                00330-80000-00000-AA765
Original Install Date:     5/22/2026, 9:47:38 PM
System Boot Time:          6/9/2026, 5:59:31 PM
System Manufacturer:       ASUSTeK COMPUTER INC.
System Model:              VivoBook_ASUSLaptop X513EP_X513EP
System Type:               x64-based PC
Processor(s):              8 Processor(s) Installed.
                           [01]: 11th Gen Intel(R) Core(TM) i7-1165G7 @ 2.80GHz ~2803 Mhz
BIOS Version:              American Megatrends International, LLC.. X513EP.318, 05/09/2023
Windows Directory:         C:\Windows
System Directory:          C:\Windows\system32
Boot Device:               \Device\HarddiskVolume2
System Locale:             045b
Input Locale:              en-us;English (United States)
Time Zone:                 (UTC+5:30) India Standard Time
Total Physical Memory:     7886 MB
Available Physical Memory: 847 MB
Virtual Memory: Max Size:  15545 MB
Virtual Memory: Available: 1242 MB
Virtual Memory: In Use:    14303 MB
Page File Location(s):     C:\pagefile.sys
Domain:                    WORKGROUP
Logon Server:              \\DESKTOP-HDRURUH
Hotfix(s):                 0 Hotfix(s) Installed.
Network Card(s):           [01]: Intel(R) Wi-Fi 6 AX201 160MHz
                                 Connection Name: Intel(R) Wi-Fi 6 AX201 160MHz
                                 DHCP Enabled:    Yes
                                 IP address(es)
                                 [01]: 10.136.19.128

Hyper-V Requirements:      VM Monitor Mode Extensions: Yes
                           Virtualization Enabled In Firmware: Yes
                           Second Level Address Translation: Yes
                           Data Execution Prevention Available: Yes
```

## JSON Output Format

```json
{
  "hostName": "DESKTOP-HDRURUH",
  "osName": "Windows 10 Pro",
  "osVersion": "22H2 N/A Build 19045",
  "osManufacturer": "Microsoft Corporation",
  "osConfiguration": "Standalone Workstation",
  "osBuildType": "Multiprocessor Free",
  "registeredOwner": "Windows User",
  "registeredOrganization": "No organization name provided.",
  "productId": "00330-80000-00000-AA765",
  "originalInstallDate": "5/22/2026, 9:47:38 PM",
  "systemBootTime": "6/9/2026, 5:59:31 PM",
  "systemManufacturer": "ASUSTeK COMPUTER INC.",
  "systemModel": "VivoBook_ASUSLaptop X513EP_X513EP",
  "systemType": "x64-based PC",
  "processor": "8 Processor(s) Installed.\n                           [01]: 11th Gen Intel Core i7-1165G7 @ 2.80GHz ~2803 Mhz",
  "biosVersion": "American Megatrends International, LLC.. X513EP.318, 05/09/2023",
  "windowsDirectory": "C:\\Windows",
  "systemDirectory": "C:\\Windows\\system32",
  "bootDevice": "\\Device\\HarddiskVolume2",
  "systemLocale": "045b",
  "inputLocale": "en-us;English (United States)",
  "timeZone": "(UTC+5:30) India Standard Time",
  "totalPhysicalMemory": "7886 MB",
  "availablePhysicalMemory": "847 MB",
  "virtualMemoryMaxSize": "15545 MB",
  "virtualMemoryAvailable": "1242 MB",
  "virtualMemoryInUse": "14303 MB",
  "pageFileLocation": "C:\\pagefile.sys",
  "domain": "WORKGROUP",
  "logonServer": "\\\\DESKTOP-HDRURUH",
  "hotfixes": "0 Hotfix(s) Installed.",
  "networkCards": "...",
  "hyperV": "VM Monitor Mode Extensions: Yes\n..."
}
```

## License

Public domain. Use however you want.
