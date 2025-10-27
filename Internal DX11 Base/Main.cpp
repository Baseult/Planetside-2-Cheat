#include "pch.h"
#include "Engine.h"
#include "Menu.h"
#include "Utils/Logger.h"
#include "Utils/CrashDump.h"
#include "Utils/ProcessMonitor.h"
#include "Game/Game.h"
#include "Game/BulletHarvester.h"
#include "Renderer/Renderer.h"
#include "Features/ESP.h"
#include "Features/MagicBullet.h"
#include "Features/TargetManager.h"
#include "Features/Misc.h"
#include "Features/Noclip.h"
#include "Features/NoRecoil.h"
#include "Features/Aimbot.h"
#include "Hooking/hookmain.h"
#include "Hooking/MinHook.h"
#include "Utils/SettingsManager.h"

using namespace DX11Base;

// Global thread variable so shutdown function can access it
std::thread dataThread;

// Global Process Monitor instance
std::unique_ptr<ProcessMonitor> g_ProcessMonitor;

// VEH handle for clean removal during shutdown
PVOID g_pVehHandler = nullptr;

// GameDataThread - The heart of performance
void GameDataThread()
{
    Logger::Log("---> GameDataThread has entered its loop.");

    // ================================================================
    // =================== START-UP SYNC FIX =========================
    // ================================================================
    int startup_tries = 0;
    while (!g_Running) {
        if (++startup_tries > 100) { // Timeout after approximately 1 second
            Logger::Error("GameDataThread timed out waiting for g_Running.");
            return;
        }
        std::this_thread::yield();
    }
    Logger::Log("GameDataThread: g_Running confirmed, starting main loop.");
    // ================================================================

    while (g_Running)
    {
        // Check if all objects still exist (prevents crashes during shutdown)
        if (g_Game && g_MagicBullet && g_Misc && g_Noclip && g_NoRecoil && g_Aimbot) {
            g_Game->Update();
            // MagicBullet now runs in its own thread for better performance
            g_Misc->Update();
            g_Noclip->Update();
            g_NoRecoil->Update();
            g_Aimbot->Update();
        }
        else {
            // If object no longer exists, terminate thread
            Logger::Log("GameDataThread: One or more objects destroyed, exiting...");
            break;
        }

        std::this_thread::yield();
    }
    Logger::Log("---> GameDataThread has exited its loop.");
}

// Central shutdown function for clean cleanup
void Shutdown()
{
    Logger::Log("====== SHUTDOWN SEQUENCE INITIATED ======");

    // 1. Signal all internal threads to terminate
    g_Running = false;
    Logger::Log("--> g_Running set to false, signaling all threads to stop");

    // 2. Wait for main data thread to terminate cleanly
    if (dataThread.joinable()) {
        Logger::Log("--> Waiting for GameDataThread to join...");
        dataThread.join();
        Logger::Log("... GameDataThread joined successfully.");
    }

    // 3. Wait for all feature threads
    Logger::Log("--> Waiting for feature threads to stop...");
    
    // Stop Aimbot and NoRecoil threads first (they use NtUserInjectMouseInput)
    if (g_Aimbot) {
        Logger::Log("--> Stopping Aimbot threads...");
        g_Aimbot.reset();
        Logger::Log("... Aimbot threads stopped.");
    }
    
    if (g_NoRecoil) {
        Logger::Log("--> Stopping NoRecoil threads...");
        g_NoRecoil.reset();
        Logger::Log("... NoRecoil threads stopped.");
    }
    
    // Wait for threads to actually terminate
    std::this_thread::sleep_for(500ms);
    
    // Force stop any remaining threads by destroying objects
    if (g_MagicBullet) {
        Logger::Log("--> Stopping MagicBullet threads...");
        g_MagicBullet.reset();
        Logger::Log("... MagicBullet threads stopped.");
    }
    
    if (g_TargetManager) {
        Logger::Log("--> Stopping TargetManager threads...");
        g_TargetManager.reset();
        Logger::Log("... TargetManager threads stopped.");
    }
    
    std::this_thread::sleep_for(500ms);
    
    Logger::Log("--> Final thread cleanup check...");
    std::this_thread::sleep_for(100ms);

    // Remove hooks and UI context BEFORE destroying objects

    // Restore original Window Procedure (CRITICAL!)
    if (g_D3D11Window && g_D3D11Window->m_OldWndProc)
    {
        Logger::Log("--> Restoring original WndProc...");
        SetWindowLongPtr(g_Engine->pGameWindow, GWLP_WNDPROC, (LONG_PTR)g_D3D11Window->m_OldWndProc);
        g_D3D11Window->m_OldWndProc = nullptr;
        Logger::Log("... Original WndProc restored.");
    }

    // Remove all DirectX hooks
    if (g_D3D11Window)
    {
        Logger::Log("--> Unhooking D3D...");
        g_D3D11Window->UnhookD3D();
        Logger::Log("... D3D unhooked.");
    }

    // Shutdown ImGui cleanly
    if (g_D3D11Window && g_D3D11Window->bInitImGui)
    {
        Logger::Log("--> Shutting down ImGui...");
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        Logger::Log("... ImGui context destroyed.");
    }

    // Remove Vectored Exception Handler (prevents delayed crash)
    if (g_pVehHandler)
    {
        Logger::Log("--> Removing Vectored Exception Handler...");
        if (RemoveVectoredExceptionHandler(g_pVehHandler))
        {
            Logger::Log("... Vectored Exception Handler removed successfully.");
        }
        else
        {
            Logger::Error("... FAILED to remove Vectored Exception Handler!");
        }
        g_pVehHandler = nullptr;
    }

    // Stop process monitoring
    if (g_ProcessMonitor) {
        Logger::Log("--> Stopping process monitor...");
        g_ProcessMonitor->StopMonitoring();
        g_ProcessMonitor.reset();
        Logger::Log("... Process monitor stopped.");
    }
    
    // Stop BulletHarvester threads with timeout
    if (g_BulletHarvester) {
        Logger::Log("--> Stopping BulletHarvester threads...");
        g_BulletHarvester.reset();
        Logger::Log("... BulletHarvester threads stopped.");
    }

    // Explicitly destroy remaining global feature objects (in reverse order of creation)
    Logger::Log("--> Destroying remaining feature classes...");
    
    // Aimbot and NoRecoil were already destroyed earlier to stop their threads first
    
    delete g_Noclip;
    g_Noclip = nullptr;
    Logger::Log("Noclip destroyed.");
    
    g_Misc.reset();
    Logger::Log("Misc destroyed.");
    
    g_ESP.reset();
    Logger::Log("ESP destroyed.");
    
    // MagicBullet and TargetManager were already destroyed earlier
    
    g_Renderer.reset();
    g_Game.reset();
    g_Engine.reset();
    Logger::Log("... All classes destroyed.");

    // Longer pause to ensure all threads terminate properly
    Logger::Log("Waiting for threads to finish...");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Deinitialize MinHook second to last
    Logger::Log("--> Uninitializing MinHook...");
    MH_Uninitialize();
    
    // Final cleanup - ensure all resources are freed
    Logger::Log("--> Performing final cleanup...");
    
    // Clear any remaining global state
    g_Running = false;
    
    // Force garbage collection of any remaining objects
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    Logger::Log("====== SHUTDOWN SEQUENCE COMPLETED ======");

    // Shutdown logger last
    Logger::Log("--> Finalizing shutdown. Shutting down logger.");
    Logger::Shutdown();

    // Cleanly remove DLL from memory
    FreeLibraryAndExitThread(g_hModule, 0);
}


