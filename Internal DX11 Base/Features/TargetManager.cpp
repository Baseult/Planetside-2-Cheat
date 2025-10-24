#include "pch.h"
#include "TargetManager.h"
#include "../Game/Game.h"
#include "../Game/GameData.h"
#include "../Utils/Logger.h"
#include "../Utils/Settings.h"
#include "../Game/SDK.h"
#include "../Renderer/Renderer.h"
#include <string>
#include <thread>
#include <optional>
#include <algorithm>
#include <cmath>
#include <vector>
#include <utility>

TargetManager::TargetManager() : m_isRunning(true) {
    m_updateThread = std::thread(&TargetManager::UpdateThread, this);
    Logger::Log("TargetManager created");
}

TargetManager::~TargetManager() {
    m_isRunning = false;
    if (m_updateThread.joinable()) {
        m_updateThread.join();
    }
    Logger::Log("TargetManager destroyed");
}

std::optional<EntitySnapshot> TargetManager::GetCurrentTarget() const {
    std::lock_guard<std::mutex> lock(m_targetMutex);
    return m_currentTarget;
}

void TargetManager::UpdateThread() {
    Logger::Log("TargetManager::UpdateThread entered.");

    // ================================================================
    // =================== START-UP SYNC FIX =========================
    // ================================================================
    int startup_tries = 0;
    while (!DX11Base::g_Running) {
        if (++startup_tries > 100) { // Timeout after ~1 second
            Logger::Error("TargetManager::UpdateThread timed out waiting for g_Running.");
            return;
        }
        std::this_thread::yield();
    }
    Logger::Log("TargetManager::UpdateThread: g_Running confirmed, starting main loop.");
    // ================================================================

    while (m_isRunning && DX11Base::g_Running) {
        if (g_Game && g_Settings.Targeting.bEnabled) {
            FindBestTarget();
        } else {
            std::lock_guard<std::mutex> lock(m_targetMutex);
            m_currentTarget.reset();
        }
        std::this_thread::yield();
    }
    Logger::Log("TargetManager::UpdateThread exited loop.");
}

void TargetManager::FindBestTarget() {
    if (!g_Game) return;
    
    auto worldSnapshot = g_Game->GetWorldSnapshot();
    const EntitySnapshot* bestTarget = nullptr;
    
    // Name-based search (priority)
    if (g_Settings.Targeting.bTargetName && !g_Settings.Targeting.sTargetName.empty()) {
        bestTarget = FindTargetByName(*worldSnapshot, g_Settings.Targeting.sTargetName);
        
        // Wenn Continuous Search aktiviert ist und kein Target gefunden wurde,
        // behalte das aktuelle Target bei (falls vorhanden)
        if (!bestTarget && g_Settings.Targeting.bContinuousSearch) {
            std::lock_guard<std::mutex> lock(m_targetMutex);
            // Behalte das aktuelle Target bei, wenn es existiert
            if (m_currentTarget) {
                return; // No change to target
            }
        }
    }
    
    // Normale Targeting-Modi
    if (!bestTarget) {
        switch (g_Settings.Targeting.Mode) {
            case ETargetingMode::FOV:
                bestTarget = FindTargetByFOV(*worldSnapshot);
                break;
            case ETargetingMode::SmartFOV:
                bestTarget = FindTargetBySmartFOV(*worldSnapshot);
                break;
            case ETargetingMode::Distance:
                bestTarget = FindTargetByDistance(*worldSnapshot);
                break;
            case ETargetingMode::Health:
                bestTarget = FindTargetByHealth(*worldSnapshot);
                break;
        }
    }
    
    // Speichere Ergebnis
    std::lock_guard<std::mutex> lock(m_targetMutex);
    if (bestTarget) {
        m_currentTarget = *bestTarget;  // Kopie speichern
    } else {
        m_currentTarget.reset();
    }
}

const EntitySnapshot* TargetManager::FindTargetByFOV(const WorldSnapshot& worldSnapshot) {
    const EntitySnapshot* bestTarget = nullptr;
    float bestFov = g_Settings.Targeting.fFOV;
    
    for (const auto& entity : worldSnapshot.entities) {
        if (!IsValidTarget(entity, worldSnapshot.localPlayer)) continue;
        
        // Only for player entities we use the new ESP box-based FOV calculation
        if (!IsPlayerType(entity.type)) continue;
        
        // Calculate FOV based on the closest ESP box edge to the crosshair
        float boxFov = CalculateClosestPointOnBox(entity);
        
        if (boxFov < bestFov) {
            bestFov = boxFov;
            bestTarget = &entity;
        }
    }
    
    return bestTarget;
}

