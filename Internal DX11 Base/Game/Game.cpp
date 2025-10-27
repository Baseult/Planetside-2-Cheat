#include "pch.h"
#include "Game.h"
#include "Offsets.h"
#include "../Utils/Logger.h"
#include <chrono>
#include "../Renderer/Renderer.h"

Game::Game() {
    // Initialisiere shared_ptr Snapshots
    m_snapshot_front = std::make_shared<const WorldSnapshot>();
    m_snapshot_back = std::make_unique<WorldSnapshot>();
    
    if (!Initialize()) {
        Logger::Error("!!! CRITICAL: Failed to initialize Game class! Expect a crash. !!!");
    }
}

Game::~Game() {
    try {
        Shutdown();
    } catch (...) {
        // Ignore errors in destructor - prevents crashes during shutdown
    }
}

bool Game::Initialize() {
    Logger::Log("--> Game::Initialize started.");
    
    m_memory = std::make_unique<Utils::SafeMemory>();
    Logger::Log("Game::Initialize: SafeMemory object created.");
    
    if (!m_memory->AttachToProcess("PlanetSide2_x64.exe")) {
        Logger::Error("Game::Initialize: Failed to attach to process.");
        return false;
    }
    
    // ✅ PERFORMANCE: Cache baseAddress once during initialization
    m_cachedBaseAddress = m_memory->GetBaseAddress();
    m_baseAddressInitialized = true;
    
    Logger::Log("Game::Initialize: Successfully attached to process.");
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Base address: 0x%llX", (unsigned long long)m_cachedBaseAddress);
    Logger::Log(buffer);
    
    Logger::Log("--> Game::Initialize finished.");
    return true;
}

void Game::Shutdown() {
    Logger::Log("Shutting down Game class...");
    
    try {
        if (m_memory) {
            m_memory->DetachFromProcess();
            m_memory.reset();
        }
        
        // Cleanup shared_ptr
        m_snapshot_front.reset();
        m_snapshot_back.reset();
        
        Logger::Log("Game class shutdown complete!");
    } catch (...) {
        // Ignore errors during Game shutdown
        Logger::Log("Game class shutdown with errors (ignored)");
    }
}

void Game::Update() {
    static bool firstUpdate = true;
    if (firstUpdate) {
        Logger::Log("----> Game::Update called for the first time."); // Only for first call!
        firstUpdate = false;
    }
    
    try {
        auto start = std::chrono::high_resolution_clock::now();
        
        // 1. Read all data and pre-calculate
        ReadAllGameData();
        
        // 2. Atomic swap
        SwapBuffers();
        
        auto end = std::chrono::high_resolution_clock::now();
        m_updateTime = std::chrono::duration<float, std::milli>(end - start).count();
    } catch (...) {
        // Ignore errors in game update - prevents crashes during shutdown
        m_updateTime = 0.0f;
    }
}

// CRITICAL: Returns shared_ptr (no copy!)
std::shared_ptr<const WorldSnapshot> Game::GetWorldSnapshot() const {
    std::lock_guard<std::mutex> lock(m_snapshotMutex);
    return m_snapshot_front;  // shared_ptr kopieren ist billig!
}

int Game::GetEntityCount() const {
    std::lock_guard<std::mutex> lock(m_snapshotMutex);
    return static_cast<int>(m_snapshot_front->entities.size());
}

void Game::ReadAllGameData() {
    m_snapshot_back->timestamp = GetTickCount64();
    ReadViewMatrix();
    ReadLocalPlayer();
    ReadEntityList();
}

void Game::SwapBuffers() {
    std::lock_guard<std::mutex> lock(m_snapshotMutex);
    
    // Move back to shared_ptr (no copy!)
    m_snapshot_front = std::shared_ptr<const WorldSnapshot>(m_snapshot_back.release());
    
    // Neuen Back-Buffer erstellen
    m_snapshot_back = std::make_unique<WorldSnapshot>();
}

// Old getter functions removed - replaced by GetWorldSnapshot()

