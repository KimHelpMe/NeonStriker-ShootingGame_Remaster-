#pragma once
// ============================================================
//  탄환 풀 (Object Pool)
//
//  [원본과 가장 크게 달라진 점]
//  원본은 탄 종류를 '속도값'으로 구분했다 (50.0f=일반탄, 33.33f=화염, 88.88f=기가미사일,
//  99.99f=폭탄파편, 1000.0f=레이저 ...). 그래서 속도를 바꾸면 판정 코드가 전부 깨졌다.
//  여기서는 BulletType 열거형으로 종류를 명시하고, 속도(vel)는 순수하게 이동에만 쓴다.
//
//  레이저 / 기가 레이저 / 화염구 같은 '기체에 붙어 있는 공격'은 탄환이 아니라
//  Player의 상태로 관리한다 (원본은 탄환을 매 프레임 기체 위치로 되돌리는 방식이었음).
// ============================================================
#include "Config.h"

class Game;

enum class BulletType {
    P_NORMAL,    // 일반탄
    P_FLAME,     // 화염 입자 (관통)
    P_MISSILE,   // NORMAL 차지샷: 기가 미사일 (전진할수록 세로로 넓어짐)
    P_BOMB,      // 폭탄 본체 (C)
    P_SHRAPNEL,  // 폭탄 파편
    E_SMALL,     // 몬스터 탄
    E_BOSS,      // 보스 탄
};

struct Bullet {
    BulletType type;
    Vector2 pos, vel;
    float   age;        // 생성 후 경과 시간
    float   life;       // 최대 수명 (초)
    float   startX;     // 발사 위치 (미사일 폭 계산용)
    float   radius;
    int     damage;
    Color   color;
    bool    active;

    bool IsPlayerSide() const { return type <= BulletType::P_SHRAPNEL; }
    float MissileHalfHeight() const;   // 기가 미사일의 현재 세로 반폭
};

class BulletSystem {
public:
    void Reset();
    Bullet* Spawn(BulletType type, Vector2 pos, Vector2 vel, int damage, Color color, float life = 6.0f);
    void Update(Game& g, float dt);
    void Draw() const;

    // 적 탄환 제거 (폭탄 / 보스 등장 시). withEffect면 사라지는 자리에 작은 빛을 남김
    void ClearEnemyBullets(Game& g, bool withEffect);
    void ShatterBomb(Game& g, Vector2 pos);   // 폭탄 폭발: 적탄 제거 + 파편 200개

    Bullet* Data() { return pool; }

private:
    Bullet pool[BULLET_COUNT];
    int searchStart = 0;   // 빈 슬롯 탐색 시작점 (매번 0부터 찾지 않도록)
};
