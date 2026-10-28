#include "Bullet.h"
#include "Game.h"
#include "Assets.h"

float Bullet::MissileHalfHeight() const
{
    // 원본: spread = (전진거리 / 2.5) + 3, 최대 20줄
    // 강화: 24px에서 시작해 더 빨리 넓어지고, 최대 260px (플레이 영역 세로 600px의 대부분)
    float h = 24.0f + (pos.x - startX) * 0.5f;
    return h > 260.0f ? 260.0f : h;
}

void BulletSystem::Reset()
{
    for (auto& b : pool) b.active = false;
    searchStart = 0;
}

Bullet* BulletSystem::Spawn(BulletType type, Vector2 pos, Vector2 vel, int damage, Color color, float life)
{
    for (int n = 0; n < BULLET_COUNT; n++) {
        int i = (searchStart + n) % BULLET_COUNT;
        if (pool[i].active) continue;
        searchStart = (i + 1) % BULLET_COUNT;

        Bullet& b = pool[i];
        b.type = type; b.pos = pos; b.vel = vel;
        b.age = 0; b.life = life; b.startX = pos.x;
        b.damage = damage; b.color = color; b.active = true;
        switch (type) {
            case BulletType::P_NORMAL:   b.radius = 6;  break;
            case BulletType::P_FLAME:    b.radius = 10; break;
            case BulletType::P_MISSILE:  b.radius = 16; break;
            case BulletType::P_BOMB:     b.radius = 14; break;
            case BulletType::P_SHRAPNEL: b.radius = 6;  break;
            case BulletType::E_SMALL:    b.radius = 6;  break;
            case BulletType::E_BOSS:     b.radius = 7;  break;
        }
        return &b;
    }
    return nullptr;   // 풀이 가득 참
}

void BulletSystem::ClearEnemyBullets(Game& g, bool withEffect)
{
    for (auto& b : pool) {
        if (!b.active || b.IsPlayerSide()) continue;
        if (withEffect) g.fx.Spawn(b.pos, { 0, 0 }, 0.3f, 10, 0, WHITE, ColorAlpha(b.color, 0));
        b.active = false;
    }
}

void BulletSystem::ShatterBomb(Game& g, Vector2 pos)
{
    ClearEnemyBullets(g, true);   // 폭탄은 적 미사일을 '두 번' 제거 (1. 투척 시, 2. 폭발 시)

    const int fireworkCount = 200;      // 파편 수 (원본과 동일)
    const float expansionSpeed = 650;   // 폭발 확산 속도
    for (int i = 0; i < fireworkCount; i++) {
        float a = i * (2 * PI / fireworkCount);
        float s = expansionSpeed * RandF(0.6f, 1.0f);
        Color c = (i % 3 == 0) ? Color{ 180, 255, 120, 255 } : Color{ 120, 255, 200, 255 };
        Spawn(BulletType::P_SHRAPNEL, pos, { cosf(a) * s, sinf(a) * s }, 5, c, 1.2f);
    }
    g.fx.Explosion(pos, 3.0f, { 120, 255, 160, 255 });
    g.fx.Shake(14, 0.6f);
    g.fx.Flash({ 200, 255, 220, 140 }, 0.35f);
    Assets::Play(S_BOMB_EXPLODE);
}

