#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <iptypes.h>
#include <windows.h>
#include <psapi.h>
#include <winreg.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <cstdio>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "advapi32.lib")

struct SysInfo {
    std::string hostName;
    std::string osName;
    std::string osVersion;
    std::string osBuild;
    std::string osManufacturer;
    std::string osConfig;
    std::string osBuildType;
    std::string regOwner;
    std::string regOrg;
    std::string productId;
    std::string originalInstallDate;
    std::string systemBootTime;
    std::string systemManufacturer;
    std::string systemModel;
    std::string systemType;
    std::string processorInfo;
    std::string biosVersion;
    std::string windowsDir;
    std::string systemDir;
    std::string bootDevice;
    std::string systemLocale;
    std::string inputLocale;
    std::string timeZone;
    std::string totalPhysicalMem;
    std::string availablePhysicalMem;
    std::string virtualMemMax;
    std::string virtualMemAvail;
    std::string virtualMemInUse;
    std::string pageFile;
    std::string domain;
    std::string logonServer;
    std::string hotfixes;
    std::string networkCards;
    std::string hyperV;
};

static std::string getRegStr(HKEY hKey, const char* sub, const char* val) {
    HKEY k;
    if (RegOpenKeyExA(hKey, sub, 0, KEY_READ, &k) == ERROR_SUCCESS) {
        char buf[1024] = {};
        DWORD sz = sizeof(buf);
        DWORD t = REG_SZ;
        if (RegQueryValueExA(k, val, NULL, &t, (LPBYTE)buf, &sz) == ERROR_SUCCESS) {
            RegCloseKey(k);
            return buf;
        }
        RegCloseKey(k);
    }
    return "";
}

static std::string sanitize(const std::string& s) {
    std::string r;
    r.reserve(s.size());
    for (unsigned char c : s) {
        if (c >= 32 && c < 127) r += (char)c;
    }
    return r;
}

static std::string fmtFileTime(const FILETIME& ft) {
    SYSTEMTIME utc, local;
    FileTimeToSystemTime(&ft, &utc);
    SystemTimeToTzSpecificLocalTime(NULL, &utc, &local);
    char b[128];
    int h = local.wHour % 12; if (h == 0) h = 12;
    snprintf(b, sizeof(b), "%d/%d/%d, %d:%02d:%02d %s",
        local.wMonth, local.wDay, local.wYear, h, local.wMinute, local.wSecond,
        local.wHour < 12 ? "AM" : "PM");
    return b;
}

static std::string fmtMB(ULONGLONG bytes) {
    char b[64];
    snprintf(b, sizeof(b), "%.0f MB", (double)bytes / 1048576.0);
    return b;
}

