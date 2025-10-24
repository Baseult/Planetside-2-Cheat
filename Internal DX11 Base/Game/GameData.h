/**
 * @file GameData.h
 * @brief Game data structures and snapshot definitions
 * @details This file contains thread-safe snapshot structures for game data.
 *          These structures are designed for high-performance, thread-safe
 *          data sharing between the game data thread and render thread.
 */

#pragma once
#include "../Utils/Vector.h"
#include "SDK.h"
#include <vector>
#include <string>
#include <cstdint>

/**
 * @brief Entity snapshot structure for thread-safe data sharing
 * @details This structure contains a complete snapshot of an entity's state
 *          at a specific point in time. It replaces the old CachedEntity
 *          structure and eliminates the risk of accessing invalid memory
 *          by removing direct pointer access.
 */
struct EntitySnapshot {
    // ===== Identification =====
    uintptr_t id;  ///< Unique entity identifier (replaces old pAddress)
    
    // ===== Basic Data (for all features) =====
    Utils::Vector3 position;      ///< World position
    Utils::Vector3 headPosition;  ///< Head position for aiming
    Utils::Vector3 viewAngle;    ///< View angles
    EntityType type;              ///< Entity type
    EFaction team;               ///< Faction
    bool isAlive;                ///< Alive status
    
    // ===== Detailed Data (primarily for ESP) =====
    std::string name;            ///< Entity name
    float health;                ///< Current health
    float maxHealth;             ///< Maximum health
    float shield;                ///< Current shield
    float maxShield;             ///< Maximum shield
    uint8_t gmFlags;             ///< Game master flags
    
    // ===== Pre-calculated Data (Performance Boost!) =====
    Utils::Vector2 feetScreenPos;        ///< Screen position of feet
    Utils::Vector2 headScreenPos;        ///< Screen position of head
    bool isOnScreen;                     ///< Whether entity is visible on screen
    float distanceToLocalPlayer;         ///< Distance to local player
    float fovDistanceToCenter;          ///< FOV distance to screen center
    
    /**
     * @brief Get health percentage (0.0 to 1.0)
     * @return Health percentage
     */
    float GetHealthPercent() const { return maxHealth > 0 ? (health / maxHealth) : 0.0f; }
    
    /**
     * @brief Get shield percentage (0.0 to 1.0)
     * @return Shield percentage
     */
    float GetShieldPercent() const { return maxShield > 0 ? (shield / maxShield) : 0.0f; }
    
    /**
     * @brief Check if entity is enemy to local player
     * @param localTeam Local player's faction
     * @return true if entity is enemy, false otherwise
     */
    bool IsEnemy(EFaction localTeam) const { return team != localTeam && team != EFaction::NSO; }
    
    /**
     * @brief Check if entity is a game master
     * @return true if entity is GM, false otherwise
     */
    bool IsGM() const { return gmFlags != 0 && gmFlags != 2 && gmFlags != 4; }
};

/**
 * @brief Local player snapshot structure
 * @details Contains the current state of the local player
 */
struct LocalPlayerSnapshot {
    bool isValid;               ///< Whether the snapshot is valid
    Utils::Vector3 position;   ///< World position
    Utils::Vector3 eyePosition; ///< Eye position for aiming
    Utils::Vector3 viewAngles; ///< Current view angles
    EFaction team;             ///< Player's faction
    float health;              ///< Current health
    float maxHealth;           ///< Maximum health
    bool isAlive;              ///< Alive status
    bool isShooting;           ///< Whether player is currently shooting
    
    /**
     * @brief Get health percentage (0.0 to 1.0)
     * @return Health percentage
     */
    float GetHealthPercent() const { return maxHealth > 0 ? (health / maxHealth) : 0.0f; }
};

/**
 * @brief Complete world snapshot structure
 * @details Contains a complete snapshot of the game world at a specific time
 */
struct WorldSnapshot {
    uint64_t timestamp;                    ///< Snapshot timestamp
    ViewMatrix_t viewMatrix;               ///< Current view matrix
    LocalPlayerSnapshot localPlayer;       ///< Local player data
    std::vector<EntitySnapshot> entities;  ///< All visible entities
};

/**
 * @brief Bullet snapshot structure
 * @details Contains bullet tracking data for magic bullet features
 */
struct BulletSnapshot {
    uintptr_t id;                          ///< Unique bullet identifier
    Utils::Vector3 position;               ///< Current position
    Utils::Vector3 startPosition;          ///< Initial position
    Utils::Vector3 direction;              ///< Movement direction
    float speed;                           ///< Current speed
    bool isAlive;                          ///< Whether bullet is alive
    uint64_t creationTime;                 ///< Creation timestamp
    uint32_t hash;                         ///< Bullet hash for identification
    bool freezed;                          ///< Track if bullet speed has been set to 0.0f
    
    // Pre-calculated Screen Positions
    Utils::Vector2 tracerStartScreenPos;   ///< Screen position of tracer start
    Utils::Vector2 tracerEndScreenPos;     ///< Screen position of tracer end
    bool isOnScreen;                       ///< Whether bullet is visible on screen
    
    /**
     * @brief Check if bullet is valid
     * @return true if bullet is valid, false otherwise
     */
    bool IsValid() const { return id != 0 && isAlive; }
    
    /**
     * @brief Check if bullet has expired
     * @param currentTime Current timestamp
     * @param maxAge Maximum age in milliseconds
     * @return true if bullet has expired, false otherwise
     */
    bool IsExpired(uint64_t currentTime, uint64_t maxAge = 5000) const {
        return (currentTime - creationTime) > maxAge;
    }
};

/**
 * @brief Bullet world snapshot structure
 * @details Contains all bullets in the world at a specific time
 */
struct BulletWorldSnapshot {
    uint64_t timestamp;                    ///< Snapshot timestamp
    std::vector<BulletSnapshot> bullets;  ///< All bullets in the world
};