bool Game::WorldToScreen(const Utils::Vector3& world, Utils::Vector2& screen) const {
    // Use CACHED front ViewMatrix (no lock needed!)
    std::lock_guard<std::mutex> lock(m_snapshotMutex);
    auto gameMatrix = m_snapshot_front->viewMatrix.BuildLegacyGameMatrix();
    
    float sx = gameMatrix.M11 * world.x + gameMatrix.M12 * world.y + gameMatrix.M13 * world.z + gameMatrix.M14;
    float sy = gameMatrix.M21 * world.x + gameMatrix.M22 * world.y + gameMatrix.M23 * world.z + gameMatrix.M24;
    float W = gameMatrix.M41 * world.x + gameMatrix.M42 * world.y + gameMatrix.M43 * world.z + gameMatrix.M44;
    
    if (W < 0.01f)
        return false;
    
    float invW = 1.0f / W;
    sx *= invW;
    sy *= invW;
    
    // IMPROVEMENT: Get screen size from Renderer, not from ImGui
    Utils::Vector2 screenSize = g_Renderer->GetScreenSize();
    if (screenSize.x == 0 || screenSize.y == 0) return false; // Protection if renderer not yet initialized
    
    screen.x = screenSize.x / 2.0f + 0.5f * sx * screenSize.x + 0.5f;
    screen.y = screenSize.y / 2.0f - 0.5f * sy * screenSize.y + 0.5f;
    
    return true; // We do the on-screen check in ESP, that is cleaner
}

bool Game::IsOnScreen(const Utils::Vector2& screen) const {
    // Dynamic on-screen check based on current screen resolution
    Utils::Vector2 screenSize = g_Renderer->GetScreenSize();
    return screen.x >= 0 && screen.x <= screenSize.x && screen.y >= 0 && screen.y <= screenSize.y;
}