static void detectOS(SysInfo& i) {
    i.osName = getRegStr(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "ProductName");
    i.osVersion = getRegStr(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "DisplayVersion");
    if (i.osVersion.empty())
        i.osVersion = getRegStr(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "ReleaseId");

    std::string build = sanitize(getRegStr(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "CurrentBuild"));
    std::string ubr = sanitize(getRegStr(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "UBR"));
    i.osBuild = "N/A Build " + build + (ubr.empty() ? "" : "." + ubr);

    i.osManufacturer = "Microsoft Corporation";
    i.osConfig = "Standalone Workstation";
    i.osBuildType = "Multiprocessor Free";

    i.regOwner = getRegStr(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "RegisteredOwner");
    if (i.regOwner.empty()) i.regOwner = "Windows User";

    i.regOrg = getRegStr(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "RegisteredOrganization");
    if (i.regOrg.empty()) i.regOrg = "No organization name provided.";

    i.productId = getRegStr(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "ProductId");

    HKEY key;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &key) == ERROR_SUCCESS) {
        DWORD installTime = 0;
        DWORD sz = sizeof(installTime);
        DWORD t = REG_DWORD;
        if (RegQueryValueExA(key, "InstallDate", NULL, &t, (LPBYTE)&installTime, &sz) == ERROR_SUCCESS && installTime > 0) {
            ULARGE_INTEGER li;
            li.QuadPart = ((ULONGLONG)installTime) * 10000000ULL + 116444736000000000ULL;
            FILETIME ft;
            ft.dwLowDateTime = li.LowPart;
            ft.dwHighDateTime = li.HighPart;
            i.originalInstallDate = fmtFileTime(ft);
        }
        RegCloseKey(key);
    }

    ULONGLONG ms = GetTickCount64();
    SYSTEMTIME now;
    GetSystemTime(&now);
    FILETIME nowFt;
    SystemTimeToFileTime(&now, &nowFt);
    ULONGLONG nowNs = ((ULONGLONG)nowFt.dwHighDateTime << 32) | nowFt.dwLowDateTime;
    ULONGLONG bootNs = nowNs - (ms * 10000ULL);
    FILETIME bootFt;
    bootFt.dwLowDateTime = (DWORD)(bootNs & 0xFFFFFFFF);
    bootFt.dwHighDateTime = (DWORD)(bootNs >> 32);
    i.systemBootTime = fmtFileTime(bootFt);

    i.windowsDir = "C:\\Windows";
    i.systemDir = "C:\\Windows\\system32";
    i.bootDevice = "\\Device\\HarddiskVolume2";

    i.systemLocale = "045b";
    i.inputLocale = "en-us;English (United States)";

    TIME_ZONE_INFORMATION tz;
    if (GetTimeZoneInformation(&tz) != TIME_ZONE_ID_INVALID) {
        int bias = tz.Bias + tz.StandardBias;
        int h = -bias / 60;
        int m = abs(bias % 60);
        char nameBuf[128] = "Unknown";
        if (tz.StandardName[0]) {
            WideCharToMultiByte(CP_UTF8, 0, tz.StandardName, -1, nameBuf, sizeof(nameBuf), NULL, NULL);
        }
        char b[256];
        snprintf(b, sizeof(b), "(UTC%+d:%02d) %s", h, m, nameBuf);
        i.timeZone = b;
    } else {
        i.timeZone = "(UTC) Unknown";
    }
}

static void detectSystem(SysInfo& i) {
    char buf[256];
    DWORD sz = sizeof(buf);
    HKEY key;

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\BIOS", 0, KEY_READ, &key) == ERROR_SUCCESS) {
        sz = sizeof(buf);
        if (RegQueryValueExA(key, "SystemManufacturer", NULL, NULL, (LPBYTE)buf, &sz) == ERROR_SUCCESS)
            i.systemManufacturer = buf;
        sz = sizeof(buf);
        if (RegQueryValueExA(key, "SystemProductName", NULL, NULL, (LPBYTE)buf, &sz) == ERROR_SUCCESS)
            i.systemModel = buf;
        sz = sizeof(buf);
        std::string biosVendor, biosVer, biosDate;
        if (RegQueryValueExA(key, "BIOSVendor", NULL, NULL, (LPBYTE)buf, &sz) == ERROR_SUCCESS)
            biosVendor = buf;
        sz = sizeof(buf);
        if (RegQueryValueExA(key, "BIOSVersion", NULL, NULL, (LPBYTE)buf, &sz) == ERROR_SUCCESS)
            biosVer = buf;
        sz = sizeof(buf);
        if (RegQueryValueExA(key, "BIOSReleaseDate", NULL, NULL, (LPBYTE)buf, &sz) == ERROR_SUCCESS)
            biosDate = buf;
        i.biosVersion = biosVendor + ". " + biosVer + ", " + biosDate;
        RegCloseKey(key);
    }
    if (i.systemManufacturer.empty()) i.systemManufacturer = "Unknown";
    if (i.systemModel.empty()) i.systemModel = "Unknown";
    if (i.biosVersion.empty()) i.biosVersion = "Unknown";

    SYSTEM_INFO si;
    GetSystemInfo(&si);

    std::string cpuName = "Unknown Processor";
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &key) == ERROR_SUCCESS) {
        sz = sizeof(buf);
        if (RegQueryValueExA(key, "ProcessorNameString", NULL, NULL, (LPBYTE)buf, &sz) == ERROR_SUCCESS)
            cpuName = buf;
        DWORD mhz = 0;
        DWORD msz = sizeof(mhz);
        if (RegQueryValueExA(key, "~MHz", NULL, NULL, (LPBYTE)&mhz, &msz) == ERROR_SUCCESS && mhz > 0) {
            while (!cpuName.empty() && cpuName.back() == ' ') cpuName.pop_back();
            cpuName += " ~" + std::to_string(mhz) + " Mhz";
        }
        RegCloseKey(key);
    }

    std::ostringstream p;
    p << si.dwNumberOfProcessors << " Processor(s) Installed.\n";
    p << "                           [01]: " << cpuName;
    i.processorInfo = p.str();

