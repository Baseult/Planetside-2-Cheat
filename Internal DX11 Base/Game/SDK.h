/**
 * @file SDK.h
 * @brief Game SDK definitions and structures for PlanetSide 2
 * @details This file contains all game-specific enums, structures, and helper functions
 *          used throughout the PS2Base framework. It provides type-safe access to
 *          game entities, factions, and mathematical operations.
 */

#pragma once
#include "../Utils/Vector.h"
#include "Offsets.h"
#include <string>

/**
 * @brief Game faction enumeration
 * @details Represents the different factions in PlanetSide 2
 */
enum class EFaction : int { 
    VS = 1,   ///< Vanu Sovereignty (Purple)
    NC = 2,   ///< New Conglomerate (Blue)
    TR = 3,   ///< Terran Republic (Red)
    NSO = 4   ///< Nanite Systems (White/grayish)
};

/**
 * @brief Entity type enumeration
 * @details Complete enumeration of all entity types in PlanetSide 2
 *          This is an exact copy from the game's internal definitions
 */
enum class EntityType : uint8_t
{
    Unknown = 0,

    Citizen = 1,
    NC_Infiltrator = 2,
    NC_LightAssault = 4,
    NC_CombatMedic = 5,
    NC_Engineer = 6,
    NC_HeavyAssault = 7,
    NC_MAX = 8,
    TR_Infiltrator = 10,
    TR_LightAssault = 12,
    TR_CombatMedic = 13,
    TR_Engineer = 14,
    TR_HeavyAssault = 15,
    TR_MAX = 16,
    VS_Infiltrator = 17,
    VS_LightAssault = 19,
    VS_CombatMedic = 20,
    VS_Engineer = 21,
    VS_HeavyAssault = 22,
    VS_MAX = 23,
    
    Soldier = 253,

    Flash = 24,
    Lightning = 26,
    Harasser = 27,
    Valkyrie = 28,
    Magrider = 29,
    Prowler = 30,
    Vanguard = 31,
    Dervish = 32,
    Sunderer = 33,
    Scythe = 34,
    Mosquito = 35,
    Reaver = 36,
    Liberator = 37,
    Galaxy = 38,
    Chimera = 42,
    DropPod = 53,
    ANT = 95,
    Colossus = 156,
    Javelin = 202,

    // NS Infantry
    NS_Infiltrator = 190,
    NS_LightAssault = 191,
    NS_Medic = 192,
    NS_Engineer = 193,
    NS_HeavyAssault = 194,
    NS_MAX = 252,

    // Mines
    AV_Mines = 61,
    AI_Mines = 58,

    // Utility
    Motion_Spotter = 88,
    Ammo_Pack = 81,
    Spawn_Beacon = 69,
    Spawn_Pad = 158,
    Repair_Station = 80,

    // Turrets
    MANA_AV_Turret = 50,
    Xiphos_AI_Tower = 105,
    The_Flail = 169,
    Spitfire_Auto_Turret = 92,
    Aspis_AA_Turret = 84,
    Spear_AV_Turret = 83,
    Spear_AV_Tower = 99,
    Aspis_AA_Phalanx_Tower = 104,
    Spear_AV_Turret_2 = 140,

    // Legacy/Other
    Orbital_Mining_Drill = 66,
    Gate_Shield = 72,
    Glaive_IPC = 137,
};

/**
 * @brief Check if entity type is a player
 * @param type The entity type to check
 * @return true if the entity is a player type, false otherwise
 */
