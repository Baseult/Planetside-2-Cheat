#pragma once
#include "GameData.h"  // Unsere neuen, sicheren Strukturen!
#include "../Utils/SafeMemory.h"
#include <memory>
#include <mutex>
#include <chrono>

class Game {
public:
    Game();
    ~Game();
    
    // Initialization
    bool Initialize();
    void Shutdown();
    
    // Main function - called by GameDataThread
    void Update();
    
    // CRITICAL: shared_ptr instead of copy! (Performance)
    std::shared_ptr<const WorldSnapshot> GetWorldSnapshot() const;
    
    // Helper functions
    bool WorldToScreen(const Utils::Vector3& world, Utils::Vector2& screen) const;
    bool IsOnScreen(const Utils::Vector2& screen) const;
    
    // Memory access (only for Producer!)
    Utils::SafeMemory* GetMemory() const { return m_memory.get(); }
    
    // Performance monitoring
    float GetUpdateTime() const { return m_updateTime; }
    int GetEntityCount() const;
    
private:
    std::unique_ptr<Utils::SafeMemory> m_memory;
    
    // CRITICAL: shared_ptr for Front, unique_ptr for Back
    std::shared_ptr<const WorldSnapshot> m_snapshot_front;
    std::unique_ptr<WorldSnapshot> m_snapshot_back;
    mutable std::mutex m_snapshotMutex;
    
    float m_updateTime = 0.0f;
    
    // Helper for address calculation
    uintptr_t GetAbsoluteAddress(uintptr_t offset) const {
        return m_memory->GetBaseAddress() + offset;
    }
    
    // Address Cache (Performance optimization)
    struct AddressCache {
        uintptr_t cGameBase = 0;
        uintptr_t cGraphicsBase = 0;
        uintptr_t cameraPtr = 0;
        uintptr_t matrixAddress = 0;
        ViewMatrix_t cachedViewMatrix;
        std::chrono::steady_clock::time_point lastCacheUpdate;
        std::chrono::milliseconds cacheValidityDuration{100};
        
        bool IsValid() const {
            return cGameBase != 0 && 
                   std::chrono::steady_clock::now() - lastCacheUpdate < cacheValidityDuration;
        }
    };
    AddressCache m_addressCache;
    std::mutex m_cacheMutex;
    
    // ✅ PERFORMANCE: Cache baseAddress to avoid repeated GetBaseAddress() calls
    uintptr_t m_cachedBaseAddress = 0;
    bool m_baseAddressInitialized = false;
    
    // Producer-Funktionen (PRIVATE!)
    void ReadAllGameData();
    void ReadViewMatrix();
    void ReadLocalPlayer();
    void ReadEntityList();
    void ReadHealthShield(uintptr_t entityAddress, EntitySnapshot& snapshot);
    void ReadHeadPosition(uintptr_t entityAddress, EntitySnapshot& snapshot);
    std::string ReadEntityName(uintptr_t entityAddress);
    void SwapBuffers();
    
    bool ShouldEntityHaveHealth(EntityType type) const;
    bool IsEntityValid(uintptr_t entityAddress) const;
};

// Global instance
inline std::unique_ptr<Game> g_Game;
