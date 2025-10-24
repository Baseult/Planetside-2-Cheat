#pragma once
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <windows.h>
#include "MouseInjection.h"

class NoRecoil {
public:
    NoRecoil();
    ~NoRecoil();
    void Update();
    
private:
    std::thread m_recoilThread;
    std::atomic<bool> m_threadRunning;
    std::atomic<bool> m_recoilActive;
    std::mutex m_recoilMutex;
    
    bool(*NtUserInjectMouseInput)(InjectedInputMouseInfo*, int) = nullptr;
    
    void NoRecoilWorker();
    bool IsLeftMousePressed();
    void ApplyRecoilCompensation();
    bool InitializeNtUserInjectMouseInput();
    bool InjectMouseMovement(int x, int y);
    
    // Game state detection
    bool IsPlayerShooting();
};

inline std::unique_ptr<NoRecoil> g_NoRecoil;