void Game::ReadEntityList() {
    m_snapshot_back->entities.clear();
    
    // Use cached cGameBase
    uintptr_t cGameBase;
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        if (m_addressCache.IsValid() && m_addressCache.cGameBase != 0) {
            cGameBase = m_addressCache.cGameBase;
        } else {
            if (!m_memory->Read(GetAbsoluteAddress(Offsets::pCGameAddress), cGameBase) || cGameBase == 0) {
                return;
            }
            m_addressCache.cGameBase = cGameBase;
            m_addressCache.lastCacheUpdate = std::chrono::steady_clock::now();
        }
    }
    if (cGameBase == 0) return;
    
    uintptr_t currentEntityPtr;
    if (!m_memory->Read(cGameBase + Offsets::o_cGame_pFirstObject, currentEntityPtr))
        return;
    
    int actualEntityCount = 0;
    m_memory->Read(cGameBase + Offsets::o_EntityCount, actualEntityCount);
    int maxEntities = actualEntityCount;
    
    m_snapshot_back->entities.reserve(maxEntities);
    
    int entityIdCounter = 1;
    uintptr_t localPlayerPtr = currentEntityPtr;
    
    while (currentEntityPtr != 0 && entityIdCounter < maxEntities) {
        if (IsEntityValid(currentEntityPtr) && currentEntityPtr != localPlayerPtr) {
            
            EntitySnapshot snapshot;
            snapshot.id = currentEntityPtr;
            
            // ===== SCHRITT 1: ROHDATEN LESEN =====
            snapshot.position = m_memory->ReadVector3(currentEntityPtr + Offsets::Entity::Position);
            
            uint8_t typeValue;
            m_memory->Read(currentEntityPtr + Offsets::Entity::Type, typeValue);
            snapshot.type = static_cast<EntityType>(typeValue);
            
            int teamValue;
            m_memory->Read(currentEntityPtr + Offsets::Entity::TeamID, teamValue);
            snapshot.team = static_cast<EFaction>(teamValue);
            
            if (IsPlayerType(snapshot.type)) {
                snapshot.viewAngle = m_memory->ReadVector3(currentEntityPtr + Offsets::Entity::ViewAngle);
            }
            
            snapshot.headPosition = snapshot.position;
            snapshot.headPosition.y += Offsets::GameConstants::DEFAULT_HEAD_HEIGHT;
            
            snapshot.name = ReadEntityName(currentEntityPtr);
            ReadHealthShield(currentEntityPtr, snapshot);
            
            uint8_t isDeadValue;
            if (m_memory->Read(currentEntityPtr + Offsets::Entity::IsDead, isDeadValue)) {
                snapshot.isAlive = (isDeadValue == 128);
            } else {
                snapshot.isAlive = true;
            }
            
            if (!m_memory->Read(currentEntityPtr + Offsets::Entity::GMFlags, snapshot.gmFlags)) {
                snapshot.gmFlags = 0;
            }
            
            ReadHeadPosition(currentEntityPtr, snapshot);
            
            // ===== SCHRITT 2: VORAUSBERECHNEN =====
            snapshot.distanceToLocalPlayer = snapshot.position.Distance(
                m_snapshot_back->localPlayer.position
            );
            
            Utils::Vector3 feetPos = snapshot.position;
            feetPos.y -= Offsets::GameConstants::FEET_OFFSET;
            
            Utils::Vector3 headPos;
            if (IsMAXUnit(snapshot.type)) {
                headPos = snapshot.position;
                headPos.y += Offsets::GameConstants::MAX_HEAD_HEIGHT;
            } else if (IsOthersType(snapshot.type)) {
                headPos = snapshot.position;
                headPos.y += Offsets::GameConstants::OTHERS_HEAD_HEIGHT;
            }
            else if (IsGroundVehicleType(snapshot.type)) {
                headPos = snapshot.position;
                headPos.y += Offsets::GameConstants::VEHICLE_HEAD_HEIGHT;
            }
            else {
                headPos = snapshot.headPosition;
                headPos.y += Offsets::GameConstants::NORMAL_HEAD_HEIGHT;
            }
            
            bool feetOnScreen = WorldToScreen(feetPos, snapshot.feetScreenPos);
            bool headOnScreen = WorldToScreen(headPos, snapshot.headScreenPos);
            snapshot.isOnScreen = feetOnScreen || headOnScreen;
            
            // FOV distance
            if (snapshot.isOnScreen && g_Renderer) {
                Utils::Vector2 screenCenter = g_Renderer->GetScreenCenter();
                snapshot.fovDistanceToCenter = screenCenter.Distance(snapshot.headScreenPos);
            } else {
                snapshot.fovDistanceToCenter = FLT_MAX;
            }
            
            m_snapshot_back->entities.push_back(std::move(snapshot));
        }
        
        if (!m_memory->Read(currentEntityPtr + Offsets::o_pNextObject, currentEntityPtr))
            break;
        
        entityIdCounter++;
    }
}

void Game::ReadLocalPlayer() {
    // ✅ PERFORMANCE: Use cached baseAddress instead of calling GetBaseAddress() every frame
    if (!m_baseAddressInitialized) {
        m_cachedBaseAddress = m_memory->GetBaseAddress();
        m_baseAddressInitialized = true;
    }
    
    uintptr_t cGameBase;
    if (!m_memory->Read(m_cachedBaseAddress + Offsets::pCGameAddress, cGameBase) || cGameBase == 0) {
        m_snapshot_back->localPlayer.isValid = false;
        return;
    }
    
    uintptr_t localPlayerPtr;
    if (!m_memory->Read(cGameBase + Offsets::o_cGame_pFirstObject, localPlayerPtr) || localPlayerPtr == 0) {
        m_snapshot_back->localPlayer.isValid = false;
        return;
    }
    
    m_snapshot_back->localPlayer.position = m_memory->ReadVector3(localPlayerPtr + Offsets::Entity::Position);
    m_snapshot_back->localPlayer.eyePosition = m_snapshot_back->localPlayer.position;
    m_snapshot_back->localPlayer.eyePosition.z += Offsets::GameConstants::EYE_HEIGHT_OFFSET;
    m_snapshot_back->localPlayer.viewAngles = m_memory->ReadVector3(localPlayerPtr + Offsets::Entity::ViewAngle);
    
    int teamValue;
    if (m_memory->Read(localPlayerPtr + Offsets::Entity::TeamID, teamValue)) {
        m_snapshot_back->localPlayer.team = static_cast<EFaction>(teamValue);
    }
    
    // Shooting Status
    uint8_t shootingStatus;
    if (m_memory->Read(localPlayerPtr + Offsets::Entity::ShootingStatus, shootingStatus)) {
        m_snapshot_back->localPlayer.isShooting = (shootingStatus == 168);
    } else {
        m_snapshot_back->localPlayer.isShooting = false;
    }
    
    m_snapshot_back->localPlayer.isAlive = true;
    m_snapshot_back->localPlayer.isValid = true;
}