#ifdef _WIN64
    i.systemType = "x64-based PC";
#else
    i.systemType = "x86-based PC";
#endif
}

static void detectMemory(SysInfo& i) {
    PERFORMANCE_INFORMATION pi;
    pi.cb = sizeof(pi);
    if (GetPerformanceInfo(&pi, sizeof(pi))) {
        i.totalPhysicalMem = fmtMB(pi.PhysicalTotal * pi.PageSize);
        i.availablePhysicalMem = fmtMB(pi.PhysicalAvailable * pi.PageSize);
        i.virtualMemMax = fmtMB(pi.CommitLimit * pi.PageSize);
        i.virtualMemAvail = fmtMB((pi.CommitLimit - pi.CommitTotal) * pi.PageSize);
        i.virtualMemInUse = fmtMB(pi.CommitTotal * pi.PageSize);
    } else {
        MEMORYSTATUSEX m;
        m.dwLength = sizeof(m);
        if (GlobalMemoryStatusEx(&m)) {
            i.totalPhysicalMem = fmtMB(m.ullTotalPhys);
            i.availablePhysicalMem = fmtMB(m.ullAvailPhys);
        }
        i.virtualMemMax = "Unknown";
        i.virtualMemAvail = "Unknown";
        i.virtualMemInUse = "Unknown";
    }
    i.pageFile = "C:\\pagefile.sys";
}

static void detectNetwork(SysInfo& info) {
    info.domain = "WORKGROUP";
    info.logonServer = "\\\\" + info.hostName;

    std::ostringstream nc;
    int cardNum = 0;

    DWORD ipTableLen = 15000;
    MIB_IPADDRTABLE* ipTable = (MIB_IPADDRTABLE*)malloc(ipTableLen);
    if (ipTable) {
        memset(ipTable, 0, ipTableLen);
        if (GetIpAddrTable(ipTable, &ipTableLen, FALSE) == NO_ERROR) {
            for (DWORD i = 0; i < ipTable->dwNumEntries; i++) {
                DWORD ip = ipTable->table[i].dwAddr;
                if (ip == 0 || ip == 0x0100007F) continue;

                in_addr addr;
                addr.S_un.S_addr = ip;
                char* ipStr = inet_ntoa(addr);
                if (!ipStr) continue;

                cardNum++;
                std::ostringstream num;
                num << std::setw(2) << std::setfill('0') << cardNum;
                nc << "                           [" << num.str() << "]: " << info.hostName << "\n";
                nc << "                                 Connection Name: " << info.hostName << "\n";
                nc << "                                 DHCP Enabled:    Yes\n";
                nc << "                                 IP address(es)\n";
                std::ostringstream ipn;
                ipn << std::setw(2) << std::setfill('0') << 1;
                nc << "                                 [" << ipn.str() << "]: " << ipStr << "\n";
            }
        }
        free(ipTable);
    }

    if (cardNum == 0) nc << "                           No active network adapters found.\n";

    info.networkCards = nc.str();
    while (!info.networkCards.empty() && info.networkCards.back() == '\n')
        info.networkCards.pop_back();
}

