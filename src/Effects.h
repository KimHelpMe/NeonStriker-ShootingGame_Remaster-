#pragma once
// ============================================================
//  시각 효과 시스템
//  - 파티클(폭발, 불꽃, 스파크), 화면 흔들림, 화면 번쩍임, 떠오르는 글자
//  - 원본에서는 효과를 각 클래스의 Draw 안에서 rand()로 매 프레임 찍었지만,
//    여기서는 '수명을 가진 파티클'로 만들어 한곳에서 관리한다.
// ============================================================
#include "Config.h"

struct Particle {
    Vector2 pos, vel;
    float life, maxLife;
    float size, endSize;
    float drag;            // 1초당 속도 감쇠 비율 (0 = 감쇠 없음)
    Color color, endColor;
    bool  active;
};

struct Popup {
    char  text[32];
    Vector2 pos;
    float life;
    Color color;
    float size;
};

class Effects {
public:
    void Reset();
    void Update(float dt);
    void Draw() const;          // 월드 위에 가산 합성으로 그림
    void DrawOverlay() const;   // 화면 번쩍임 + 팝업 글자 (가장 위)

    void Spawn(Vector2 pos, Vector2 vel, float life, float size, float endSize,
               Color color, Color endColor, float drag = 0.0f);
    void Burst(Vector2 pos, int count, float speed, float size, float life, Color color, Color endColor);
    void Explosion(Vector2 pos, float scale, Color color);   // 불꽃 + 스파크 + 섬광 묶음
    void Shake(float amount, float time);
    void Flash(Color color, float time);
    void PopupText(const char* text, Vector2 pos, Color color, float size = 28.0f);

    Vector2 ShakeOffset() const { return shakeOffset; }

private:
    Particle particles[PARTICLE_COUNT];
    int      cursor = 0;            // 다음에 덮어쓸 위치 (가득 차면 오래된 것부터 재사용)
    Popup    popups[16];
    float    shakeAmount = 0, shakeTime = 0;
    Vector2  shakeOffset = { 0, 0 };
    Color    flashColor = BLANK;
    float    flashTime = 0, flashMax = 1;
};
