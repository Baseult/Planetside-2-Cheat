#pragma once
#include "Settings.h"
#include <string>
#include <fstream>

namespace DX11Base {

/**
 * @brief Settings Manager for persistent configuration storage
 * @details Handles saving and loading of settings to/from simple text file
 */
class SettingsManager {
public:
    static const std::string CONFIG_FILE_PATH;
    
    /**
     * @brief Load settings from configuration file
     * @return true if settings were loaded successfully, false otherwise
     */
    static bool LoadSettings();
    
    /**
     * @brief Save current settings to configuration file
     * @return true if settings were saved successfully, false otherwise
     */
    static bool SaveSettings();
    
    /**
     * @brief Reset settings to default values
     */
    static void ResetToDefaults();

private:
    /**
     * @brief Write a boolean value to file
     */
    static void WriteBool(std::ofstream& file, const std::string& name, bool value);
    
    /**
     * @brief Write an integer value to file
     */
    static void WriteInt(std::ofstream& file, const std::string& name, int value);
    
    /**
     * @brief Write a float value to file
     */
    static void WriteFloat(std::ofstream& file, const std::string& name, float value);
    
    /**
     * @brief Write a string value to file
     */
    static void WriteString(std::ofstream& file, const std::string& name, const std::string& value);
    
    /**
     * @brief Write a color array to file
     */
    static void WriteColor(std::ofstream& file, const std::string& name, const float color[4]);
    
    /**
     * @brief Read a boolean value from file
     */
    static bool ReadBool(std::ifstream& file, const std::string& name, bool defaultValue = false);
    
    /**
     * @brief Read an integer value from file
     */
    static int ReadInt(std::ifstream& file, const std::string& name, int defaultValue = 0);
    
    /**
     * @brief Read a float value from file
     */
    static float ReadFloat(std::ifstream& file, const std::string& name, float defaultValue = 0.0f);
    
    /**
     * @brief Read a string value from file
     */
    static std::string ReadString(std::ifstream& file, const std::string& name, const std::string& defaultValue = "");
    
    /**
     * @brief Read a color array from file
     */
    static void ReadColor(std::ifstream& file, const std::string& name, float color[4], const float defaultColor[4] = nullptr);
};

} // namespace DX11Base
