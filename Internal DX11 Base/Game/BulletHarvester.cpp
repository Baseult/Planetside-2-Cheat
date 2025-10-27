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
    m_collectBulletsThread = std::thread(&BulletHarvester::CollectBulletsThread, this);
    Logger::Log("BulletHarvester created with 2 threads (UpdateThread + CollectBulletsThread)");
}

BulletHarvester::~BulletHarvester() {
    Logger::Log("BulletHarvester destructor called, stopping threads...");
    m_isRunning = false;
    
    // Give threads a moment to exit gracefully
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    if (m_updateThread.joinable()) {
        Logger::Log("Joining UpdateThread...");
        m_updateThread.join();
        Logger::Log("UpdateThread joined successfully.");
    }
    if (m_collectBulletsThread.joinable()) {
        Logger::Log("Joining CollectBulletsThread...");
        m_collectBulletsThread.join();
        Logger::Log("CollectBulletsThread joined successfully.");
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
        ProcessBulletQueue();
        CleanupExpiredBullets();
        SwapBuffers();
        
        // Check shutdown conditions more frequently
        if (!m_isRunning || !DX11Base::g_Running) break;
        
        std::this_thread::yield(); // Yield for better performance
    }
    Logger::Log("BulletHarvester::UpdateThread exited loop.");
}

void BulletHarvester::CollectBulletsThread() {
    Logger::Log("BulletHarvester::CollectBulletsThread entered.");

    // Wait for startup sync
    int startup_tries = 0;
    while (!DX11Base::g_Running) {
        if (++startup_tries > 100) { 
            Logger::Error("BulletHarvester::CollectBulletsThread timed out waiting for g_Running.");
            return;
        }
        std::this_thread::yield();
    }
    Logger::Log("BulletHarvester::CollectBulletsThread: g_Running confirmed, starting bullet collection loop.");

    // Wait for addresses to be initialized by UpdateThread
    while (!m_addressesInitialized) {
        std::this_thread::yield();
    }

    // Cache basePtr once
    if (!g_Game || !g_Game->GetMemory()->Read(m_cachedBulletPointerBase, m_cachedBulletBasePtr) || m_cachedBulletBasePtr == 0) {
        Logger::Error("BulletHarvester::CollectBulletsThread: Failed to cache basePtr");
        return;
    }

    while (m_isRunning && DX11Base::g_Running) {
        uintptr_t bulletPtr = 0;
        
        // Read current bullet pointer
        if (g_Game->GetMemory()->Read(m_cachedBulletBasePtr + Offsets::MagicBullet::pCurrentBulletPointerOffsets[0], bulletPtr) && bulletPtr != 0) {
            // Check if bullet is alive
            bool isAlive = false;
            if (g_Game->GetMemory()->Read(bulletPtr + Offsets::MagicBullet::bullet_is_alive_offset, isAlive) && isAlive) {
                // Add bullet to queue if not already processed
                {
                    std::lock_guard<std::mutex> lock(m_bulletQueueMutex);
                    // Simple check to avoid duplicates (could be improved with a set)
                    m_bulletQueue.push(bulletPtr);
                }
                
                // If Magic Bullet is enabled, freeze the bullet
                if (g_Settings.MagicBullet.bEnabled) {
                    // Check if this bullet has already been freezed
                    {
                        std::lock_guard<std::mutex> lock(m_freezedBulletsMutex);
                        if (m_freezedBullets.find(bulletPtr) == m_freezedBullets.end()) {
                            // Set bullet speed to 0.0f in game memory
                            if (g_Game->GetMemory()->Write(bulletPtr + Offsets::MagicBullet::bullet_speed_offset, 0.0f)) {
                                // Mark bullet as freezed
                                m_freezedBullets.insert(bulletPtr);
                            }
                        }
                    }
                }
            }
        }
        
        // Check shutdown conditions more frequently
        if (!m_isRunning || !DX11Base::g_Running) break;
        
        // No sleep - maximum speed, but yield to other threads
        std::this_thread::yield();
    }
    Logger::Log("BulletHarvester::CollectBulletsThread exited loop.");
}

void BulletHarvester::ProcessBulletQueue() {
    if (!g_Game || !m_isRunning || !DX11Base::g_Running) return;
    
    // Process multiple bullets from queue per cycle for better performance
    int processedCount = 0;
    const int maxProcessPerCycle = 10; // Process up to 10 bullets per cycle
    
    while (processedCount < maxProcessPerCycle) {
        uintptr_t bulletPtr = 0;
        
        // Get bullet from queue
        {
            std::lock_guard<std::mutex> lock(m_bulletQueueMutex);
            if (m_bulletQueue.empty()) break;
            bulletPtr = m_bulletQueue.front();
            m_bulletQueue.pop();
        }
        
        if (bulletPtr == 0) break;
        
        // Check if bullet is still alive and not already processed
        bool isAlive = false;
        if (!g_Game->GetMemory()->Read(bulletPtr + Offsets::MagicBullet::bullet_is_alive_offset, isAlive) || !isAlive) {
            processedCount++;
            continue; // Skip dead bullets
        }
        
        // Check if we already have this bullet in our snapshot
        bool alreadyExists = false;
        for (const auto& existingBullet : m_snapshot_back->bullets) {
            if (existingBullet.id == bulletPtr) {
                alreadyExists = true;
                break;
            }
        }
        
        if (!alreadyExists) {
            // Create detailed snapshot for this bullet
            BulletSnapshot snapshot;
            if (ReadBulletSnapshot(bulletPtr, snapshot)) {
                m_snapshot_back->bullets.push_back(snapshot);
            }
        }
        
        processedCount++;
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
    // Only do memory reads if we're still running and game is available
    if (m_isRunning && DX11Base::g_Running && g_Game) {
        std::lock_guard<std::mutex> lock(m_freezedBulletsMutex);
        auto it = m_freezedBullets.begin();
        while (it != m_freezedBullets.end()) {
            // Check if bullet is still alive by reading from memory
            bool isAlive = false;
            if (!g_Game->GetMemory()->Read(*it + Offsets::MagicBullet::bullet_is_alive_offset, isAlive) || !isAlive) {
                it = m_freezedBullets.erase(it);
            } else {
                ++it;
            }
        }
    } else {
        // If shutting down, just clear all freezed bullets
        std::lock_guard<std::mutex> lock(m_freezedBulletsMutex);
        m_freezedBullets.clear();
    }
}

void BulletHarvester::SwapBuffers() {
    std::lock_guard<std::mutex> lock(m_snapshotMutex);
    m_snapshot_front = std::shared_ptr<const BulletWorldSnapshot>(m_snapshot_back.release());
    m_snapshot_back = std::make_unique<BulletWorldSnapshot>();
    m_snapshot_back->timestamp = GetTickCount64();
}
