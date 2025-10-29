#include "pch.h"
#include "Misc.h"
#include "TargetManager.h"
#include "../Game/BulletHarvester.h"
#include "../Renderer/Renderer.h"
#include "../Utils/Logger.h"
#include <sstream>
#include <iomanip>

#include "ESP.h"
#include "MagicBullet.h"
#include "../Game/Game.h"
#include "../Game/Offsets.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static inline float ClampFloat(float v, float lo, float hi) {
    return (v < lo) ? lo : ((v > hi) ? hi : v);
}

Misc::Misc() {
    Logger::Log("Misc class created");
    m_lastFrameTime = std::chrono::high_resolution_clock::now();
    m_lastFPSTime = m_lastFrameTime;
}

Misc::~Misc() {
    Logger::Log("Misc class destroyed");
}

void Misc::Render() {
    if (g_Settings.Misc.bShowFPS) {
        DrawFPS();
    }
    
    if (g_Settings.Misc.bShowFOVCircle) {
        DrawFOVCircle();
    }
    
    if (g_Settings.Misc.bShowCrosshair) {
        DrawCrosshair();
    }
    
    if (g_Settings.Targeting.bShowTracer) {
        DrawTargetTracer();
    }
    if (g_Settings.Targeting.bShowTargetInfo) {
        DrawTargetInfoPanel();
    }
    
    DrawMagicBulletInfo();
    
    if (g_Settings.Misc.bShowRadar) {
        DrawRadar();
    }
    
    DrawPerformanceInfo();
}

void Misc::Update() {
    CalculateFPS();
    UpdateFrameTime();
}

void Misc::DrawFPS() {
    if (!g_Renderer) return;
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << m_fps << " FPS";
    
    Utils::Vector2 screenSize = g_Renderer->GetScreenSize();
    Utils::Vector2 fpsPos = Utils::Vector2(screenSize.x - 100, 10);
    
    const float* color;
    if (m_fps >= 60) color = Renderer::COLOR_GREEN;
    else if (m_fps >= 30) color = Renderer::COLOR_YELLOW;
    else color = Renderer::COLOR_RED;
    
    g_Renderer->DrawText(fpsPos, ss.str().c_str(), color, 14.0f);
}

void Misc::DrawFOVCircle() {
    if (!g_Renderer) return;
    
    Utils::Vector2 screenCenter = g_Renderer->GetScreenCenter();
    float fovRadius = g_Settings.Targeting.fFOV; // Use Targeting FOV as radius
    
    g_Renderer->DrawFOVCircle(screenCenter, fovRadius, Renderer::COLOR_WHITE, 1.5f);
}

void Misc::DrawCrosshair() {
    if (!g_Renderer) return;
    
    Utils::Vector2 screenCenter = g_Renderer->GetScreenCenter();
    float crosshairSize = 10.0f;
    
    g_Renderer->DrawCrosshair(screenCenter, crosshairSize, Renderer::COLOR_WHITE, 2.0f);
}

void Misc::DrawPerformanceInfo() {
    if (!g_Renderer) return;
    
    std::stringstream ss;
    ss << "Frame Time: " << std::fixed << std::setprecision(2) << m_frameTime << "ms\n";
    ss << "Entities: " << g_Game->GetEntityCount() << "\n";
    ss << "Game Update: " << std::fixed << std::setprecision(2) << g_Game->GetUpdateTime() << "ms\n";
    if (g_ESP) {
        ss << "ESP Render: " << std::fixed << std::setprecision(2) << g_ESP->GetRenderTime() << "ms\n";
        ss << "ESP Entities: " << g_ESP->GetRenderedEntities() << "\n";
    }
    if (g_MagicBullet) {
        ss << "Magic Bullet Update: " << std::fixed << std::setprecision(2) 
           << g_MagicBullet->GetUpdateTime() << "ms\n";
        if (g_BulletHarvester) {
            ss << "Tracked Bullets: " << g_BulletHarvester->GetBulletCount();
        }
    }
    
    Utils::Vector2 perfPos = Utils::Vector2(10, 10);
    g_Renderer->DrawText(perfPos, ss.str().c_str(), Renderer::COLOR_CYAN, 12.0f);
}

