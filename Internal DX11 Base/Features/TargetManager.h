#pragma once
#include "../Game/GameData.h"  // KRITISCH!
#include <thread>
#include <mutex>
#include <atomic>
#include <string>
#include <optional>  // C++17

class TargetManager {
public:
    TargetManager();
    ~TargetManager();
    
    // KRITISCH: std::optional statt Pointer!
    std::optional<EntitySnapshot> GetCurrentTarget() const;
    
private:
    void UpdateThread();
    void FindBestTarget();
    
    // Targeting-Methoden (private)
    const EntitySnapshot* FindTargetByFOV(const WorldSnapshot& worldSnapshot);
    const EntitySnapshot* FindTargetBySmartFOV(const WorldSnapshot& worldSnapshot);
    const EntitySnapshot* FindTargetByDistance(const WorldSnapshot& worldSnapshot);
    const EntitySnapshot* FindTargetByHealth(const WorldSnapshot& worldSnapshot);
    const EntitySnapshot* FindTargetByName(const WorldSnapshot& worldSnapshot, const std::string& targetName);
    
    bool IsValidTarget(const EntitySnapshot& entity, const LocalPlayerSnapshot& localPlayer) const;
    void UpdateAvailablePlayers(const WorldSnapshot& worldSnapshot);
    
    // Helper functions for string operations
    std::string ToLower(const std::string& str) const;
    std::string Trim(const std::string& str) const;
    
    // Line-based FOV calculation functions (DEPRECATED - kept for compatibility)
    float CalculateLineBasedFOV(const EntitySnapshot& entity, const LocalPlayerSnapshot& localPlayer) const;
    float CalculateDistanceToLine(const Utils::Vector2& point, const Utils::Vector2& lineStart, const Utils::Vector2& lineEnd) const;
    
    // ESP Box-based FOV calculation functions
    float CalculateClosestPointOnBox(const EntitySnapshot& entity) const;
    float CalculateDistanceToLineSegment(const Utils::Vector2& point, const Utils::Vector2& lineStart, const Utils::Vector2& lineEnd) const;
    bool IsCrosshairInsideBox(const EntitySnapshot& entity) const;
    
    // KRITISCH: optional statt Pointer!
    std::optional<EntitySnapshot> m_currentTarget;
    mutable std::mutex m_targetMutex;
    
    std::thread m_updateThread;
    std::atomic<bool> m_isRunning;
};

// Global instance
inline std::unique_ptr<TargetManager> g_TargetManager;