inline bool IsPlayerType(EntityType type)
{
    switch (type)
    {
        // All infantry types
    case EntityType::Citizen:
    case EntityType::NC_Infiltrator:
    case EntityType::NC_LightAssault:
    case EntityType::NC_CombatMedic:
    case EntityType::NC_Engineer:
    case EntityType::NC_HeavyAssault:
    case EntityType::NC_MAX:
    case EntityType::TR_Infiltrator:
    case EntityType::TR_LightAssault:
    case EntityType::TR_CombatMedic:
    case EntityType::TR_Engineer:
    case EntityType::TR_HeavyAssault:
    case EntityType::TR_MAX:
    case EntityType::VS_Infiltrator:
    case EntityType::VS_LightAssault:
    case EntityType::VS_CombatMedic:
    case EntityType::VS_Engineer:
    case EntityType::VS_HeavyAssault:
    case EntityType::VS_MAX:
    case EntityType::NS_Infiltrator:
    case EntityType::NS_LightAssault:
    case EntityType::NS_Medic:
    case EntityType::NS_Engineer:
    case EntityType::NS_HeavyAssault:
    case EntityType::NS_MAX:
    case EntityType::Soldier:
        return true;

    default:
        return false;
    }
}

/**
 * @brief Check if entity type is a turret
 * @param type The entity type to check
 * @return true if the entity is a turret type, false otherwise
 */
inline bool IsTurretType(EntityType type)
{
    switch (type)
    {
        // All stationary objects and turrets
    case EntityType::MANA_AV_Turret:
    case EntityType::Spear_AV_Turret:
    case EntityType::Aspis_AA_Turret:
    case EntityType::Spitfire_Auto_Turret:
    case EntityType::Spear_AV_Tower:
    case EntityType::Aspis_AA_Phalanx_Tower:
    case EntityType::Xiphos_AI_Tower:
    case EntityType::The_Flail:
    case EntityType::Spear_AV_Turret_2:
    case EntityType::Glaive_IPC:
        return true;

    default:
        return false;
    }
}

/**
 * @brief Check if entity type is a utility/other object
 * @param type The entity type to check
 * @return true if the entity is a utility type, false otherwise
 */
inline bool IsOthersType(EntityType type)
{
    switch (type)
    {
        // Mines
    case EntityType::AV_Mines:
    case EntityType::AI_Mines:
        // Utility
    case EntityType::Motion_Spotter:
    case EntityType::Ammo_Pack:
    case EntityType::Spawn_Beacon:
    case EntityType::Spawn_Pad:
    case EntityType::Repair_Station:
        // Legacy/Other
    case EntityType::Orbital_Mining_Drill:
    case EntityType::Gate_Shield:
        return true;

    default:
        return false;
    }
}

/**
 * @brief Check if entity type is a ground vehicle
 * @param type The entity type to check
 * @return true if the entity is a ground vehicle type, false otherwise
 */
inline bool IsGroundVehicleType(EntityType type)
{
    switch (type)
    {
        // Ground Vehicles
    case EntityType::Flash:
    case EntityType::Lightning:
    case EntityType::Harasser:
    case EntityType::Magrider:
    case EntityType::Prowler:
    case EntityType::Vanguard:
    case EntityType::Sunderer:
    case EntityType::Chimera:
    case EntityType::ANT:
    case EntityType::Colossus:
    case EntityType::Javelin:
        return true;

    default:
        return false;
    }
}

/**
 * @brief Check if entity type is an air vehicle
 * @param type The entity type to check
 * @return true if the entity is an air vehicle type, false otherwise
 */
inline bool IsAirVehicleType(EntityType type)
{
    switch (type)
    {
        // Air Vehicles
    case EntityType::Valkyrie:
    case EntityType::Dervish:
    case EntityType::Scythe:
    case EntityType::Mosquito:
    case EntityType::Reaver:
    case EntityType::Liberator:
    case EntityType::Galaxy:
    case EntityType::DropPod:
        return true;

    default:
        return false;
    }
}

/**
 * @brief Check if entity type is a MAX unit
 * @param type The entity type to check
 * @return true if the entity is a MAX unit type, false otherwise
 */
inline bool IsMAXUnit(EntityType type) {
    switch (type) {
    case EntityType::NC_MAX:
    case EntityType::TR_MAX:
    case EntityType::VS_MAX:
    case EntityType::NS_MAX:
        return true;
    default:
        return false;
    }
}

/**
 * @brief Check if entity type is a known/recognized type
 * @param type The entity type to check
 * @return true if the entity type is recognized, false otherwise
 */
