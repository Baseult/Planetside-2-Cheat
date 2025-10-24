#include "pch.h"
#include "CrashDump.h"
#include "Logger.h"
#include "../Engine.h"
#include "../Features/MagicBullet.h"
#include <filesystem>
#include <tlhelp32.h>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

namespace DX11Base
{
	// Static variables
    std::string CrashDump::s_targetProcessName = "Planetside2_x64.exe";
    std::string CrashDump::s_dumpDirectory = "CrashDumps";
    bool CrashDump::s_autoCrashDetection = true;
    HANDLE CrashDump::s_targetProcessHandle = nullptr;
    DWORD CrashDump::s_targetProcessId = 0;

    PVOID CrashDump::Initialize()
    {
        Logger::Log("Initializing CrashDump system...");
        
        // Find target process
        if (IsTargetProcessRunning())
        {
            Logger::Log("Target process %s found with PID: %d", s_targetProcessName.c_str(), s_targetProcessId);
        }
        else
        {
            Logger::Warning("Target process %s not found. Will monitor for it.", s_targetProcessName.c_str());
        }
        
        // Create dump directory
        if (CreateDirectoryIfNotExists(s_dumpDirectory))
        {
            Logger::Log("CrashDump directory created/verified: %s", s_dumpDirectory.c_str());
        }
        else
        {
            Logger::Error("Failed to create CrashDump directory: %s", s_dumpDirectory.c_str());
        }
        
        // Set unhandled exception filter
        SetUnhandledExceptionFilter(UnhandledExceptionFilter);
        
        // Vectored Exception Handler for better crash detection
        PVOID vehHandler = AddVectoredExceptionHandler(1, [](PEXCEPTION_POINTERS pExceptionInfo) -> LONG
        {
            // Enhanced exception detection
            DWORD exceptionCode = pExceptionInfo->ExceptionRecord->ExceptionCode;
            
            if (exceptionCode == EXCEPTION_ACCESS_VIOLATION ||
                exceptionCode == EXCEPTION_STACK_OVERFLOW ||
                exceptionCode == EXCEPTION_INT_DIVIDE_BY_ZERO ||
                exceptionCode == EXCEPTION_BREAKPOINT)
            {
                Logger::Error("Critical exception detected! Code: 0x%08X, Address: 0x%p", 
                             exceptionCode, pExceptionInfo->ExceptionRecord->ExceptionAddress);
                
                // Check if exception is in our DLL
                HMODULE hOurDLL = GetModuleHandleA("Internal DX11 Base.dll");
                if (hOurDLL)
                {
                    MODULEINFO modInfo;
                    if (GetModuleInformation(GetCurrentProcess(), hOurDLL, &modInfo, sizeof(modInfo)))
                    {
                        PVOID exceptionAddr = pExceptionInfo->ExceptionRecord->ExceptionAddress;
                        if (exceptionAddr >= modInfo.lpBaseOfDll && 
                            exceptionAddr < (char*)modInfo.lpBaseOfDll + modInfo.SizeOfImage)
                        {
                            Logger::Error("⚠ EXCEPTION IN OUR DLL! This is likely our fault!");
                            CreateCrashDump("DLL Internal Exception");
                        }
                        else
                        {
                            Logger::Error("Exception in external code, analyzing...");
                            CreateCrashDump("External Exception");
                        }
                    }
                }
                else
                {
                    Logger::Error("Our DLL not found during exception!");
                    CreateCrashDump("DLL Missing Exception");
                }
                
                return EXCEPTION_EXECUTE_HANDLER;
            }
            return EXCEPTION_CONTINUE_SEARCH;
        });
        
        Logger::Log("CrashDump system initialized successfully!");
        return vehHandler;
    }

