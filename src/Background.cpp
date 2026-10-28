#include "Background.h"
#include "Assets.h"

static void InitLayer(BackgroundParticle* layer, int count, float minSpd, float maxSpd,
                      float minSize, float maxSize, Color color)
{
    for (int i = 0; i < count; i++) {
        layer[i].x = RandF(0, SCREEN_W);
        layer[i].y = RandF(0, SCREEN_H);
        layer[i].speed = RandF(minSpd, maxSpd);
        layer[i].size = RandF(minSize, maxSize);
        layer[i].color = color;
    }
}

void Background::Init()
{
    // 1. 원경: 가장 작고 느림 / 2. 중경: 푸른 별 / 3. 근경: 크고 빠른 파편
    InitLayer(farLayer, 30, 25, 50, 2, 4, { 150, 150, 200, 255 });
    InitLayer(midLayer, 10, 80, 130, 4, 7, { 80, 160, 255, 255 });
    InitLayer(nearLayer, 3, 250, 350, 10, 16, { 255, 120, 230, 255 });
}

void Background::Update(float dt, float speedScale)
{
    auto UpdateLayer = [&](BackgroundParticle* layer, int count) {
        for (int i = 0; i < count; i++) {
            layer[i].x -= layer[i].speed * speedScale * dt;
            if (layer[i].x < -20) {                // 화면 왼쪽 끝을 넘어가면 오른쪽으로 (루프)
                layer[i].x = SCREEN_W + RandF(0, 100);
                layer[i].y = RandF(0, SCREEN_H);
            }
        }
    };
    UpdateLayer(farLayer, 30);
    UpdateLayer(midLayer, 10);
    UpdateLayer(nearLayer, 3);

    scrollX -= 15.0f * speedScale * dt;
    if (scrollX <= -SCREEN_W) scrollX += SCREEN_W;
}

void Background::Draw() const
{
    // 성운 이미지 2장을 이어 붙여 무한 스크롤
    const Texture2D& bg = Assets::Tex(T_BG);
    DrawTexture(bg, (int)scrollX, 0, WHITE);
    DrawTexture(bg, (int)scrollX + SCREEN_W, 0, WHITE);

    BeginBlendMode(BLEND_ADDITIVE);
    for (const auto& p : farLayer)  DrawGlow({ p.x, p.y }, p.size, p.color);
    for (const auto& p : midLayer)  DrawGlow({ p.x, p.y }, p.size, p.color);
    for (const auto& p : nearLayer) {
        // 근경은 속도감 표현을 위해 꼬리를 길게
        DrawGlow({ p.x, p.y }, p.size, ColorAlpha(p.color, 0.7f));
        for (int k = 1; k <= 6; k++)
            DrawGlow({ p.x + k * 8.0f, p.y }, p.size * (1 - k / 7.0f), ColorAlpha(p.color, 0.25f));
    }
    EndBlendMode();
}
