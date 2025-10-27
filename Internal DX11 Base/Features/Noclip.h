#pragma once
#include "../Utils/Vector.h"
#include "../Utils/SafeMemory.h"
#include <memory>
#include <algorithm>
#include <thread>
#include <atomic>
#include <mutex>

class Noclip {
public:
    Noclip();
    ~Noclip();

    void Update();
    void ProcessInput();
    
    // Direct coordinate access (no conversion needed!)
    Utils::Vector3 GetWorldPosition();
    void SetWorldPosition(const Utils::Vector3& worldPos);
    
    // Viewangle-based movement
    Utils::Vector3 GetViewAngles();
    Utils::Vector3 GetForwardVector(const Utils::Vector3& viewAngles);
    Utils::Vector3 GetRightVector(const Utils::Vector3& viewAngles);
    
    // Threading for constant position override
    void StartPositionThread();
    void StopPositionThread();
    void PositionThreadWorker();
    
private:
    std::unique_ptr<Utils::SafeMemory> m_memory;
    
    // Movement settings
    Utils::Vector3 m_velocity;
    Utils::Vector3 m_targetPosition;
    Utils::Vector3 m_currentPosition;
    
    // Threading
    std::thread m_positionThread;
    std::atomic<bool> m_threadRunning;
    std::atomic<bool> m_noclipActive;
    std::mutex m_positionMutex;
    
    // ✅ PERFORMANCE: Cached pointers to avoid repeated calculations
    uintptr_t m_cachedPositionPtr = 0;
    uintptr_t m_cachedVelocityPtr = 0;
    bool m_pointersInitialized = false;
    
    void ApplyMovement(const Utils::Vector3& worldPos);
    uintptr_t GetPositionPointer();
    uintptr_t GetVelocityPointer();
    void SetVelocity(float velocityX, float velocityY, float velocityZ);
};

inline Noclip* g_Noclip = nullptr;
