#include "pch.h"
#include "Aimbot.h"
#include "TargetManager.h"
#include "../Utils/Settings.h"
#include "../Utils/Logger.h"
#include "../Engine.h"
#include "../Game/Game.h"
#include "../Game/Offsets.h"
#include "../Renderer/Renderer.h"
#include <Windows.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <cmath>

Aimbot::Aimbot() {
    m_threadRunning = false;
    m_aimbotActive = false;
    
    // Old caching members removed - replaced by snapshot system
    
    if (!InitializeNtUserInjectMouseInput()) {
        Logger::Error("Aimbot: Failed to initialize NtUserInjectMouseInput");
    }
    
    Logger::Log("Aimbot: Initialized");
}

Aimbot::~Aimbot() {
    if (m_threadRunning) {
        m_threadRunning = false;
        if (m_aimbotThread.joinable()) {
            m_aimbotThread.join();
        }
    }
}

void Aimbot::Update() {
    if (!g_Settings.Aimbot.bEnabled) {
        if (m_aimbotActive) {
            m_threadRunning = false;
            if (m_aimbotThread.joinable()) {
                m_aimbotThread.join();
            }
            m_aimbotActive = false;
        }
        return;
    }
    
    if (!m_aimbotActive) {
        m_threadRunning = true;
        m_aimbotActive = true;
        m_aimbotThread = std::thread(&Aimbot::AimbotWorker, this);
    }
}

