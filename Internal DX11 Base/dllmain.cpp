#include "pch.h"
#include "helper.h"
#include "Utils/Logger.h"
#include "Features/Aimbot.h"
#include "Features/NoRecoil.h"

// Global flag to track if cleanup is in progress
static std::atomic<bool> g_cleanupInProgress = false;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  dwCallReason, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(lpReserved);
    
    switch (dwCallReason)
    {
    case DLL_PROCESS_ATTACH:
    {
        DX11Base::g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);
        
        // Initialize cleanup flag
        g_cleanupInProgress = false;
        
        HANDLE hThread = CreateThread(0, 0, MainThread_Initialize, DX11Base::g_hModule, 0, 0);
        if (hThread)
            CloseHandle(hThread);
        break;
    }
    
    case DLL_PROCESS_DETACH:
    {
        // Prevent multiple cleanup attempts
        if (g_cleanupInProgress.exchange(true)) {
            return TRUE;
        }
        
        Logger::Log("DLL_PROCESS_DETACH: Starting emergency cleanup...");
        
        // Set global cleanup flag to prevent ESP rendering during unload
        DX11Base::g_CleanupInProgress = true;
        
        // Signal all threads to stop immediately
        if (DX11Base::g_Running) {
            DX11Base::g_Running = false;
        }
        
        // Emergency cleanup - minimal time allowed
        try {
            // Force stop Aimbot and NoRecoil threads immediately
            // These are the most likely to cause crashes during unload
            if (g_Aimbot) {
                g_Aimbot.reset();
            }
            if (g_NoRecoil) {
                g_NoRecoil.reset();
            }
            
            // Minimal wait for threads to terminate
            Sleep(100);
            
            // Clear any remaining global state
            Logger::Log("DLL_PROCESS_DETACH: Emergency cleanup completed");
        }
        catch (...) {
            Logger::Log("DLL_PROCESS_DETACH: Exception during emergency cleanup");
        }
        break;
    }
    
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        // Nothing to do for thread attach/detach
        break;
    }

    return TRUE;
}