void Misc::CalculateFPS() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - m_lastFPSTime);
    
    float currentFPS = 1000000.0f / deltaTime.count();
    m_fpsSamples[m_fpsSampleIndex] = currentFPS;
    m_fpsSampleIndex = (m_fpsSampleIndex + 1) % FPS_SAMPLE_COUNT;
    
    float totalFPS = 0.0f;
    for (int i = 0; i < FPS_SAMPLE_COUNT; i++) {
        totalFPS += m_fpsSamples[i];
    }
    m_fps = totalFPS / FPS_SAMPLE_COUNT;
    
    m_lastFPSTime = currentTime;
}

void Misc::UpdateFrameTime() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - m_lastFrameTime);
    
    m_frameTime = static_cast<float>(deltaTime.count()) / 1000.0f;
    m_lastFrameTime = currentTime;
    m_frameCount++;
}


void Misc::DrawTargetTracer() {
    if (!g_TargetManager || !g_Settings.Targeting.bShowTracer) return;

    auto targetOpt = g_TargetManager->GetCurrentTarget();
    if (!targetOpt) return;

    const EntitySnapshot& target = *targetOpt;

    g_Renderer->DrawLine(
        g_Renderer->GetScreenCenter(), 
        target.headScreenPos, 
        Renderer::COLOR_ORANGE, 
        2.0f
    );
}

