/**
 * @file Offsets.h
 * @brief Memory offsets for PlanetSide 2
 * @details This file contains all memory offsets used to access game data.
 *          These offsets are specific to the current game version and may
 *          need to be updated when the game is patched.
 */

#pragma once
#include <cstdint>

/**
 * @brief Memory offsets namespace
 * @details Contains all memory offsets for accessing game data structures
 */
namespace Offsets {
    // Base Addresses (absolute - relative to BaseAddress)
    constexpr uintptr_t pCGameAddress = 0x40B7AE0;    ///< Main game manager address
    constexpr uintptr_t pCGraphics = 0x40B7C50;       ///< Graphics manager address
    constexpr uintptr_t o_FOV = 0x144260CFC;          ///< Field of view offset
    
    // CGame Offsets
    constexpr uintptr_t o_cGame_pFirstObject = 0x1060; ///< First entity object pointer
    constexpr uintptr_t o_pNextObject = 0x5A8;         ///< Next entity object pointer
    
    /**
     * @brief Entity-specific offsets
     * @details Offsets for accessing entity data structures
     */
    namespace Entity {
        constexpr uintptr_t Position = 0x920;        ///< Entity position offset
        constexpr uintptr_t TeamID = 0x1E8;         ///< Team ID offset
        constexpr uintptr_t Type = 0x5E0;           ///< Entity type offset
        constexpr uintptr_t PlayerStance = 0x2A00;  ///< Player stance offset
        constexpr uintptr_t ViewAngle = 0x2C20;     ///< View angle offset
        constexpr uintptr_t Name = 0x2AF8;          ///< Entity name offset
        constexpr uintptr_t NameBackup = 0x2B48;    ///< Backup name offset
        constexpr uintptr_t PlayerBase = 0x3B0;     ///< Player base offset
        
        // Health/Shield (Pointer indirection!)
        constexpr uintptr_t PointerToHealthAndShield = 0x120;
        constexpr uintptr_t CurHealth = 0x160;
        constexpr uintptr_t MaxHealth = 0x164;
        constexpr uintptr_t CurShield = 0x470;
        constexpr uintptr_t MaxShield = 0x474;
        
        // IsDead Check
        constexpr uintptr_t IsDead = 0x9C1;
        
        // GM/Admin Flags (based on IDA analysis)
        constexpr uintptr_t GMFlags = 0x9C4; // 2500 decimal
        
        // Shooting Status (168 = shooting, 160 = not shooting)
        constexpr uintptr_t ShootingStatus = 0x3088;
        
        // NEW: Complete pointer chain for bones
        constexpr uintptr_t pActor = 0x828;
        constexpr uintptr_t pSkeleton = 0x1D8;
        constexpr uintptr_t pSkeletonInfo = 0x50;
        constexpr uintptr_t pBoneInfo = 0x68;
        constexpr uintptr_t BoneArrayOffset = 0x194; // Relative to boneInfo pointer
    }
    
    // Camera/ViewMatrix Offsets
    namespace Camera {
        constexpr uintptr_t ViewMatrixOffset1 = 0xF0; // cGraphicsBase -> cameraPtr
        constexpr uintptr_t ViewMatrixOffset2 = 0xB0; // cameraPtr -> matrixAddress
    }
    
    // Entity count offset (for performance optimization)
    constexpr uintptr_t o_EntityCount = 0x2710;
}

// Magic Bullet Offsets (directly in Offsets namespace)
namespace Offsets {
    namespace MagicBullet {
        static constexpr uintptr_t pCurrentBulletPointerBase = 0x40B82C0;
        static constexpr int pCurrentBulletPointerOffsets[] = { 0x871E8 };
        static constexpr int bullet_direction_offset = 0xB40;
        static constexpr int bullet_position_offset = 0x870;
        static constexpr int bullet_start_position_offset = 0x930;
        static constexpr int bullet_is_alive_offset = 0x4;
        static constexpr int bullet_speed_offset = 0x8C0;
        static constexpr int o_BulletHash = 0x4F4;  // CRC32 Hash offset
    }
    
    // Noclip/Flight Offsets - Direct world coordinates as Double
    namespace Noclip {
        // Pointer chain to real world coordinates (Double)
        // Base: "PlanetSide2_x64.exe"+040B84D0
        // Chain: [base+58] -> [result+2A0] -> [result+C00] -> X,Y,Z as Double
        static constexpr uintptr_t pBaseAddress = 0x04260B60;
        static constexpr uintptr_t o_Offset1 = 0x248;
        static constexpr uintptr_t o_Offset2 = 0xE0;
        static constexpr uintptr_t o_Offset3 = 0x180;
        static constexpr uintptr_t o_Offset4 = 0x268;
        static constexpr uintptr_t o_Offset5 = 0x208;
        
        // Relative Offsets for X, Y, Z (8 bytes each for Double)
        static constexpr uintptr_t o_PositionX = 0x0;   // +0
        static constexpr uintptr_t o_PositionY = 0x8;   // +8
        static constexpr uintptr_t o_PositionZ = 0x10;  // +16
        
        // Velocity Offsets - Pointer chain for Velocity
        // Base: "PlanetSide2_x64.exe"+04024928
        // Chain: [base+5B0] -> [result+3B8] -> X,Y,Z Velocity as Double
        static constexpr uintptr_t pVelocityBaseAddress = 0x04260B60;
        static constexpr uintptr_t o_VelocityOffset1 = 0x248;
        static constexpr uintptr_t o_VelocityOffset2 = 0xE0;
        static constexpr uintptr_t o_VelocityOffset3 = 0x180;
        static constexpr uintptr_t o_VelocityOffset4 = 0x300;
        
