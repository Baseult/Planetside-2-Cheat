#include "pch.h"
#include "BulletHarvester.h"
#include "Game.h"
#include "Offsets.h"
#include "../Utils/Logger.h"
#include "../Utils/Settings.h"
#include "../Renderer/Renderer.h"
#include <Windows.h>

BulletHarvester::BulletHarvester() : m_isRunning(true) {
    m_snapshot_front = std::make_shared<const BulletWorldSnapshot>();
    m_snapshot_back = std::make_unique<BulletWorldSnapshot>();
    
    m_updateThread = std::thread(&BulletHarvester::UpdateThread, this);
    m_magicBulletThread = std::thread(&BulletHarvester::MagicBulletThread, this);
    Logger::Log("BulletHarvester created with 2 threads");
}

BulletHarvester::~BulletHarvester() {
    m_isRunning = false;
    if (m_updateThread.joinable()) {
        m_updateThread.join();
    }
    if (m_magicBulletThread.joinable()) {
        m_magicBulletThread.join();
    }
    Logger::Log("BulletHarvester destroyed");
}

std::shared_ptr<const BulletWorldSnapshot> BulletHarvester::GetBulletSnapshot() const {
    std::lock_guard<std::mutex> lock(m_snapshotMutex);
    return m_snapshot_front;
}

int BulletHarvester::GetBulletCount() const {
    std::lock_guard<std::mutex> lock(m_snapshotMutex);
    return static_cast<int>(m_snapshot_front->bullets.size());
}

void BulletHarvester::UpdateThread() {
    Logger::Log("BulletHarvester::UpdateThread entered.");

    // ================================================================
    // =================== START-UP SYNC FIX =========================
    // ================================================================
    int startup_tries = 0;
    while (!DX11Base::g_Running) {
        if (++startup_tries > 100) { 
            Logger::Error("BulletHarvester::UpdateThread timed out waiting for g_Running.");
            return;
        }
        std::this_thread::yield();
    }
    Logger::Log("BulletHarvester::UpdateThread: g_Running confirmed, starting main loop.");
    // ================================================================

    if (!m_addressesInitialized) {
        m_cachedBaseAddress = g_Game->GetMemory()->GetBaseAddress();
        m_cachedBulletPointerBase = m_cachedBaseAddress + Offsets::MagicBullet::pCurrentBulletPointerBase;
        
        // Cache basePtr for UpdateThread as well
        if (!g_Game->GetMemory()->Read(m_cachedBulletPointerBase, m_cachedBulletBasePtr) || m_cachedBulletBasePtr == 0) {
            Logger::Error("BulletHarvester::UpdateThread: Failed to cache basePtr");
            return;
        }
        
        m_addressesInitialized = true;
        Logger::Log("BulletHarvester: Base addresses cached for performance");
    }

    while (m_isRunning && DX11Base::g_Running) {
        ReadBulletData();
        CleanupExpiredBullets();
        SwapBuffers();
        std::this_thread::yield(); // Yield for better performance
    }
    Logger::Log("BulletHarvester::UpdateThread exited loop.");
}

void BulletHarvester::MagicBulletThread() {
    Logger::Log("BulletHarvester::MagicBulletThread entered.");

    // Wait for startup sync
    int startup_tries = 0;
    while (!DX11Base::g_Running) {
        if (++startup_tries > 100) { 
            Logger::Error("BulletHarvester::MagicBulletThread timed out waiting for g_Running.");
            return;
        }
        std::this_thread::yield();
    }
    Logger::Log("BulletHarvester::MagicBulletThread: g_Running confirmed, starting magic bullet loop.");

    // Wait for addresses to be initialized by UpdateThread
    while (!m_addressesInitialized) {
        std::this_thread::yield();
    }

    // Cache basePtr once
    if (!g_Game || !g_Game->GetMemory()->Read(m_cachedBulletPointerBase, m_cachedBulletBasePtr) || m_cachedBulletBasePtr == 0) {
        Logger::Error("BulletHarvester::MagicBulletThread: Failed to cache basePtr");
        return;
    }

    while (m_isRunning && DX11Base::g_Running) {
        // Only freeze bullets if Magic Bullet is enabled
        if (g_Settings.MagicBullet.bEnabled) {
            uintptr_t bulletPtr = 0;
     
            if (g_Game->GetMemory()->Read(m_cachedBulletBasePtr + Offsets::MagicBullet::pCurrentBulletPointerOffsets[0], bulletPtr) && bulletPtr != 0) {
                // Check if this bullet has already been freezed
                {
                    std::lock_guard<std::mutex> lock(m_freezedBulletsMutex);
                    if (m_freezedBullets.find(bulletPtr) != m_freezedBullets.end()) {
                        // Bullet already freezed, skip
                        std::this_thread::yield();
                        continue;
                    }
                }
                
                // Set bullet speed to 0.0f in game memory
                if (g_Game->GetMemory()->Write(bulletPtr + Offsets::MagicBullet::bullet_speed_offset, 0.0f)) {
                    // Mark bullet as freezed
                    std::lock_guard<std::mutex> lock(m_freezedBulletsMutex);
                    m_freezedBullets.insert(bulletPtr);
                }
            }
        }
        
        // No sleep - maximum speed, but yield to other threads
        std::this_thread::yield();
    }
    Logger::Log("BulletHarvester::MagicBulletThread exited loop.");
}