void BulletSystem::Update(Game& g, float dt)
{
    for (auto& b : pool) {
        if (!b.active) continue;
        b.age += dt;

        switch (b.type) {
        case BulletType::P_SHRAPNEL: {
            float k = 1.0f / (1.0f + 1.6f * dt);   // 감속 (공기 저항)
            b.vel.x *= k; b.vel.y *= k;
            break;
        }
        case BulletType::P_FLAME: {
            float k = 1.0f / (1.0f + 2.0f * dt);
            b.vel.x *= k; b.vel.y *= k;
            if (GetRandomValue(0, 3) == 0)   // 화염 꼬리 연기
                g.fx.Spawn(b.pos, { RandF(-20, 20), RandF(-40, -10) }, 0.3f, 8, 2, ColorAlpha(b.color, 0.4f), BLANK);
            break;
        }
        case BulletType::P_BOMB:
            if (b.age >= b.life) {           // 일정 거리를 날아가면 폭발
                b.active = false;
                ShatterBomb(g, b.pos);
                continue;
            }
            break;
        case BulletType::P_MISSILE: {
            // 미사일 벽 전체에서 분사 화염 (윗줄~아랫줄 골고루)
            float h = b.MissileHalfHeight();
            for (int k = 0; k < 3; k++)
                g.fx.Spawn({ b.pos.x - 24, b.pos.y + RandF(-h, h) }, { RandF(-260, -140), RandF(-20, 20) },
                           RandF(0.2f, 0.35f), 12, 2, { 160, 240, 255, 230 }, { 40, 80, 255, 0 }, 2.0f);
            break;
        }
        default: break;
        }

        b.pos.x += b.vel.x * dt;
        b.pos.y += b.vel.y * dt;

        // 수명 / 화면 밖 체크
        if (b.age >= b.life ||
            b.pos.x < -60 || b.pos.x > SCREEN_W + 60 ||
            b.pos.y < PLAY_TOP - 60 || b.pos.y > PLAY_BOTTOM + 60) {
            b.active = false;
        }
    }
}

void BulletSystem::Draw() const
{
    // 1패스: 빛번짐(가산 합성)
    BeginBlendMode(BLEND_ADDITIVE);
    for (const auto& b : pool) {
        if (!b.active) continue;
        float t = b.age / b.life;
        switch (b.type) {
        case BulletType::P_NORMAL:
            DrawGlow(b.pos, 14, ColorAlpha(b.color, 0.5f));
            DrawSprite(T_BULLET_P, b.pos, 1.0f, atan2f(b.vel.y, b.vel.x) * RAD2DEG, b.color);
            break;
        case BulletType::P_FLAME: {
            // 시간에 따라 흰색 -> 노랑 -> 빨강 -> 사라짐, 크기는 커짐 (원본 레벨별 화염 색상 단계)
            Color c = t < 0.2f ? ColorLerp(WHITE, { 255, 230, 80, 255 }, t / 0.2f)
                                : ColorLerp({ 255, 230, 80, 255 }, { 255, 40, 20, 0 }, (t - 0.2f) / 0.8f);
            DrawGlow(b.pos, 10 + t * 26, c);
            break;
        }
        case BulletType::P_MISSILE: {
            // 기가 미사일: 세로로 늘어선 미사일 벽
            float h = b.MissileHalfHeight();
            DrawGlow(b.pos, h * 0.9f + 40, { 40, 120, 255, 70 });                  // 벽 전체를 감싸는 푸른 빛
            for (float y = -h; y <= h; y += 18) {
                Vector2 p = { b.pos.x - fabsf(y) * 0.15f, b.pos.y + y };   // 가운데가 앞서는 화살촉 대형
                DrawGlow(p, 22, { 60, 180, 255, 140 });
                DrawSprite(T_MISSILE, p, 1.0f, 0, { 200, 245, 255, 255 });
            }
            break;
        }
        case BulletType::P_BOMB:
            DrawGlow(b.pos, 30, { 100, 255, 120, 160 });
            DrawSprite(T_BOMB, b.pos, 1.2f, b.age * 720.0f, WHITE);
            break;
        case BulletType::P_SHRAPNEL:
            DrawGlow(b.pos, 10 * (1 - t) + 3, ColorAlpha(b.color, 1 - t * 0.7f));
            break;
        case BulletType::E_SMALL:
            DrawGlow(b.pos, 16, ColorAlpha(b.color, 0.6f));
            DrawSprite(T_BULLET_E, b.pos, 0.8f, 0, b.color);
            break;
        case BulletType::E_BOSS:
            DrawGlow(b.pos, 18, ColorAlpha(b.color, 0.6f));
            DrawSprite(T_BULLET_E, b.pos, 1.0f, 0, b.color);
            break;
        }
    }
    EndBlendMode();

    // 2패스: 적 탄 중심은 일반 합성으로 하얗게 (밝은 배경에서도 탄이 잘 보이도록)
    for (const auto& b : pool) {
        if (!b.active || b.IsPlayerSide()) continue;
        DrawCircleV(b.pos, b.radius * 0.45f, WHITE);
    }
}