void Aimbot::AimbotWorker() {
    Logger::Log("Aimbot::AimbotWorker entered.");

    // ================================================================
    // =================== START-UP SYNC FIX =========================
    // ================================================================
    int startup_tries = 0;
    while (!DX11Base::g_Running) {
        if (++startup_tries > 100) { // Timeout after ~1 second
            Logger::Error("Aimbot::AimbotWorker timed out waiting for g_Running.");
            return;
        }
        std::this_thread::yield();
    }
    Logger::Log("Aimbot::AimbotWorker: g_Running confirmed, starting main loop.");
    // ================================================================

    while (m_threadRunning) {
        try {
            if (!g_Settings.Aimbot.bEnabled) {
                break;
            }
            
            // Check if required objects still exist
            if (!DX11Base::g_Engine || !g_TargetManager || !g_Game) {
                break;
            }
            
            // Only apply aimbot if:
            // 1. Menu is not open
            // 2. Aimbot is active (shooting or hotkey)
            if (!DX11Base::g_Engine->bShowMenu && IsAimbotActive()) {
                auto targetOpt = g_TargetManager->GetCurrentTarget();
                if (targetOpt) {
                    const EntitySnapshot& target = *targetOpt;
                    Utils::Vector3 headPos = target.headPosition;
                    
                    if (IsMAXUnit(target.type)) {
                        headPos = target.position;
                        headPos.y += Offsets::GameConstants::AIMBOT_MAX_HEAD_HEIGHT;
                    }
                    
                    Utils::Vector3 mouseMovement = CalculateAimAngles(headPos);
                    
                    int mouseX = static_cast<int>(mouseMovement.x);
                    int mouseY = static_cast<int>(mouseMovement.y);

                    // Apply mouse movement directly (like InputSys::Get().move_mouse in your C# code)
                    MoveMouse(mouseX, mouseY);
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Yield for smoother movement
        }
        catch (...) {
            // Silent catch to prevent crashes during shutdown
            break;
        }
    }
    Logger::Log("Aimbot::AimbotWorker exited loop.");
}

bool Aimbot::IsAimbotActive() {
    if (!g_Settings.Aimbot.bEnabled) return false;
    if (DX11Base::g_Engine->bShowMenu) return false;
    
    bool shootingActive = g_Settings.Aimbot.bAutoAim && IsPlayerShooting();
    bool hotkeyActive = g_Settings.Aimbot.bUseHotkey && IsHotkeyPressed();
    
    return shootingActive || hotkeyActive;
}

bool Aimbot::IsPlayerShooting() {
    if (!g_Game) return false;
    
    auto worldSnapshot = g_Game->GetWorldSnapshot();
    return worldSnapshot->localPlayer.isShooting;
}

bool Aimbot::IsHotkeyPressed() {
    if (!g_Settings.Aimbot.bUseHotkey) return false;
    return (GetAsyncKeyState(g_Settings.Aimbot.iHotkey) & 0x8000) != 0;
}

// GetBonePosition removed - no longer needed since EntitySnapshot already contains headPosition

Utils::Vector3 Aimbot::CalculateAimAngles(const Utils::Vector3& targetPos) {
    if (!g_Game) {
        return Utils::Vector3(0, 0, 0);
    }

    // Step 1: Convert target world coordinates to screen coordinates
    Utils::Vector2 targetScreenPos;
    if (!g_Game->WorldToScreen(targetPos, targetScreenPos)) {
        return Utils::Vector3(0, 0, 0);
    }

    // Step 2: Get screen center
    Utils::Vector2 screenSize = g_Renderer->GetScreenSize();
    Utils::Vector2 screenCenter = Utils::Vector2(screenSize.x / 2.0f, screenSize.y / 2.0f);

    // Step 3: Calculate relative movement vector and distance to target
    Utils::Vector2 relativeMovement = targetScreenPos - screenCenter;
    float distanceToTarget = relativeMovement.Length();

    // Deadzone: If we are already very close to target (e.g. 1 pixel), stop to avoid jitter.
    if (distanceToTarget < 5.0f) {
        return Utils::Vector3(0, 0, 0);
    }

    // Step 4: Robust method - Fixed speed per frame
    // Define a maximum speed (e.g. 300 pixels per second at 100 FPS -> 3 pixels per frame)
    // We scale this with the smoothing value. A high smoothing value reduces speed.
    float speedFactor = 50.0f; // Base speed. Adjust this value to control overall speed.
    float speed = speedFactor;
    if (g_Settings.Aimbot.fSmoothing > 0.001f) {
        speed = speedFactor / g_Settings.Aimbot.fSmoothing;
    }
    if (g_Settings.Aimbot.fSmoothing < 1.f) speed = speedFactor; // Max speed if smoothing is off

    // Calculate the distance we want to travel in this frame
    float distanceToMove = speed; // In a 10ms loop this is the movement per 10ms

    Utils::Vector2 mouseMovement;
    if (distanceToMove > distanceToTarget) {
        // If the distance to move is greater than the remaining distance,
        // we only move the remaining distance to avoid overshooting the target.
        mouseMovement = relativeMovement;
    } else {
        // Otherwise move with our constant speed towards the target.
        // Normalize the vector to get direction, then multiply by distance
        Utils::Vector2 direction;
        if (distanceToTarget > 0.001f) {
            direction = Utils::Vector2(relativeMovement.x / distanceToTarget, relativeMovement.y / distanceToTarget);
        } else {
            direction = Utils::Vector2(0, 0);
        }
        mouseMovement = Utils::Vector2(direction.x * distanceToMove, direction.y * distanceToMove);
    }

    // IMPORTANT: To avoid the problem with static_cast<int>, we should not round
    // before it is absolutely necessary. The `MoveMouse` function expects int, so it is ok here,
    // but be aware that precision is lost here.
    // If the calculated movement is between -1 and 1, it becomes 0, which stops aiming.
    // A minimal movement can help here.
    if (abs(mouseMovement.x) < 1.0f && abs(relativeMovement.x) > 1.0f) {
        mouseMovement.x = (mouseMovement.x > 0) ? 1.0f : -1.0f;
    }
    if (abs(mouseMovement.y) < 1.0f && abs(relativeMovement.y) > 1.0f) {
        mouseMovement.y = (mouseMovement.y > 0) ? 1.0f : -1.0f;
    }

    return Utils::Vector3(mouseMovement.x, mouseMovement.y, 0);
}

bool Aimbot::InitializeNtUserInjectMouseInput() {
    HMODULE win32u = LoadLibrary(L"win32u.dll");
    if (!win32u) {
        Logger::Error("Aimbot: Failed to load win32u.dll");
        return false;
    }
    
    void* syscallAddress = GetProcAddress(win32u, "NtUserInjectMouseInput");
    if (!syscallAddress) {
        Logger::Error("Aimbot: Failed to find NtUserInjectMouseInput");
        return false;
    }
    
    *(void**)&NtUserInjectMouseInput = syscallAddress;
    Logger::Log("Aimbot: NtUserInjectMouseInput initialized");
    return true;
}

bool Aimbot::MoveMouse(int x, int y) {
    if (!NtUserInjectMouseInput) {
        return false;
    }
    
    InjectedInputMouseInfo mouseInfo = {};
    mouseInfo.move_direction_x = x;
    mouseInfo.move_direction_y = y;
    mouseInfo.mouse_data = 0;
    mouseInfo.mouse_options = (InjectedInputMouseOptions)0;
    mouseInfo.time_offset_in_miliseconds = 0;
    mouseInfo.extra_info = nullptr;
    
    return NtUserInjectMouseInput(&mouseInfo, 1);
}