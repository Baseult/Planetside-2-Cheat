#pragma once
#include "../Renderer/Renderer.h"
#include "../Utils/Settings.h"
#include "../Game/SDK.h"
#include <chrono>
#include <memory>

class Misc {
public:
    Misc();
    ~Misc();
    
    // Main functions
    void Render();  // FPS Counter, FOV Circle, etc.
    void Update();  // Performance measurements
    
    // Performance-Monitoring
    float GetFPS() const { return m_fps; }
    float GetFrameTime() const { return m_frameTime; }
    
private:
    // Rendering functions
    void DrawFPS();
    void DrawFOVCircle();
    void DrawCrosshair();
    void DrawTargetTracer(); // NEW: Tracer to target
    void DrawTargetInfoPanel(); // NEW: Target info panel
    void DrawMagicBulletInfo(); // NEW: Magic Bullet visualization
    void DrawPerformanceInfo();
    
    // Update functions
    void CalculateFPS();
    void UpdateFrameTime();
    
    // Performance variables
    float m_fps = 0.0f;
    float m_frameTime = 0.0f;
    std::chrono::high_resolution_clock::time_point m_lastFrameTime;
    std::chrono::high_resolution_clock::time_point m_lastFPSTime;
    int m_frameCount = 0;
    
    // FPS calculation
    static constexpr int FPS_SAMPLE_COUNT = 60;
    float m_fpsSamples[FPS_SAMPLE_COUNT] = {0};
    int m_fpsSampleIndex = 0;
};

// Global instance
inline std::unique_ptr<Misc> g_Misc;
