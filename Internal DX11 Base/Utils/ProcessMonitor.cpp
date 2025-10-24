#include "pch.h"
#include "ProcessMonitor.h"
#include "CrashDump.h"
#include "Logger.h"
#include <tlhelp32.h>

namespace DX11Base
{
    ProcessMonitor::ProcessMonitor()
        : m_processId(0)
        , m_processHandle(nullptr)
        , m_monitoring(false)
        , m_shouldStop(false)
    {
    }

    ProcessMonitor::~ProcessMonitor()
    {
        StopMonitoring();
    }

    void ProcessMonitor::StartMonitoring(const std::string& processName)
    {
        if (m_monitoring.load())
        {
            Logger::Warning("Process monitoring is already active!");
            return;
        }
        
        m_processName = processName;
        m_shouldStop = false;
        
        if (FindAndOpenProcess())
        {
            m_monitoring = true;
            m_monitorThread = std::thread(&ProcessMonitor::MonitorThread, this);
            Logger::Log("Started monitoring process: %s (PID: %d)", 
                       m_processName.c_str(), m_processId);
        }
        else
        {
            Logger::Warning("Target process not found: %s. Will retry periodically.", 
                           m_processName.c_str());
            // Still start monitor thread to wait for process
            m_monitoring = true;
            m_monitorThread = std::thread(&ProcessMonitor::MonitorThread, this);
        }
    }

    void ProcessMonitor::StopMonitoring()
    {
        if (!m_monitoring.load())
            return;
        
        m_shouldStop = true;
        
        if (m_monitorThread.joinable())
        {
            m_monitorThread.join();
        }
        
        if (m_processHandle)
        {
            CloseHandle(m_processHandle);
            m_processHandle = nullptr;
        }
        
        m_monitoring = false;
        Logger::Log("Stopped monitoring process: %s", m_processName.c_str());
    }

    bool ProcessMonitor::IsProcessRunning()
    {
        if (!m_processHandle)
            return false;
        
        DWORD exitCode;
        if (GetExitCodeProcess(m_processHandle, &exitCode))
        {
            return exitCode == STILL_ACTIVE;
        }
        
        return false;
    }

    void ProcessMonitor::SetCrashCallback(std::function<void()> callback)
    {
        m_crashCallback = callback;
    }

    void ProcessMonitor::MonitorThread()
    {
        Logger::Log("Process monitor thread started for: %s", m_processName.c_str());
        
        while (!m_shouldStop.load())
        {
            try
            {
                CheckProcessStatus();
                std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // Check every second
            }
            catch (const std::exception& e)
            {
                Logger::Error("Exception in monitor thread: %s", e.what());
            }
            catch (...)
            {
                Logger::Error("Unknown exception in monitor thread");
            }
        }
        
        Logger::Log("Process monitor thread stopped");
    }

    bool ProcessMonitor::FindAndOpenProcess()
    {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE)
        {
            Logger::Error("Failed to create process snapshot. Error: %d", GetLastError());
            return false;
        }
        
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        
        bool found = false;
        if (Process32First(hSnapshot, &pe32))
        {
            do
            {
                // Convert WCHAR to char for comparison
                char processName[260];
                WideCharToMultiByte(CP_ACP, 0, pe32.szExeFile, -1, processName, sizeof(processName), NULL, NULL);
                if (_stricmp(processName, m_processName.c_str()) == 0)
                {
                    m_processId = pe32.th32ProcessID;
                    m_processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 
                                                 FALSE, m_processId);
                    if (m_processHandle)
                    {
                        found = true;
                        Logger::Log("Found and opened process: %s (PID: %d)", 
                                   m_processName.c_str(), m_processId);
                    }
                    else
                    {
                        Logger::Error("Failed to open process handle. Error: %d", GetLastError());
                    }
                    break;
                }
            } while (Process32Next(hSnapshot, &pe32));
        }
        
        CloseHandle(hSnapshot);
        return found;
    }

    void ProcessMonitor::CheckProcessStatus()
    {
        // If we still have no handle, try to find the process
        if (!m_processHandle)
        {
            if (FindAndOpenProcess())
            {
                Logger::Log("Target process found and monitoring started: %s", m_processName.c_str());
            }
            return;
        }
        
        // Check if process is still running
        if (!IsProcessRunning())
        {
            Logger::Error("Target process has crashed or terminated: %s (PID: %d)", 
                         m_processName.c_str(), m_processId);
            
            // Create crash dump
            std::string dumpPath = "CrashDumps\\" + m_processName + "_" + 
                                  CrashDump::GetCurrentDateTimeStringPublic() + "_ProcessTerminated.dmp";
            
            // Try to create a dump of the terminated process
            // (only works if process is still in memory)
            if (CrashDump::CreateProcessDump(m_processId, dumpPath))
            {
                Logger::Log("Process dump created: %s", dumpPath.c_str());
            }
            else
            {
                Logger::Warning("Could not create process dump - process may have already been cleaned up");
            }
            
            // Call callback
            if (m_crashCallback)
            {
                try
                {
                    m_crashCallback();
                }
                catch (...)
                {
                    Logger::Error("Exception in crash callback");
                }
            }
            
            // Close handle and reset
            CloseHandle(m_processHandle);
            m_processHandle = nullptr;
            m_processId = 0;
        }
    }
}
