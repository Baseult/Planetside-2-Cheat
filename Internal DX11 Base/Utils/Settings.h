/**
 * @file Settings.h
 * @brief Global settings and configuration structures
 * @details This file contains all configuration structures used throughout
 *          the PS2Base framework. Settings are organized hierarchically
 *          for easy access and modification.
 */

#pragma once
#include <string>
#include <vector>
#include <Windows.h>

/**
 * @brief Targeting mode enumeration
 * @details Defines different targeting algorithms for aimbot system
 */
enum class ETargetingMode {
    FOV,        ///< Field of view based targeting
    SmartFOV,   ///< FOV filter with closest 3D distance selection
    Distance,   ///< Closest distance based targeting
    Health      ///< Lowest health based targeting
};

/**
 * @brief Global settings structure
 * @details Contains all configuration options for the framework
 */
struct Settings {
    /**
     * @brief ESP settings structure
     * @details Configuration options for ESP (Extra Sensory Perception) system
     */
    struct ESP_t {
        bool bEnabled = true;          ///< Enable/disable ESP
        bool bBoxes = true;            ///< Draw entity boxes
        bool bSkeletons = false;      ///< Draw player skeletons
        bool bHealthBars = true;       ///< Draw health bars
        bool bTracers = true;          ///< Draw tracers
        bool bNames = true;            ///< Show entity names
        bool bShowDistance = true;     ///< Show distance to entities
        bool bShowType = true;         ///< Show entity type
        bool bHighlightTarget = true;  ///< Highlight current target
        bool bBulletESP = false;       ///< Show bullet ESP
        bool bTeamESP = false;        ///< Show team members
        float fMaxDistance = 500.0f;   ///< Maximum ESP distance
        
        /**
         * @brief Color settings for different factions
         */
        struct Colors_t {
            float VS[4] = {0.5f, 0.0f, 1.0f, 1.0f};   ///< Vanu Sovereignty (Purple)
            float NC[4] = {0.0f, 0.5f, 1.0f, 1.0f};   ///< New Conglomerate (Blue)
            float TR[4] = {1.0f, 0.0f, 0.0f, 1.0f};   ///< Terran Republic (Red)
            float NSO[4] = {0.9f, 0.9f, 0.9f, 1.0f};  ///< Nanite Systems (White/grayish)
        } Colors;
    } ESP;
    
    // NEW CENTRAL TARGETING STRUCTURE
    struct Targeting_t {
        bool bEnabled = true;
        ETargetingMode Mode = ETargetingMode::SmartFOV;
        float fMaxDistance = 300.0f;
        float fFOV = 150.0f;
        bool bShowTargetInfo = true;
        bool bShowTracer = true;
        bool bTargetTeam = false;
        bool bTargetName = false;
        std::string sTargetName = "";
        bool bContinuousSearch = false;
        std::vector<std::string> availablePlayers;
        
        // Target filters (moved from MagicBullet)
        bool bIgnoreMaxUnits = true;
        bool bIgnoreVehicles = true;
    } Targeting;
    
    struct MagicBullet_t {
        bool bEnabled = false;
        // Targeting settings removed - now centralized in Targeting_t
        bool bTargetHead = true;
    } MagicBullet;
    
    struct Misc_t {
        bool bShowFPS = true;
        bool bShowFOVCircle = true;
        bool bShowCrosshair = false;
        bool bShowTargetInfo = true;
        bool bNoRecoil = false;
        
        // No Recoil settings
        struct NoRecoil_t {
            bool bEnabled = false;
            int iStrength = 10;  // Strength (1 - 100)
        } NoRecoil;
    } Misc;
    
    struct Aimbot_t {
        bool bEnabled = false;
        float fSmoothing = 5.0f;       // 1.0 = fast, 20.0 = sehr smooth
        bool bUseHotkey = false;       // Hotkey-Aktivierung
        int iHotkey = VK_RBUTTON;      // Standard: Rechte Maustaste
        bool bAutoAim = true;          // Automatically when shooting
        bool bWaitingForHotkey = false; // Hotkey-Auswahl aktiv
        
        // Advanced
        float fMaxDistance = 300.0f;   // Max Aim Distance
        bool bOnlyWhenShooting = true; // Only when shooting (like NoRecoil)
    } Aimbot;
    
    struct Noclip_t {
        bool bEnabled = false;
        float fSpeed = 0.2f;  // Movement speed per frame
        bool bUseHotkey = false;       // Hotkey-Aktivierung
        int iHotkey = VK_F1;           // Standard: F1
        bool bWaitingForHotkey = false; // Hotkey-Auswahl aktiv
        // Note: Direct memory access - no coordinate conversion needed!
        // Reading/Writing Double values directly from memory
    } Noclip;
};

inline Settings g_Settings;
