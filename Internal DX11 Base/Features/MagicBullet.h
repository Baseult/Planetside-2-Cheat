#pragma once
#include "../Game/GameData.h"
#include <thread>
#include <atomic>
#include <memory>

class MagicBullet {
public:
    MagicBullet();
    ~MagicBullet();
    
    float GetUpdateTime() const { return m_updateTime; }
    int GetActiveBullets() const;
    
private:
    void UpdateThread();
    void ManipulateFreshBullets();
    void ManipulateBullet(const BulletSnapshot& bullet, const Utils::Vector3& targetPos, const Utils::Vector3& viewAngle);
    Utils::Vector3 CalculateBulletPosition(const Utils::Vector3& targetPos, const Utils::Vector3& viewAngle);
    
    std::thread m_updateThread;
    std::atomic<bool> m_isRunning = true;
    float m_updateTime = 0.0f;
    bool m_wasEnabled = false;
};

inline std::unique_ptr<MagicBullet> g_MagicBullet;