DWORD WINAPI MainThread_Initialize(LPVOID dwModule) {
    // Store DLL handle for clean unloading
    DX11Base::g_hModule = static_cast<HMODULE>(dwModule);

    // Initialize logger
    Logger::Init();
    Logger::Log("-> MainThread_Initialize started.");
    
    // Load settings from config file
    SettingsManager::LoadSettings();
    Logger::Log("Settings loaded from config file.");

    // Initialize MinHook first
    if (MH_Initialize() != MH_OK) {
        Logger::Error("!!! CRITICAL: Failed to initialize MinHook! !!!");
        return EXIT_FAILURE;
    }
    Logger::Log("0. MinHook initialized successfully.");

    // Initialize CrashDump system and store VEH handle
    g_pVehHandler = CrashDump::Initialize();
    Logger::Log("1. CrashDump system initialized.");

    // Initialize classes
    g_Engine = std::make_unique<Engine>();
    Logger::Log("2. g_Engine created.");

    g_Game = std::make_unique<Game>();
    Logger::Log("3. g_Game created.");

    g_BulletHarvester = std::make_unique<BulletHarvester>();
    Logger::Log("3a. g_BulletHarvester created.");

    g_Renderer = std::make_unique<Renderer>();
    Logger::Log("4. g_Renderer created.");

    g_ESP = std::make_unique<ESP>();
    Logger::Log("5. g_ESP created.");

    // Set global variable g_Running before creating threads
    g_Running = true;
    Logger::Log("5a. g_Running set to true.");

    g_MagicBullet = std::make_unique<MagicBullet>();
    Logger::Log("6. g_MagicBullet created.");

    g_TargetManager = std::make_unique<TargetManager>();
    Logger::Log("6a. g_TargetManager created.");

    // Give threads a moment to initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    g_Misc = std::make_unique<Misc>();
    Logger::Log("7. g_Misc created.");
    
    g_Noclip = new Noclip();
    Logger::Log("7a. g_Noclip created.");
    
    g_NoRecoil = std::make_unique<NoRecoil>();
    Logger::Log("7b. g_NoRecoil created.");
    
    g_Aimbot = std::make_unique<Aimbot>();
    Logger::Log("7c. g_Aimbot created.");

    // Initialize and start Process Monitor
    g_ProcessMonitor = std::make_unique<ProcessMonitor>();
    g_ProcessMonitor->SetCrashCallback([]() {
        Logger::Error("PlanetSide 2 process crash detected! Creating emergency dump...");
        CrashDump::CreateCrashDump("Process Crash Detected");
        });
    g_ProcessMonitor->StartMonitoring("Planetside2_x64.exe");
    Logger::Log("8. Process monitor started for Planetside2_x64.exe");

    // Set hooks
    g_D3D11Window->HookD3D();
    Logger::Log("9. D3D Hooks created.");

    g_Hooking->Initialize();
    Logger::Log("10. All hooks enabled.");

    // Start GameDataThread
    dataThread = std::thread(GameDataThread);
    Logger::Log("11. GameDataThread started.");

    Logger::Log("-> PS2 Cheat started successfully!");

    // Hotkey loop
    while (g_Running)
    {
        // Use GetAsyncKeyState & 1 to only react to key press
        if (GetAsyncKeyState(VK_INSERT) & 1)
        {
            g_Engine->bShowMenu = !g_Engine->bShowMenu;
        }
        if (GetAsyncKeyState(VK_END) & 1)
        {
            break;
        }
        std::this_thread::yield();
    }

    // Save settings before shutdown
    SettingsManager::SaveSettings();
    Logger::Log("Settings saved to config file.");

    // Call clean shutdown function
    Shutdown();

    // Terminate thread without unloading DLL
    return EXIT_SUCCESS;
}