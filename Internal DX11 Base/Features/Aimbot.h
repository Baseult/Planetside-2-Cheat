#pragma once
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <windows.h>
#include "../Game/Game.h"
#include "MouseInjection.h" // Include for shared mouse injection structures

class Aimbot {
public:
    Aimbot();
    ~Aimbot();
    void Update();
    
private:
    // Threading
    std::thread m_aimbotThread;
    std::atomic<bool> m_threadRunning;
    std::atomic<bool> m_aimbotActive;
    std::mutex m_aimbotMutex;
    
    // Undocumented Windows Syscall
    bool(*NtUserInjectMouseInput)(InjectedInputMouseInfo*, int) = nullptr;
    
    // Worker thread function
    void AimbotWorker();
    
    // Core aimbot functions
    bool IsAimbotActive();
    bool IsPlayerShooting();
    bool IsHotkeyPressed();
    
    // Target and aim calculations
    Utils::Vector3 CalculateAimAngles(const Utils::Vector3& targetPos);
    void ApplySmoothing(const Utils::Vector3& targetMouseMovement);
    
    // Mouse movement
    bool InitializeNtUserInjectMouseInput();
    bool MoveMouse(int x, int y);
};

inline std::unique_ptr<Aimbot> g_Aimbot;