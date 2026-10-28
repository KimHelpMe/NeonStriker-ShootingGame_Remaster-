#pragma once
// ============================================================
//  일반 몬스터 3종 (NORMAL 90% / MEDIUM 8% / HEAVY 2%)
// ============================================================
#include "Config.h"

class Game;

enum class MonsterType { NORMAL, MEDIUM, HEAVY };

struct Monster {
    MonsterType type;
    Vector2 pos;
    Vector2 dir;          // 원본과 같이 x는 -1, y는 -1/0/1
    float   speed;
    int     hp, maxHp;
    float   fireTimer;    // 다음 발사까지 남은 시간
    float   hitCooldown;  // 관통형 공격(화염/레이저)에 연속으로 맞는 간격
    float   flash;        // 피격 시 하얗게 번쩍임
    bool    active;

    float Radius() const { return type == MonsterType::NORMAL ? 17.0f : type == MonsterType::MEDIUM ? 24.0f : 34.0f; }
};

class MonsterSystem {
public:
    void Start(const DifficultyRules& rules);   // 난이도 규칙에 맞춰 생성
    void Reset();                 // 전체 비활성 (보스전 / 재시작)
    void Update(Game& g, float dt);
    void Draw() const;

    // 피해 처리. cooldown > 0이면 그 시간 동안 같은 대상에게 다시 피해를 주지 않음
    // 반환값: 실제로 피해가 들어갔는지
    bool Hit(Game& g, int idx, int damage, float cooldown);

    Monster* Data() { return m; }
    bool enabled = true;          // 보스전 동안 false

private:
    void Init(int i);
    void Kill(Game& g, int i, bool rammed);

    Monster m[MONSTER_COUNT];
    int activeCount = 20;         // 이번 판에 쓰는 슬롯 수 (난이도별)
    int normalCount = 18;
    int mediumCount = 1;
};