    void CrashDump::CreateCrashDump(const std::string& reason)
    {
        try
        {
            std::string dumpPath = GetDumpFileName(reason);
            
            HANDLE hFile = CreateFileA(
                dumpPath.c_str(),
                GENERIC_WRITE,
                0,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL
            );
            
            if (hFile == INVALID_HANDLE_VALUE)
            {
                Logger::Error("Failed to create crash dump file: %s", dumpPath.c_str());
                return;
            }
            
            MINIDUMP_EXCEPTION_INFORMATION mdei;
            mdei.ThreadId = GetCurrentThreadId();
            mdei.ExceptionPointers = nullptr;
            mdei.ClientPointers = FALSE;
            
            // Create complete MiniDump with enhanced information
            BOOL success = MiniDumpWriteDump(
                GetCurrentProcess(),
                GetCurrentProcessId(),
                hFile,
                static_cast<MINIDUMP_TYPE>(MiniDumpWithFullMemory | 
                MiniDumpWithHandleData | 
                MiniDumpWithUnloadedModules |
                MiniDumpWithIndirectlyReferencedMemory |
                MiniDumpWithProcessThreadData |
                MiniDumpWithPrivateReadWriteMemory |
                MiniDumpWithThreadInfo),
                &mdei,
                NULL,
                NULL
            );
            
            CloseHandle(hFile);
            
            if (success)
            {
                Logger::Log("Crash dump created successfully: %s", dumpPath.c_str());
                LogCrashInfo(reason, dumpPath);
                
                // Perform enhanced analysis
                PerformEnhancedCrashAnalysis(reason, dumpPath);
            }
            else
            {
                Logger::Error("Failed to write crash dump. Error: %d", GetLastError());
            }
        }
        catch (...)
        {
            Logger::Error("Exception occurred while creating crash dump!");
        }
    }

    LONG WINAPI CrashDump::UnhandledExceptionFilter(EXCEPTION_POINTERS* pExceptionInfo)
    {
        Logger::Error("Unhandled exception detected! Exception Code: 0x%08X", 
                     pExceptionInfo->ExceptionRecord->ExceptionCode);
        
        // Log detailed exception information
        switch (pExceptionInfo->ExceptionRecord->ExceptionCode)
        {
            case EXCEPTION_ACCESS_VIOLATION:
                Logger::Error("Access Violation at address: 0x%p", 
                             pExceptionInfo->ExceptionRecord->ExceptionAddress);
                break;
            case EXCEPTION_STACK_OVERFLOW:
                Logger::Error("Stack Overflow detected");
                break;
            case EXCEPTION_INT_DIVIDE_BY_ZERO:
                Logger::Error("Integer Division by Zero");
                break;
            case EXCEPTION_FLT_DIVIDE_BY_ZERO:
                Logger::Error("Floating Point Division by Zero");
                break;
            case EXCEPTION_BREAKPOINT:
                Logger::Error("Breakpoint Exception (0x80000003) - Possible anti-cheat detection or hook failure");
                break;
            default:
                Logger::Error("Unknown exception type: 0x%08X", pExceptionInfo->ExceptionRecord->ExceptionCode);
                break;
        }
        
        // Enhanced analysis before creating dump
        Logger::Error("Exception occurred in thread: %d", GetCurrentThreadId());
        Logger::Error("Exception flags: 0x%08X", pExceptionInfo->ExceptionRecord->ExceptionFlags);
        
        CreateCrashDump("Unhandled Exception");
        
        // Disable standard Windows Error Reporting
        return EXCEPTION_EXECUTE_HANDLER;
    }

