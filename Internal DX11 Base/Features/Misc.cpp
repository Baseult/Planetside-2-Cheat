#include "pch.h"
#include "Misc.h"
#include "TargetManager.h"
#include "../Game/BulletHarvester.h" // WICHTIG!
#include "../Renderer/Renderer.h"
#include "../Utils/Logger.h"
#include <sstream>
#include <iomanip>

#include "ESP.h"
#include "MagicBullet.h"
#include "../Game/Game.h"

Misc::Misc() {
    Logger::Log("Misc class created");
    m_lastFrameTime = std::chrono::high_resolution_clock::now();
    m_lastFPSTime = m_lastFrameTime;
}

Misc::~Misc() {
    Logger::Log("Misc class destroyed");
}

void Misc::Render() {
    // FPS Counter
    if (g_Settings.Misc.bShowFPS) {
        DrawFPS();
    }
    
    // FOV Circle
    if (g_Settings.Misc.bShowFOVCircle) {
        DrawFOVCircle();
    }
    
    // Crosshair
    if (g_Settings.Misc.bShowCrosshair) {
        DrawCrosshair();
    }
    
    // Target visualization
    if (g_Settings.Targeting.bShowTracer) {
        DrawTargetTracer();
    }
    if (g_Settings.Targeting.bShowTargetInfo) {
        DrawTargetInfoPanel();
    }
    
    // Magic Bullet visualization
    DrawMagicBulletInfo();
    
    // Performance Info
    DrawPerformanceInfo();
}

void Misc::Update() {
    CalculateFPS();
    UpdateFrameTime();
}

void Misc::DrawFPS() {
    if (!g_Renderer) return;
    
    // Format FPS text
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << m_fps << " FPS";
    
    // Position (top right)
    Utils::Vector2 screenSize = g_Renderer->GetScreenSize();
    Utils::Vector2 fpsPos = Utils::Vector2(screenSize.x - 100, 10);
    
    // Color based on FPS
    const float* color;
    if (m_fps >= 60) color = Renderer::COLOR_GREEN;
    else if (m_fps >= 30) color = Renderer::COLOR_YELLOW;
    else color = Renderer::COLOR_RED;
    
    g_Renderer->DrawText(fpsPos, ss.str().c_str(), color, 14.0f);
}

void Misc::DrawFOVCircle() {
    if (!g_Renderer) return;
    
    // FOV circle in the center of the screen
    Utils::Vector2 screenCenter = g_Renderer->GetScreenCenter();
    float fovRadius = g_Settings.Targeting.fFOV; // Use Targeting FOV as radius
    
    // Draw circle
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
    
    // Performance info
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
        // NEU: Holt die Zahl vom Harvester
        if (g_BulletHarvester) {
            ss << "Tracked Bullets: " << g_BulletHarvester->GetBulletCount();
        }
    }
    
    // Position (top left)
    Utils::Vector2 perfPos = Utils::Vector2(10, 10);
    g_Renderer->DrawText(perfPos, ss.str().c_str(), Renderer::COLOR_CYAN, 12.0f);
}

void Misc::CalculateFPS() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - m_lastFPSTime);
    
    // Add FPS sample
    float currentFPS = 1000000.0f / deltaTime.count();
    m_fpsSamples[m_fpsSampleIndex] = currentFPS;
    m_fpsSampleIndex = (m_fpsSampleIndex + 1) % FPS_SAMPLE_COUNT;
    
    // Calculate average FPS
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

// DrawTargetInfo() function removed - now DrawTargetInfoPanel()

