#include "pch.h"
#include "SettingsManager.h"
#include "Logger.h"
#include <sstream>

namespace DX11Base {

const std::string SettingsManager::CONFIG_FILE_PATH = "ps2_cheat_config.txt";

bool SettingsManager::LoadSettings() {
    try {
        std::ifstream file(CONFIG_FILE_PATH);
        if (!file.is_open()) {
            Logger::Log("SettingsManager: No config file found, using defaults");
            return false;
        }

        // Load ESP settings
        g_Settings.ESP.bEnabled = ReadBool(file, "ESP.bEnabled", true);
        g_Settings.ESP.bBoxes = ReadBool(file, "ESP.bBoxes", true);
        g_Settings.ESP.bSkeletons = ReadBool(file, "ESP.bSkeletons", false);
        g_Settings.ESP.bHealthBars = ReadBool(file, "ESP.bHealthBars", true);
        g_Settings.ESP.bTracers = ReadBool(file, "ESP.bTracers", true);
        g_Settings.ESP.bNames = ReadBool(file, "ESP.bNames", true);
        g_Settings.ESP.bShowDistance = ReadBool(file, "ESP.bShowDistance", true);
        g_Settings.ESP.bShowType = ReadBool(file, "ESP.bShowType", true);
        g_Settings.ESP.bHighlightTarget = ReadBool(file, "ESP.bHighlightTarget", true);
        g_Settings.ESP.bBulletESP = ReadBool(file, "ESP.bBulletESP", false);
        g_Settings.ESP.bTeamESP = ReadBool(file, "ESP.bTeamESP", false);
        g_Settings.ESP.fMaxDistance = ReadFloat(file, "ESP.fMaxDistance", 500.0f);
        g_Settings.ESP.bShowInfantry = ReadBool(file, "ESP.bShowInfantry", true);
        g_Settings.ESP.bShowMAX = ReadBool(file, "ESP.bShowMAX", true);
        g_Settings.ESP.bShowGroundVehicles = ReadBool(file, "ESP.bShowGroundVehicles", true);
        g_Settings.ESP.bShowAirVehicles = ReadBool(file, "ESP.bShowAirVehicles", true);
        g_Settings.ESP.bShowTurrets = ReadBool(file, "ESP.bShowTurrets", true);
        g_Settings.ESP.bShowOthers = ReadBool(file, "ESP.bShowOthers", false);
        
        // Load ESP Colors
        float defaultVS[4] = {0.5f, 0.0f, 1.0f, 1.0f};
        ReadColor(file, "ESP.Colors.VS", g_Settings.ESP.Colors.VS, defaultVS);
        float defaultNC[4] = {0.0f, 0.5f, 1.0f, 1.0f};
        ReadColor(file, "ESP.Colors.NC", g_Settings.ESP.Colors.NC, defaultNC);
        float defaultTR[4] = {1.0f, 0.0f, 0.0f, 1.0f};
        ReadColor(file, "ESP.Colors.TR", g_Settings.ESP.Colors.TR, defaultTR);
        float defaultNSO[4] = {0.9f, 0.9f, 0.9f, 1.0f};
        ReadColor(file, "ESP.Colors.NSO", g_Settings.ESP.Colors.NSO, defaultNSO);
        
        // Load Targeting settings
        g_Settings.Targeting.bEnabled = ReadBool(file, "Targeting.bEnabled", true);
        g_Settings.Targeting.Mode = static_cast<ETargetingMode>(ReadInt(file, "Targeting.Mode", 1));
        g_Settings.Targeting.fMaxDistance = ReadFloat(file, "Targeting.fMaxDistance", 300.0f);
        g_Settings.Targeting.fFOV = ReadFloat(file, "Targeting.fFOV", 150.0f);
        g_Settings.Targeting.bShowTargetInfo = ReadBool(file, "Targeting.bShowTargetInfo", true);
        g_Settings.Targeting.bShowTracer = ReadBool(file, "Targeting.bShowTracer", true);
        g_Settings.Targeting.bTargetTeam = ReadBool(file, "Targeting.bTargetTeam", false);
        g_Settings.Targeting.bTargetName = ReadBool(file, "Targeting.bTargetName", false);
        g_Settings.Targeting.sTargetName = ReadString(file, "Targeting.sTargetName", "");
        g_Settings.Targeting.bContinuousSearch = ReadBool(file, "Targeting.bContinuousSearch", false);
        g_Settings.Targeting.bIgnoreMaxUnits = ReadBool(file, "Targeting.bIgnoreMaxUnits", true);
        g_Settings.Targeting.bIgnoreVehicles = ReadBool(file, "Targeting.bIgnoreVehicles", true);
        
        // Load MagicBullet settings
        g_Settings.MagicBullet.bEnabled = ReadBool(file, "MagicBullet.bEnabled", false);
        // Removed: MagicBullet.bTargetHead (always head)
        
        // Load Misc settings
        g_Settings.Misc.bShowFPS = ReadBool(file, "Misc.bShowFPS", true);
        g_Settings.Misc.bShowFOVCircle = ReadBool(file, "Misc.bShowFOVCircle", true);
        g_Settings.Misc.bShowCrosshair = ReadBool(file, "Misc.bShowCrosshair", false);
        g_Settings.Misc.bShowTargetInfo = ReadBool(file, "Misc.bShowTargetInfo", true);
        g_Settings.Misc.bNoRecoil = ReadBool(file, "Misc.bNoRecoil", false);
        g_Settings.Misc.NoRecoil.bEnabled = ReadBool(file, "Misc.NoRecoil.bEnabled", false);
        g_Settings.Misc.NoRecoil.iStrength = ReadInt(file, "Misc.NoRecoil.iStrength", 10);
        g_Settings.Misc.bShowRadar = ReadBool(file, "Misc.bShowRadar", false);
        g_Settings.Misc.fRadarSize = ReadFloat(file, "Misc.fRadarSize", 180.0f);
        g_Settings.Misc.fRadarZoom = ReadFloat(file, "Misc.fRadarZoom", 1.0f);
        
        // Load Aimbot settings
        g_Settings.Aimbot.bEnabled = ReadBool(file, "Aimbot.bEnabled", false);
        g_Settings.Aimbot.fSmoothing = ReadFloat(file, "Aimbot.fSmoothing", 5.0f);
        g_Settings.Aimbot.bUseHotkey = ReadBool(file, "Aimbot.bUseHotkey", false);
        g_Settings.Aimbot.iHotkey = ReadInt(file, "Aimbot.iHotkey", VK_RBUTTON);
        g_Settings.Aimbot.bAutoAim = ReadBool(file, "Aimbot.bAutoAim", true);
        g_Settings.Aimbot.bWaitingForHotkey = ReadBool(file, "Aimbot.bWaitingForHotkey", false);
        g_Settings.Aimbot.fMaxDistance = ReadFloat(file, "Aimbot.fMaxDistance", 300.0f);
        g_Settings.Aimbot.bOnlyWhenShooting = ReadBool(file, "Aimbot.bOnlyWhenShooting", true);
        
        // Load Noclip settings
        g_Settings.Noclip.bEnabled = ReadBool(file, "Noclip.bEnabled", false);
        g_Settings.Noclip.fSpeed = ReadFloat(file, "Noclip.fSpeed", 0.2f);
        g_Settings.Noclip.bUseHotkey = ReadBool(file, "Noclip.bUseHotkey", false);
        g_Settings.Noclip.iHotkey = ReadInt(file, "Noclip.iHotkey", VK_F1);
        g_Settings.Noclip.bWaitingForHotkey = ReadBool(file, "Noclip.bWaitingForHotkey", false);
        
        Logger::Log("SettingsManager: Settings loaded successfully from %s", CONFIG_FILE_PATH.c_str());
        return true;
    }
    catch (const std::exception& e) {
        Logger::Error("SettingsManager: Exception loading settings: %s", e.what());
        return false;
    }
}

bool SettingsManager::SaveSettings() {
    try {
        std::ofstream file(CONFIG_FILE_PATH);
        if (!file.is_open()) {
            Logger::Error("SettingsManager: Failed to open config file for writing");
            return false;
        }

        file << "# PS2 Cheat Configuration File\n";
        file << "# Generated automatically - do not edit manually\n\n";
        
        // Save ESP settings
        file << "[ESP]\n";
        WriteBool(file, "ESP.bEnabled", g_Settings.ESP.bEnabled);
        WriteBool(file, "ESP.bBoxes", g_Settings.ESP.bBoxes);
        WriteBool(file, "ESP.bSkeletons", g_Settings.ESP.bSkeletons);
        WriteBool(file, "ESP.bHealthBars", g_Settings.ESP.bHealthBars);
        WriteBool(file, "ESP.bTracers", g_Settings.ESP.bTracers);
        WriteBool(file, "ESP.bNames", g_Settings.ESP.bNames);
        WriteBool(file, "ESP.bShowDistance", g_Settings.ESP.bShowDistance);
        WriteBool(file, "ESP.bShowType", g_Settings.ESP.bShowType);
        WriteBool(file, "ESP.bHighlightTarget", g_Settings.ESP.bHighlightTarget);
        WriteBool(file, "ESP.bBulletESP", g_Settings.ESP.bBulletESP);
        WriteBool(file, "ESP.bTeamESP", g_Settings.ESP.bTeamESP);
        WriteFloat(file, "ESP.fMaxDistance", g_Settings.ESP.fMaxDistance);
        WriteBool(file, "ESP.bShowInfantry", g_Settings.ESP.bShowInfantry);
        WriteBool(file, "ESP.bShowMAX", g_Settings.ESP.bShowMAX);
        WriteBool(file, "ESP.bShowGroundVehicles", g_Settings.ESP.bShowGroundVehicles);
        WriteBool(file, "ESP.bShowAirVehicles", g_Settings.ESP.bShowAirVehicles);
        WriteBool(file, "ESP.bShowTurrets", g_Settings.ESP.bShowTurrets);
        WriteBool(file, "ESP.bShowOthers", g_Settings.ESP.bShowOthers);
        WriteColor(file, "ESP.Colors.VS", g_Settings.ESP.Colors.VS);
        WriteColor(file, "ESP.Colors.NC", g_Settings.ESP.Colors.NC);
        WriteColor(file, "ESP.Colors.TR", g_Settings.ESP.Colors.TR);
        WriteColor(file, "ESP.Colors.NSO", g_Settings.ESP.Colors.NSO);
        file << "\n";
        
        // Save Targeting settings
        file << "[Targeting]\n";
        WriteBool(file, "Targeting.bEnabled", g_Settings.Targeting.bEnabled);
        WriteInt(file, "Targeting.Mode", static_cast<int>(g_Settings.Targeting.Mode));
        WriteFloat(file, "Targeting.fMaxDistance", g_Settings.Targeting.fMaxDistance);
        WriteFloat(file, "Targeting.fFOV", g_Settings.Targeting.fFOV);
        WriteBool(file, "Targeting.bShowTargetInfo", g_Settings.Targeting.bShowTargetInfo);
        WriteBool(file, "Targeting.bShowTracer", g_Settings.Targeting.bShowTracer);
        WriteBool(file, "Targeting.bTargetTeam", g_Settings.Targeting.bTargetTeam);
        WriteBool(file, "Targeting.bTargetName", g_Settings.Targeting.bTargetName);
        WriteString(file, "Targeting.sTargetName", g_Settings.Targeting.sTargetName);
        WriteBool(file, "Targeting.bContinuousSearch", g_Settings.Targeting.bContinuousSearch);
        WriteBool(file, "Targeting.bIgnoreMaxUnits", g_Settings.Targeting.bIgnoreMaxUnits);
        WriteBool(file, "Targeting.bIgnoreVehicles", g_Settings.Targeting.bIgnoreVehicles);
        file << "\n";
        
        // Save MagicBullet settings
        file << "[MagicBullet]\n";
        WriteBool(file, "MagicBullet.bEnabled", g_Settings.MagicBullet.bEnabled);
        // Removed: MagicBullet.bTargetHead (always head)
        file << "\n";
        
        // Save Misc settings
        file << "[Misc]\n";
        WriteBool(file, "Misc.bShowFPS", g_Settings.Misc.bShowFPS);
        WriteBool(file, "Misc.bShowFOVCircle", g_Settings.Misc.bShowFOVCircle);
        WriteBool(file, "Misc.bShowCrosshair", g_Settings.Misc.bShowCrosshair);
        WriteBool(file, "Misc.bShowTargetInfo", g_Settings.Misc.bShowTargetInfo);
        WriteBool(file, "Misc.bNoRecoil", g_Settings.Misc.bNoRecoil);
        WriteBool(file, "Misc.NoRecoil.bEnabled", g_Settings.Misc.NoRecoil.bEnabled);
        WriteInt(file, "Misc.NoRecoil.iStrength", g_Settings.Misc.NoRecoil.iStrength);
        WriteBool(file, "Misc.bShowRadar", g_Settings.Misc.bShowRadar);
        WriteFloat(file, "Misc.fRadarSize", g_Settings.Misc.fRadarSize);
        WriteFloat(file, "Misc.fRadarZoom", g_Settings.Misc.fRadarZoom);
        file << "\n";
        
        // Save Aimbot settings
        file << "[Aimbot]\n";
        WriteBool(file, "Aimbot.bEnabled", g_Settings.Aimbot.bEnabled);
        WriteFloat(file, "Aimbot.fSmoothing", g_Settings.Aimbot.fSmoothing);
        WriteBool(file, "Aimbot.bUseHotkey", g_Settings.Aimbot.bUseHotkey);
        WriteInt(file, "Aimbot.iHotkey", g_Settings.Aimbot.iHotkey);
        WriteBool(file, "Aimbot.bAutoAim", g_Settings.Aimbot.bAutoAim);
        WriteBool(file, "Aimbot.bWaitingForHotkey", g_Settings.Aimbot.bWaitingForHotkey);
        WriteFloat(file, "Aimbot.fMaxDistance", g_Settings.Aimbot.fMaxDistance);
        WriteBool(file, "Aimbot.bOnlyWhenShooting", g_Settings.Aimbot.bOnlyWhenShooting);
        file << "\n";
        
        // Save Noclip settings
        file << "[Noclip]\n";
        WriteBool(file, "Noclip.bEnabled", g_Settings.Noclip.bEnabled);
        WriteFloat(file, "Noclip.fSpeed", g_Settings.Noclip.fSpeed);
        WriteBool(file, "Noclip.bUseHotkey", g_Settings.Noclip.bUseHotkey);
        WriteInt(file, "Noclip.iHotkey", g_Settings.Noclip.iHotkey);
        WriteBool(file, "Noclip.bWaitingForHotkey", g_Settings.Noclip.bWaitingForHotkey);
        file << "\n";
        
        Logger::Log("SettingsManager: Settings saved successfully to %s", CONFIG_FILE_PATH.c_str());
        return true;
    }
    catch (const std::exception& e) {
        Logger::Error("SettingsManager: Exception saving settings: %s", e.what());
        return false;
    }
}

void SettingsManager::ResetToDefaults() {
    g_Settings = Settings(); // Use default constructor
    Logger::Log("SettingsManager: Settings reset to defaults");
}

void SettingsManager::WriteBool(std::ofstream& file, const std::string& name, bool value) {
    file << name << "=" << (value ? "true" : "false") << "\n";
}

void SettingsManager::WriteInt(std::ofstream& file, const std::string& name, int value) {
    file << name << "=" << value << "\n";
}

void SettingsManager::WriteFloat(std::ofstream& file, const std::string& name, float value) {
    file << name << "=" << value << "\n";
}

void SettingsManager::WriteString(std::ofstream& file, const std::string& name, const std::string& value) {
    file << name << "=" << value << "\n";
}

void SettingsManager::WriteColor(std::ofstream& file, const std::string& name, const float color[4]) {
    file << name << "=" << color[0] << "," << color[1] << "," << color[2] << "," << color[3] << "\n";
}

bool SettingsManager::ReadBool(std::ifstream& file, const std::string& name, bool defaultValue) {
    file.seekg(0);
    std::string line;
    while (std::getline(file, line)) {
        if (line.find(name + "=") == 0) {
            std::string value = line.substr(name.length() + 1);
            return (value == "true");
        }
    }
    return defaultValue;
}

int SettingsManager::ReadInt(std::ifstream& file, const std::string& name, int defaultValue) {
    file.seekg(0);
    std::string line;
    while (std::getline(file, line)) {
        if (line.find(name + "=") == 0) {
            std::string value = line.substr(name.length() + 1);
            return std::stoi(value);
        }
    }
    return defaultValue;
}

float SettingsManager::ReadFloat(std::ifstream& file, const std::string& name, float defaultValue) {
    file.seekg(0);
    std::string line;
    while (std::getline(file, line)) {
        if (line.find(name + "=") == 0) {
            std::string value = line.substr(name.length() + 1);
            return std::stof(value);
        }
    }
    return defaultValue;
}

std::string SettingsManager::ReadString(std::ifstream& file, const std::string& name, const std::string& defaultValue) {
    file.seekg(0);
    std::string line;
    while (std::getline(file, line)) {
        if (line.find(name + "=") == 0) {
            return line.substr(name.length() + 1);
        }
    }
    return defaultValue;
}

void SettingsManager::ReadColor(std::ifstream& file, const std::string& name, float color[4], const float defaultColor[4]) {
    file.seekg(0);
    std::string line;
    while (std::getline(file, line)) {
        if (line.find(name + "=") == 0) {
            std::string value = line.substr(name.length() + 1);
            std::stringstream ss(value);
            std::string item;
            int i = 0;
            while (std::getline(ss, item, ',') && i < 4) {
                color[i] = std::stof(item);
                i++;
            }
            return;
        }
    }
    // Use default values if not found
    if (defaultColor) {
        for (int i = 0; i < 4; i++) {
            color[i] = defaultColor[i];
        }
    }
}

} // namespace DX11Base
