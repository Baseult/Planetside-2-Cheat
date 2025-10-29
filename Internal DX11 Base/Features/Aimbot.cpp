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

    int startup_tries = 0;
    while (!DX11Base::g_Running) {
        if (++startup_tries > 100) { // Timeout after ~1 second
            Logger::Error("Aimbot::AimbotWorker timed out waiting for g_Running.");
            return;
        }
        std::this_thread::yield();
    }
    Logger::Log("Aimbot::AimbotWorker: g_Running confirmed, starting main loop.");

    while (m_threadRunning) {
        try {
            if (!g_Settings.Aimbot.bEnabled) {
                break;
            }
            
            if (!DX11Base::g_Engine || !g_TargetManager || !g_Game) {
                break;
            }
            
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

                    MoveMouse(mouseX, mouseY);
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        catch (...) {
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


Utils::Vector3 Aimbot::CalculateAimAngles(const Utils::Vector3& targetPos) {
    if (!g_Game) {
        return Utils::Vector3(0, 0, 0);
    }

    Utils::Vector2 targetScreenPos;
    if (!g_Game->WorldToScreen(targetPos, targetScreenPos)) {
        return Utils::Vector3(0, 0, 0);
    }

    Utils::Vector2 screenSize = g_Renderer->GetScreenSize();
    Utils::Vector2 screenCenter = Utils::Vector2(screenSize.x / 2.0f, screenSize.y / 2.0f);

    Utils::Vector2 relativeMovement = targetScreenPos - screenCenter;
    float distanceToTarget = relativeMovement.Length();

    if (distanceToTarget < 5.0f) {
        return Utils::Vector3(0, 0, 0);
    }

    float speedFactor = 50.0f;
    float speed = speedFactor;
    if (g_Settings.Aimbot.fSmoothing > 0.001f) {
        speed = speedFactor / g_Settings.Aimbot.fSmoothing;
    }
    if (g_Settings.Aimbot.fSmoothing < 1.f) speed = speedFactor; // Max speed if smoothing is off

    float distanceToMove = speed;

    Utils::Vector2 mouseMovement;
    if (distanceToMove > distanceToTarget) {
        mouseMovement = relativeMovement;
    } else {
        Utils::Vector2 direction;
        if (distanceToTarget > 0.001f) {
            direction = Utils::Vector2(relativeMovement.x / distanceToTarget, relativeMovement.y / distanceToTarget);
        } else {
            direction = Utils::Vector2(0, 0);
        }
        mouseMovement = Utils::Vector2(direction.x * distanceToMove, direction.y * distanceToMove);
    }

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