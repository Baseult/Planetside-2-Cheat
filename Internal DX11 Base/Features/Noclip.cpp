#include "pch.h"
#include "Noclip.h"
#include "../Game/Offsets.h"
#include "../Game/Game.h"
#include "../Utils/Settings.h"
#include "../Utils/Logger.h"
#include <Windows.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Noclip::Noclip() {
    m_memory = std::make_unique<Utils::SafeMemory>();
    if (!m_memory->AttachToProcess("PlanetSide2_x64.exe")) {
        Logger::Error("Noclip: Failed to attach to process");
    }
    
    // Initialize threading
    m_threadRunning = false;
    m_noclipActive = false;
    m_velocity = Utils::Vector3(0, 0, 0);
    m_targetPosition = Utils::Vector3(0, 0, 0);
    m_currentPosition = Utils::Vector3(0, 0, 0);
}

Noclip::~Noclip() {
    StopPositionThread();
}

void Noclip::Update() {
    // Check if Noclip should be enabled (either always on or via hotkey)
    bool shouldBeEnabled = g_Settings.Noclip.bEnabled;
    
    // If hotkey is enabled, check for hotkey press
    if (g_Settings.Noclip.bUseHotkey) {
        static bool hotkeyPressed = false;
        bool currentlyPressed = (GetAsyncKeyState(g_Settings.Noclip.iHotkey) & 0x8000) != 0;
        
        // Toggle on key press (not hold)
        if (currentlyPressed && !hotkeyPressed) {
            g_Settings.Noclip.bEnabled = !g_Settings.Noclip.bEnabled;
        }
        hotkeyPressed = currentlyPressed;
        
        shouldBeEnabled = g_Settings.Noclip.bEnabled;
    }
    
    if (!shouldBeEnabled) {
        if (m_noclipActive) {
            StopPositionThread();
            m_noclipActive = false;
        }
        return;
    }
    
    // Start thread when Noclip is activated
    if (!m_noclipActive) {
        // Set initial position
        Utils::Vector3 currentPos = GetWorldPosition();
        if (currentPos.x != 0 || currentPos.y != 0 || currentPos.z != 0) {
            std::lock_guard<std::mutex> lock(m_positionMutex);
            m_targetPosition = currentPos;
        }
        
        StartPositionThread();
        m_noclipActive = true;
    }
    
    ProcessInput();
}

void Noclip::ProcessInput() {
    // Get current world position directly from memory
    Utils::Vector3 currentPosition = GetWorldPosition();
    if (currentPosition.x == 0 && currentPosition.y == 0 && currentPosition.z == 0) {
        return; // Failed to read position
    }
    
    // Get view angles for direction-based movement
    Utils::Vector3 viewAngles = GetViewAngles();
    Utils::Vector3 forward = GetForwardVector(viewAngles);
    Utils::Vector3 right = GetRightVector(viewAngles);

    Utils::Vector3 newPosition = currentPosition;
    float speed = g_Settings.Noclip.fSpeed / 10.0f;  // Divide by 10 for more precise control
    
    // ✅ PERFORMANCE: Batch all GetAsyncKeyState calls together instead of 6 separate calls
    bool wPressed = (GetAsyncKeyState('W') & 0x8000) != 0;
    bool sPressed = (GetAsyncKeyState('S') & 0x8000) != 0;
    bool aPressed = (GetAsyncKeyState('A') & 0x8000) != 0;
    bool dPressed = (GetAsyncKeyState('D') & 0x8000) != 0;
    bool capsPressed = (GetAsyncKeyState(VK_CAPITAL) & 0x8000) != 0;
    bool shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    
    // Viewangle-based WASD Movement using cached key states
    if (wPressed) {
        // Forward in view direction
        newPosition.x += forward.x * speed;
        newPosition.y += forward.y * speed;
        newPosition.z += forward.z * speed;
    }
    if (sPressed) {
        // Backward (opposite of view direction)
        newPosition.x -= forward.x * speed;
        newPosition.y -= forward.y * speed;
        newPosition.z -= forward.z * speed;
    }
    if (aPressed) {
        // Left (perpendicular to view direction)
        newPosition.x += right.x * speed;
        newPosition.y += right.y * speed;
        newPosition.z += right.z * speed;
    }
    if (dPressed) {
        // Right (perpendicular to view direction)
        newPosition.x -= right.x * speed;
        newPosition.y -= right.y * speed;
        newPosition.z -= right.z * speed;
    }
    
    // Vertical Movement (always world up/down)
    if (capsPressed) {
        newPosition.y += speed;  // Up (CAPSLOCK)
    }
    if (shiftPressed) {
        newPosition.y -= speed;  // Down (SHIFT)
    }
    
    // Update target position for thread
    if (newPosition.x != currentPosition.x || 
        newPosition.y != currentPosition.y || 
        newPosition.z != currentPosition.z) {
        {
            std::lock_guard<std::mutex> lock(m_positionMutex);
            m_targetPosition = newPosition;
        }
    }
}

