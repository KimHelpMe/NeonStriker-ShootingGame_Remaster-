#include "Item.h"
#include "Game.h"
#include "Assets.h"

void ItemSystem::Reset()
{
    for (auto& it : pool) it.active = false;
}

void ItemSystem::Drop(Vector2 pos)
{
    for (auto& it : pool) {
        if (it.active) continue;
        // ===== 드랍 밸런스 (가중치 방식, 원본과 동일) =====
        int r = GetRandomValue(0, 99);
        if      (r < 35) it.type = ItemType::NORMAL;   // 35%
        else if (r < 60) it.type = ItemType::FLAME;    // 25%
        else if (r < 85) it.type = ItemType::LASER;    // 25%
        else if (r < 95) it.type = ItemType::BOMB;     // 10%
        else             it.type = ItemType::FEVER;    //  5%

        float a = RandF(0, 2 * PI);
        it.dir = { cosf(a), sinf(a) };                 // 정규화된 무작위 방향
        it.pos = pos;
        it.lifeTimer = 0;
        it.active = true;
        return;
    }
}

void ItemSystem::Update(Game& g, float dt)
{
    const float speed = 80.0f;
    for (auto& it : pool) {
        if (!it.active) continue;
        it.lifeTimer += dt;
        if (it.lifeTimer >= ITEM_LIFETIME) { it.active = false; continue; }

        it.pos.x += it.dir.x * speed * dt;
        it.pos.y += it.dir.y * speed * dt;
        // 벽 튕기기 (좌우 / 상하)
        if (it.pos.x < 20 || it.pos.x > SCREEN_W - 20) { it.dir.x = -it.dir.x; it.pos.x = Clamp(it.pos.x, 20, SCREEN_W - 20); }
        if (it.pos.y < PLAY_TOP + 20 || it.pos.y > PLAY_BOTTOM - 20) { it.dir.y = -it.dir.y; it.pos.y = Clamp(it.pos.y, PLAY_TOP + 20, PLAY_BOTTOM - 20); }

        // 획득 판정
        if (g.player.Alive() && CircleHit(it.pos, 22, g.player.pos, 22)) {
            it.active = false;
            g.fx.Burst(it.pos, 16, 220, 7, 0.4f, WHITE, BLANK);
            g.player.ApplyItem(g, it.type);
        }
    }
}

void ItemSystem::Draw() const
{
    static const TexId TEX[] = { T_ITEM_NORMAL, T_ITEM_FLAME, T_ITEM_LASER, T_ITEM_BOMB, T_ITEM_FEVER };
    static const Color COL[] = { { 0, 230, 255, 255 }, { 255, 110, 30, 255 }, { 255, 240, 60, 255 },
                                 { 120, 255, 120, 255 }, WHITE };
    float t = (float)GetTime();
    for (const auto& it : pool) {
        if (!it.active) continue;
        // 사라지기 3초 전부터 깜빡임 (마지막 1초는 2배 빠르게)
        float remain = ITEM_LIFETIME - it.lifeTimer;
        if (remain < 3.0f) {
            float speed = remain < 1.0f ? 20.0f : 10.0f;
            if ((int)(it.lifeTimer * speed) % 2 == 0) continue;
        }
        Color c = it.type == ItemType::FEVER ? Rainbow(t * 1.5f) : COL[(int)it.type];   // FEVER는 무지개 순환
        float pulse = 1.0f + sinf(t * 6) * 0.08f;
        BeginBlendMode(BLEND_ADDITIVE);
        DrawGlow(it.pos, 32 * pulse, ColorAlpha(c, 0.55f));
        EndBlendMode();
        DrawSprite(TEX[(int)it.type], it.pos, pulse, 0, it.type == ItemType::FEVER ? c : WHITE);
    }
}
