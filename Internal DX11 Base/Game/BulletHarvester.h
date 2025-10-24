#pragma once
#include "GameData.h"
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <set>

class BulletHarvester {
public:
    BulletHarvester();
    ~BulletHarvester();
    
    // KRITISCH: shared_ptr statt Kopie!
    std::shared_ptr<const BulletWorldSnapshot> GetBulletSnapshot() const;
    int GetBulletCount() const;
    
private:
    void UpdateThread();
    void MagicBulletThread();
    void ReadBulletData();
    bool ReadBulletSnapshot(uintptr_t bulletPtr, BulletSnapshot& snapshot);
    void CleanupExpiredBullets();
    void SwapBuffers();
    
    // CRITICAL: shared_ptr for Front
    std::shared_ptr<const BulletWorldSnapshot> m_snapshot_front;
    std::unique_ptr<BulletWorldSnapshot> m_snapshot_back;
    mutable std::mutex m_snapshotMutex;
    
    std::thread m_updateThread;
    std::thread m_magicBulletThread;
    std::atomic<bool> m_isRunning;
    uintptr_t m_lastTrackedBulletPtr = 0;
    
    // ✅ PERFORMANCE: Einmalig gecachte Adressen
    uintptr_t m_cachedBaseAddress = 0;
    uintptr_t m_cachedBulletPointerBase = 0;
    uintptr_t m_cachedBulletBasePtr = 0;
    bool m_addressesInitialized = false;
    
    // Track freezed bullets to avoid overwriting manipulated bullets
    std::set<uintptr_t> m_freezedBullets;
    mutable std::mutex m_freezedBulletsMutex;
};

inline std::unique_ptr<BulletHarvester> g_BulletHarvester;