const EntitySnapshot* TargetManager::FindTargetByDistance(const WorldSnapshot& worldSnapshot) {
    const EntitySnapshot* bestTarget = nullptr;
    float bestDistSq = FLT_MAX;
    
    for (const auto& entity : worldSnapshot.entities) {
        if (!IsValidTarget(entity, worldSnapshot.localPlayer)) continue;
        
        // NUTZT VORAUSBERECHNETE DISTANZ!
        float distSq = entity.distanceToLocalPlayer * entity.distanceToLocalPlayer;
        if (distSq < bestDistSq) {
            bestDistSq = distSq;
            bestTarget = &entity;
        }
    }
    
    return bestTarget;
}

const EntitySnapshot* TargetManager::FindTargetByHealth(const WorldSnapshot& worldSnapshot) {
    const EntitySnapshot* bestTarget = nullptr;
    float bestHealth = FLT_MAX;
    
    for (const auto& entity : worldSnapshot.entities) {
        if (!IsValidTarget(entity, worldSnapshot.localPlayer)) continue;
        
        if (entity.health < bestHealth) {
            bestHealth = entity.health;
            bestTarget = &entity;
        }
    }
    
    return bestTarget;
}

const EntitySnapshot* TargetManager::FindTargetBySmartFOV(const WorldSnapshot& worldSnapshot) {
    const EntitySnapshot* bestTarget = nullptr;
    float bestScore = FLT_MAX;
    
    // Collect all valid entities with their FOV distances
    std::vector<std::pair<const EntitySnapshot*, float>> validEntities;
    
    for (const auto& entity : worldSnapshot.entities) {
        if (!IsValidTarget(entity, worldSnapshot.localPlayer)) continue;
        
        // Only for player entities we use the new Smart FOV logic
        if (!IsPlayerType(entity.type)) continue;
        
        // Calculate FOV based on the closest ESP box edge to the crosshair
        float boxFov = CalculateClosestPointOnBox(entity);
        
        // Nur Entities innerhalb des konfigurierten FOV-Radius betrachten
        if (boxFov > g_Settings.Targeting.fFOV) continue;
        
        validEntities.push_back({&entity, boxFov});
    }
    
    // Spezielle Logik: Wenn das Fadenkreuz direkt innerhalb einer ESP-Box ist
    for (const auto& [entity, boxFov] : validEntities) {
        if (IsCrosshairInsideBox(*entity)) {
            // If crosshair is in this box, check if it goes through other boxes
            bool hasOverlappingBoxes = false;
            const EntitySnapshot* closestOverlappingEntity = nullptr;
            float closestOverlappingDistance = FLT_MAX;
            
            for (const auto& [otherEntity, otherBoxFov] : validEntities) {
                if (otherEntity == entity) continue; // Nicht mit sich selbst vergleichen
                
                if (IsCrosshairInsideBox(*otherEntity)) {
                    hasOverlappingBoxes = true;
                    // If both boxes contain the crosshair, choose the closer one
                    if (otherEntity->distanceToLocalPlayer < closestOverlappingDistance) {
                        closestOverlappingDistance = otherEntity->distanceToLocalPlayer;
                        closestOverlappingEntity = otherEntity;
                    }
                }
            }
            
            if (hasOverlappingBoxes && closestOverlappingEntity) {
                // If crosshair goes through multiple boxes, choose the closer one
                if (closestOverlappingEntity->distanceToLocalPlayer < entity->distanceToLocalPlayer) {
                    return closestOverlappingEntity;
                }
            }
            
            // If crosshair is only in this box, select it directly
            return entity;
        }
    }
    
    // Normal Smart FOV logic for entities outside boxes
    for (const auto& [entity, boxFov] : validEntities) {
        // Smart FOV logic: Prioritize entities closer to player
        // Use 3D distance as main criterion (lower distance = better score)
        float distanceScore = entity->distanceToLocalPlayer;
        
        // Additionally: FOV distance as tie-breaker (lower FOV distance = better score)
        // But only with lower weight since distance is the main priority
        float fovWeight = 0.1f; // FOV has only 10% weight
        float combinedScore = distanceScore + (boxFov * fovWeight);
        
        if (combinedScore < bestScore) {
            bestScore = combinedScore;
            bestTarget = entity;
        }
    }
    
    return bestTarget;
}

