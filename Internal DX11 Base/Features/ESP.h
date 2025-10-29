/**
 * @file ESP.h
 * @brief ESP (Extra Sensory Perception) system for visual overlays
 * @details This file contains the ESP system that provides visual overlays
 *          for game entities including boxes, health bars, skeletons, and tracers.
 */

#pragma once
#include "../Game/GameData.h"
#include <set>
#include <string>
#include <vector>
#include <mutex>
#include <optional>

// Forward declarations
namespace Utils { struct Vector2; }
enum class EFaction;

/**
 * @brief ESP bullet data structure
 * @details Contains bullet tracking data for ESP rendering
 */
struct ESPBulletData {
    uintptr_t pBullet;        ///< Bullet pointer
    uint64_t CreationTime;    ///< Creation timestamp
    bool IsAlive;            ///< Whether bullet is alive
};

/**
 * @brief ESP system class
 * @details Provides visual overlays for game entities including boxes,
 *          health bars, skeletons, tracers, and information displays.
 */
class ESP {
public:
    /**
     * @brief Constructor
     */
    ESP();
    
    /**
     * @brief Destructor
     */
    ~ESP();

    /**
     * @brief Render ESP overlays
     * @details Renders all ESP elements including boxes, health bars, skeletons, and tracers
     */
    void Render();
    
    /**
     * @brief Get render time in milliseconds
     * @return Render time in milliseconds
     */
    float GetRenderTime() const { return m_renderTime; }
    
    /**
     * @brief Get number of rendered entities
     * @return Number of entities rendered in last frame
     */
    int GetRenderedEntities() const { return m_renderedEntities; }

private:
    // Rendering functions
    void DrawPlayerESP(const EntitySnapshot& entity, const Utils::Vector2& feetScreenPos, const Utils::Vector2& headScreenPos, float alphaMultiplier, const std::optional<EntitySnapshot>& targetOpt);
    void DrawGroundVehicleESP(const EntitySnapshot& entity, const Utils::Vector2& screenPos, float alphaMultiplier, const std::optional<EntitySnapshot>& targetOpt);
    void DrawAirVehicleESP(const EntitySnapshot& entity, const Utils::Vector2& screenPos, float alphaMultiplier, const std::optional<EntitySnapshot>& targetOpt);
    void DrawTurretESP(const EntitySnapshot& entity, const Utils::Vector2& screenPos, float alphaMultiplier, const std::optional<EntitySnapshot>& targetOpt);
    void DrawOthersESP(const EntitySnapshot& entity, const Utils::Vector2& screenPos, float alphaMultiplier, const std::optional<EntitySnapshot>& targetOpt);

    // ESP components
    void DrawBox(const EntitySnapshot& entity, const Utils::Vector2& topLeft, float width, float height, float alphaMultiplier, const std::optional<EntitySnapshot>& targetOpt);
    void DrawSkeleton(const EntitySnapshot& entity, const Utils::Vector2& screenPos, const std::optional<EntitySnapshot>& targetOpt);
    void DrawHealthBar(const EntitySnapshot& entity, const Utils::Vector2& topLeft, float width, float height, float alphaMultiplier, const std::optional<EntitySnapshot>& targetOpt);
    void DrawTracer(const EntitySnapshot& entity, const Utils::Vector2& screenPos);
    void DrawInfoLabel(const EntitySnapshot& entity, float alphaMultiplier, const std::optional<EntitySnapshot>& targetOpt);
    void DrawBulletESP();
    void DrawGMDetection();
    void DrawCentralGMWarning(int gmCount);
    void DrawViewDirection(const EntitySnapshot& entity);

    // Bullet tracking
    void UpdateBulletTracking();

    // Helpers
    const float* GetTeamColor(EFaction team) const;
    bool IsCurrentTarget(const EntitySnapshot& entity, const std::optional<EntitySnapshot>& currentTargetOpt) const;
    void LogUnknownEntity(const EntitySnapshot& entity) const;
    bool IsKnownEntityType(EntityType type) const;

    // Shape Drawing
    void DrawVehicleBoxWithBorder(const Utils::Vector2& topLeft, float width, float height, const float* color, float alphaMultiplier);
    void DrawTriangleWithBorder(const Utils::Vector2& center, float size, const float* color, float alphaMultiplier);
    void DrawXWithBorder(const Utils::Vector2& center, float size, const float* color, float alphaMultiplier);
    void DrawHexagonWithBorder(const Utils::Vector2& center, float size, const float* color, float alphaMultiplier);

    // Performance
    float m_renderTime = 0.0f;
    int m_renderedEntities = 0;

    // Bullet tracking data
    std::vector<ESPBulletData> m_trackedBullets;
    std::mutex m_bulletTrackingMutex;
    uintptr_t m_lastTrackedBulletPtr = 0;

    // Static variable for already logged Unknown Entities
    static std::set<std::string> s_loggedUnknownEntities;
};

inline std::unique_ptr<ESP> g_ESP;