inline bool IsKnownEntityType(EntityType type) {
    switch (type) {
        // Player Classes
    case EntityType::Citizen:
    case EntityType::NC_Infiltrator:
    case EntityType::NC_LightAssault:
    case EntityType::NC_CombatMedic:
    case EntityType::NC_Engineer:
    case EntityType::NC_HeavyAssault:
    case EntityType::NC_MAX:
    case EntityType::TR_Infiltrator:
    case EntityType::TR_LightAssault:
    case EntityType::TR_CombatMedic:
    case EntityType::TR_Engineer:
    case EntityType::TR_HeavyAssault:
    case EntityType::TR_MAX:
    case EntityType::VS_Infiltrator:
    case EntityType::VS_LightAssault:
    case EntityType::VS_CombatMedic:
    case EntityType::VS_Engineer:
    case EntityType::VS_HeavyAssault:
    case EntityType::VS_MAX:
    case EntityType::NS_Infiltrator:
    case EntityType::NS_LightAssault:
    case EntityType::NS_Medic:
    case EntityType::NS_Engineer:
    case EntityType::NS_HeavyAssault:
    case EntityType::NS_MAX:
    case EntityType::Soldier:

        // Vehicles
    case EntityType::Flash:
    case EntityType::Lightning:
    case EntityType::Colossus:
    case EntityType::Javelin:
    case EntityType::Harasser:
    case EntityType::Valkyrie:
    case EntityType::Magrider:
    case EntityType::Prowler:
    case EntityType::Vanguard:
    case EntityType::Dervish:
    case EntityType::Sunderer:
    case EntityType::Scythe:
    case EntityType::Mosquito:
    case EntityType::Reaver:
    case EntityType::Liberator:
    case EntityType::Galaxy:
    case EntityType::Chimera:
    case EntityType::DropPod:
    case EntityType::ANT:

        // Mines
    case EntityType::AV_Mines:
    case EntityType::AI_Mines:

        // Utility
    case EntityType::Motion_Spotter:
    case EntityType::Ammo_Pack:
    case EntityType::Spawn_Beacon:
    case EntityType::Spawn_Pad:
    case EntityType::Repair_Station:

        // Turrets
    case EntityType::MANA_AV_Turret:
    case EntityType::Xiphos_AI_Tower:
    case EntityType::The_Flail:
    case EntityType::Spitfire_Auto_Turret:
    case EntityType::Aspis_AA_Turret:
    case EntityType::Spear_AV_Turret:
    case EntityType::Spear_AV_Tower:
    case EntityType::Aspis_AA_Phalanx_Tower:
    case EntityType::Spear_AV_Turret_2:
    case EntityType::Glaive_IPC:

        // Legacy/Other
    case EntityType::Orbital_Mining_Drill:
        return true;

    default:
        return false;
    }
}

/**
 * @brief Get human-readable string representation of entity type
 * @param type The entity type to convert
 * @return String representation of the entity type
 */