const EntitySnapshot* TargetManager::FindTargetByName(const WorldSnapshot& worldSnapshot, const std::string& targetName) {
    // Update available players for suggestions
    UpdateAvailablePlayers(worldSnapshot);
    
    // Trim and convert target name to lowercase for case-insensitive search
    std::string trimmedTargetName = Trim(targetName);
    if (trimmedTargetName.empty()) {
        return nullptr;
    }
    std::string lowerTargetName = ToLower(trimmedTargetName);
    
    for (const auto& entity : worldSnapshot.entities) {
        // For name-based search we use less strict criteria
        if (!entity.isAlive) continue;
        if (entity.type == EntityType::Unknown) continue;
        if (entity.health <= 0.0f) continue;
        if (entity.distanceToLocalPlayer > g_Settings.Targeting.fMaxDistance) continue;
        
        // Ignore team check for name-based search (find any player with the name)
        // Ignore MAX Units check for name-based search
        
        // Convert entity name to lowercase and check if target name is contained
        std::string lowerEntityName = ToLower(entity.name);
        if (lowerEntityName.find(lowerTargetName) != std::string::npos) {
            return &entity;
        }
    }
    
    return nullptr;
}

bool TargetManager::IsValidTarget(const EntitySnapshot& entity, const LocalPlayerSnapshot& localPlayer) const {
    if (!entity.isAlive) return false;
    if (entity.type == EntityType::Unknown) return false;
    if (entity.health <= 0.0f) return false;
    
    // NUTZT VORAUSBERECHNETE DISTANZ!
    if (entity.distanceToLocalPlayer > g_Settings.Targeting.fMaxDistance) return false;
    
    if (!g_Settings.Targeting.bTargetTeam && !entity.IsEnemy(localPlayer.team) && entity.team != EFaction::NSO) {
        return false;
    }
    
    if (g_Settings.Targeting.bIgnoreMaxUnits && IsMAXUnit(entity.type)) return false;
    
    return true;
}

void TargetManager::UpdateAvailablePlayers(const WorldSnapshot& worldSnapshot) {
    g_Settings.Targeting.availablePlayers.clear();
    
    for (const auto& entity : worldSnapshot.entities) {
        if (!IsValidTarget(entity, worldSnapshot.localPlayer)) continue;
        if (entity.name.empty()) continue;
        
        // Only add infantry types (players) to the list
        if (!IsPlayerType(entity.type)) continue;
        
        // Add player if not already in list
        bool alreadyExists = false;
        for (const auto& existingPlayer : g_Settings.Targeting.availablePlayers) {
            if (existingPlayer == entity.name) {
                alreadyExists = true;
                break;
            }
        }
        
        if (!alreadyExists) {
            g_Settings.Targeting.availablePlayers.push_back(entity.name);
        }
    }
}

