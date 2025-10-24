#include "pch.h"
#include "MagicBullet.h"
#include "TargetManager.h"
#include "../Game/Game.h"
#include "../Game/BulletHarvester.h"
#include "../Game/Offsets.h"
#include "../Utils/Logger.h"
#include "../Utils/Settings.h"
#include "../framework.h"

MagicBullet::MagicBullet() {
    Logger::Log("MagicBullet class created");
    m_isRunning = true;
    m_updateThread = std::thread(&MagicBullet::UpdateThread, this);
    Logger::Log("UpdateThread started");
}

MagicBullet::~MagicBullet() {
    Logger::Log("MagicBullet class destroying...");
    m_isRunning = false;
    if (m_updateThread.joinable()) {
        m_updateThread.join();
    }
    Logger::Log("MagicBullet class destroyed");
}

int MagicBullet::GetActiveBullets() const {
    if (g_BulletHarvester) {
        return g_BulletHarvester->GetBulletCount();
    }
    return 0;
}

void MagicBullet::UpdateThread() {
    Logger::Log("MagicBullet::UpdateThread entered.");

    // ================================================================
    // =================== START-UP SYNC FIX =========================
    // ================================================================
    int startup_tries = 0;
    while (!DX11Base::g_Running) {
        if (++startup_tries > 100) { // Timeout after ~1 second
            Logger::Error("MagicBullet::UpdateThread timed out waiting for g_Running.");
            return;
        }
        std::this_thread::yield();
    }
    Logger::Log("MagicBullet::UpdateThread: g_Running confirmed, starting main loop.");
    // ================================================================

    while (m_isRunning && DX11Base::g_Running) {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        if (g_Settings.MagicBullet.bEnabled) {
            m_wasEnabled = true;
            ManipulateFreshBullets();
        } else {
            m_wasEnabled = false;
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        m_updateTime = std::chrono::duration<float, std::milli>(endTime - startTime).count();
        std::this_thread::yield();
    }
    Logger::Log("MagicBullet::UpdateThread exited loop.");
}

void MagicBullet::ManipulateFreshBullets() {
    if (!g_BulletHarvester || !g_TargetManager || !g_Game) return;

    auto targetOpt = g_TargetManager->GetCurrentTarget();
    if (!targetOpt) return;

    const EntitySnapshot& currentTarget = *targetOpt;
    Utils::Vector3 targetPos = g_Settings.MagicBullet.bTargetHead ? currentTarget.headPosition : currentTarget.position;

    auto bulletSnapshot = g_BulletHarvester->GetBulletSnapshot();
    auto worldSnapshot = g_Game->GetWorldSnapshot();

    if (!bulletSnapshot || !worldSnapshot) return;

    // ✅ NEW: Debug counter
    int validBullets = 0;
    int manipulatedBullets = 0;

    for (const auto& bullet : bulletSnapshot->bullets) {
        validBullets++;

        float distSquared = worldSnapshot->localPlayer.position.DistanceSquared(bullet.startPosition);
        if (distSquared > Offsets::GameConstants::MAGIC_BULLET_DISTANCE_THRESHOLD) continue;

        ManipulateBullet(bullet, targetPos);
        manipulatedBullets++;
    }
    
    // ✅ NEW: Log only if bullets exist but were not manipulated
    static int logCounter = 0;
    if (validBullets > 0 && manipulatedBullets == 0 && ++logCounter % 100 == 0) {
        Logger::Log("[MagicBullet] %d valid bullets, but 0 manipulated (check live validation)", validBullets);
    }
}

void MagicBullet::ManipulateBullet(const BulletSnapshot& bullet, const Utils::Vector3& targetPos) {
    if (!g_Game) return;
    
    // Use the bullet.id as the live pointer for memory operations
    uintptr_t bulletPtr = bullet.id;
    
    // Now manipulate (as before)...
    Utils::Vector3 bulletPosition = CalculateBulletPosition(targetPos);
    
    float positionData[4] = { bulletPosition.x, bulletPosition.y, bulletPosition.z, 1.0f };
    if (!g_Game->GetMemory()->WriteBytes(bulletPtr + Offsets::MagicBullet::bullet_position_offset, positionData, 16)) return;
    
    uint32_t newHash = g_Game->GetMemory()->CalculateBulletHashInternal(positionData, 16);
    if (!g_Game->GetMemory()->Write(bulletPtr + Offsets::MagicBullet::o_BulletHash, newHash)) return;
    
    Utils::Vector3 direction = (targetPos - bulletPosition).Normalized();
    if (!g_Game->GetMemory()->Write(bulletPtr + Offsets::MagicBullet::bullet_direction_offset, direction)) return;
    
    if (!g_Game->GetMemory()->Write(bulletPtr + Offsets::MagicBullet::bullet_speed_offset, Offsets::GameConstants::MAGIC_BULLET_SPEED)) return;
}

Utils::Vector3 MagicBullet::CalculateBulletPosition(const Utils::Vector3& targetPos) {
    Utils::Vector3 bulletPosition = targetPos;
    bulletPosition.y += Offsets::GameConstants::MAGIC_BULLET_POSITION_OFFSET;
    return bulletPosition;
}