inline std::string GetEntityTypeString(EntityType type) {
    switch (type) {
        // Player Classes
    case EntityType::Citizen: return "Citizen";
    case EntityType::NC_Infiltrator: return "NC Infiltrator";
    case EntityType::NC_LightAssault: return "NC Light Assault";
    case EntityType::NC_CombatMedic: return "NC Medic";
    case EntityType::NC_Engineer: return "NC Engineer";
    case EntityType::NC_HeavyAssault: return "NC Heavy";
    case EntityType::NC_MAX: return "NC MAX";
    case EntityType::TR_Infiltrator: return "TR Infiltrator";
    case EntityType::TR_LightAssault: return "TR Light Assault";
    case EntityType::TR_CombatMedic: return "TR Medic";
    case EntityType::TR_Engineer: return "TR Engineer";
    case EntityType::TR_HeavyAssault: return "TR Heavy";
    case EntityType::TR_MAX: return "TR MAX";
    case EntityType::VS_Infiltrator: return "VS Infiltrator";
    case EntityType::VS_LightAssault: return "VS Light Assault";
    case EntityType::VS_CombatMedic: return "VS Medic";
    case EntityType::VS_Engineer: return "VS Engineer";
    case EntityType::VS_HeavyAssault: return "VS Heavy";
    case EntityType::VS_MAX: return "VS MAX";
    case EntityType::NS_Infiltrator: return "NS Infiltrator";
    case EntityType::NS_LightAssault: return "NS Light Assault";
    case EntityType::NS_Medic: return "NS Medic";
    case EntityType::NS_Engineer: return "NS Engineer";
    case EntityType::NS_HeavyAssault: return "NS Heavy";
    case EntityType::NS_MAX: return "NS MAX";
    case EntityType::Soldier: return "Soldier";

        // Ground Vehicles
    case EntityType::Flash: return "Flash";
    case EntityType::Lightning: return "Lightning";
    case EntityType::Harasser: return "Harasser";
    case EntityType::Magrider: return "Magrider";
    case EntityType::Prowler: return "Prowler";
    case EntityType::Vanguard: return "Vanguard";
    case EntityType::Sunderer: return "Sunderer";
    case EntityType::Chimera: return "Chimera";
    case EntityType::ANT: return "ANT";
    case EntityType::Colossus: return "Colossus";
    case EntityType::Javelin: return "Javelin";

        // Air Vehicles
    case EntityType::Valkyrie: return "Valkyrie";
    case EntityType::Dervish: return "Dervish";
    case EntityType::Scythe: return "Scythe";
    case EntityType::Mosquito: return "Mosquito";
    case EntityType::Reaver: return "Reaver";
    case EntityType::Liberator: return "Liberator";
    case EntityType::Galaxy: return "Galaxy";
    case EntityType::DropPod: return "Drop Pod";

        // Turrets
    case EntityType::MANA_AV_Turret: return "MANA AV Turret";
    case EntityType::Xiphos_AI_Tower: return "Xiphos AI Tower";
    case EntityType::The_Flail: return "The Flail";
    case EntityType::Spitfire_Auto_Turret: return "Spitfire Auto Turret";
    case EntityType::Aspis_AA_Turret: return "Aspis AA Turret";
    case EntityType::Spear_AV_Turret: return "Spear AV Turret";
    case EntityType::Spear_AV_Tower: return "Spear AV Tower";
    case EntityType::Aspis_AA_Phalanx_Tower: return "Aspis AA Phalanx Tower";
    case EntityType::Spear_AV_Turret_2: return "Spear AV Turret";
    case EntityType::Glaive_IPC: return "Glaive IPC";

        // Mines
    case EntityType::AV_Mines: return "AV Mines";
    case EntityType::AI_Mines: return "AI Mines";

        // Utility
    case EntityType::Motion_Spotter: return "Motion Spotter";
    case EntityType::Ammo_Pack: return "Ammo Pack";
    case EntityType::Spawn_Beacon: return "Spawn Beacon";
    case EntityType::Spawn_Pad: return "Spawn Pad";
    case EntityType::Repair_Station: return "Repair Station";

        // Legacy/Other
    case EntityType::Orbital_Mining_Drill: return "Orbital Mining Drill";
    case EntityType::Gate_Shield: return "Gate Shield";
	
        // Default
    default: return "Unknown";
    }
}



/**
 * @brief View matrix structure for 3D transformations
 * @details This structure represents a 4x4 transformation matrix used for
 *          world-to-screen coordinate conversions. It's an exact copy from
 *          the game's internal definitions.
 */
struct ViewMatrix_t
{
    float M11, M12, M13, M14;
    float M21, M22, M23, M24;
    float M31, M32, M33, M34;
    float M41, M42, M43, M44;
    
    /**
     * @brief Default constructor - initializes as identity matrix
     */
    ViewMatrix_t()
    {
        // Initialize as identity matrix
        M11 = M22 = M33 = M44 = 1.0f;
        M12 = M13 = M14 = M21 = M23 = M24 = M31 = M32 = M34 = M41 = M42 = M43 = 0.0f;
    }
    
    /**
     * @brief Create an identity matrix
     * @return A new identity matrix
     */
    static ViewMatrix_t Identity()
    {
        return ViewMatrix_t();
    }
    