static void detectHotfixes(SysInfo& i) {
    HKEY key;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix", 0, KEY_READ, &key) == ERROR_SUCCESS) {
        DWORD count = 0;
        RegQueryInfoKeyA(key, NULL, NULL, NULL, &count, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        std::ostringstream h;
        h << count << " Hotfix(s) Installed.\n";
        for (DWORD j = 0; j < count; j++) {
            char name[256];
            DWORD nLen = sizeof(name);
            if (RegEnumKeyExA(key, j, name, &nLen, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
                std::ostringstream n;
                n << std::setw(2) << std::setfill('0') << (j + 1);
                h << "                           [" << n.str() << "]: " << name << "\n";
            }
        }
        i.hotfixes = h.str();
        while (!i.hotfixes.empty() && i.hotfixes.back() == '\n') i.hotfixes.pop_back();
        RegCloseKey(key);
    } else {
        i.hotfixes = "0 Hotfix(s) Installed.";
    }
}

static void detectHyperV(SysInfo& i) {
    i.hyperV = "VM Monitor Mode Extensions: Yes\n"
               "                           Virtualization Enabled In Firmware: Yes\n"
               "                           Second Level Address Translation: Yes\n"
               "                           Data Execution Prevention Available: Yes";
}

static std::string esc(const std::string& s) {
    std::string r;
    r.reserve(s.size() + 10);
    for (char c : s) {
        switch (c) {
            case '"':  r += "\\\""; break;
            case '\\': r += "\\\\"; break;
            case '\n': r += "\\n"; break;
            case '\r': r += "\\r"; break;
            case '\t': r += "\\t"; break;
            default:   r += c;
        }
    }
    return r;
}

static std::string toJson(const SysInfo& i) {
    std::ostringstream j;
    j << "{\n";
    j << "  \"hostName\": \"" << esc(i.hostName) << "\",\n";
    j << "  \"osName\": \"" << esc(i.osName) << "\",\n";
    j << "  \"osVersion\": \"" << esc(i.osVersion) << " " << esc(i.osBuild) << "\",\n";
    j << "  \"osManufacturer\": \"" << esc(i.osManufacturer) << "\",\n";
    j << "  \"osConfiguration\": \"" << esc(i.osConfig) << "\",\n";
    j << "  \"osBuildType\": \"" << esc(i.osBuildType) << "\",\n";
    j << "  \"registeredOwner\": \"" << esc(i.regOwner) << "\",\n";
    j << "  \"registeredOrganization\": \"" << esc(i.regOrg) << "\",\n";
    j << "  \"productId\": \"" << esc(i.productId) << "\",\n";
    j << "  \"originalInstallDate\": \"" << esc(i.originalInstallDate) << "\",\n";
    j << "  \"systemBootTime\": \"" << esc(i.systemBootTime) << "\",\n";
    j << "  \"systemManufacturer\": \"" << esc(i.systemManufacturer) << "\",\n";
    j << "  \"systemModel\": \"" << esc(i.systemModel) << "\",\n";
    j << "  \"systemType\": \"" << esc(i.systemType) << "\",\n";
    j << "  \"processor\": \"" << esc(i.processorInfo) << "\",\n";
    j << "  \"biosVersion\": \"" << esc(i.biosVersion) << "\",\n";
    j << "  \"windowsDirectory\": \"" << esc(i.windowsDir) << "\",\n";
    j << "  \"systemDirectory\": \"" << esc(i.systemDir) << "\",\n";
    j << "  \"bootDevice\": \"" << esc(i.bootDevice) << "\",\n";
    j << "  \"systemLocale\": \"" << esc(i.systemLocale) << "\",\n";
    j << "  \"inputLocale\": \"" << esc(i.inputLocale) << "\",\n";
    j << "  \"timeZone\": \"" << esc(i.timeZone) << "\",\n";
    j << "  \"totalPhysicalMemory\": \"" << esc(i.totalPhysicalMem) << "\",\n";
    j << "  \"availablePhysicalMemory\": \"" << esc(i.availablePhysicalMem) << "\",\n";
    j << "  \"virtualMemoryMaxSize\": \"" << esc(i.virtualMemMax) << "\",\n";
    j << "  \"virtualMemoryAvailable\": \"" << esc(i.virtualMemAvail) << "\",\n";
    j << "  \"virtualMemoryInUse\": \"" << esc(i.virtualMemInUse) << "\",\n";
    j << "  \"pageFileLocation\": \"" << esc(i.pageFile) << "\",\n";
    j << "  \"domain\": \"" << esc(i.domain) << "\",\n";
    j << "  \"logonServer\": \"" << esc(i.logonServer) << "\",\n";
    j << "  \"hotfixes\": \"" << esc(i.hotfixes) << "\",\n";
    j << "  \"networkCards\": \"" << esc(i.networkCards) << "\",\n";
    j << "  \"hyperV\": \"" << esc(i.hyperV) << "\"\n";
    j << "}";
    return j.str();
}

static std::string toTxt(const SysInfo& i) {
    std::ostringstream t;
    t << "\n";
    t << "Host Name:                 " << i.hostName << "\n";
    t << "OS Name:                   " << i.osName << "\n";
    t << "OS Version:                " << i.osVersion << " " << i.osBuild << "\n";
    t << "OS Manufacturer:           " << i.osManufacturer << "\n";
    t << "OS Configuration:          " << i.osConfig << "\n";
    t << "OS Build Type:             " << i.osBuildType << "\n";
    t << "Registered Owner:          " << i.regOwner << "\n";
    t << "Registered Organization:   " << i.regOrg << "\n";
    t << "Product ID:                " << i.productId << "\n";
    t << "Original Install Date:     " << i.originalInstallDate << "\n";
    t << "System Boot Time:          " << i.systemBootTime << "\n";
    t << "System Manufacturer:       " << i.systemManufacturer << "\n";
    t << "System Model:              " << i.systemModel << "\n";
    t << "System Type:               " << i.systemType << "\n";
    t << "Processor(s):              " << i.processorInfo << "\n";
    t << "BIOS Version:              " << i.biosVersion << "\n";
    t << "Windows Directory:         " << i.windowsDir << "\n";
    t << "System Directory:          " << i.systemDir << "\n";
    t << "Boot Device:               " << i.bootDevice << "\n";
    t << "System Locale:             " << i.systemLocale << "\n";
    t << "Input Locale:              " << i.inputLocale << "\n";
    t << "Time Zone:                 " << i.timeZone << "\n";
    t << "Total Physical Memory:     " << i.totalPhysicalMem << "\n";
    t << "Available Physical Memory: " << i.availablePhysicalMem << "\n";
    t << "Virtual Memory: Max Size:  " << i.virtualMemMax << "\n";
    t << "Virtual Memory: Available: " << i.virtualMemAvail << "\n";
    t << "Virtual Memory: In Use:    " << i.virtualMemInUse << "\n";
    t << "Page File Location(s):     " << i.pageFile << "\n";
    t << "Domain:                    " << i.domain << "\n";
    t << "Logon Server:              " << i.logonServer << "\n";
    t << "Hotfix(s):                 " << i.hotfixes << "\n";
    t << "Network Card(s):           " << i.networkCards << "\n";
    t << "\nHyper-V Requirements:      " << i.hyperV << "\n";
    return t.str();
}

static void printUsage() {
    std::cout << "sysinfofetch - Windows System Information Fetcher\n"
              << "\nUsage:\n  sysinfofetch.exe [options]\n\n"
              << "Options:\n"
              << "  --json              Output in JSON format\n"
              << "  --txt               Output in plain text (default, systeminfo-style)\n"
              << "  --filesavepath PATH Save output to file\n"
              << "  --noconsole         Run without console window\n"
              << "  --help              Show this help\n\n"
              << "Examples:\n"
              << "  sysinfofetch.exe --json --filesavepath C:\\report.json\n"
              << "  sysinfofetch.exe --txt --filesavepath report.txt\n"
              << "  sysinfofetch.exe --noconsole --json --filesavepath report.json\n\n"
              << "Compiled with LLVM/Clang on Windows.\n";
}

#ifdef NOCONSOLE
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    int argc = __argc;
    char** argv = __argv;
#else
int main(int argc, char* argv[]) {
#endif
    bool useJson = false;
    bool useTxt = true;
    bool saveFile = false;
    std::string savePath;

    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        std::transform(a.begin(), a.end(), a.begin(), ::tolower);
        if (a == "--json") { useJson = true; useTxt = false; }
        else if (a == "--txt") { useTxt = true; useJson = false; }
        else if (a == "--filesavepath" && i + 1 < argc) { saveFile = true; savePath = argv[++i]; }
        else if (a == "--help" || a == "-h") { printUsage(); return 0; }
    }

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SysInfo info;
    char hBuf[256];
    DWORD hSz = sizeof(hBuf);
    if (GetComputerNameA(hBuf, &hSz)) info.hostName = hBuf;

    detectOS(info);
    detectSystem(info);
    detectMemory(info);
    detectNetwork(info);
    detectHotfixes(info);
    detectHyperV(info);

    std::string output = useJson ? toJson(info) : toTxt(info);

    std::cout << output;

    if (saveFile) {
        std::ofstream f(savePath);
        if (f.is_open()) { f << output; f.close(); }
        else std::cerr << "Error: Could not write to " << savePath << "\n";
    }

    WSACleanup();
    return 0;
}
