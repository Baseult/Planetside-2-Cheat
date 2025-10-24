#include "pch.h"
#include "NoRecoil.h"
#include "../Utils/Settings.h"
#include "../Utils/Logger.h"
#include "../Engine.h"
#include "../Game/Game.h"
#include "../Game/Offsets.h"
#include <Windows.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>

NoRecoil::NoRecoil() {
    m_threadRunning = false;
    m_recoilActive = false;
    
    if (!InitializeNtUserInjectMouseInput()) {
        Logger::Error("NoRecoil: Failed to initialize NtUserInjectMouseInput");
    }
    
    Logger::Log("NoRecoil: Initialized");
}

NoRecoil::~NoRecoil() {
    if (m_threadRunning) {
        m_threadRunning = false;
        if (m_recoilThread.joinable()) {
            m_recoilThread.join();
        }
    }
}

void NoRecoil::Update() {
    if (!g_Settings.Misc.NoRecoil.bEnabled) {
        if (m_recoilActive) {
            m_threadRunning = false;
            if (m_recoilThread.joinable()) {
                m_recoilThread.join();
            }
            m_recoilActive = false;
        }
        return;
    }
    
    if (!m_recoilActive) {
        m_threadRunning = true;
        m_recoilActive = true;
        m_recoilThread = std::thread(&NoRecoil::NoRecoilWorker, this);
    }
}

void NoRecoil::NoRecoilWorker() {
    Logger::Log("NoRecoil::NoRecoilWorker entered.");

    // ================================================================
    // =================== START-UP SYNC FIX =========================
    // ================================================================
    int startup_tries = 0;
    while (!DX11Base::g_Running) {
        if (++startup_tries > 100) { // Timeout after ~1 second
            Logger::Error("NoRecoil::NoRecoilWorker timed out waiting for g_Running.");
            return;
        }
        std::this_thread::yield();
    }
    Logger::Log("NoRecoil::NoRecoilWorker: g_Running confirmed, starting main loop.");
    // ================================================================

    while (m_threadRunning) {
        try {
            if (!g_Settings.Misc.NoRecoil.bEnabled) {
                break;
            }
            
            // Check if required objects still exist
            if (!DX11Base::g_Engine || !g_Game) {
                break;
            }
            
            // Only apply recoil compensation if:
            // 1. Menu is not open
            // 2. Player is actually shooting (not just holding mouse button)
            if (!DX11Base::g_Engine->bShowMenu && IsPlayerShooting()) {
                ApplyRecoilCompensation();
            }
            
            std::this_thread::yield();
        }
        catch (...) {
            // Silent catch to prevent crashes during shutdown
            break;
        }
    }
    Logger::Log("NoRecoil::NoRecoilWorker exited loop.");
}

bool NoRecoil::IsLeftMousePressed() {
    return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
}

void NoRecoil::ApplyRecoilCompensation() {
    // Dynamic integer-based strength (1 - 100)
    int strength = g_Settings.Misc.NoRecoil.iStrength;
    
    // Dynamic compensation calculation
    // 1 = 0.1 pixel, 10 = 1 pixel, 50 = 5 pixels, 100 = 10 pixels
    float compensationFloat = (strength / 10.0f);
    int compensation = static_cast<int>(compensationFloat + 0.5f);
    
    // Ensure minimum movement
    if (compensation < 1 && strength > 0) {
        compensation = 1;
    }
    
    // Use the undocumented Windows syscall for mouse movement
    InjectMouseMovement(0, compensation);
    
    // Dynamic sleep based on strength (inverse relationship)
    // Lower strength = longer sleep = more precise
    // Formula: extraSleep = (100 - strength) / 5
    // 1 = 19.8ms, 10 = 18ms, 50 = 10ms, 100 = 0ms
    int extraSleep = static_cast<int>((100 - strength) / 5.0f);
    
    // Ensure minimum sleep for very low values
    if (strength <= 5) {
        extraSleep = 20; // Maximum precision for very low values
    }
    
    if (extraSleep > 0) {
        Sleep(extraSleep);
    }
}

bool NoRecoil::InitializeNtUserInjectMouseInput() {
    HMODULE win32u = LoadLibrary(L"win32u.dll");
    if (!win32u) {
        Logger::Error("NoRecoil: Failed to load win32u.dll");
        return false;
    }
    
    void* syscallAddress = GetProcAddress(win32u, "NtUserInjectMouseInput");
    if (!syscallAddress) {
        Logger::Error("NoRecoil: Failed to find NtUserInjectMouseInput");
        return false;
    }
    
    *(void**)&NtUserInjectMouseInput = syscallAddress;
    Logger::Log("NoRecoil: NtUserInjectMouseInput initialized");
    return true;
}

bool NoRecoil::InjectMouseMovement(int x, int y) {
    InjectedInputMouseInfo mouseInfo = {};
    mouseInfo.move_direction_x = x;
    mouseInfo.move_direction_y = y;
    mouseInfo.mouse_data = 0;
    mouseInfo.mouse_options = (InjectedInputMouseOptions)0;
    mouseInfo.time_offset_in_miliseconds = 0;
    mouseInfo.extra_info = nullptr;
    
    return NtUserInjectMouseInput(&mouseInfo, 1);
}


bool NoRecoil::IsPlayerShooting() {
    if (!g_Game) return false;
    auto worldSnapshot = g_Game->GetWorldSnapshot();
    return worldSnapshot->localPlayer.isShooting;
}