void Misc::DrawTargetInfoPanel() {
    if (!g_TargetManager || !g_Settings.Targeting.bShowTargetInfo) return;

    auto targetOpt = g_TargetManager->GetCurrentTarget();
    if (!targetOpt) return;

    const EntitySnapshot& target = *targetOpt;

    Utils::Vector2 screenSize = g_Renderer->GetScreenSize();
    const float panelWidth = 180.0f;
    const float panelHeight = 190.0f;
    const float margin = 20.0f;

    Utils::Vector2 panelPos = {
        screenSize.x - panelWidth - margin,
        screenSize.y - panelHeight - margin
    };

    const float panelBgColor[4] = { 0.0f, 0.0f, 0.0f, 0.8f };
    const float panelBorderColor[4] = { 1.0f, 0.6f, 0.0f, 1.0f };

    g_Renderer->DrawRoundedFilledBox(panelPos, panelWidth, panelHeight, panelBgColor, 5.0f);
    g_Renderer->DrawBox(panelPos, panelWidth, panelHeight, panelBorderColor, 2.0f);

    const float headerColor[4] = { 1.0f, 0.6f, 0.0f, 1.0f };
    Utils::Vector2 headerPos = { panelPos.x + 10.0f, panelPos.y + 8.0f };
    g_Renderer->DrawText(headerPos, "TARGET INFO", headerColor, 16.0f);

    const float nameColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    Utils::Vector2 namePos = { panelPos.x + 10.0f, panelPos.y + 30.0f };
    g_Renderer->DrawText(namePos, target.name.c_str(), nameColor, 14.0f);

    float distance = target.distanceToLocalPlayer;
    char distanceText[32];
    sprintf_s(distanceText, sizeof(distanceText), "Distance: %.0fm", distance);
    Utils::Vector2 distancePos = { panelPos.x + 10.0f, panelPos.y + 50.0f };
    g_Renderer->DrawText(distancePos, distanceText, nameColor, 12.0f);

    const char* teamName = "Unknown";
    const float* teamColor = Renderer::COLOR_WHITE;
    switch (target.team) {
    case EFaction::VS: teamName = "Vanu Sovereignty"; teamColor = g_Settings.ESP.Colors.VS; break;
    case EFaction::NC: teamName = "New Conglomerate"; teamColor = g_Settings.ESP.Colors.NC; break;
    case EFaction::TR: teamName = "Terran Republic"; teamColor = g_Settings.ESP.Colors.TR; break;
    case EFaction::NSO: teamName = "Nanite Systems"; teamColor = g_Settings.ESP.Colors.NSO; break;
    }

    char teamText[64];
    sprintf_s(teamText, sizeof(teamText), "Team: %s", teamName);
    Utils::Vector2 teamPos = { panelPos.x + 10.0f, panelPos.y + 70.0f };
    g_Renderer->DrawText(teamPos, teamText, teamColor, 12.0f);

    std::string typeString = GetEntityTypeString(target.type);
    char typeText[64];
    sprintf_s(typeText, sizeof(typeText), "Type: %s", typeString.c_str());
    Utils::Vector2 typePos = { panelPos.x + 10.0f, panelPos.y + 90.0f };
    g_Renderer->DrawText(typePos, typeText, nameColor, 12.0f);

    const float shieldPercent = target.GetShieldPercent();
    const float shieldBarWidth = 150.0f;
    const float shieldBarHeight = 12.0f;
    const float shieldBarX = panelPos.x + 10.0f;
    const float shieldBarY = panelPos.y + 130.0f;

    const float shieldBgColor[4] = { 0.3f, 0.3f, 0.3f, 0.8f };
    g_Renderer->DrawRoundedFilledBox({ shieldBarX, shieldBarY }, shieldBarWidth, shieldBarHeight, shieldBgColor, 2.0f);

    const float shieldColor[4] = { 0.0f, 0.5f, 1.0f, 1.0f };
    const float shieldFillWidth = shieldBarWidth * shieldPercent;
    g_Renderer->DrawRoundedFilledBox({ shieldBarX, shieldBarY }, shieldFillWidth, shieldBarHeight, shieldColor, 2.0f);

    char shieldText[32];
    sprintf_s(shieldText, sizeof(shieldText), "Shield: %.0f/%.0f", target.shield, target.maxShield);
    Utils::Vector2 shieldTextPos = { shieldBarX, shieldBarY - 15.0f };
    g_Renderer->DrawText(shieldTextPos, shieldText, nameColor, 11.0f);

    const float healthPercent = target.GetHealthPercent();
    const float healthBarWidth = 150.0f;
    const float healthBarHeight = 12.0f;
    const float healthBarX = panelPos.x + 10.0f;
    const float healthBarY = panelPos.y + 160.0f;

    const float healthBgColor[4] = { 0.3f, 0.3f, 0.3f, 0.8f };
    g_Renderer->DrawRoundedFilledBox({ healthBarX, healthBarY }, healthBarWidth, healthBarHeight, healthBgColor, 2.0f);

    const float healthColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
    const float healthFillWidth = healthBarWidth * healthPercent;
    g_Renderer->DrawRoundedFilledBox({ healthBarX, healthBarY }, healthFillWidth, healthBarHeight, healthColor, 2.0f);

    char healthText[32];
    sprintf_s(healthText, sizeof(healthText), "Health: %.0f/%.0f", target.health, target.maxHealth);
    Utils::Vector2 healthTextPos = { healthBarX, healthBarY - 15.0f };
    g_Renderer->DrawText(healthTextPos, healthText, nameColor, 11.0f);
}

void Misc::DrawMagicBulletInfo() {
    if (!g_Settings.MagicBullet.bEnabled) return;
    
}

