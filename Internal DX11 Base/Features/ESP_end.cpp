
// NEW FUNCTIONS: Diagonal box and HealthBar
void ESP::DrawDiagonalBox(const EntitySnapshot& entity, const Utils::Vector2& tl, const Utils::Vector2& tr, const Utils::Vector2& bl, const Utils::Vector2& br, float alphaMultiplier) {
    bool isTarget = IsCurrentTarget(entity);
    const float* color;
    float thickness = 1.5f;

    if (isTarget && g_Settings.ESP.bHighlightTarget) {
        const static float targetColor[4] = { 1.0f, 0.6f, 0.0f, 1.0f };
        color = targetColor;
        thickness = 2.5f;
    } else {
        color = GetTeamColor(entity.team);
    }
    
    float finalColor[4] = { color[0], color[1], color[2], alphaMultiplier };
    const float black[4] = { 0.0f, 0.0f, 0.0f, alphaMultiplier };

    // Black border
    g_Renderer->DrawLine(tl, tr, black, thickness + 1.0f);
    g_Renderer->DrawLine(tr, br, black, thickness + 1.0f);
    g_Renderer->DrawLine(br, bl, black, thickness + 1.0f);
    g_Renderer->DrawLine(bl, tl, black, thickness + 1.0f);

    // Main color
    g_Renderer->DrawLine(tl, tr, finalColor, thickness);
    g_Renderer->DrawLine(tr, br, finalColor, thickness);
    g_Renderer->DrawLine(br, bl, finalColor, thickness);
    g_Renderer->DrawLine(bl, tl, finalColor, thickness);
}

void ESP::DrawDiagonalHealthBar(const EntitySnapshot& entity, const Utils::Vector2& tl, const Utils::Vector2& tr, const Utils::Vector2& bl, const Utils::Vector2& br, float alphaMultiplier) {
    const float barWidth = 4.0f;
    const float barGap = 2.0f;

    // Vector of the left edge of the box
    Utils::Vector2 sideVec = bl - tl;
    // Perpendicular vector to the left
    Utils::Vector2 perpVec = { -sideVec.y, sideVec.x };
    perpVec.Normalize();

    // Eckpunkte f�r den Hintergrund der Bar
    Utils::Vector2 bar_tr = tl - perpVec * barGap;
    Utils::Vector2 bar_br = bl - perpVec * barGap;
    Utils::Vector2 bar_tl = bar_tr - perpVec * barWidth;
    Utils::Vector2 bar_bl = bar_br - perpVec * barWidth;

    const float bgColor[4] = { 0.1f, 0.1f, 0.1f, 0.7f * alphaMultiplier };
    g_Renderer->DrawQuadFilled(bar_tl, bar_tr, bar_br, bar_bl, bgColor);

    // Schild (obere H�lfte, f�llt sich von oben nach unten)
    float shieldPercent = entity.GetShieldPercent();
    if (shieldPercent > 0.0f) {
        Utils::Vector2 shield_bl = bar_tl + (bar_bl - bar_tl) * shieldPercent;
        Utils::Vector2 shield_br = bar_tr + (bar_br - bar_tr) * shieldPercent;
        const float shieldColor[4] = { 0.0f, 1.0f, 1.0f, 0.9f * alphaMultiplier };
        g_Renderer->DrawQuadFilled(bar_tl, bar_tr, shield_br, shield_bl, shieldColor);
    }

    // Gesundheit (untere H�lfte, f�llt sich von unten nach oben)
    float healthPercent = entity.GetHealthPercent();
    if (healthPercent > 0.0f) {
        Utils::Vector2 health_tl = bar_bl - (bar_bl - bar_tl) * healthPercent;
        Utils::Vector2 health_tr = bar_br - (bar_br - bar_tr) * healthPercent;
        
        float healthColorRGB[3];
        if (healthPercent > 0.6f) { healthColorRGB[0] = 0.0f; healthColorRGB[1] = 1.0f; healthColorRGB[2] = 0.0f; }
        else if (healthPercent > 0.3f) { healthColorRGB[0] = 1.0f; healthColorRGB[1] = 0.6f; healthColorRGB[2] = 0.0f; }
        else { healthColorRGB[0] = 1.0f; healthColorRGB[1] = 0.0f; healthColorRGB[2] = 0.0f; }
        const float healthColor[4] = { healthColorRGB[0], healthColorRGB[1], healthColorRGB[2], 0.9f * alphaMultiplier };
        
        g_Renderer->DrawQuadFilled(health_tl, health_tr, bar_br, bar_bl, healthColor);
    }
}
