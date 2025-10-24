/**
 * @file Renderer.h
 * @brief DirectX 11 rendering wrapper for ImGui
 * @details This file contains the rendering system that provides a clean API
 *          for drawing 2D elements using DirectX 11 and ImGui. It includes
 *          optimized drawing functions for ESP, UI elements, and overlays.
 */

#pragma once
#include "../Utils/Vector.h"
#include "../Framework/imgui.h"
#include <memory>

/**
 * @brief Rendering system class
 * @details Provides a high-performance rendering API for 2D elements
 *          including lines, boxes, text, circles, health bars, and more.
 */
class Renderer {
public:
    /**
     * @brief Constructor
     */
    Renderer();
    
    /**
     * @brief Destructor
     */
    ~Renderer();
    
    /**
     * @brief Initialize renderer for each frame
     * @param drawList ImGui draw list
     * @param screenSize Screen dimensions
     */
    void Initialize(ImDrawList* drawList, ImVec2 screenSize);
    
    // Drawing Functions
    /**
     * @brief Draw a line
     * @param from Start position
     * @param to End position
     * @param color Line color (RGBA)
     * @param thickness Line thickness
     */
    void DrawLine(const Utils::Vector2& from, const Utils::Vector2& to, const float* color, float thickness = 1.5f);
    
    /**
     * @brief Draw a box outline
     * @param pos Position
     * @param width Box width
     * @param height Box height
     * @param color Box color (RGBA)
     * @param thickness Line thickness
     */
    void DrawBox(const Utils::Vector2& pos, float width, float height, const float* color, float thickness = 1.5f);
    
    /**
     * @brief Draw a filled box
     * @param pos Position
     * @param width Box width
     * @param height Box height
     * @param color Box color (RGBA)
     */
    void DrawFilledBox(const Utils::Vector2& pos, float width, float height, const float* color);
    
    /**
     * @brief Draw a rounded filled box
     * @param pos Position
     * @param width Box width
     * @param height Box height
     * @param color Box color (RGBA)
     * @param rounding Corner rounding
     */
    void DrawRoundedFilledBox(const Utils::Vector2& pos, float width, float height, const float* color, float rounding);
    
    /**
     * @brief Draw text
     * @param pos Position
     * @param text Text to draw
     * @param color Text color (RGBA)
     * @param fontSize Font size
     */
    void DrawText(const Utils::Vector2& pos, const char* text, const float* color, float fontSize = 13.0f);
    
    /**
     * @brief Draw centered text
     * @param pos Position
     * @param text Text to draw
     * @param color Text color (RGBA)
     * @param fontSize Font size
     */
    void DrawTextCentered(const Utils::Vector2& pos, const char* text, const float* color, float fontSize = 13.0f);
    
    /**
     * @brief Calculate text size
     * @param text Text to measure
     * @param fontSize Font size
     * @return Text dimensions
     */
    ImVec2 CalcTextSize(const char* text, float fontSize);
    
    /**
     * @brief Draw a circle outline
     * @param center Center position
     * @param radius Circle radius
     * @param color Circle color (RGBA)
     * @param segments Number of segments
     * @param thickness Line thickness
     */
    void DrawCircle(const Utils::Vector2& center, float radius, const float* color, int segments = 64, float thickness = 1.5f);
    
    /**
     * @brief Draw a filled circle
     * @param center Center position
     * @param radius Circle radius
     * @param color Circle color (RGBA)
     * @param segments Number of segments
     */
    void DrawFilledCircle(const Utils::Vector2& center, float radius, const float* color, int segments = 64);
    
    /**
     * @brief Draw a health bar
     * @param pos Position
     * @param width Bar width
     * @param height Bar height
     * @param healthPercent Health percentage (0.0 to 1.0)
     * @param shieldPercent Shield percentage (0.0 to 1.0)
     */
    void DrawHealthBar(const Utils::Vector2& pos, float width, float height, float healthPercent, float shieldPercent = 0.0f);
    
    /**
     * @brief Draw a skeleton
     * @param bonePositions Bone positions
     * @param color Skeleton color (RGBA)
     * @param thickness Line thickness
     */
    void DrawSkeleton(const std::vector<Utils::Vector2>& bonePositions, const float* color, float thickness = 1.5f);
    
    /**
     * @brief Draw a tracer line
     * @param from Start position
     * @param to End position
     * @param color Tracer color (RGBA)
     * @param thickness Line thickness
     */
    void DrawTracer(const Utils::Vector2& from, const Utils::Vector2& to, const float* color, float thickness = 1.0f);
    
    // Helper functions
    void DrawFOVCircle(const Utils::Vector2& center, float radius, const float* color, float thickness = 1.5f);
    void DrawCrosshair(const Utils::Vector2& center, float size, const float* color, float thickness = 1.5f);
    void DrawCornerBox(const Utils::Vector2& pos, float width, float height, const float* color, float thickness = 1.5f);
    
    // Screen info
    Utils::Vector2 GetScreenSize() const { return Utils::Vector2(m_screenSize.x, m_screenSize.y); }
    Utils::Vector2 GetScreenCenter() const { 
        // ✅ PERFORMANCE: Return cached screen center (calculated in Initialize)
        return m_screenCenter;
    }
    
private:
    ImDrawList* m_drawList;
    ImVec2 m_screenSize;
    
    // ✅ PERFORMANCE: Cache screen center to avoid repeated calculations
    mutable Utils::Vector2 m_screenCenter;
    mutable bool m_screenCenterInitialized = false;
    
    // Helper functions
    ImU32 ColorToImU32(const float* color) const;
    ImVec2 Vector2ToImVec2(const Utils::Vector2& vec) const;
    
    // Colors (public for external use)
public:
    static const float COLOR_WHITE[4];
    static const float COLOR_RED[4];
    static const float COLOR_GREEN[4];
    static const float COLOR_BLUE[4];
    static const float COLOR_YELLOW[4];
    static const float COLOR_PURPLE[4];
    static const float COLOR_ORANGE[4];
    static const float COLOR_CYAN[4];
};

// Global instance
inline std::unique_ptr<Renderer> g_Renderer;