void Misc::DrawRadar() {
    if (!g_Game || !g_Renderer) return;
    auto world = g_Game->GetWorldSnapshot();
    if (!world || !world->localPlayer.isValid) return;

    // Radar placement (top-right)
    const float size = g_Settings.Misc.fRadarSize;
    const float half = size * 0.5f;
    const float margin = 10.0f;
    Utils::Vector2 screen = g_Renderer->GetScreenSize();
    Utils::Vector2 topLeft = { screen.x - size - margin, margin };
    Utils::Vector2 center = { topLeft.x + half, topLeft.y + half };

    // Background
    const float bg[4] = { 0.0f, 0.0f, 0.0f, 0.4f };
    const float border[4] = { 1.0f, 1.0f, 1.0f, 0.8f };
    g_Renderer->DrawRoundedFilledBox(topLeft, size, size, bg, 4.0f);
    g_Renderer->DrawBox(topLeft, size, size, border, 1.5f);

    // Cross lines
    g_Renderer->DrawLine({ center.x, topLeft.y }, { center.x, topLeft.y + size }, border, 1.0f);
    g_Renderer->DrawLine({ topLeft.x, center.y }, { topLeft.x + size, center.y }, border, 1.0f);

    // Orientation: rotate by local yaw to keep forward up
    const float yaw = world->localPlayer.viewAngles.x; // matches usage elsewhere
    const float rotationAngle = yaw + Offsets::GameConstants::ROTATION_OFFSET - (float)(M_PI / 2.0);
    const float cosA = std::cos(-rotationAngle); // inverse to rotate world into radar space
    const float sinA = std::sin(-rotationAngle);

    float zoom = g_Settings.Misc.fRadarZoom;
    if (zoom < 0.1f) zoom = 0.1f;
    const float metersPerPixel = 2.0f / zoom; // tune scale

    auto drawEntityPoint = [&](const EntitySnapshot& e, const float* color) {
        Utils::Vector3 delta = e.position - world->localPlayer.position;
        // Map X/Z to radar plane
        float rx = delta.x;
        float ry = delta.z;
        // rotate
        float x = rx * cosA - ry * sinA;
        float y = rx * sinA + ry * cosA;
        x /= metersPerPixel;
        y /= metersPerPixel;
        // clamp to box
        x = ClampFloat(x, -half + 2.0f, half - 2.0f);
        y = ClampFloat(y, -half + 2.0f, half - 2.0f);
        Utils::Vector2 p = { center.x + x, center.y - y };
        g_Renderer->DrawFilledCircle(p, 2.0f, color);
    };

    // Draw entities with same filters as ESP
    for (const auto& e : world->entities) {
        if (!e.isAlive) continue;
        if (!g_Settings.ESP.bTeamESP && e.team == world->localPlayer.team && e.team != EFaction::NSO) continue;
        if (e.distanceToLocalPlayer > g_Settings.ESP.fMaxDistance) continue;

        // Filters
        if (IsPlayerType(e.type)) {
            if (IsMAXUnit(e.type)) { if (!g_Settings.ESP.bShowMAX) continue; }
            else { if (!g_Settings.ESP.bShowInfantry) continue; }
        } else if (IsGroundVehicleType(e.type)) { if (!g_Settings.ESP.bShowGroundVehicles) continue; }
        else if (IsAirVehicleType(e.type)) { if (!g_Settings.ESP.bShowAirVehicles) continue; }
        else if (IsTurretType(e.type)) { if (!g_Settings.ESP.bShowTurrets) continue; }
        else if (IsOthersType(e.type)) { if (!g_Settings.ESP.bShowOthers) continue; }

        const float* color = g_Settings.ESP.Colors.VS;
        switch (e.team) {
        case EFaction::VS: color = g_Settings.ESP.Colors.VS; break;
        case EFaction::NC: color = g_Settings.ESP.Colors.NC; break;
        case EFaction::TR: color = g_Settings.ESP.Colors.TR; break;
        case EFaction::NSO: color = g_Settings.ESP.Colors.NSO; break;
        }
        drawEntityPoint(e, color);
    }

    // Local player marker
    const float selfColor[4] = { 1.0f, 0.8f, 0.0f, 1.0f };
    g_Renderer->DrawFilledCircle(center, 3.0f, selfColor);
}