void Misc::DrawTargetTracer() {
    if (!g_TargetManager || !g_Settings.Targeting.bShowTracer) return;

    auto targetOpt = g_TargetManager->GetCurrentTarget();
    if (!targetOpt) return;

    const EntitySnapshot& target = *targetOpt;

    // Use the pre-calculated screen position!
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

    // Draw Target Info Panel bottom right
    Utils::Vector2 screenSize = g_Renderer->GetScreenSize();
    const float panelWidth = 180.0f; // Reduced from 280 to 250 (-30px)
    const float panelHeight = 190.0f; // Reduced from 200 to 190 (-10px)
    const float margin = 20.0f;

    // Position bottom right
    Utils::Vector2 panelPos = {
        screenSize.x - panelWidth - margin,
        screenSize.y - panelHeight - margin
    };

    // Panel background
    const float panelBgColor[4] = { 0.0f, 0.0f, 0.0f, 0.8f };
    const float panelBorderColor[4] = { 1.0f, 0.6f, 0.0f, 1.0f }; // Orange-yellow border

    g_Renderer->DrawRoundedFilledBox(panelPos, panelWidth, panelHeight, panelBgColor, 5.0f);
    g_Renderer->DrawBox(panelPos, panelWidth, panelHeight, panelBorderColor, 2.0f);

    // Header "TARGET INFO"
    const float headerColor[4] = { 1.0f, 0.6f, 0.0f, 1.0f };
    Utils::Vector2 headerPos = { panelPos.x + 10.0f, panelPos.y + 8.0f };
    g_Renderer->DrawText(headerPos, "TARGET INFO", headerColor, 16.0f);

    // Target name
    const float nameColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    Utils::Vector2 namePos = { panelPos.x + 10.0f, panelPos.y + 30.0f };
    g_Renderer->DrawText(namePos, target.name.c_str(), nameColor, 14.0f);

    // Distance (moved further up)
    float distance = target.distanceToLocalPlayer;
    char distanceText[32];
    sprintf_s(distanceText, sizeof(distanceText), "Distance: %.0fm", distance);
    Utils::Vector2 distancePos = { panelPos.x + 10.0f, panelPos.y + 50.0f };
    g_Renderer->DrawText(distancePos, distanceText, nameColor, 12.0f);

    // Team (moved further up)
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

    // Entity Type (moved further up)
    std::string typeString = GetEntityTypeString(target.type);
    char typeText[64];
    sprintf_s(typeText, sizeof(typeText), "Type: %s", typeString.c_str());
    Utils::Vector2 typePos = { panelPos.x + 10.0f, panelPos.y + 90.0f };
    g_Renderer->DrawText(typePos, typeText, nameColor, 12.0f);

    // Shield Bar (top - first)
    const float shieldPercent = target.GetShieldPercent();
    const float shieldBarWidth = 150.0f; // Reduced from 180 to 150
    const float shieldBarHeight = 12.0f;
    const float shieldBarX = panelPos.x + 10.0f;
    const float shieldBarY = panelPos.y + 130.0f; // Moved further down from 110 to 130

    // Shield Bar background
    const float shieldBgColor[4] = { 0.3f, 0.3f, 0.3f, 0.8f };
    g_Renderer->DrawRoundedFilledBox({ shieldBarX, shieldBarY }, shieldBarWidth, shieldBarHeight, shieldBgColor, 2.0f);

    // Shield Bar foreground
    const float shieldColor[4] = { 0.0f, 0.5f, 1.0f, 1.0f }; // Blue
    const float shieldFillWidth = shieldBarWidth * shieldPercent;
    g_Renderer->DrawRoundedFilledBox({ shieldBarX, shieldBarY }, shieldFillWidth, shieldBarHeight, shieldColor, 2.0f);

    // Shield Text
    char shieldText[32];
    sprintf_s(shieldText, sizeof(shieldText), "Shield: %.0f/%.0f", target.shield, target.maxShield);
    Utils::Vector2 shieldTextPos = { shieldBarX, shieldBarY - 15.0f };
    g_Renderer->DrawText(shieldTextPos, shieldText, nameColor, 11.0f);

    // Health Bar (bottom - after)
    const float healthPercent = target.GetHealthPercent();
    const float healthBarWidth = 150.0f; // Reduced from 180 to 150
    const float healthBarHeight = 12.0f;
    const float healthBarX = panelPos.x + 10.0f;
    const float healthBarY = panelPos.y + 160.0f; // Moved further down from 140 to 160

    // Health Bar background
    const float healthBgColor[4] = { 0.3f, 0.3f, 0.3f, 0.8f };
    g_Renderer->DrawRoundedFilledBox({ healthBarX, healthBarY }, healthBarWidth, healthBarHeight, healthBgColor, 2.0f);

    // Health Bar foreground
    const float healthColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f }; // Green
    const float healthFillWidth = healthBarWidth * healthPercent;
    g_Renderer->DrawRoundedFilledBox({ healthBarX, healthBarY }, healthFillWidth, healthBarHeight, healthColor, 2.0f);

    // Health Text
    char healthText[32];
    sprintf_s(healthText, sizeof(healthText), "Health: %.0f/%.0f", target.health, target.maxHealth);
    Utils::Vector2 healthTextPos = { healthBarX, healthBarY - 15.0f };
    g_Renderer->DrawText(healthTextPos, healthText, nameColor, 11.0f);
}

void Misc::DrawMagicBulletInfo() {
    if (!g_Settings.MagicBullet.bEnabled) return;
    
    // Magic Bullet Info removed - now via TargetManager
}