std::string TargetManager::ToLower(const std::string& str) const {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string TargetManager::Trim(const std::string& str) const {
    size_t first = str.find_first_not_of(' ');
    if (first == std::string::npos) {
        return "";
    }
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

// DEPRECATED: This function is no longer used for FOV targeting.
// Use CalculateClosestPointOnBox() instead for better ESP box-based calculation.
float TargetManager::CalculateLineBasedFOV(const EntitySnapshot& entity, const LocalPlayerSnapshot& localPlayer) const {
    if (!g_Game) return FLT_MAX;
    
    // ✅ PERFORMANCE: Use pre-calculated screen positions instead of redundant WorldToScreen calls
    Utils::Vector2 headScreenPos = entity.headScreenPos;
    Utils::Vector2 feetScreenPos = entity.feetScreenPos;
    bool headOnScreen = entity.isOnScreen;  // Already calculated in ReadEntityList()
    bool feetOnScreen = entity.isOnScreen;
    
    // If neither head nor feet are on screen, use original FOV calculation
    if (!headOnScreen && !feetOnScreen) {
        return entity.fovDistanceToCenter;
    }
    
    // Wenn nur eine Position auf dem Bildschirm ist, verwende diese
    if (!headOnScreen) {
        Utils::Vector2 screenSize = g_Renderer->GetScreenSize();
        Utils::Vector2 screenCenter = Utils::Vector2(screenSize.x / 2.0f, screenSize.y / 2.0f);
        Utils::Vector2 relativePos = feetScreenPos - screenCenter;
        return relativePos.Length();
    }
    
    if (!feetOnScreen) {
        Utils::Vector2 screenSize = g_Renderer->GetScreenSize();
        Utils::Vector2 screenCenter = Utils::Vector2(screenSize.x / 2.0f, screenSize.y / 2.0f);
        Utils::Vector2 relativePos = headScreenPos - screenCenter;
        return relativePos.Length();
    }
    
    // Both positions are on screen - calculate distance to imaginary line
    Utils::Vector2 screenSize = g_Renderer->GetScreenSize();
    Utils::Vector2 screenCenter = Utils::Vector2(screenSize.x / 2.0f, screenSize.y / 2.0f);
    
    // Calculate distance from screen center to line between head and feet
    float distanceToLine = CalculateDistanceToLine(screenCenter, headScreenPos, feetScreenPos);
    
    return distanceToLine;
}

// DEPRECATED: This function is no longer used.
// Use CalculateDistanceToLineSegment() instead for better line segment calculation.
float TargetManager::CalculateDistanceToLine(const Utils::Vector2& point, const Utils::Vector2& lineStart, const Utils::Vector2& lineEnd) const {
    // Berechne die Distanz von einem Punkt zu einer Linie
    // Verwendet die Formel: |(y2-y1)x0 - (x2-x1)y0 + x2y1 - y2x1| / sqrt((y2-y1)² + (x2-x1)²)
    
    float x0 = point.x, y0 = point.y;
    float x1 = lineStart.x, y1 = lineStart.y;
    float x2 = lineEnd.x, y2 = lineEnd.y;
    
    // Berechne die Distanz zur Linie
    float numerator = std::abs((y2 - y1) * x0 - (x2 - x1) * y0 + x2 * y1 - y2 * x1);
    float denominator = std::sqrt((y2 - y1) * (y2 - y1) + (x2 - x1) * (x2 - x1));
    
    // Vermeide Division durch Null
    if (denominator < 0.001f) {
        // Wenn die Linie sehr kurz ist, verwende die Distanz zum Mittelpunkt
        Utils::Vector2 lineMidpoint = Utils::Vector2((x1 + x2) / 2.0f, (y1 + y2) / 2.0f);
        Utils::Vector2 relativePos = point - lineMidpoint;
        return relativePos.Length();
    }
    
    return numerator / denominator;
}

float TargetManager::CalculateClosestPointOnBox(const EntitySnapshot& entity) const {
    if (!g_Game || !g_Renderer) return FLT_MAX;
    
    // Verwende die bereits berechneten Screen-Positionen aus dem Snapshot
    Utils::Vector2 headScreenPos = entity.headScreenPos;
    Utils::Vector2 feetScreenPos = entity.feetScreenPos;
    
    // If entity is not on screen, use original FOV calculation
    if (!entity.isOnScreen) {
        return entity.fovDistanceToCenter;
    }
    
    // Berechne ESP-Box-Dimensionen (wie in ESP.cpp)
    const float boxHeight = std::abs(feetScreenPos.y - headScreenPos.y);
    if (boxHeight < 1.0f) return FLT_MAX; // Too small for a meaningful box
    
    const float boxWidth = boxHeight * 0.65f; // Gleiche Berechnung wie in ESP.cpp
    
    // Berechne die 4 Eckpunkte der ESP-Box
    Utils::Vector2 topLeft = { feetScreenPos.x - boxWidth / 2.0f, headScreenPos.y };
    Utils::Vector2 topRight = { feetScreenPos.x + boxWidth / 2.0f, headScreenPos.y };
    Utils::Vector2 bottomLeft = { feetScreenPos.x - boxWidth / 2.0f, feetScreenPos.y };
    Utils::Vector2 bottomRight = { feetScreenPos.x + boxWidth / 2.0f, feetScreenPos.y };
    
    // Hole das Bildschirmzentrum
    Utils::Vector2 screenSize = g_Renderer->GetScreenSize();
    Utils::Vector2 screenCenter = Utils::Vector2(screenSize.x / 2.0f, screenSize.y / 2.0f);
    
    // Berechne die Distanz zu allen 4 Kanten und finde das Minimum
    float minDistance = FLT_MAX;
    
    // Obere Kante (topLeft -> topRight)
    float distTop = CalculateDistanceToLineSegment(screenCenter, topLeft, topRight);
    if (distTop < minDistance) minDistance = distTop;
    
    // Rechte Kante (topRight -> bottomRight)
    float distRight = CalculateDistanceToLineSegment(screenCenter, topRight, bottomRight);
    if (distRight < minDistance) minDistance = distRight;
    
    // Untere Kante (bottomRight -> bottomLeft)
    float distBottom = CalculateDistanceToLineSegment(screenCenter, bottomRight, bottomLeft);
    if (distBottom < minDistance) minDistance = distBottom;
    
    // Linke Kante (bottomLeft -> topLeft)
    float distLeft = CalculateDistanceToLineSegment(screenCenter, bottomLeft, topLeft);
    if (distLeft < minDistance) minDistance = distLeft;
    
    return minDistance;
}

float TargetManager::CalculateDistanceToLineSegment(const Utils::Vector2& point, const Utils::Vector2& lineStart, const Utils::Vector2& lineEnd) const {
    // Berechne die Distanz von einem Punkt zu einem Liniensegment (nicht unendliche Linie)
    // Verwendet die Projektion des Punktes auf das Liniensegment
    
    Utils::Vector2 lineVector = lineEnd - lineStart;
    Utils::Vector2 pointVector = point - lineStart;
    
    float lineLengthSq = lineVector.x * lineVector.x + lineVector.y * lineVector.y;
    
    // Wenn das Liniensegment sehr kurz ist, verwende die Distanz zum Startpunkt
    if (lineLengthSq < 0.001f) {
        return pointVector.Length();
    }
    
    // Berechne die Projektion des Punktes auf das Liniensegment
    float t = (pointVector.x * lineVector.x + pointVector.y * lineVector.y) / lineLengthSq;
    
    // Begrenze t auf das Liniensegment [0, 1]
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    
    // Calculate closest point on line segment
    Utils::Vector2 closestPoint = lineStart + Utils::Vector2(lineVector.x * t, lineVector.y * t);
    
    // Calculate distance to closest point
    Utils::Vector2 distanceVector = point - closestPoint;
    return distanceVector.Length();
}

bool TargetManager::IsCrosshairInsideBox(const EntitySnapshot& entity) const {
    if (!g_Game || !g_Renderer) return false;
    
    // Verwende die bereits berechneten Screen-Positionen aus dem Snapshot
    Utils::Vector2 headScreenPos = entity.headScreenPos;
    Utils::Vector2 feetScreenPos = entity.feetScreenPos;
    
    // Wenn das Entity nicht auf dem Bildschirm ist, kann das Fadenkreuz nicht in der Box sein
    if (!entity.isOnScreen) return false;
    
    // Berechne ESP-Box-Dimensionen (wie in ESP.cpp)
    const float boxHeight = std::abs(feetScreenPos.y - headScreenPos.y);
    if (boxHeight < 1.0f) return false; // Too small for a meaningful box
    
    const float boxWidth = boxHeight * 0.65f; // Gleiche Berechnung wie in ESP.cpp
    
    // Berechne die 4 Eckpunkte der ESP-Box
    Utils::Vector2 topLeft = { feetScreenPos.x - boxWidth / 2.0f, headScreenPos.y };
    Utils::Vector2 topRight = { feetScreenPos.x + boxWidth / 2.0f, headScreenPos.y };
    Utils::Vector2 bottomLeft = { feetScreenPos.x - boxWidth / 2.0f, feetScreenPos.y };
    Utils::Vector2 bottomRight = { feetScreenPos.x + boxWidth / 2.0f, feetScreenPos.y };
    
    // Get screen center (crosshair position)
    Utils::Vector2 screenSize = g_Renderer->GetScreenSize();
    Utils::Vector2 screenCenter = Utils::Vector2(screenSize.x / 2.0f, screenSize.y / 2.0f);
    
    // Check if crosshair is inside the box
    // Use Point-in-Rectangle test
    float minX = topLeft.x;
    if (topRight.x < minX) minX = topRight.x;
    if (bottomLeft.x < minX) minX = bottomLeft.x;
    if (bottomRight.x < minX) minX = bottomRight.x;
    
    float maxX = topLeft.x;
    if (topRight.x > maxX) maxX = topRight.x;
    if (bottomLeft.x > maxX) maxX = bottomLeft.x;
    if (bottomRight.x > maxX) maxX = bottomRight.x;
    
    float minY = topLeft.y;
    if (topRight.y < minY) minY = topRight.y;
    if (bottomLeft.y < minY) minY = bottomLeft.y;
    if (bottomRight.y < minY) minY = bottomRight.y;
    
    float maxY = topLeft.y;
    if (topRight.y > maxY) maxY = topRight.y;
    if (bottomLeft.y > maxY) maxY = bottomLeft.y;
    if (bottomRight.y > maxY) maxY = bottomRight.y;
    
    return (screenCenter.x >= minX && screenCenter.x <= maxX && 
            screenCenter.y >= minY && screenCenter.y <= maxY);
}