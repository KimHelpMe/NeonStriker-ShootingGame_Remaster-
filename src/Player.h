#pragma once
// ============================================================
//  플레이어
//  이동 / 무기 3종 x 레벨 1~5 / 차지샷 / 폭탄 / 피버 / 레벨·EXP / 목숨·부활
// ============================================================
#include "Config.h"

class Game;

class Player {
public:
    void Reset();                    // 게임 처음부터 (원본 PlayerStart + PlayerInfo)
    void Update(Game& g, float dt);
    void Draw(const Game& g) const;

    void Damage(Game& g, int damage);
    void AddExp(Game& g);
    void ApplyItem(Game& g, ItemType type);

    // --- 기체에 붙어 있는 공격 (탄환 풀이 아닌 상태로 관리) ---
    bool      LaserOn() const { return laserOn; }
    Rectangle LaserRect() const;            // 일반 레이저 영역 (끝점은 laserEndX)
    bool      GigaLaserOn() const { return gigaLaserTimer > 0; }
    Rectangle GigaLaserRect() const;        // 차지 레이저 (세로 360px)
    bool      FlameOrbsOn() const { return flameOrbTimer > 0; }
    void      FlameOrbCenters(Vector2 out[4]) const;
    float     OrbSpread() const;          // 방출 직후 퍼져 나가는 정도 (0~1)
    static constexpr float FLAME_ORB_TIME = 4.0f;
    static constexpr float FLAME_ORB_RADIUS = 85.0f;   // 화염구 하나의 반지름 (강화 전 58)

    // 레이저가 보스에 막히는 지점 (Game이 매 프레임 계산해서 넣어줌)
    float laserEndX = SCREEN_W;
    float gigaLaserEndX = SCREEN_W;

    // --- 상태 조회 ---
    Vector2 pos{};
    static constexpr float HIT_RADIUS = 9.0f;   // 피격 판정 (기체 중심의 작은 원)

    bool  Alive() const { return !dead; }
    bool  Invincible() const { return invincibleTimer > 0 || hurtTimer > 0; }
    bool  GameOverReady() const { return dead && lives < 0 && respawnTimer <= 0; }
    int   hp = 5, maxHp = 5;
    int   level = 1, exp = 0, maxExp = 10;
    int   bombs = 3;
    int   lives = 2;            // 남은 '추가' 목숨 (원본과 동일: 2 = 총 3기)
    int   kills = 0;
    WeaponType weapon = WeaponType::NORMAL;
    int   weaponLevel = 1;
    bool  fever = false;
    float feverTimer = 0;
    bool  charging = false;
    float chargeTimer = 0;

private:
    void Fire(Game& g, float dt);
    void FireNormal(Game& g);
    void FireFlame(Game& g);
    void ReleaseCharge(Game& g);
    void Die(Game& g);

    bool  dead = false;
    float respawnTimer = 0;     // 폭발 연출 + 부활 대기
    float invincibleTimer = 0;  // 부활 후 무적 (깜빡임)
    float hurtTimer = 0;        // 피격 후 짧은 무적 (원본엔 없음: 몸통 접촉 시 매 프레임 HP가 깎이던 문제 방지)
    float fireTimer = 0;
    float flameOrbit = 0;       // 5레벨 화염 회전각
    bool  laserOn = false;
    float laserSoundTimer = 0;
    bool  chargeReadyPlayed = false;
    int   missileQueue = 0;     // NORMAL 차지: 남은 연사 횟수 (10발)
    float missileTimer = 0;
    float gigaLaserTimer = 0;   // LASER 차지 지속 시간 (5초)
    float flameOrbTimer = 0;    // FLAME 차지 지속 시간 (4초)
    float orbAngle = 0;
    float orbFxTimer = 0;       // 화염구 이펙트 방출 간격 (1/60초 고정)
    float time = 0;             // 연출용 누적 시간
};
