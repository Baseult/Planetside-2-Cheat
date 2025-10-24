#pragma once
#include <Windows.h>
#include <string>
#include <thread>
#include <atomic>
#include <functional>

namespace DX11Base
{
    class ProcessMonitor
    {
    public:
        ProcessMonitor();
        ~ProcessMonitor();
        
        // Starts monitoring of target process
        void StartMonitoring(const std::string& processName);
        
        // Stops monitoring
        void StopMonitoring();
        
        // Checks if process is still running
        bool IsProcessRunning();
        
        // Returns current process ID
        DWORD GetProcessId() const { return m_processId; }
        
        // Sets callback for crash detection
        void SetCrashCallback(std::function<void()> callback);
        
    private:
        std::string m_processName;
        DWORD m_processId;
        HANDLE m_processHandle;
        std::atomic<bool> m_monitoring;
        std::atomic<bool> m_shouldStop;
        std::thread m_monitorThread;
        std::function<void()> m_crashCallback;
        
        // Thread function for monitoring
        void MonitorThread();
        
        // Finds process and opens handle
        bool FindAndOpenProcess();
        
        // Monitors process status
        void CheckProcessStatus();
    };
}
