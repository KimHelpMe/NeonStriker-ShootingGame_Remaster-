#include "Effects.h"
#include "Assets.h"
#include <cstring>

void Effects::Reset()
{
    for (auto& p : particles) p.active = false;
    for (auto& p : popups) p.life = 0;
    shakeTime = 0; flashTime = 0; shakeOffset = { 0, 0 };
}

void Effects::Spawn(Vector2 pos, Vector2 vel, float life, float size, float endSize,
                    Color color, Color endColor, float drag)
{
    Particle& p = particles[cursor];
    cursor = (cursor + 1) % PARTICLE_COUNT;
    p = { pos, vel, life, life, size, endSize, drag, color, endColor, true };
}

void Effects::Burst(Vector2 pos, int count, float speed, float size, float life, Color color, Color endColor)
{
    for (int i = 0; i < count; i++) {
        float a = RandF(0, 2 * PI);
        float s = RandF(speed * 0.3f, speed);
        Spawn(pos, { cosf(a) * s, sinf(a) * s }, RandF(life * 0.5f, life), size, 0, color, endColor, 2.5f);
    }
}

void Effects::Explosion(Vector2 pos, float scale, Color color)
{
    // 1) 중심 섬광
    Spawn(pos, { 0, 0 }, 0.25f, 60 * scale, 120 * scale, WHITE, ColorAlpha(color, 0), 0);
    // 2) 불덩이 (흰색 -> 고유색 -> 사라짐)
    for (int i = 0; i < (int)(18 * scale); i++) {
        float a = RandF(0, 2 * PI), s = RandF(40, 220) * scale;
        Spawn(pos, { cosf(a) * s, sinf(a) * s }, RandF(0.3f, 0.7f), RandF(14, 30) * scale, 4,
              ColorLerp(WHITE, color, 0.3f), ColorAlpha(color, 0), 3.0f);
    }
    // 3) 스파크
    for (int i = 0; i < (int)(24 * scale); i++) {
        float a = RandF(0, 2 * PI), s = RandF(200, 600) * scale;
        Spawn(pos, { cosf(a) * s, sinf(a) * s }, RandF(0.2f, 0.5f), 5, 1, WHITE, ColorAlpha(color, 0), 4.0f);
    }
}

void Effects::Shake(float amount, float time)
{
    if (amount > shakeAmount || shakeTime <= 0) shakeAmount = amount;
    if (time > shakeTime) shakeTime = time;
}

void Effects::Flash(Color color, float time) { flashColor = color; flashTime = flashMax = time; }

void Effects::PopupText(const char* text, Vector2 pos, Color color, float size)
{
    for (auto& p : popups) {
        if (p.life > 0) continue;
        strncpy(p.text, text, sizeof(p.text) - 1);
        p.text[sizeof(p.text) - 1] = 0;
        p.pos = pos; p.life = 1.2f; p.color = color; p.size = size;
        return;
    }
}

void Effects::Update(float dt)
{
    for (auto& p : particles) {
        if (!p.active) continue;
        p.life -= dt;
        if (p.life <= 0) { p.active = false; continue; }
        float k = 1.0f / (1.0f + p.drag * dt);   // 공기저항 (원본 폭탄 파편의 감속 효과와 같은 역할)
        p.vel.x *= k; p.vel.y *= k;
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
    }
    for (auto& p : popups) {
        if (p.life <= 0) continue;
        p.life -= dt;
        p.pos.y -= 40 * dt;
    }
    if (shakeTime > 0) {
        shakeTime -= dt;
        shakeOffset = { RandF(-shakeAmount, shakeAmount), RandF(-shakeAmount, shakeAmount) };
        if (shakeTime <= 0) { shakeOffset = { 0, 0 }; shakeAmount = 0; }
    }
    if (flashTime > 0) flashTime -= dt;
}

void Effects::Draw() const
{
    BeginBlendMode(BLEND_ADDITIVE);   // 빛이 겹칠수록 밝아지는 '네온' 느낌의 핵심
    for (const auto& p : particles) {
        if (!p.active) continue;
        float t = 1.0f - p.life / p.maxLife;          // 0 -> 1
        float size = Lerp(p.size, p.endSize, t);
        DrawGlow(p.pos, size, ColorLerp(p.color, p.endColor, t));
    }
    EndBlendMode();
}

void Effects::DrawOverlay() const
{
    for (const auto& p : popups) {
        if (p.life <= 0) continue;
        float a = p.life > 0.3f ? 1.0f : p.life / 0.3f;
        DrawTextC(p.text, p.pos.x, p.pos.y, p.size, ColorAlpha(p.color, a));
    }
    if (flashTime > 0) {
        DrawRectangle(0, 0, SCREEN_W, SCREEN_H, ColorAlpha(flashColor, (flashTime / flashMax) * (flashColor.a / 255.0f)));
    }
}