void Noclip::ApplyMovement(const Utils::Vector3& worldPos) {
    // Write world coordinates directly
    SetWorldPosition(worldPos);
}

void Noclip::StartPositionThread() {
    if (m_threadRunning) return;
    
    m_threadRunning = true;
    m_positionThread = std::thread(&Noclip::PositionThreadWorker, this);
    Logger::Log("Noclip: Position thread started");
}

void Noclip::StopPositionThread() {
    if (!m_threadRunning) return;
    
    m_threadRunning = false;
    if (m_positionThread.joinable()) {
        m_positionThread.join();
    }
    Logger::Log("Noclip: Position thread stopped");
}

void Noclip::PositionThreadWorker() {
    Logger::Log("Noclip::PositionThreadWorker entered.");

    // ================================================================
    // =================== START-UP SYNC FIX =========================
    // ================================================================
    int startup_tries = 0;
    while (!DX11Base::g_Running) {
        if (++startup_tries > 100) { // Timeout after ~1 second
            Logger::Error("Noclip::PositionThreadWorker timed out waiting for g_Running.");
            return;
        }
        std::this_thread::yield();
    }
    Logger::Log("Noclip::PositionThreadWorker: g_Running confirmed, starting main loop.");
    // ================================================================
    
    Utils::Vector3 lastWrittenPosition(0, 0, 0);
    
    while (m_threadRunning) {
        Utils::Vector3 targetPos;
        
        // Read thread-safe target position
        {
            std::lock_guard<std::mutex> lock(m_positionMutex);
            targetPos = m_targetPosition;
        }
        
        // Only write if position has changed
        if (targetPos.x != lastWrittenPosition.x || 
            targetPos.y != lastWrittenPosition.y || 
            targetPos.z != lastWrittenPosition.z) {
            
            // ✅ KLASSENWECHSEL-FIX: Pointer vor jedem Write neu lesen
            uintptr_t currentPositionPtr = GetPositionPointer();
            if (currentPositionPtr == 0) {
                Logger::Error("Noclip: Failed to get position pointer in thread");
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            
            // Write double values directly
            double x = static_cast<double>(targetPos.x);
            double y = static_cast<double>(targetPos.y);
            double z = static_cast<double>(targetPos.z);
            
            if (m_memory->Write(currentPositionPtr + Offsets::Noclip::o_PositionX, x) &&
                m_memory->Write(currentPositionPtr + Offsets::Noclip::o_PositionY, y) &&
                m_memory->Write(currentPositionPtr + Offsets::Noclip::o_PositionZ, z)) {
                
                lastWrittenPosition = targetPos;
            }
        }
        
        // ✅ KLASSENWECHSEL-FIX: Velocity Pointer vor jedem Write neu lesen
        SetVelocity(Offsets::GameConstants::NOCLIP_VELOCITY_X, Offsets::GameConstants::NOCLIP_VELOCITY_Y, Offsets::GameConstants::NOCLIP_VELOCITY_Z);
        
        // Brief pause to save CPU
        std::this_thread::yield();
    }
    Logger::Log("Noclip::PositionThreadWorker exited loop.");
}

uintptr_t Noclip::GetPositionPointer() {
    uintptr_t baseAddress = m_memory->GetBaseAddress();
    
    // Follow pointer chain: [base+58] -> [result+2A0] -> [result+C00]
    uintptr_t ptr1, ptr2, ptr3, ptr4, ptr5;
    
    if (!m_memory->Read(baseAddress + Offsets::Noclip::pBaseAddress, ptr1)) return 0;
    if (!m_memory->Read(ptr1 + Offsets::Noclip::o_Offset1, ptr2)) return 0;
    if (!m_memory->Read(ptr2 + Offsets::Noclip::o_Offset2, ptr3)) return 0;
    if (!m_memory->Read(ptr3 + Offsets::Noclip::o_Offset3, ptr4)) return 0;
    if (!m_memory->Read(ptr4 + Offsets::Noclip::o_Offset4, ptr5)) return 0;
    
    return ptr5 + Offsets::Noclip::o_Offset5;
}

uintptr_t Noclip::GetVelocityPointer() {
    uintptr_t baseAddress = m_memory->GetBaseAddress();
    
    // Follow velocity pointer chain: [base+5B0] -> [result+3B8]
    uintptr_t ptr1, ptr2, ptr3, ptr4;
    
    if (!m_memory->Read(baseAddress + Offsets::Noclip::pVelocityBaseAddress, ptr1)) return 0;
    if (!m_memory->Read(ptr1 + Offsets::Noclip::o_VelocityOffset1, ptr2)) return 0;
    if (!m_memory->Read(ptr2 + Offsets::Noclip::o_VelocityOffset2, ptr3)) return 0;
    if (!m_memory->Read(ptr3 + Offsets::Noclip::o_VelocityOffset3, ptr4)) return 0;
    
    return ptr4 + Offsets::Noclip::o_VelocityOffset4;
}

void Noclip::SetVelocity(float velocityX, float velocityY, float velocityZ) {
    // ✅ KLASSENWECHSEL-FIX: Velocity Pointer vor jedem Write neu lesen
    uintptr_t currentVelocityPtr = GetVelocityPointer();
    if (currentVelocityPtr == 0) {
        return; // Failed to get velocity pointer
    }
    
    // Write all velocity values
    m_memory->Write(currentVelocityPtr + Offsets::Noclip::o_VelocityX, velocityX);
    m_memory->Write(currentVelocityPtr + Offsets::Noclip::o_VelocityY, velocityY);
    m_memory->Write(currentVelocityPtr + Offsets::Noclip::o_VelocityZ, velocityZ);
}

Utils::Vector3 Noclip::GetViewAngles() {
    if (!g_Game) return {};
    auto worldSnapshot = g_Game->GetWorldSnapshot();
    if (!worldSnapshot) return {};
    return worldSnapshot->localPlayer.viewAngles;
}

Utils::Vector3 Noclip::GetForwardVector(const Utils::Vector3& viewAngles) {
    // PlanetSide 2 ViewAngle conversion
    float yaw = viewAngles.x; // X is Yaw in PS2
    
    // Calculate rotation angle (yaw + 1.6f - M_PI / 2.f)
    float rotationAngle = yaw + Offsets::GameConstants::ROTATION_OFFSET - (float)(M_PI / 2.0);
    
    // Create rotation matrix around Y-axis
    float cos_angle = cos(rotationAngle);
    float sin_angle = sin(rotationAngle);
    
    // Transform Forward-Vector (0, 0, 1) through Y-rotation matrix (rotated 90°)
    Utils::Vector3 forward;
    forward.x = 0.0f * cos_angle + 1.0f * sin_angle;  // X * cos + Z * sin
    forward.y = 0.0f;  // Y remains unchanged
    forward.z = -0.0f * sin_angle + 1.0f * cos_angle; // -X * sin + Z * cos
    
    return forward;
}

Utils::Vector3 Noclip::GetRightVector(const Utils::Vector3& viewAngles) {
    // PlanetSide 2 ViewAngle conversion for Right-Vector
    float yaw = viewAngles.x; // X is Yaw in PS2
    
    // Calculate rotation angle (yaw + 1.6f - M_PI / 2.f)
    float rotationAngle = yaw + Offsets::GameConstants::ROTATION_OFFSET - (float)(M_PI / 2.0);
    
    // Create rotation matrix around Y-axis
    float cos_angle = cos(rotationAngle);
    float sin_angle = sin(rotationAngle);
    
    // Transform Right-Vector (1, 0, 0) through Y-rotation matrix (rotated 90°)
    Utils::Vector3 right;
    right.x = 1.0f * cos_angle + 0.0f * sin_angle;  // X * cos + Z * sin
    right.y = 0.0f;  // Y remains unchanged
    right.z = -1.0f * sin_angle + 0.0f * cos_angle; // -X * sin + Z * cos
    
    return right;
}

Utils::Vector3 Noclip::GetWorldPosition() {
    Utils::Vector3 position;
    // ✅ KLASSENWECHSEL-FIX: Pointer vor jedem Read neu lesen
    uintptr_t posPtr = GetPositionPointer();
    
    if (posPtr == 0) {
        Logger::Error("Noclip: Failed to get position pointer");
        return Utils::Vector3(0, 0, 0);
    }
    
    // Read double values directly
    double x, y, z;
    if (!m_memory->Read(posPtr + Offsets::Noclip::o_PositionX, x) ||
        !m_memory->Read(posPtr + Offsets::Noclip::o_PositionY, y) ||
        !m_memory->Read(posPtr + Offsets::Noclip::o_PositionZ, z)) {
        Logger::Error("Noclip: Failed to read world position");
        return Utils::Vector3(0, 0, 0);
    }
    
    position.x = static_cast<float>(x);
    position.y = static_cast<float>(y);
    position.z = static_cast<float>(z);
    
    return position;
}

void Noclip::SetWorldPosition(const Utils::Vector3& worldPos) {
    // ✅ KLASSENWECHSEL-FIX: Pointer vor jedem Write neu lesen
    uintptr_t posPtr = GetPositionPointer();
    
    if (posPtr == 0) {
        Logger::Error("Noclip: Failed to get position pointer for writing");
        return;
    }
    
    // Write double values directly
    double x = static_cast<double>(worldPos.x);
    double y = static_cast<double>(worldPos.y);
    double z = static_cast<double>(worldPos.z);
    
    if (!m_memory->Write(posPtr + Offsets::Noclip::o_PositionX, x) ||
        !m_memory->Write(posPtr + Offsets::Noclip::o_PositionY, y) ||
        !m_memory->Write(posPtr + Offsets::Noclip::o_PositionZ, z)) {
        Logger::Error("Noclip: Failed to write world position");
    }
}