    /**
     * @brief Build legacy game matrix from camera data
     * @param cameraData Raw camera data from game memory
     * @return Constructed view matrix
     */
    static ViewMatrix_t BuildLegacyGameMatrix(const float* cameraData)
    {
        ViewMatrix_t matrix;
        
        // Copy the 4x4 matrix from camera data
        matrix.M11 = cameraData[0];  matrix.M12 = cameraData[1];  matrix.M13 = cameraData[2];  matrix.M14 = cameraData[3];
        matrix.M21 = cameraData[4];  matrix.M22 = cameraData[5];  matrix.M23 = cameraData[6];  matrix.M24 = cameraData[7];
        matrix.M31 = cameraData[8];  matrix.M32 = cameraData[9];  matrix.M33 = cameraData[10]; matrix.M34 = cameraData[11];
        matrix.M41 = cameraData[12]; matrix.M42 = cameraData[13]; matrix.M43 = cameraData[14]; matrix.M44 = cameraData[15];
        
        return matrix;
    }
    
    /**
     * @brief Build legacy game matrix from current matrix
     * @return Transformed matrix suitable for game calculations
     */
    ViewMatrix_t BuildLegacyGameMatrix() const
    {
        ViewMatrix_t result;
        
        // Transpose and negate second column (exact C# implementation)
        result.M11 = this->M11;  result.M12 = this->M21;  result.M13 = this->M31;  result.M14 = this->M41;
        result.M21 = -this->M12; result.M22 = -this->M22; result.M23 = -this->M32; result.M24 = -this->M42;
        result.M31 = this->M13;  result.M32 = this->M23;  result.M33 = this->M33;  result.M34 = this->M43;
        result.M41 = this->M14;  result.M42 = this->M24;  result.M43 = this->M34;  result.M44 = this->M44;
        
        return result;
    }
    
    /**
     * @brief Matrix multiplication operator
     * @param other The matrix to multiply with
     * @return Result of matrix multiplication
     */
    ViewMatrix_t operator*(const ViewMatrix_t& other) const
    {
        ViewMatrix_t result;
        result.M11 = M11 * other.M11 + M12 * other.M21 + M13 * other.M31 + M14 * other.M41;
        result.M12 = M11 * other.M12 + M12 * other.M22 + M13 * other.M32 + M14 * other.M42;
        result.M13 = M11 * other.M13 + M12 * other.M23 + M13 * other.M33 + M14 * other.M43;
        result.M14 = M11 * other.M14 + M12 * other.M24 + M13 * other.M34 + M14 * other.M44;
        
        result.M21 = M21 * other.M11 + M22 * other.M21 + M23 * other.M31 + M24 * other.M41;
        result.M22 = M21 * other.M12 + M22 * other.M22 + M23 * other.M32 + M24 * other.M42;
        result.M23 = M21 * other.M13 + M22 * other.M23 + M23 * other.M33 + M24 * other.M43;
        result.M24 = M21 * other.M14 + M22 * other.M24 + M23 * other.M34 + M24 * other.M44;
        
        result.M31 = M31 * other.M11 + M32 * other.M21 + M33 * other.M31 + M34 * other.M41;
        result.M32 = M31 * other.M12 + M32 * other.M22 + M33 * other.M32 + M34 * other.M42;
        result.M33 = M31 * other.M13 + M32 * other.M23 + M33 * other.M33 + M34 * other.M43;
        result.M34 = M31 * other.M14 + M32 * other.M24 + M33 * other.M34 + M34 * other.M44;
        
        result.M41 = M41 * other.M11 + M42 * other.M21 + M43 * other.M31 + M44 * other.M41;
        result.M42 = M41 * other.M12 + M42 * other.M22 + M43 * other.M32 + M44 * other.M42;
        result.M43 = M41 * other.M13 + M42 * other.M23 + M43 * other.M33 + M44 * other.M43;
        result.M44 = M41 * other.M14 + M42 * other.M24 + M43 * other.M34 + M44 * other.M44;
        
        return result;
    }
};
