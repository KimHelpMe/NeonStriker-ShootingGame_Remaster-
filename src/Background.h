#pragma once
// ============================================================
//  3레이어 패럴랙스 배경 (원본 Background와 같은 구조)
//  원경 30 / 중경 10 / 근경 3 + 성운 이미지 스크롤
// ============================================================
#include "Config.h"

struct BackgroundParticle {
    float x, y, speed, size;
    Color color;
};

class Background {
public:
    void Init();
    void Update(float dt, float speedScale = 1.0f);
    void Draw() const;

private:
    BackgroundParticle farLayer[30];
    BackgroundParticle midLayer[10];
    BackgroundParticle nearLayer[3];
    float scrollX = 0;   // 성운 이미지 스크롤 위치
};
