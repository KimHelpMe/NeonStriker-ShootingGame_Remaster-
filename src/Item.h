#pragma once
// ============================================================
//  아이템: 5종 가중치 드랍 / 벽 튕김 / 15초 수명 + 사라지기 전 깜빡임
// ============================================================
#include "Config.h"

class Game;

struct Item {
    ItemType type;
    Vector2  pos, dir;
    float    lifeTimer;
    bool     active;
};

class ItemSystem {
public:
    void Reset();
    void Drop(Vector2 pos);           // 몬스터 사망 위치에 드랍
    void Update(Game& g, float dt);   // 이동 + 플레이어와 충돌하면 획득
    void Draw() const;

private:
    Item pool[ITEM_COUNT];
};