    bool CrashDump::IsTargetProcessRunning()
    {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE)
        {
            return false;
        }
        
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        
        if (Process32First(hSnapshot, &pe32))
        {
            do
            {
                // Convert WCHAR to char for comparison
                char processName[260];
                WideCharToMultiByte(CP_ACP, 0, pe32.szExeFile, -1, processName, sizeof(processName), NULL, NULL);
                if (_stricmp(processName, s_targetProcessName.c_str()) == 0)
                {
                    s_targetProcessId = pe32.th32ProcessID;
                    s_targetProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, s_targetProcessId);
                    CloseHandle(hSnapshot);
                    return true;
                }
            } while (Process32Next(hSnapshot, &pe32));
        }
        
        CloseHandle(hSnapshot);
        return false;
    }

    bool CrashDump::CreateProcessDump(DWORD processId, const std::string& dumpPath)
    {
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
        if (!hProcess)
        {
            Logger::Error("Failed to open target process. Error: %d", GetLastError());
            return false;
        }
        
        HANDLE hFile = CreateFileA(
            dumpPath.c_str(),
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
        
        if (hFile == INVALID_HANDLE_VALUE)
        {
            CloseHandle(hProcess);
            Logger::Error("Failed to create dump file: %s", dumpPath.c_str());
            return false;
        }
        
        BOOL success = MiniDumpWriteDump(
            hProcess,
            processId,
            hFile,
            static_cast<MINIDUMP_TYPE>(MiniDumpWithFullMemory | 
            MiniDumpWithHandleData | 
            MiniDumpWithUnloadedModules |
            MiniDumpWithIndirectlyReferencedMemory |
            MiniDumpWithProcessThreadData |
            MiniDumpWithPrivateReadWriteMemory),
            NULL,
            NULL,
            NULL
        );
        
        CloseHandle(hFile);
        CloseHandle(hProcess);
        
        if (success)
        {
            Logger::Log("Process dump created successfully: %s", dumpPath.c_str());
            return true;
        }
        else
        {
            Logger::Error("Failed to write process dump. Error: %d", GetLastError());
            return false;
        }
    }

    void CrashDump::SetTargetProcessName(const std::string& processName)
    {
        s_targetProcessName = processName;
        Logger::Log("Target process name set to: %s", processName.c_str());
    }

    void CrashDump::SetAutoCrashDetection(bool enabled)
    {
        s_autoCrashDetection = enabled;
        Logger::Log("Auto crash detection %s", enabled ? "enabled" : "disabled");
    }

    std::string CrashDump::GetCurrentDateTimeString()
    {
        SYSTEMTIME st;
        GetLocalTime(&st);
        
        char buffer[64];
        sprintf_s(buffer, sizeof(buffer), "%04d-%02d-%02d_%02d-%02d-%02d",
                  st.wYear, st.wMonth, st.wDay,
                  st.wHour, st.wMinute, st.wSecond);
        
        return std::string(buffer);
    }

    std::string CrashDump::GetDumpFileName(const std::string& reason)
    {
        std::string timestamp = GetCurrentDateTimeString();
        std::string filename = s_dumpDirectory + "\\" + s_targetProcessName + "_" + 
                              timestamp + "_" + reason + ".dmp";
        
        // Remove invalid characters from filename
        std::replace(filename.begin(), filename.end(), ' ', '_');
        std::replace(filename.begin(), filename.end(), ':', '-');
        
        return filename;
    }

    bool CrashDump::CreateDirectoryIfNotExists(const std::string& path)
    {
        try
        {
            return std::filesystem::create_directories(path);
        }
        catch (...)
        {
            return false;
        }
    }

    void CrashDump::LogCrashInfo(const std::string& reason, const std::string& dumpPath)
    {
        // Write additional information to a log file
        std::string logPath = dumpPath.substr(0, dumpPath.find_last_of('.')) + "_info.txt";
        
        HANDLE hLogFile = CreateFileA(
            logPath.c_str(),
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
        
        if (hLogFile != INVALID_HANDLE_VALUE)
        {
            std::string logContent = "=== ENHANCED CRASH DUMP INFORMATION ===\n";
            logContent += "Timestamp: " + GetCurrentDateTimeString() + "\n";
            logContent += "Target Process: " + s_targetProcessName + "\n";
            logContent += "Process ID: " + std::to_string(GetCurrentProcessId()) + "\n";
            logContent += "Thread ID: " + std::to_string(GetCurrentThreadId()) + "\n";
            logContent += "Reason: " + reason + "\n";
            logContent += "Dump File: " + dumpPath + "\n\n";
            
            // Add DLL status
            HMODULE hOurDLL = GetModuleHandleA("Internal DX11 Base.dll");
            if (hOurDLL)
            {
                MODULEINFO modInfo;
                if (GetModuleInformation(GetCurrentProcess(), hOurDLL, &modInfo, sizeof(modInfo)))
                {
                    logContent += "=== DLL STATUS ===\n";
                    logContent += "Internal DX11 Base.dll: LOADED\n";
                    logContent += "Base Address: 0x" + std::to_string(reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll)) + "\n";
                    logContent += "Size: 0x" + std::to_string(modInfo.SizeOfImage) + " bytes\n";
                    logContent += "Entry Point: 0x" + std::to_string(reinterpret_cast<uintptr_t>(modInfo.EntryPoint)) + "\n\n";
                }
            }
            else
            {
                logContent += "=== DLL STATUS ===\n";
                logContent += "Internal DX11 Base.dll: NOT LOADED (CRITICAL!)\n\n";
            }
            
            // Add hook status
            logContent += "=== HOOK STATUS ===\n";
            
            logContent += "g_Running: " + std::string(DX11Base::g_Running ? "true" : "false") + "\n";
            logContent += "g_Engine: " + std::string(DX11Base::g_Engine ? "valid" : "NULL") + "\n";
            logContent += "g_D3D11Window: " + std::string(DX11Base::g_D3D11Window ? "valid" : "NULL") + "\n";
            logContent += "g_MagicBullet: " + std::string(g_MagicBullet ? "valid" : "NULL") + "\n";
            
            if (DX11Base::g_D3D11Window)
            {
                logContent += "D3D11Window->bInit: " + std::string(DX11Base::g_D3D11Window->bInit ? "true" : "false") + "\n";
                logContent += "D3D11Window->bInitImGui: " + std::string(DX11Base::g_D3D11Window->bInitImGui ? "true" : "false") + "\n";
                logContent += "D3D11Window->m_OldWndProc: 0x" + std::to_string(reinterpret_cast<uintptr_t>(DX11Base::g_D3D11Window->m_OldWndProc)) + "\n";
            }
            
            if (DX11Base::g_Engine)
            {
                logContent += "Engine->pGameWindow: 0x" + std::to_string(reinterpret_cast<uintptr_t>(DX11Base::g_Engine->pGameWindow)) + "\n";
                logContent += "Engine->bShowMenu: " + std::string(DX11Base::g_Engine->bShowMenu ? "true" : "false") + "\n";
            }
            logContent += "\n";
            
            // Add system info
            logContent += "=== SYSTEM INFO ===\n";
            SYSTEM_INFO sysInfo;
            GetSystemInfo(&sysInfo);
            logContent += "Processor Count: " + std::to_string(sysInfo.dwNumberOfProcessors) + "\n";
            logContent += "Page Size: 0x" + std::to_string(sysInfo.dwPageSize) + "\n";
            
            MEMORYSTATUSEX memStatus;
            memStatus.dwLength = sizeof(memStatus);
            if (GlobalMemoryStatusEx(&memStatus))
            {
                logContent += "Memory Load: " + std::to_string(memStatus.dwMemoryLoad) + "%\n";
                logContent += "Available Physical: " + std::to_string(memStatus.ullAvailPhys / (1024 * 1024)) + " MB\n";
            }
            logContent += "\n";
            
            // Add anti-cheat detection
            logContent += "=== ANTI-CHEAT DETECTION ===\n";
            const char* suspiciousModules[] = {
                "BattlEye.dll", "EasyAntiCheat.dll", "VAC.dll", "VAC3.dll", "nProtect.dll", "GameGuard.dll"
            };
            
            bool foundAntiCheat = false;
            for (const char* moduleName : suspiciousModules)
            {
                HMODULE hModule = GetModuleHandleA(moduleName);
                if (hModule)
                {
                    logContent += "DETECTED: " + std::string(moduleName) + " at 0x" + 
                                std::to_string(reinterpret_cast<uintptr_t>(hModule)) + "\n";
                    foundAntiCheat = true;
                }
            }
            
            if (!foundAntiCheat)
            {
                logContent += "No known anti-cheat modules detected\n";
            }
            
            logContent += "Debugger Present: " + std::string(IsDebuggerPresent() ? "YES" : "NO") + "\n";
            logContent += "================================\n";
            
            DWORD bytesWritten;
            WriteFile(hLogFile, logContent.c_str(), static_cast<DWORD>(logContent.length()), &bytesWritten, NULL);
            CloseHandle(hLogFile);
            
            Logger::Log("Enhanced crash information logged to: %s", logPath.c_str());
        }
    }

    std::string CrashDump::GetCurrentDateTimeStringPublic()
    {
        return GetCurrentDateTimeString();
    }

    void CrashDump::PerformEnhancedCrashAnalysis(const std::string& reason, const std::string& dumpPath)
    {
        Logger::Log("=== ENHANCED CRASH ANALYSIS STARTED ===");
        Logger::Log("Analyzing crash: %s", reason.c_str());
        
        // Perform comprehensive analysis
        AnalyzeDLLInvolvement();
        AnalyzeHookStatus();
        AnalyzeThreadStates();
        AnalyzeMemoryCorruption();
        CheckAntiCheatDetection();
        LogSystemState();
        
        Logger::Log("=== ENHANCED CRASH ANALYSIS COMPLETED ===");
    }

    void CrashDump::AnalyzeDLLInvolvement()
    {
        Logger::Log("--- Analyzing DLL Involvement ---");
        
        // Check if our DLL is loaded
        HMODULE hModule = GetModuleHandleA("Internal DX11 Base.dll");
        if (hModule)
        {
            Logger::Log("✓ Internal DX11 Base.dll is loaded at: 0x%p", hModule);
            
            // Get module info
            MODULEINFO modInfo;
            if (GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(modInfo)))
            {
                Logger::Log("  - Base Address: 0x%p", modInfo.lpBaseOfDll);
                Logger::Log("  - Size: 0x%X bytes", modInfo.SizeOfImage);
                Logger::Log("  - Entry Point: 0x%p", modInfo.EntryPoint);
            }
        }
        else
        {
            Logger::Error("✗ Internal DX11 Base.dll is NOT loaded!");
        }
        
        // Check for other suspicious modules
        HMODULE hKiero = GetModuleHandleA("kiero.dll");
        if (hKiero)
        {
            Logger::Log("⚠ Kiero.dll detected at: 0x%p", hKiero);
        }
        
        // Check MinHook status
        // Note: MH_IsInitialized() requires MinHook header, simplified check
        Logger::Log("MinHook status check skipped (requires MinHook header)");
    }

    void CrashDump::AnalyzeHookStatus()
    {
        Logger::Log("--- Analyzing Hook Status ---");
        
        // Check D3D11Window status
        if (DX11Base::g_D3D11Window)
        {
            Logger::Log("✓ D3D11Window exists");
            Logger::Log("  - bInit: %s", DX11Base::g_D3D11Window->bInit ? "true" : "false");
            Logger::Log("  - bInitImGui: %s", DX11Base::g_D3D11Window->bInitImGui ? "true" : "false");
            Logger::Log("  - m_OldWndProc: 0x%p", DX11Base::g_D3D11Window->m_OldWndProc);
        }
        else
        {
            Logger::Error("✗ D3D11Window is NULL!");
        }
        
        // Check Engine status
        if (DX11Base::g_Engine)
        {
            Logger::Log("✓ Engine exists");
            Logger::Log("  - pGameWindow: 0x%p", DX11Base::g_Engine->pGameWindow);
            Logger::Log("  - bShowMenu: %s", DX11Base::g_Engine->bShowMenu ? "true" : "false");
        }
        else
        {
            Logger::Error("✗ Engine is NULL!");
        }
        
        // Check MagicBullet status
        if (g_MagicBullet)
        {
            Logger::Log("✓ MagicBullet exists (potential crash source)");
        }
        else
        {
            Logger::Log("✗ MagicBullet is NULL");
        }
    }

    void CrashDump::AnalyzeThreadStates()
    {
        Logger::Log("--- Analyzing Thread States ---");
        
        // Get current thread info
        DWORD currentThreadId = GetCurrentThreadId();
        Logger::Log("Current Thread ID: %d", currentThreadId);
        
        // Check if our data thread is still running
        extern std::thread dataThread;
        Logger::Log("g_Running: %s", DX11Base::g_Running ? "true" : "false");
        
        if (dataThread.joinable())
        {
            Logger::Log("✓ GameDataThread is still joinable");
        }
        else
        {
            Logger::Log("✗ GameDataThread is not joinable (may have crashed)");
        }
        
        // Count total threads
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (hSnapshot != INVALID_HANDLE_VALUE)
        {
            THREADENTRY32 te32;
            te32.dwSize = sizeof(THREADENTRY32);
            
            int threadCount = 0;
            if (Thread32First(hSnapshot, &te32))
            {
                do
                {
                    if (te32.th32OwnerProcessID == GetCurrentProcessId())
                    {
                        threadCount++;
                    }
                } while (Thread32Next(hSnapshot, &te32));
            }
            
            Logger::Log("Total threads in process: %d", threadCount);
            CloseHandle(hSnapshot);
        }
    }

    void CrashDump::AnalyzeMemoryCorruption()
    {
        Logger::Log("--- Analyzing Memory Corruption ---");
        
        // Check for heap corruption
        HANDLE hHeap = GetProcessHeap();
        if (hHeap)
        {
            ULONG heapFlags = 0;
            SIZE_T heapSize = 0;
            
            if (HeapQueryInformation(hHeap, HeapCompatibilityInformation, &heapFlags, sizeof(heapFlags), &heapSize))
            {
                Logger::Log("Heap compatibility: %d", heapFlags);
            }
        }
        
        // Check for stack overflow indicators
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(&mbi, &mbi, sizeof(mbi)) != 0)
        {
            Logger::Log("Stack region: 0x%p - 0x%p", mbi.BaseAddress, (char*)mbi.BaseAddress + mbi.RegionSize);
        }
        
        // Check critical global variables
        Logger::Log("Critical globals status:");
        Logger::Log("  - g_Running: %s (0x%p)", DX11Base::g_Running ? "true" : "false", &DX11Base::g_Running);
        Logger::Log("  - g_Engine: %s (0x%p)", DX11Base::g_Engine ? "valid" : "NULL", &DX11Base::g_Engine);
        Logger::Log("  - g_D3D11Window: %s (0x%p)", DX11Base::g_D3D11Window ? "valid" : "NULL", &DX11Base::g_D3D11Window);
    }

    void CrashDump::CheckAntiCheatDetection()
    {
        Logger::Log("--- Checking Anti-Cheat Detection ---");
        
        // Check for common anti-cheat modules
        const char* suspiciousModules[] = {
            "BattlEye.dll",
            "EasyAntiCheat.dll", 
            "VAC.dll",
            "VAC3.dll",
            "nProtect.dll",
            "GameGuard.dll"
        };
        
        for (const char* moduleName : suspiciousModules)
        {
            HMODULE hModule = GetModuleHandleA(moduleName);
            if (hModule)
            {
                Logger::Log("⚠ Anti-cheat module detected: %s at 0x%p", moduleName, hModule);
            }
        }
        
        // Check for debugging flags
        BOOL isDebuggerPresent = IsDebuggerPresent();
        Logger::Log("Debugger present: %s", isDebuggerPresent ? "YES" : "NO");
        
        // Check for remote debugger
        BOOL isRemoteDebuggerPresent = FALSE;
        CheckRemoteDebuggerPresent(GetCurrentProcess(), &isRemoteDebuggerPresent);
        Logger::Log("Remote debugger present: %s", isRemoteDebuggerPresent ? "YES" : "NO");
        
        // Check NtGlobalFlag (anti-debugging) - simplified version
        // Note: PEB access requires specific system includes and may not be available in all contexts
        Logger::Log("NtGlobalFlag check skipped (requires specific system includes)");
    }

    void CrashDump::LogSystemState()
    {
        Logger::Log("--- System State Information ---");
        
        // System info
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        Logger::Log("Processor count: %d", sysInfo.dwNumberOfProcessors);
        Logger::Log("Page size: 0x%X", sysInfo.dwPageSize);
        
        // Memory info
        MEMORYSTATUSEX memStatus;
        memStatus.dwLength = sizeof(memStatus);
        if (GlobalMemoryStatusEx(&memStatus))
        {
            Logger::Log("Memory load: %d%%", memStatus.dwMemoryLoad);
            Logger::Log("Total physical: %llu MB", memStatus.ullTotalPhys / (1024 * 1024));
            Logger::Log("Available physical: %llu MB", memStatus.ullAvailPhys / (1024 * 1024));
        }
        
        // Process info
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        {
            Logger::Log("Process memory usage:");
            Logger::Log("  - Working set: %zu KB", pmc.WorkingSetSize / 1024);
            Logger::Log("  - Peak working set: %zu KB", pmc.PeakWorkingSetSize / 1024);
            Logger::Log("  - Page file usage: %zu KB", pmc.PagefileUsage / 1024);
        }
        
        // Check for recent DLL unloads
        Logger::Log("Recent DLL activity logged in crash info file");
    }
}
