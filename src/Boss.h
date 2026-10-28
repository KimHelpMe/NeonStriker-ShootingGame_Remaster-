#pragma once
// ============================================================
//  보스 (스테이지당 1기, 4단계)
//  상태: NONE -> WARNING(등장 경고 3초 + 진입) -> FIGHT -> EXPLODING -> NONE
// ============================================================
#include "Config.h"

class Game;

enum class BossState { NONE, WARNING, FIGHT, EXPLODING };

class Boss {
public:
    void Init(int stage);              // 스테이지 시작 시 (킬 카운트 0)
    void AddKillCount(Game& g);        // 난이도별 킬수(Rules().killRequirement)를 채우면 등장
    void ForceAppear(Game& g);         // 테스트 키 6
    void Update(Game& g, float dt);
    void Draw() const;
    void DrawUI() const;               // 상단 HP바 + WARNING 연출

    bool Damage(Game& g, int damage, float invincible);   // 피격 (반환: 피해가 들어갔는지)
    Rectangle Hitbox() const;

    BossState state = BossState::NONE;
    bool  Fighting() const { return state == BossState::FIGHT; }
    bool  Present() const  { return state != BossState::NONE; }
    bool  defeated = false;            // 폭발 연출이 끝나면 true (Game이 읽고 다음 스테이지로)
    int   killCount = 0;
    int   step = 1;                    // 1~4
    int   hp = 0, maxHp = 1;
    Vector2 pos{};                     // 중심 좌표

private:
    void Fire(Game& g);
    void SpiralPattern(Game& g, Vector2 p);

    float warningTimer = 0;
    float explodeTimer = 0;
    float moveAcc = 0;
    float fireTimer = 0;
    float spiralAngle = 0;
    float invincibleTimer = 0;
    float flash = 0;
    int   burstCount = 0;
    float time = 0;
};
