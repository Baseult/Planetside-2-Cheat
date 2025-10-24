#include "pch.h"
#include "Renderer.h"
#include "../Utils/Logger.h"
#include <cmath>

// Static colors
const float Renderer::COLOR_WHITE[4] = {1.0f, 1.0f, 1.0f, 1.0f};
const float Renderer::COLOR_RED[4] = {1.0f, 0.0f, 0.0f, 1.0f};
const float Renderer::COLOR_GREEN[4] = {0.0f, 1.0f, 0.0f, 1.0f};
const float Renderer::COLOR_BLUE[4] = {0.0f, 0.0f, 1.0f, 1.0f};
const float Renderer::COLOR_YELLOW[4] = {1.0f, 1.0f, 0.0f, 1.0f};
const float Renderer::COLOR_PURPLE[4] = {1.0f, 0.0f, 1.0f, 1.0f};
const float Renderer::COLOR_ORANGE[4] = {1.0f, 0.5f, 0.0f, 1.0f};
const float Renderer::COLOR_CYAN[4] = {0.0f, 1.0f, 1.0f, 1.0f};

Renderer::Renderer() {
    Logger::Log("Renderer class created");
}

Renderer::~Renderer() {
    Logger::Log("Renderer class destroyed");
}

void Renderer::Initialize(ImDrawList* drawList, ImVec2 screenSize) {
    m_drawList = drawList;
    m_screenSize = screenSize;
    
    // ✅ PERFORMANCE: Update cached screen center when screen size changes
    m_screenCenter = Utils::Vector2(screenSize.x * 0.5f, screenSize.y * 0.5f);
    m_screenCenterInitialized = true;
}

void Renderer::DrawLine(const Utils::Vector2& from, const Utils::Vector2& to, const float* color, float thickness) {
    if (!m_drawList) return;
    
    m_drawList->AddLine(
        Vector2ToImVec2(from),
        Vector2ToImVec2(to),
        ColorToImU32(color),
        thickness
    );
}

void Renderer::DrawBox(const Utils::Vector2& pos, float width, float height, const float* color, float thickness) {
    if (!m_drawList) return;
    
    ImVec2 topLeft = Vector2ToImVec2(pos);
    ImVec2 bottomRight = ImVec2(pos.x + width, pos.y + height);
    
    m_drawList->AddRect(
        topLeft,
        bottomRight,
        ColorToImU32(color),
        0.0f, // rounding
        ImDrawFlags_None,
        thickness
    );
}

void Renderer::DrawFilledBox(const Utils::Vector2& pos, float width, float height, const float* color) {
    if (!m_drawList) return;
    
    ImVec2 topLeft = Vector2ToImVec2(pos);
    ImVec2 bottomRight = ImVec2(pos.x + width, pos.y + height);
    
    m_drawList->AddRectFilled(
        topLeft,
        bottomRight,
        ColorToImU32(color)
    );
}

void Renderer::DrawRoundedFilledBox(const Utils::Vector2& pos, float width, float height, const float* color, float rounding) {
    if (!m_drawList) return;
    
    ImVec2 topLeft = Vector2ToImVec2(pos);
    ImVec2 bottomRight = ImVec2(pos.x + width, pos.y + height);
    
    m_drawList->AddRectFilled(
        topLeft,
        bottomRight,
        ColorToImU32(color),
        rounding
    );
}

void Renderer::DrawText(const Utils::Vector2& pos, const char* text, const float* color, float fontSize) {
    if (!m_drawList || !text) return;
    
    m_drawList->AddText(
        nullptr, // font (nullptr = default)
        fontSize,
        Vector2ToImVec2(pos),
        ColorToImU32(color),
        text
    );
}

void Renderer::DrawTextCentered(const Utils::Vector2& pos, const char* text, const float* color, float fontSize) {
    if (!m_drawList || !text) return;
    
    // Verwende die neue CalcTextSize Funktion mit der korrekten fontSize
    ImVec2 textSize = CalcTextSize(text, fontSize);
    Utils::Vector2 centeredPos = Utils::Vector2(pos.x - textSize.x * 0.5f, pos.y - textSize.y * 0.5f);
    
    DrawText(centeredPos, text, color, fontSize);
}

ImVec2 Renderer::CalcTextSize(const char* text, float fontSize) {
    if (!text) return ImVec2(0, 0);
    
    // Hole den aktuellen Font aus ImGui
    ImGuiIO& io = ImGui::GetIO();
    ImFont* font = io.Fonts->Fonts[0]; // Verwende den Default-Font
    
    // Verwende CalcTextSizeA mit der spezifischen fontSize
    return font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text);
}