        // Relative Offsets for Velocity X, Y, Z (8 bytes each for Double)
        static constexpr uintptr_t o_VelocityX = 0x0;   // +0
        static constexpr uintptr_t o_VelocityY = 0x4;   // +8
        static constexpr uintptr_t o_VelocityZ = 0x8;  // +16
    }
    
    // Game Constants and Values
    namespace GameConstants {
        // Bone System Constants
        constexpr int HEAD_BONE_ID = 73;                    ///< Head bone ID for skeleton system
        constexpr int BONE_SIZE = 0xC;                      ///< Size of each bone entry (12 bytes)
        
        // Position and Height Offsets
        constexpr float EYE_HEIGHT_OFFSET = 1.7f;          ///< Eye position height offset
        constexpr float FEET_OFFSET = 0.20f;                ///< Feet position offset
        constexpr float MAX_HEAD_HEIGHT = 2.2f;             ///< MAX unit head height
        constexpr float NORMAL_HEAD_HEIGHT = 0.35f;         ///< Normal unit head height
        constexpr float DEFAULT_HEAD_HEIGHT = 1.8f;        ///< Default head position height
        constexpr float AIMBOT_MAX_HEAD_HEIGHT = 2.0f;      ///< Aimbot MAX unit head height
        
        // Rotation Constants
        constexpr float ROTATION_OFFSET = 1.6f;             ///< Rotation offset for view angles
        constexpr float PI_OVER_2 = 1.57079632679f;        ///< PI/2 constant
        constexpr float PI = 3.14159265359f;                ///< PI constant
        
        // ESP Box Constants
        constexpr float PLAYER_BOX_WIDTH_FACTOR = 0.65f;    ///< Player ESP box width factor
        constexpr float VEHICLE_BOX_WIDTH_FACTOR = 3.5f;     ///< Vehicle ESP box width factor
        constexpr float AIR_VEHICLE_SIZE_FACTOR = 2.0f;     ///< Air vehicle hexagon size factor
        constexpr float TURRET_SIZE_FACTOR = 0.8f;           ///< Turret X size factor
        constexpr float OTHERS_SIZE_FACTOR = 0.3f;           ///< Other entities (ammo packs, etc.) size factor
        constexpr float OTHERS_HEAD_HEIGHT = 0.15f;          ///< Other entities head height
        constexpr float VEHICLE_HEAD_HEIGHT = 3.5f;
        
        // Health Bar Constants
        constexpr float HEALTH_BAR_WIDTH_FACTOR = 0.06f;    ///< Health bar width factor
        constexpr float HEALTH_BAR_GAP_FACTOR = 0.04f;      ///< Health bar gap factor
        constexpr float HEALTH_BAR_SPACING_FACTOR = 0.02f;  ///< Health bar spacing factor
        constexpr float HEALTH_BAR_HEIGHT_FACTOR = 0.5f;    ///< Health bar height factor (50/50 split)
        
        // ESP Label Position Offsets
        constexpr float LABEL_PLAYER_OFFSET = -0.35f;       ///< Player label Y offset
        constexpr float LABEL_GROUND_VEHICLE_OFFSET = -0.75f; ///< Ground vehicle label Y offset
        constexpr float LABEL_AIR_VEHICLE_OFFSET = -3.5f;   ///< Air vehicle label Y offset
        constexpr float LABEL_TURRET_OFFSET = -0.5f;        ///< Turret label Y offset
        constexpr float LABEL_OTHER_OFFSET = -0.25f;         ///< Other entity label Y offset
        constexpr float LABEL_DEFAULT_OFFSET = -0.35f;      ///< Default label Y offset
        
        // ESP Drawing Constants
        constexpr float SKELETON_CIRCLE_RADIUS_FACTOR = 0.08f; ///< Skeleton circle radius factor
        constexpr float TRACER_LENGTH = 2.5f;               ///< Bullet tracer length
        
        // Magic Bullet Constants
        constexpr float MAGIC_BULLET_DISTANCE_THRESHOLD = 32.0f; ///< Magic bullet distance threshold
        constexpr float MAGIC_BULLET_POSITION_OFFSET = 0.5f;     ///< Magic bullet position Y offset
        constexpr float MAGIC_BULLET_SPEED = 10000.0f;           ///< Magic bullet speed
        
        // Noclip Constants
        constexpr float NOCLIP_VELOCITY_X = 0.0f;           ///< Noclip velocity X
        constexpr float NOCLIP_VELOCITY_Y = 0.35f;         ///< Noclip velocity Y
        constexpr float NOCLIP_VELOCITY_Z = 0.0f;           ///< Noclip velocity Z
        
        // Memory Constants
        constexpr uintptr_t PATTERN_SCAN_SIZE = 0x10000000;  ///< Pattern scan size (256MB)
        constexpr uint32_t CRC32_SEED = 0xA5D208F7;         ///< CRC32 seed value
        
        // Hexagon Drawing Constants
        constexpr float HEXAGON_ANGLE_STEP = 60.0f;         ///< Hexagon angle step in degrees
        constexpr float DEGREES_TO_RADIANS = 0.01745329252f; ///< Degrees to radians conversion
    }
}
