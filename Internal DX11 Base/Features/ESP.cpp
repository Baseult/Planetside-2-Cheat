#include "pch.h"
#include "ESP.h"
#include "TargetManager.h"
#include "../Game/Game.h"
#include "../Game/BulletHarvester.h"
#include "../Game/SDK.h"
#include "../Game/Offsets.h"
#include "../Renderer/Renderer.h"
#include "../Utils/Logger.h"
#include "../Utils/Settings.h"
#include "../framework.h"
#include <algorithm>
#include <vector>
#include <Windows.h>
#include <chrono>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdint>

ESP::ESP() {
    Logger::Log("ESP class created");
}

// Initialize static member
std::set<std::string> ESP::s_loggedUnknownEntities;

ESP::~ESP() {
    Logger::Log("ESP class destroying...");
}

void ESP::Render() {
    if (!g_Game || !g_Renderer || !g_TargetManager || DX11Base::g_CleanupInProgress) return;

    auto worldSnapshot = g_Game->GetWorldSnapshot();
    if (!worldSnapshot || !worldSnapshot->localPlayer.isValid) return;

    DrawGMDetection();

    // Log Unknown Entities even when ESP is disabled
    for (const auto& entity : worldSnapshot->entities) {
        if (!IsKnownEntityType(entity.type)) {
            LogUnknownEntity(entity);
        }
    }

    if (!g_Settings.ESP.bEnabled) return;

    m_renderedEntities = 0;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // ✅ PERFORMANCE: Cache CurrentTarget once per frame instead of multiple Mutex locks
    auto targetOpt = g_TargetManager->GetCurrentTarget();

    // Sort entities by distance (far to near) so boxes overlap correctly
    std::vector<EntitySnapshot> sortedEntities = worldSnapshot->entities;
    std::sort(sortedEntities.begin(), sortedEntities.end(), [](const EntitySnapshot& a, const EntitySnapshot& b) {
        return a.distanceToLocalPlayer > b.distanceToLocalPlayer;
    });

    for (const auto& entity : sortedEntities) {
        if (!entity.isAlive || !entity.isOnScreen) continue;
        if (entity.distanceToLocalPlayer > g_Settings.ESP.fMaxDistance) continue;
        if (!g_Settings.ESP.bTeamESP && entity.team == worldSnapshot->localPlayer.team && entity.team != EFaction::NSO) continue;

        bool isTarget = targetOpt && targetOpt->id == entity.id;
        if (isTarget) continue; // Targets are drawn later

        float alpha = 1.0f;
        if (IsPlayerType(entity.type)) DrawPlayerESP(entity, entity.feetScreenPos, entity.headScreenPos, alpha, targetOpt);
        else if (IsGroundVehicleType(entity.type)) DrawGroundVehicleESP(entity, entity.feetScreenPos, alpha, targetOpt);
        else if (IsAirVehicleType(entity.type)) DrawAirVehicleESP(entity, entity.feetScreenPos, alpha, targetOpt);
        else if (IsTurretType(entity.type)) DrawTurretESP(entity, entity.feetScreenPos, alpha, targetOpt);
        else if (IsOthersType(entity.type)) DrawOthersESP(entity, entity.feetScreenPos, alpha, targetOpt);
        m_renderedEntities++;
    }

    if (targetOpt) {
        const EntitySnapshot& target = *targetOpt;
        if (target.isAlive && target.isOnScreen) {
            float alpha = 1.0f;
            if (IsPlayerType(target.type)) DrawPlayerESP(target, target.feetScreenPos, target.headScreenPos, alpha, targetOpt);
            else if (IsGroundVehicleType(target.type)) DrawGroundVehicleESP(target, target.feetScreenPos, alpha, targetOpt);
            else if (IsAirVehicleType(target.type)) DrawAirVehicleESP(target, target.feetScreenPos, alpha, targetOpt);
            else if (IsTurretType(target.type)) DrawTurretESP(target, target.feetScreenPos, alpha, targetOpt);
            else if (IsOthersType(target.type)) DrawOthersESP(target, target.feetScreenPos, alpha, targetOpt);
             m_renderedEntities++;
        }
    }

    if (g_Settings.ESP.bBulletESP) {
        DrawBulletESP();
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    m_renderTime = std::chrono::duration<float, std::milli>(endTime - startTime).count();
}

bool ESP::IsCurrentTarget(const EntitySnapshot& entity, const std::optional<EntitySnapshot>& currentTargetOpt) const {
    // ✅ PERFORMANCE: Use passed target instead of calling GetCurrentTarget() again (Mutex lock!)
    if (!currentTargetOpt) return false;
    return currentTargetOpt->id == entity.id;
}

void ESP::DrawPlayerESP(const EntitySnapshot& entity, const Utils::Vector2& feetScreenPos, const Utils::Vector2& headScreenPos, float alphaMultiplier, const std::optional<EntitySnapshot>& targetOpt) {
    const float boxHeight = abs(feetScreenPos.y - headScreenPos.y);
    if (boxHeight < 1.0f) return;
    const float boxWidth = boxHeight * Offsets::GameConstants::PLAYER_BOX_WIDTH_FACTOR;
    const Utils::Vector2 topLeft = { feetScreenPos.x - boxWidth / 2.0f, headScreenPos.y };

    if (g_Settings.ESP.bBoxes) DrawBox(entity, topLeft, boxWidth, boxHeight, alphaMultiplier, targetOpt);
    if (g_Settings.ESP.bHealthBars) DrawHealthBar(entity, topLeft, boxWidth, boxHeight, alphaMultiplier, targetOpt);
    DrawInfoLabel(entity, alphaMultiplier, targetOpt);
    if (g_Settings.ESP.bSkeletons) DrawSkeleton(entity, feetScreenPos, targetOpt);
    if (g_Settings.ESP.bTracers) DrawTracer(entity, feetScreenPos);
}

void ESP::DrawGroundVehicleESP(const EntitySnapshot& entity, const Utils::Vector2& screenPos, float alphaMultiplier, const std::optional<EntitySnapshot>& targetOpt) {
    // Dynamic size based on distance (via box height in snapshot)
    float boxHeight = abs(entity.feetScreenPos.y - entity.headScreenPos.y);
    float boxWidth = boxHeight * Offsets::GameConstants::VEHICLE_BOX_WIDTH_FACTOR;
    Utils::Vector2 topLeft = { entity.feetScreenPos.x - boxWidth / 2.0f, entity.headScreenPos.y };

    if (g_Settings.ESP.bBoxes) {
        // Check if this entity is the current target
        bool isTarget = IsCurrentTarget(entity, targetOpt);
        const float* color;
        if (isTarget && g_Settings.ESP.bHighlightTarget) {
            static const float targetColor[4] = {1.0f, 0.6f, 0.0f, 1.0f};
            color = targetColor;
        } else {
            color = GetTeamColor(entity.team);
        }
        DrawVehicleBoxWithBorder(topLeft, boxWidth, boxHeight, color, alphaMultiplier);
    }
    DrawInfoLabel(entity, alphaMultiplier, targetOpt);
}

void ESP::DrawAirVehicleESP(const EntitySnapshot& entity, const Utils::Vector2& screenPos, float alphaMultiplier, const std::optional<EntitySnapshot>& targetOpt) {
    float boxHeight = abs(entity.feetScreenPos.y - entity.headScreenPos.y);
    float hexagonSize = boxHeight * Offsets::GameConstants::AIR_VEHICLE_SIZE_FACTOR; // Adjust size
    Utils::Vector2 center = { entity.feetScreenPos.x, entity.headScreenPos.y + boxHeight / 2.0f };

    if (g_Settings.ESP.bBoxes) {
        // Check if this entity is the current target
        bool isTarget = IsCurrentTarget(entity, targetOpt);
        const float* color;
        if (isTarget && g_Settings.ESP.bHighlightTarget) {
            static const float targetColor[4] = {1.0f, 0.6f, 0.0f, 1.0f};
            color = targetColor;
        } else {
            color = GetTeamColor(entity.team);
        }
        DrawHexagonWithBorder(center, hexagonSize, color, alphaMultiplier);
    }
    DrawInfoLabel(entity, alphaMultiplier, targetOpt);
}

void ESP::DrawTurretESP(const EntitySnapshot& entity, const Utils::Vector2& screenPos, float alphaMultiplier, const std::optional<EntitySnapshot>& targetOpt) {
    float boxHeight = abs(entity.feetScreenPos.y - entity.headScreenPos.y);
    float size = boxHeight * Offsets::GameConstants::TURRET_SIZE_FACTOR;
    Utils::Vector2 center = { entity.feetScreenPos.x, entity.headScreenPos.y + boxHeight / 2.0f };
    
    if (g_Settings.ESP.bBoxes) {
        // Check if this entity is the current target
        bool isTarget = IsCurrentTarget(entity, targetOpt);
        const float* color;
        if (isTarget && g_Settings.ESP.bHighlightTarget) {
            static const float targetColor[4] = {1.0f, 0.6f, 0.0f, 1.0f};
            color = targetColor;
        } else {
            color = GetTeamColor(entity.team);
        }
        DrawXWithBorder(center, size, color, alphaMultiplier);
    }
    DrawInfoLabel(entity, alphaMultiplier, targetOpt);
}

void ESP::DrawOthersESP(const EntitySnapshot& entity, const Utils::Vector2& screenPos, float alphaMultiplier, const std::optional<EntitySnapshot>& targetOpt) {
    float boxHeight = abs(entity.feetScreenPos.y - entity.headScreenPos.y);
    float size = boxHeight;
    Utils::Vector2 center = { entity.feetScreenPos.x, entity.headScreenPos.y + boxHeight / 2.0f };

    if (g_Settings.ESP.bBoxes) {
        // Check if this entity is the current target
        bool isTarget = IsCurrentTarget(entity, targetOpt);
        const float* color;
        if (isTarget && g_Settings.ESP.bHighlightTarget) {
            static const float targetColor[4] = {1.0f, 0.6f, 0.0f, 1.0f};
            color = targetColor;
        } else {
            color = GetTeamColor(entity.team);
        }
        DrawTriangleWithBorder(center, size, color, alphaMultiplier);
    }
    DrawInfoLabel(entity, alphaMultiplier, targetOpt);
}

void ESP::DrawBox(const EntitySnapshot& entity, const Utils::Vector2& topLeft, float width, float height, float alphaMultiplier, const std::optional<EntitySnapshot>& targetOpt) {
    bool isTarget = IsCurrentTarget(entity, targetOpt);
    if (isTarget && g_Settings.ESP.bHighlightTarget) {
        const float targetColor[4] = { 1.0f, 0.6f, 0.0f, 1.0f };
        const float targetBorder[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        g_Renderer->DrawBox(topLeft, width, height, targetBorder, 3.0f);
        g_Renderer->DrawBox(topLeft, width, height, targetColor, 2.5f);
        g_Renderer->DrawCornerBox(topLeft, width, height, targetColor, 2.0f);
    } else {
        const float* color = GetTeamColor(entity.team);
        float teamColorWithAlpha[4] = { color[0], color[1], color[2], color[3] * alphaMultiplier };
        float blackBorder[4] = { 0.0f, 0.0f, 0.0f, 1.0f * alphaMultiplier };
        g_Renderer->DrawBox(topLeft, width, height, blackBorder, 2.5f);
        g_Renderer->DrawBox(topLeft, width, height, teamColorWithAlpha, 1.5f);
        g_Renderer->DrawCornerBox(topLeft, width, height, teamColorWithAlpha, 1.0f);
    }
}

void ESP::DrawHealthBar(const EntitySnapshot& entity, const Utils::Vector2& topLeft, float width, float height, float alphaMultiplier, const std::optional<EntitySnapshot>& targetOpt) {
    // Bar parameters - dynamically scale based on box height
    const float barWidth = height * Offsets::GameConstants::HEALTH_BAR_WIDTH_FACTOR;
    const float barGap = height * Offsets::GameConstants::HEALTH_BAR_GAP_FACTOR;
    const float barSpacing = height * Offsets::GameConstants::HEALTH_BAR_SPACING_FACTOR;
    const Utils::Vector2 barPos = { topLeft.x - barGap - barWidth, topLeft.y };

    // Calculate health and shield percentages
    float healthPercent = entity.GetHealthPercent();
    float shieldPercent = entity.GetShieldPercent();

    // Distance-based border calculation
    float distance = entity.distanceToLocalPlayer;

    // Border color interpolation: Black at 40m, Bar color at 50m
    float colorInterpolation = 0.0f;
    if (distance <= 50.0f) {
        if (distance <= 40.0f) {
            colorInterpolation = 0.0f;
        }
        else {
            // Linear interpolation between 40m and 50m (0.0 = Black, 1.0 = Bar color)
            colorInterpolation = (distance - 40.0f) / (50.0f - 40.0f); // 0.0 bis 1.0
        }
    }
    else {
        colorInterpolation = 1.0f; // 100% Bar color at 50m and beyond
    }

    // Shield bar (upper half) - 50/50 split
    const float shieldBarHeight = (height - barSpacing) * 0.5f;
    const Utils::Vector2 shieldBarPos = { barPos.x, barPos.y };

    // Shield bar foreground
    if (shieldPercent > 0.0f) {
        float shieldHeight = shieldBarHeight * shieldPercent;
        Utils::Vector2 shieldPos = { shieldBarPos.x, shieldBarPos.y + (shieldBarHeight - shieldHeight) };

        // Brighter cyan for better visibility
        float brightCyan[4] = { 0.0f, 1.0f, 1.0f, 0.9f * alphaMultiplier };
        g_Renderer->DrawFilledBox(shieldPos, barWidth, shieldHeight, brightCyan);

        // Border only around actual shield value (only if alpha > 0)
        if (alphaMultiplier > 0.0f) {
            // Interpolate between black and cyan based on distance
            float shieldBorderColor[4] = {
                0.0f * (1.0f - colorInterpolation) + 0.0f * colorInterpolation,  // R: Black to cyan
                0.0f * (1.0f - colorInterpolation) + 1.0f * colorInterpolation,  // G: Black to cyan
                0.0f * (1.0f - colorInterpolation) + 1.0f * colorInterpolation,  // B: Black to cyan
                alphaMultiplier
            };
            g_Renderer->DrawBox(shieldPos, barWidth, shieldHeight, shieldBorderColor, 1.0f);
        }
    }

    // Health bar (lower half) - 50/50 split
    const float healthBarHeight = (height - barSpacing) * Offsets::GameConstants::HEALTH_BAR_HEIGHT_FACTOR;
    const Utils::Vector2 healthBarPos = { barPos.x, barPos.y + shieldBarHeight + barSpacing };

    // Health bar foreground
    if (healthPercent > 0.0f) {
        float healthHeight = healthBarHeight * healthPercent;
        Utils::Vector2 healthPos = { healthBarPos.x, healthBarPos.y + (healthBarHeight - healthHeight) };

        // Improved colors based on health percentage
        float healthColor[4];
        if (healthPercent > 0.6f) {
            // Bright green
            healthColor[0] = 0.0f; healthColor[1] = 1.0f; healthColor[2] = 0.0f; healthColor[3] = 0.9f * alphaMultiplier;
        }
        else if (healthPercent > 0.3f) {
            // Bright orange/yellow
            healthColor[0] = 1.0f; healthColor[1] = 0.6f; healthColor[2] = 0.0f; healthColor[3] = 0.9f * alphaMultiplier;
        }
        else {
            // Helles Rot
            healthColor[0] = 1.0f; healthColor[1] = 0.0f; healthColor[2] = 0.0f; healthColor[3] = 0.9f * alphaMultiplier;
        }

        g_Renderer->DrawFilledBox(healthPos, barWidth, healthHeight, healthColor);

        // Border only around actual health value (only if alpha > 0)
        if (alphaMultiplier > 0.0f) {
            // Interpolate between black and health color based on distance
            float healthBorderColor[4];
            if (healthPercent > 0.6f) {
                // Bright green
                healthBorderColor[0] = 0.0f * (1.0f - colorInterpolation) + 0.0f * colorInterpolation;
                healthBorderColor[1] = 0.0f * (1.0f - colorInterpolation) + 1.0f * colorInterpolation;
                healthBorderColor[2] = 0.0f * (1.0f - colorInterpolation) + 0.0f * colorInterpolation;
            }
            else if (healthPercent > 0.3f) {
                // Bright orange/yellow
                healthBorderColor[0] = 0.0f * (1.0f - colorInterpolation) + 1.0f * colorInterpolation;  // R
                healthBorderColor[1] = 0.0f * (1.0f - colorInterpolation) + 0.6f * colorInterpolation;  // G
                healthBorderColor[2] = 0.0f * (1.0f - colorInterpolation) + 0.0f * colorInterpolation;  // B
            }
            else {
                // Helles Rot
                healthBorderColor[0] = 0.0f * (1.0f - colorInterpolation) + 1.0f * colorInterpolation;  // R
                healthBorderColor[1] = 0.0f * (1.0f - colorInterpolation) + 0.0f * colorInterpolation;  // G
                healthBorderColor[2] = 0.0f * (1.0f - colorInterpolation) + 0.0f * colorInterpolation;  // B
            }
            healthBorderColor[3] = alphaMultiplier;
            g_Renderer->DrawBox(healthPos, barWidth, healthHeight, healthBorderColor, 1.0f);
        }
    }
}

void ESP::DrawSkeleton(const EntitySnapshot& entity, const Utils::Vector2& screenPos, const std::optional<EntitySnapshot>& targetOpt) {
    // Check if this entity is the current target
    bool isTarget = IsCurrentTarget(entity, targetOpt);

    // Get color for skeleton (orange if targeted, team color otherwise)
    const float* skeletonColor;
    if (isTarget && g_Settings.ESP.bHighlightTarget) {
        const float targetColor[4] = { 1.0f, 0.6f, 0.0f, 1.0f }; // Orange
        skeletonColor = targetColor;
    }
    else {
        skeletonColor = GetTeamColor(entity.team);
    }

    // Calculate correct head position based on entity type
    Utils::Vector3 correctHeadPosition;
    if (IsMAXUnit(entity.type)) {
        // For MAX units, use entity position + AIMBOT_MAX_HEAD_HEIGHT (same as aimbot)
        correctHeadPosition = entity.position;
        correctHeadPosition.y += Offsets::GameConstants::AIMBOT_MAX_HEAD_HEIGHT;
    } else {
        // For normal players, use the calculated headPosition
        correctHeadPosition = entity.headPosition;
    }

    // Convert actual 3D positions to screen coordinates
    Utils::Vector2 actualHeadScreenPos;
    Utils::Vector2 actualFeetScreenPos;
    
    if (!g_Game->WorldToScreen(correctHeadPosition, actualHeadScreenPos)) {
        return; // Head not on screen
    }
    
    if (!g_Game->WorldToScreen(entity.position, actualFeetScreenPos)) {
        return; // Feet not on screen
    }

    // Draw circle at actual head position - size based on distance/box size
    // Calculate dynamic circle radius based on actual head-to-feet distance
    float actualHeight = abs(actualHeadScreenPos.y - actualFeetScreenPos.y);
    float circleRadius = actualHeight * Offsets::GameConstants::SKELETON_CIRCLE_RADIUS_FACTOR; // 8% of actual height, scales with distance

    g_Renderer->DrawCircle(actualHeadScreenPos, circleRadius, skeletonColor, 64, 2.0f);

    // Draw line from actual head to actual feet
    g_Renderer->DrawLine(actualHeadScreenPos, actualFeetScreenPos, skeletonColor, 2.0f);
}

void ESP::DrawTracer(const EntitySnapshot& entity, const Utils::Vector2& screenPos) {
    if (!g_Renderer) return;
    Utils::Vector2 screenCenter = g_Renderer->GetScreenCenter();
    const float* color = GetTeamColor(entity.team);
    float tracerColor[4] = { color[0], color[1], color[2], 0.7f };
    g_Renderer->DrawLine(screenCenter, entity.headScreenPos, tracerColor, 1.5f);
}

void ESP::DrawInfoLabel(const EntitySnapshot& entity, float alphaMultiplier, const std::optional<EntitySnapshot>& targetOpt) {
    // Only draw if name, distance, or type is enabled
    if (!g_Settings.ESP.bNames && !g_Settings.ESP.bShowDistance && !g_Settings.ESP.bShowType) {
        return;
    }

    // Check if this entity is the current target
    bool isTarget = IsCurrentTarget(entity, targetOpt);

    // 1. Build the text strings with corrected logic
    float distance = entity.distanceToLocalPlayer;

    char line1[128] = "";
    char line2[128] = "";
    bool line1HasContent = false;

    // Determine the content for the first line
    // MODIFIED: Only show names for Player types (infantry), not for vehicles, turrets, or others
    if (g_Settings.ESP.bNames && IsPlayerType(entity.type)) {
        strcat_s(line1, sizeof(line1), entity.name.c_str());
        line1HasContent = true;
    }
    else if (g_Settings.ESP.bShowType) {
        if (entity.type == EntityType::Unknown || !IsKnownEntityType(entity.type)) {
            //sprintf_s(line1, sizeof(line1), "Type: %d", static_cast<int>(entity.type));
        }
        else {
            strcat_s(line1, sizeof(line1), GetEntityTypeString(entity.type).c_str());
        }
        line1HasContent = true;
    }

    // Append distance to the first line if enabled
    if (g_Settings.ESP.bShowDistance) {
        char distanceStr[32];
        sprintf_s(distanceStr, "%.0fm", distance);
        if (line1HasContent) {
            strcat_s(line1, sizeof(line1), " "); // Add space
        }
        strcat_s(line1, sizeof(line1), distanceStr);
        line1HasContent = true;
    }

    // Determine content for the second line
    // MODIFIED: Only show type information for non-Player entities, or if names are disabled
    if (g_Settings.ESP.bNames && g_Settings.ESP.bShowType && IsPlayerType(entity.type)) {
        // For players, show type in second line if both name and type are enabled
        if (entity.type == EntityType::Unknown || !IsKnownEntityType(entity.type)) {
            sprintf_s(line2, sizeof(line2), "Type: %d", static_cast<int>(entity.type));
        }
        else {
            strcat_s(line2, sizeof(line2), GetEntityTypeString(entity.type).c_str());
        }
    }
    else if (g_Settings.ESP.bShowType && !IsPlayerType(entity.type)) {
        // For non-players (vehicles, turrets, others), show type in first line if names are disabled
        // This is handled in the first line logic above
    }

    // 2. Calculate text dimensions using the Renderer's precise CalcTextSize function
    const float fontSize = isTarget ? 15.0f : 13.0f;
    const float typeFontSize = fontSize * 0.9f;
    const ImVec2 primarySize = g_Renderer->CalcTextSize(line1, fontSize);
    const ImVec2 secondarySize = g_Renderer->CalcTextSize(line2, typeFontSize);
    const float lineSpacing = 0.0f;

    // DYNAMIC POSITIONING BASED ON WORLD-SPACE 
    Utils::Vector3 labelAnchorWorldPos = entity.position;

    // Adjust Y offset based on entity type for better placement
    if (IsPlayerType(entity.type))          labelAnchorWorldPos.y += Offsets::GameConstants::LABEL_PLAYER_OFFSET;
    else if (IsGroundVehicleType(entity.type)) labelAnchorWorldPos.y += Offsets::GameConstants::LABEL_GROUND_VEHICLE_OFFSET;
    else if (IsAirVehicleType(entity.type))    labelAnchorWorldPos.y += Offsets::GameConstants::LABEL_AIR_VEHICLE_OFFSET;
    else if (IsTurretType(entity.type))        labelAnchorWorldPos.y += Offsets::GameConstants::LABEL_TURRET_OFFSET;
    else if (IsOthersType(entity.type))        labelAnchorWorldPos.y += Offsets::GameConstants::LABEL_OTHER_OFFSET;
    else                                    labelAnchorWorldPos.y += Offsets::GameConstants::LABEL_DEFAULT_OFFSET; // Default fallback

    Utils::Vector2 labelAnchorScreenPos;
    if (!g_Game->WorldToScreen(labelAnchorWorldPos, labelAnchorScreenPos)) {
        return;
    }

    // 3. Render based on target status
    if (isTarget && g_Settings.ESP.bHighlightTarget) {
        const float horizontalPadding = 5.0f;
        const float verticalPadding = 2.0f;
        const float barWidth = 4.0f;
        const float rounding = 3.0f;

        float maxTextWidth = (primarySize.x > secondarySize.x) ? primarySize.x : secondarySize.x;
        float totalWidth = barWidth + horizontalPadding + maxTextWidth + horizontalPadding;

        float totalHeight = verticalPadding;
        if (strlen(line1) > 0) totalHeight += primarySize.y;
        if (strlen(line2) > 0) totalHeight += lineSpacing + secondarySize.y;
        totalHeight += verticalPadding;

        Utils::Vector2 labelTopLeft = {
            labelAnchorScreenPos.x - totalWidth / 2.0f,
            labelAnchorScreenPos.y
        };

        const float targetBgColor[4] = { 1.0f, 0.6f, 0.0f, 0.8f };
        const float targetBarColor[4] = { 1.0f, 0.8f, 0.0f, 1.0f };
        const float targetTextColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

        g_Renderer->DrawRoundedFilledBox(labelTopLeft, totalWidth, totalHeight, targetBgColor, rounding);
        g_Renderer->DrawRoundedFilledBox(labelTopLeft, barWidth, totalHeight, targetBarColor, rounding);

        float contentAreaX = labelTopLeft.x + barWidth;
        float contentAreaWidth = totalWidth - barWidth;
        float currentY = labelTopLeft.y + verticalPadding;

        if (strlen(line1) > 0) {
            Utils::Vector2 primaryPos = {
                contentAreaX + (contentAreaWidth / 2.0f) - (primarySize.x / 2.0f),
                currentY
            };
            g_Renderer->DrawText(primaryPos, line1, targetTextColor, fontSize);
            currentY += primarySize.y + lineSpacing;
        }

        if (strlen(line2) > 0) {
            Utils::Vector2 secondaryPos = {
                contentAreaX + (contentAreaWidth / 2.0f) - (secondarySize.x / 2.0f),
                currentY
            };
            g_Renderer->DrawText(secondaryPos, line2, targetTextColor, typeFontSize);
        }
    }
    else {
        // CLEAN SOLUTION: Use the *same* consistent 3D anchor for perfect centering.
        const float whiteText[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        const float blackBorder[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

        float currentY = labelAnchorScreenPos.y;

        if (strlen(line1) > 0) {
            // CORRECTED: Use labelAnchorScreenPos.x as the base for centering, no manual offset needed.
            Utils::Vector2 primaryPos = { labelAnchorScreenPos.x - primarySize.x / 2.0f, currentY };
            g_Renderer->DrawText({ primaryPos.x - 1, primaryPos.y - 1 }, line1, blackBorder, fontSize);
            g_Renderer->DrawText({ primaryPos.x + 1, primaryPos.y - 1 }, line1, blackBorder, fontSize);
            g_Renderer->DrawText({ primaryPos.x - 1, primaryPos.y + 1 }, line1, blackBorder, fontSize);
            g_Renderer->DrawText({ primaryPos.x + 1, primaryPos.y + 1 }, line1, blackBorder, fontSize);
            g_Renderer->DrawText(primaryPos, line1, whiteText, fontSize);
            currentY += primarySize.y + lineSpacing;
        }

        if (strlen(line2) > 0) {
            // CORRECTED: Use labelAnchorScreenPos.x as the base for centering, no manual offset needed.
            Utils::Vector2 secondaryPos = { labelAnchorScreenPos.x - secondarySize.x / 2.0f, currentY };
            g_Renderer->DrawText({ secondaryPos.x - 1, secondaryPos.y - 1 }, line2, blackBorder, typeFontSize);
            g_Renderer->DrawText({ secondaryPos.x + 1, secondaryPos.y - 1 }, line2, blackBorder, typeFontSize);
            g_Renderer->DrawText({ secondaryPos.x - 1, secondaryPos.y + 1 }, line2, blackBorder, typeFontSize);
            g_Renderer->DrawText({ secondaryPos.x + 1, secondaryPos.y + 1 }, line2, blackBorder, typeFontSize);
            g_Renderer->DrawText(secondaryPos, line2, whiteText, typeFontSize);
        }
    }
}

void ESP::DrawBulletESP() {
    // Check if game is available
    if (!g_Game) return;

    // Update bullet tracking (independent of Magic Bullet)
    UpdateBulletTracking();

    // Draw all tracked bullets
    std::lock_guard<std::mutex> lock(m_bulletTrackingMutex);

    for (const auto& bullet : m_trackedBullets) {
        // Check if bullet is still alive
        bool isAlive = false;
        if (!g_Game->GetMemory()->Read(bullet.pBullet + Offsets::MagicBullet::bullet_is_alive_offset, isAlive) || !isAlive) {
            continue; // Skip dead bullets
        }

        // Read current bullet position
        Utils::Vector3 bulletPosition;
        if (!g_Game->GetMemory()->Read(bullet.pBullet + Offsets::MagicBullet::bullet_position_offset, bulletPosition)) {
            continue; // Skip if position can't be read
        }

        // Read direction vector
        Utils::Vector3 bulletDirection;
        if (!g_Game->GetMemory()->Read(bullet.pBullet + Offsets::MagicBullet::bullet_direction_offset, bulletDirection)) {
            continue; // Skip if direction can't be read
        }

        // Calculate tracer start position (behind bullet)
        const float tracerLength = Offsets::GameConstants::TRACER_LENGTH;
        Utils::Vector3 tracerStartPosition = bulletPosition - (bulletDirection * tracerLength);

        // Convert to screen coordinates
        Utils::Vector2 startScreenPos, endScreenPos;
        bool startOnScreen = g_Game->WorldToScreen(tracerStartPosition, startScreenPos);
        bool endOnScreen = g_Game->WorldToScreen(bulletPosition, endScreenPos);

        // Only draw if at least one point is on screen
        if (!startOnScreen && !endOnScreen) {
            continue;
        }

        // Draw tracer line
        const float whiteColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        const float redBorder[4] = { 1.0f, 0.0f, 0.0f, 1.0f };  // Red border

        // Draw red border (thicker)
        g_Renderer->DrawLine(startScreenPos, endScreenPos, redBorder, 3.0f);

        // Draw white tracer (thinner)
        g_Renderer->DrawLine(startScreenPos, endScreenPos, whiteColor, 2.0f);

        // Draw bullet point
        if (endOnScreen) {
            const float whitePoint[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
            const float orangeBorder[4] = { 1.0f, 0.5f, 0.0f, 1.0f };  // Orange border

            // Draw orange border
            g_Renderer->DrawFilledCircle(endScreenPos, 4.0f, orangeBorder);

            // Draw white point
            g_Renderer->DrawFilledCircle(endScreenPos, 3.0f, whitePoint);
        }
    }
}

void ESP::UpdateBulletTracking() {
    // Check if game is available
    if (!g_Game) return;

    // Read bullet pointer directly from game (independent of Magic Bullet)
    uintptr_t baseAddress = g_Game->GetMemory()->GetBaseAddress();
    uintptr_t bulletPointerBase = baseAddress + Offsets::MagicBullet::pCurrentBulletPointerBase;

    // Read current bullet pointer
    uintptr_t currentBulletPtr = 0;
    uintptr_t basePtr = 0;
    uintptr_t bulletPtr = 0;

    if (!g_Game->GetMemory()->Read(bulletPointerBase, basePtr) || basePtr == 0) {
        return; // No bullet pointer found
    }

    if (!g_Game->GetMemory()->Read(basePtr + Offsets::MagicBullet::pCurrentBulletPointerOffsets[0], bulletPtr) || bulletPtr == 0) {
        return; // No current bullet
    }

    currentBulletPtr = bulletPtr;

    // Check if this is a new bullet (different from last tracked)
    if (currentBulletPtr != m_lastTrackedBulletPtr && currentBulletPtr != 0) {
        m_lastTrackedBulletPtr = currentBulletPtr;

        // ✅ HYBRID: Track ALL bullets for ESP (no distance check)
        // Add to tracked bullets
        std::lock_guard<std::mutex> lock(m_bulletTrackingMutex);

        ESPBulletData newBullet;
        newBullet.pBullet = currentBulletPtr;
        newBullet.CreationTime = GetTickCount64();
        newBullet.IsAlive = true;

        m_trackedBullets.push_back(newBullet);
    }

    // Cleanup old bullets (older than 5 seconds or dead)
    std::lock_guard<std::mutex> lock(m_bulletTrackingMutex);
    uint64_t currentTime = GetTickCount64();

    m_trackedBullets.erase(
        std::remove_if(m_trackedBullets.begin(), m_trackedBullets.end(),
            [this, currentTime](const ESPBulletData& bullet) {
                // Remove if older than 5 seconds
                if (currentTime - bullet.CreationTime > 5000) {
                    return true;
                }

                // Check if bullet is still alive
                bool isAlive = false;
                if (g_Game->GetMemory()->Read(bullet.pBullet + Offsets::MagicBullet::bullet_is_alive_offset, isAlive)) {
                    return !isAlive; // Remove if dead
                }

                return true; // Remove if we can't read the status
            }),
        m_trackedBullets.end()
    );
}

bool ESP::IsKnownEntityType(EntityType type) const {
    return IsPlayerType(type) || IsGroundVehicleType(type) || IsAirVehicleType(type) || 
           IsTurretType(type) || IsOthersType(type);
}

void ESP::LogUnknownEntity(const EntitySnapshot& entity) const {
    // Create unique key from entity type (only type, since ID is not available)
    std::string key = "Type:" + std::to_string(static_cast<int>(entity.type));

    // Check if this combination has already been logged
    if (s_loggedUnknownEntities.find(key) == s_loggedUnknownEntities.end()) {
        // New unknown entity - log it
        Logger::Log("[UNKNOWN ENTITY] Type: %d, Name: '%s', ID: 0x%llX, Position: (%.1f, %.1f, %.1f)",
            static_cast<int>(entity.type),
            entity.name.c_str(),
            entity.id,
            entity.position.x,
            entity.position.y,
            entity.position.z);

        // Add to set so it won't be logged again
        s_loggedUnknownEntities.insert(key);
    }
}

void ESP::DrawGMDetection() {
    auto worldSnapshot = g_Game->GetWorldSnapshot();
    if (!worldSnapshot) return;
    
    std::vector<const EntitySnapshot*> detectedGMs;
    for (const auto& entity : worldSnapshot->entities) {
        if (entity.IsGM() || (entity.name == "TRMillerLaw")) {
            detectedGMs.push_back(&entity);
        }
    }

    if (!detectedGMs.empty()) {
        DrawCentralGMWarning(static_cast<int>(detectedGMs.size()));
        
        for (const auto* gm : detectedGMs) {
            if (gm->isOnScreen) {
                const float gmColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
                g_Renderer->DrawBox(gm->feetScreenPos, 20.0f, 20.0f, gmColor, 3.0f);
                g_Renderer->DrawText(gm->feetScreenPos, "GM", gmColor);
            }
        }
    }
}

void ESP::DrawCentralGMWarning(int gmCount) {
    Utils::Vector2 center = g_Renderer->GetScreenCenter();
    Utils::Vector2 warningPos = { center.x - 100, center.y - 50 };
    
    std::string warningText = "WARNING: " + std::to_string(gmCount) + " GM(s) DETECTED!";
    const float warningColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
    g_Renderer->DrawText(warningPos, warningText.c_str(), warningColor);
}

const float* ESP::GetTeamColor(EFaction team) const {
    switch (team) {
        case EFaction::VS: return g_Settings.ESP.Colors.VS;
        case EFaction::NC: return g_Settings.ESP.Colors.NC;
        case EFaction::TR: return g_Settings.ESP.Colors.TR;
        case EFaction::NSO: return g_Settings.ESP.Colors.NSO;
        default: return g_Settings.ESP.Colors.VS;
    }
}

void ESP::DrawVehicleBoxWithBorder(const Utils::Vector2& topLeft, float width, float height, const float* color, float alphaMultiplier) {
    float colorWithAlpha[4] = { color[0], color[1], color[2], color[3] * alphaMultiplier };
    float blackBorder[4] = { 0.0f, 0.0f, 0.0f, alphaMultiplier };
    
    g_Renderer->DrawBox(topLeft, width, height, blackBorder, 2.0f);
    g_Renderer->DrawBox(topLeft, width, height, colorWithAlpha, 1.0f);
}

void ESP::DrawTriangleWithBorder(const Utils::Vector2& center, float size, const float* color, float alphaMultiplier) {
    float colorWithAlpha[4] = { color[0], color[1], color[2], color[3] * alphaMultiplier };
    float blackBorder[4] = { 0.0f, 0.0f, 0.0f, alphaMultiplier };
    
    Utils::Vector2 top = { center.x, center.y - size/2 };
    Utils::Vector2 left = { center.x - size/2, center.y + size/2 };
    Utils::Vector2 right = { center.x + size/2, center.y + size/2 };
    
    g_Renderer->DrawLine(top, left, blackBorder, 3.0f);
    g_Renderer->DrawLine(left, right, blackBorder, 3.0f);
    g_Renderer->DrawLine(right, top, blackBorder, 3.0f);
    
    g_Renderer->DrawLine(top, left, colorWithAlpha, 2.0f);
    g_Renderer->DrawLine(left, right, colorWithAlpha, 2.0f);
    g_Renderer->DrawLine(right, top, colorWithAlpha, 2.0f);
}

void ESP::DrawXWithBorder(const Utils::Vector2& center, float size, const float* color, float alphaMultiplier) {
    float colorWithAlpha[4] = { color[0], color[1], color[2], color[3] * alphaMultiplier };
    float blackBorder[4] = { 0.0f, 0.0f, 0.0f, alphaMultiplier };
    
    Utils::Vector2 topLeft = { center.x - size/2, center.y - size/2 };
    Utils::Vector2 topRight = { center.x + size/2, center.y - size/2 };
    Utils::Vector2 bottomLeft = { center.x - size/2, center.y + size/2 };
    Utils::Vector2 bottomRight = { center.x + size/2, center.y + size/2 };
    
    g_Renderer->DrawLine(topLeft, bottomRight, blackBorder, 3.0f);
    g_Renderer->DrawLine(topRight, bottomLeft, blackBorder, 3.0f);
    
    g_Renderer->DrawLine(topLeft, bottomRight, colorWithAlpha, 2.0f);
    g_Renderer->DrawLine(topRight, bottomLeft, colorWithAlpha, 2.0f);
}

void ESP::DrawHexagonWithBorder(const Utils::Vector2& center, float size, const float* color, float alphaMultiplier) {
    float colorWithAlpha[4] = { color[0], color[1], color[2], color[3] * alphaMultiplier };
    float blackBorder[4] = { 0.0f, 0.0f, 0.0f, alphaMultiplier };
    
    std::vector<Utils::Vector2> points;
    for (int i = 0; i < 6; i++) {
        float angle = i * Offsets::GameConstants::HEXAGON_ANGLE_STEP * Offsets::GameConstants::DEGREES_TO_RADIANS;
        Utils::Vector2 point = {
            center.x + size * cos(angle),
            center.y + size * sin(angle)
        };
        points.push_back(point);
    }
    
    for (size_t i = 0; i < points.size(); i++) {
        Utils::Vector2 current = points[i];
        Utils::Vector2 next = points[(i + 1) % points.size()];
        
        g_Renderer->DrawLine(current, next, blackBorder, 3.0f);
        g_Renderer->DrawLine(current, next, colorWithAlpha, 2.0f);
    }
}