void Game::ReadViewMatrix() {
    // OPTIMIZATION: Use cache for ViewMatrix
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        if (m_addressCache.IsValid() && m_addressCache.matrixAddress != 0) {
            // Read directly from cached matrixAddress
            float matrixData[16];
            if (m_memory->Read(m_addressCache.matrixAddress, matrixData)) {
                m_snapshot_back->viewMatrix = ViewMatrix_t::BuildLegacyGameMatrix(matrixData);
                return;
            }
        }
    }
    
    // Cache-Miss: Recalculate pointer chain
    uintptr_t cGraphicsBase;
    if (!m_memory->Read(GetAbsoluteAddress(Offsets::pCGraphics), cGraphicsBase) || cGraphicsBase == 0)
        return;
    
    uintptr_t cameraPtr;
    if (!m_memory->Read(cGraphicsBase + Offsets::Camera::ViewMatrixOffset1, cameraPtr) || cameraPtr == 0)
        return;
    
    uintptr_t matrixAddress = cameraPtr + Offsets::Camera::ViewMatrixOffset2;
    
    // Read 16 floats (4x4 Matrix)
    float matrixData[16];
    if (!m_memory->Read(matrixAddress, matrixData))
        return;
    
    m_snapshot_back->viewMatrix = ViewMatrix_t::BuildLegacyGameMatrix(matrixData);
    
    // Update cache
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        m_addressCache.cGraphicsBase = cGraphicsBase;
        m_addressCache.cameraPtr = cameraPtr;
        m_addressCache.matrixAddress = matrixAddress;
        m_addressCache.cachedViewMatrix = m_snapshot_back->viewMatrix;
        m_addressCache.lastCacheUpdate = std::chrono::steady_clock::now();
    }
}

bool Game::IsEntityValid(uintptr_t entityAddress) const {
    return entityAddress != 0;
}

void Game::ReadHealthShield(uintptr_t entityAddress, EntitySnapshot& snapshot) {
    // Check if entity should have health at all
    if (!ShouldEntityHaveHealth(snapshot.type)) {
        snapshot.health = 0.0f;
        snapshot.maxHealth = 0.0f;
        snapshot.shield = 0.0f;
        snapshot.maxShield = 0.0f;
        return;
    }
    
    // Read pointer to Health/Shield data
    uintptr_t healthShieldPtr;
    if (!m_memory->Read(entityAddress + Offsets::Entity::PointerToHealthAndShield, healthShieldPtr) || healthShieldPtr == 0) {
        // Fallback values
        snapshot.health = 100.0f;
        snapshot.maxHealth = 100.0f;
        snapshot.shield = 100.0f;
        snapshot.maxShield = 100.0f;
        return;
    }
    
    // Read Health/Shield as INT (like in Cheat Engine!)
    int currentHealth, maxHealth, currentShield, maxShield;
    if (!m_memory->Read(healthShieldPtr + Offsets::Entity::CurHealth, currentHealth) ||
        !m_memory->Read(healthShieldPtr + Offsets::Entity::MaxHealth, maxHealth) ||
        !m_memory->Read(healthShieldPtr + Offsets::Entity::CurShield, currentShield) ||
        !m_memory->Read(healthShieldPtr + Offsets::Entity::MaxShield, maxShield))
    {
        // Fallback values
        snapshot.health = 100.0f;
        snapshot.maxHealth = 100.0f;
        snapshot.shield = 100.0f;
        snapshot.maxShield = 100.0f;
        return;
    }
    
    snapshot.health = static_cast<float>(currentHealth);
    snapshot.maxHealth = static_cast<float>(maxHealth);
    snapshot.shield = static_cast<float>(currentShield);
    snapshot.maxShield = static_cast<float>(maxShield);
    
    // Validation: Values must be realistic
    bool isValid = maxHealth > 0 && maxHealth <= 100000 &&
                   maxShield >= 0 && maxShield <= 100000 &&
                   currentHealth >= 0 && currentShield >= 0 &&
                   currentHealth <= maxHealth &&
                   currentShield <= maxShield;
    
    if (!isValid) {
        // Fallback values
        snapshot.health = 100.0f;
        snapshot.maxHealth = 100.0f;
        snapshot.shield = 100.0f;
        snapshot.maxShield = 100.0f;
    }
}