void BulletHarvester::ReadBulletData() {
    if (!g_Game) return;
    
    uintptr_t bulletPtr = 0;
    
    if (!g_Game->GetMemory()->Read(m_cachedBulletBasePtr + Offsets::MagicBullet::pCurrentBulletPointerOffsets[0], bulletPtr) || bulletPtr == 0) return;
    
    if (bulletPtr != m_lastTrackedBulletPtr && bulletPtr != 0) {
        m_lastTrackedBulletPtr = bulletPtr;
        
        bool isAlive = false;
        if (!g_Game->GetMemory()->Read(bulletPtr + Offsets::MagicBullet::bullet_is_alive_offset, isAlive) || !isAlive) {
            return;  // Bullet already dead - don't track
        }
        
        BulletSnapshot snapshot;
        if (ReadBulletSnapshot(bulletPtr, snapshot)) {
            m_snapshot_back->bullets.push_back(snapshot);
        }
    }
}

bool BulletHarvester::ReadBulletSnapshot(uintptr_t bulletPtr, BulletSnapshot& snapshot) {
    if (!g_Game || !g_Renderer) return false;
    
    snapshot.id = bulletPtr;
    snapshot.creationTime = GetTickCount64();
    
    if (!g_Game->GetMemory()->Read(bulletPtr + Offsets::MagicBullet::bullet_position_offset, snapshot.position)) return false;
    if (!g_Game->GetMemory()->Read(bulletPtr + Offsets::MagicBullet::bullet_start_position_offset, snapshot.startPosition)) return false;
    if (!g_Game->GetMemory()->Read(bulletPtr + Offsets::MagicBullet::bullet_direction_offset, snapshot.direction)) return false;
    if (!g_Game->GetMemory()->Read(bulletPtr + Offsets::MagicBullet::bullet_speed_offset, snapshot.speed)) return false;
    if (!g_Game->GetMemory()->Read(bulletPtr + Offsets::MagicBullet::bullet_is_alive_offset, snapshot.isAlive)) return false;
    if (!g_Game->GetMemory()->Read(bulletPtr + Offsets::MagicBullet::o_BulletHash, snapshot.hash)) return false;
    
    // Initialize freezed flag
    snapshot.freezed = false;
    
    // VORAUSBERECHNEN
    const float tracerLength = 2.5f;
    Utils::Vector3 tracerStart = snapshot.position - (snapshot.direction * tracerLength);
    
    bool startOnScreen = g_Game->WorldToScreen(tracerStart, snapshot.tracerStartScreenPos);
    bool endOnScreen = g_Game->WorldToScreen(snapshot.position, snapshot.tracerEndScreenPos);
    snapshot.isOnScreen = startOnScreen || endOnScreen;
    
    return true;
}

void BulletHarvester::CleanupExpiredBullets() {
    uint64_t currentTime = GetTickCount64();
    
    m_snapshot_back->bullets.erase(
        std::remove_if(m_snapshot_back->bullets.begin(), m_snapshot_back->bullets.end(),
            [this, currentTime](const BulletSnapshot& bullet) {
                // ✅ Faster check: Remove if expired OR dead
                if (bullet.IsExpired(currentTime, 5000)) return true;  // Reduced from 5000ms to 1000ms
                
                // ✅ PERFORMANCE: Use cached isAlive status from snapshot instead of redundant memory read
                return !bullet.isAlive;  // Remove if dead (already cached in snapshot)
            }),
        m_snapshot_back->bullets.end()
    );
    
    // Cleanup freezed bullets set - remove expired bullet IDs
    {
        std::lock_guard<std::mutex> lock(m_freezedBulletsMutex);
        auto it = m_freezedBullets.begin();
        while (it != m_freezedBullets.end()) {
            // Check if bullet is still alive by reading from memory
            bool isAlive = false;
            if (!g_Game || !g_Game->GetMemory()->Read(*it + Offsets::MagicBullet::bullet_is_alive_offset, isAlive) || !isAlive) {
                it = m_freezedBullets.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void BulletHarvester::SwapBuffers() {
    std::lock_guard<std::mutex> lock(m_snapshotMutex);
    m_snapshot_front = std::shared_ptr<const BulletWorldSnapshot>(m_snapshot_back.release());
    m_snapshot_back = std::make_unique<BulletWorldSnapshot>();
    m_snapshot_back->timestamp = GetTickCount64();
}
