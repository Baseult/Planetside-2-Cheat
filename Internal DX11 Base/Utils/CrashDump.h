#pragma once
#include <Windows.h>
#include <DbgHelp.h>
#include <string>
#include <memory>

#pragma comment(lib, "dbghelp.lib")

namespace DX11Base
{
    class CrashDump
    {
    public:
        // Initializes the crash dump system
        static PVOID Initialize();
        
        // Creates a crash dump for the current process
        static void CreateCrashDump(const std::string& reason = "Unknown");
        
        // Exception handler for unhandled exceptions
        static LONG WINAPI UnhandledExceptionFilter(EXCEPTION_POINTERS* pExceptionInfo);
        
        // Checks if target process (PlanetSide 2) is still running
        static bool IsTargetProcessRunning();
        
        // Creates a dump for an external process
        static bool CreateProcessDump(DWORD processId, const std::string& dumpPath);
        
        // Sets target process name
        static void SetTargetProcessName(const std::string& processName);
        
        // Aktiviert/Deaktiviert automatische Crash-Erkennung
        static void SetAutoCrashDetection(bool enabled);
        
    private:
        static std::string s_targetProcessName;
        static std::string s_dumpDirectory;
        static bool s_autoCrashDetection;
        static HANDLE s_targetProcessHandle;
        static DWORD s_targetProcessId;
        
        // Interne Hilfsfunktionen
        static std::string GetCurrentDateTimeString();
        
    public:
        // Public function for external access
        static std::string GetCurrentDateTimeStringPublic();
        static std::string GetDumpFileName(const std::string& reason);
        static bool CreateDirectoryIfNotExists(const std::string& path);
        static void LogCrashInfo(const std::string& reason, const std::string& dumpPath);
        
        // Enhanced crash analysis functions
        static void PerformEnhancedCrashAnalysis(const std::string& reason, const std::string& dumpPath);
        static void AnalyzeDLLInvolvement();
        static void AnalyzeHookStatus();
        static void AnalyzeThreadStates();
        static void AnalyzeMemoryCorruption();
        static void CheckAntiCheatDetection();
        static void LogSystemState();
    };
}