void Renderer::DrawCircle(const Utils::Vector2& center, float radius, const float* color, int segments, float thickness) {
    if (!m_drawList) return;
    
    m_drawList->AddCircle(
        Vector2ToImVec2(center),
        radius,
        ColorToImU32(color),
        segments,
        thickness
    );
}

void Renderer::DrawFilledCircle(const Utils::Vector2& center, float radius, const float* color, int segments) {
    if (!m_drawList) return;
    
    m_drawList->AddCircleFilled(
        Vector2ToImVec2(center),
        radius,
        ColorToImU32(color),
        segments
    );
}

void Renderer::DrawHealthBar(const Utils::Vector2& pos, float width, float height, float healthPercent, float shieldPercent) {
    if (!m_drawList) return;
    
    // Background (black)
    DrawFilledBox(pos, width, height, COLOR_WHITE);
    
    // Health bar (red/green based on percentage)
    float healthWidth = width * healthPercent;
    Utils::Vector2 healthPos = pos;
    const float* healthColor = (healthPercent > 0.5f) ? COLOR_GREEN : 
                              (healthPercent > 0.25f) ? COLOR_YELLOW : COLOR_RED;
    DrawFilledBox(healthPos, healthWidth, height, healthColor);
    
    // Shield bar (blue, if present)
    if (shieldPercent > 0.0f) {
        float shieldWidth = width * shieldPercent;
        Utils::Vector2 shieldPos = Utils::Vector2(pos.x, pos.y - height - 2.0f);
        DrawFilledBox(shieldPos, shieldWidth, height, COLOR_CYAN);
    }
    
    // Border
    DrawBox(pos, width, height, COLOR_WHITE, 1.0f);
}

void Renderer::DrawSkeleton(const std::vector<Utils::Vector2>& bonePositions, const float* color, float thickness) {
    if (!m_drawList || bonePositions.size() < 2) return;
    
    // TODO: Implement skeleton connections based on bone IDs
    // For now: Simple lines between all bones
    for (size_t i = 0; i < bonePositions.size() - 1; i++) {
        DrawLine(bonePositions[i], bonePositions[i + 1], color, thickness);
    }
}

void Renderer::DrawTracer(const Utils::Vector2& from, const Utils::Vector2& to, const float* color, float thickness) {
    DrawLine(from, to, color, thickness);
}

void Renderer::DrawFOVCircle(const Utils::Vector2& center, float radius, const float* color, float thickness) {
    DrawCircle(center, radius, color, 64, thickness);
}

void Renderer::DrawCrosshair(const Utils::Vector2& center, float size, const float* color, float thickness) {
    if (!m_drawList) return;
    
    float halfSize = size * 0.5f;
    
    // Horizontal
    DrawLine(
        Utils::Vector2(center.x - halfSize, center.y),
        Utils::Vector2(center.x + halfSize, center.y),
        color, thickness
    );
    
    // Vertical
    DrawLine(
        Utils::Vector2(center.x, center.y - halfSize),
        Utils::Vector2(center.x, center.y + halfSize),
        color, thickness
    );
}

void Renderer::DrawCornerBox(const Utils::Vector2& pos, float width, float height, const float* color, float thickness) {
    if (!m_drawList) return;
    
    float cornerLength = (width < height ? width : height) * 0.2f;
    
    // Top left
    DrawLine(pos, Utils::Vector2(pos.x + cornerLength, pos.y), color, thickness);
    DrawLine(pos, Utils::Vector2(pos.x, pos.y + cornerLength), color, thickness);
    
    // Top right
    Utils::Vector2 topRight = Utils::Vector2(pos.x + width, pos.y);
    DrawLine(topRight, Utils::Vector2(topRight.x - cornerLength, topRight.y), color, thickness);
    DrawLine(topRight, Utils::Vector2(topRight.x, topRight.y + cornerLength), color, thickness);
    
    // Bottom left
    Utils::Vector2 bottomLeft = Utils::Vector2(pos.x, pos.y + height);
    DrawLine(bottomLeft, Utils::Vector2(bottomLeft.x + cornerLength, bottomLeft.y), color, thickness);
    DrawLine(bottomLeft, Utils::Vector2(bottomLeft.x, bottomLeft.y - cornerLength), color, thickness);
    
    // Bottom right
    Utils::Vector2 bottomRight = Utils::Vector2(pos.x + width, pos.y + height);
    DrawLine(bottomRight, Utils::Vector2(bottomRight.x - cornerLength, bottomRight.y), color, thickness);
    DrawLine(bottomRight, Utils::Vector2(bottomRight.x, bottomRight.y - cornerLength), color, thickness);
}

ImU32 Renderer::ColorToImU32(const float* color) const {
    return ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], color[3]));
}

ImVec2 Renderer::Vector2ToImVec2(const Utils::Vector2& vec) const {
    return ImVec2(vec.x, vec.y);
}
