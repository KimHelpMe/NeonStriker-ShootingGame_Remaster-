#pragma once
// ============================================================
//  리소스 관리 (이미지 / 효과음 / 폰트)
//  - 시작할 때 한 번 전부 로드하고, 종료할 때 한 번 해제한다.
//  - 게임 코드에서는 ID(enum)로만 접근 -> 파일 경로가 한 곳에만 존재
// ============================================================
#include "Config.h"

enum TexId {
    T_PLAYER, T_EN_NORMAL, T_EN_MEDIUM, T_EN_HEAVY,
    T_BOSS1, T_BOSS2, T_BOSS3, T_BOSS4,
    T_ITEM_NORMAL, T_ITEM_FLAME, T_ITEM_LASER, T_ITEM_BOMB, T_ITEM_FEVER,
    T_BULLET_P, T_BULLET_E, T_MISSILE, T_BOMB, T_GLOW, T_SPARK, T_BEAM,
    T_BG, T_LOGO,
    T_COUNT
};

enum SfxId {
    S_SHOT_NORMAL, S_SHOT_FLAME, S_SHOT_LASER,
    S_CHARGE, S_CHARGE_READY, S_CHARGE_SHOT,
    S_BOMB_THROW, S_BOMB_EXPLODE,
    S_HIT, S_EXPLODE_SMALL, S_EXPLODE_MID, S_BOSS_EXPLODE,
    S_PLAYER_HIT, S_PLAYER_DIE,
    S_ITEM, S_POWERUP, S_LEVELUP, S_FEVER,
    S_WARNING, S_SELECT, S_PAUSE, S_TICK, S_STAGE_START, S_GAME_OVER, S_CLEAR,
    S_COUNT
};

namespace Assets {
    void Load();
    void Unload();
    const Texture2D& Tex(TexId id);
    const Font& UIFont();

    // pitchVar: 0.1이면 피치를 ±10% 무작위로 흔들어 반복음이 덜 단조롭게 들림
    void Play(SfxId id, float volume = 1.0f, float pitchVar = 0.0f);
    void Stop(SfxId id);
    bool IsPlaying(SfxId id);
}

// --- 그리기 헬퍼 ---
void DrawSprite(TexId id, Vector2 center, float scale = 1.0f, float rotDeg = 0.0f, Color tint = WHITE);
void DrawGlow(Vector2 center, float radius, Color color);  // 가산 합성용 (BeginBlendMode 안에서 호출)
void DrawTextC(const char* text, float cx, float y, float size, Color color, float spacing = 2.0f); // 가운데 정렬
void DrawTextL(const char* text, float x, float y, float size, Color color, float spacing = 1.0f);  // 왼쪽 정렬
void DrawTextR(const char* text, float rx, float y, float size, Color color, float spacing = 1.0f); // 오른쪽 정렬