bool Game::ShouldEntityHaveHealth(EntityType type) const {
    return IsPlayerType(type) || IsGroundVehicleType(type) || IsAirVehicleType(type) || IsTurretType(type);
}

void Game::ReadHeadPosition(uintptr_t entityAddress, EntitySnapshot& snapshot)
{
    // Step 1: Only execute for players
    if (!IsPlayerType(snapshot.type)) return;

    // --- NEW POINTER CHAIN ---
    uintptr_t actorPtr;
    if (!m_memory->Read(entityAddress + Offsets::Entity::pActor, actorPtr) || actorPtr == 0) return;

    uintptr_t skeletonPtr;
    if (!m_memory->Read(actorPtr + Offsets::Entity::pSkeleton, skeletonPtr) || skeletonPtr == 0) return;

    uintptr_t skeletonInfoPtr;
    if (!m_memory->Read(skeletonPtr + Offsets::Entity::pSkeletonInfo, skeletonInfoPtr) || skeletonInfoPtr == 0) return;

    uintptr_t boneInfoPtr;
    if (!m_memory->Read(skeletonInfoPtr + Offsets::Entity::pBoneInfo, boneInfoPtr) || boneInfoPtr == 0) return;
    // --- END POINTER CHAIN ---


    // The final pointer to the start of the bone array is now boneInfoPtr + 0x194
    uintptr_t boneArrayStart = boneInfoPtr + Offsets::Entity::BoneArrayOffset;


    // Read the yaw angle of the enemy entity
    float yaw = snapshot.viewAngle.x;
    float rotationAngle = yaw + Offsets::GameConstants::ROTATION_OFFSET - Offsets::GameConstants::PI_OVER_2;
    float cosYaw = std::cos(rotationAngle);
    float sinYaw = std::sin(rotationAngle);


    const int HEAD_BONE_ID = Offsets::GameConstants::HEAD_BONE_ID;
    uintptr_t headBoneAddress = boneArrayStart + HEAD_BONE_ID * Offsets::GameConstants::BONE_SIZE;


    // Read raw, local bone position from correct pointer
    Utils::Vector3 rawHeadPos;
    if (m_memory->Read(headBoneAddress, rawHeadPos))
    {

        // Apply rotation
        Utils::Vector3 rotatedHeadPos;
        rotatedHeadPos.x = rawHeadPos.x * cosYaw + rawHeadPos.z * sinYaw;
        rotatedHeadPos.y = rawHeadPos.y;
        rotatedHeadPos.z = -rawHeadPos.x * sinYaw + rawHeadPos.z * cosYaw;


        // Add entity world position to get final world coordinate
        Utils::Vector3 finalHeadPos = snapshot.position + rotatedHeadPos;


        // Override placeholder with precise value
        snapshot.headPosition = finalHeadPos;
    }
}

std::string Game::ReadEntityName(uintptr_t entityAddress) {
    // Try primary name offset first
    std::string name = m_memory->ReadString(entityAddress + Offsets::Entity::Name, 64);
    
    // If name is empty or only whitespace, try backup offset
    if (name.empty() || name.find_first_not_of(" \t\n\r") == std::string::npos) {
        std::string backupName = m_memory->ReadString(entityAddress + Offsets::Entity::NameBackup, 64);
        
        // If backup name is valid, use it
        if (!backupName.empty() && backupName.find_first_not_of(" \t\n\r") != std::string::npos) {
            return backupName;
        }
    }
    
    // Return original name (even if empty)
    return name;
}

