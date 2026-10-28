#pragma once
// ============================================================
//  공용 설정값 / 열거형 / 헬퍼
//  (원본의 Defines.h + Struct.h 역할)
// ============================================================
#include "raylib.h"
#include "raymath.h"
#include <cmath>

// --- 화면 (내부 해상도. 실제 창 크기에 맞춰 확대/축소해서 출력) ---
constexpr int   SCREEN_W    = 1280;
constexpr int   SCREEN_H    = 720;
constexpr float PLAY_TOP    = 44.0f;   // 위쪽 보스 HP바 영역
constexpr float PLAY_BOTTOM = 640.0f;  // 아래쪽 HUD 영역 시작

// --- 오브젝트 풀 크기 ---
constexpr int BULLET_COUNT   = 4000;   // 원본 DF_BULLET_COUNT
constexpr int MONSTER_COUNT  = 30;     // 풀 최대 크기 (실제 동시 출현 수는 난이도 규칙의 monsterCount)
constexpr int ITEM_COUNT     = 40;
constexpr int PARTICLE_COUNT = 4000;

// --- 게임 규칙 (원본 수치를 픽셀 단위로 환산) ---
constexpr int   LAST_STAGE           = 4;
constexpr int   MAX_WEAPON_LEVEL      = 5;
constexpr float MAX_CHARGE_TIME       = 3.0f; // 풀 차징 시간
constexpr float FEVER_TIME            = 10.0f;
constexpr float ITEM_LIFETIME         = 15.0f;

enum class WeaponType { NORMAL, FLAME, LASER };
enum class ItemType   { NORMAL, FLAME, LASER, BOMB, FEVER };  // 앞 3개는 WeaponType과 순서가 같아야 함

// --- 난이도 ---
// 난이도마다 달라지는 값은 전부 이 표에만 둔다 (코드 곳곳에 if (hard) 를 흩뿌리지 않기 위해)
enum class Difficulty { NORMAL, HARD };

struct DifficultyRules {
    const char* name;
    int   monsterCount;     // 화면에 동시에 있는 몬스터 수 (MONSTER_COUNT 이하)
    int   normalCount;      // 그중 NORMAL 수
    int   mediumCount;      // 그중 MEDIUM 수 (나머지는 HEAVY)
    float bossHpScale;      // 보스 체력 배율
    int   killRequirement;  // ★보스 등장 킬수★ (원본 100)
};

constexpr DifficultyRules DIFFICULTY_RULES[2] = {
    //  이름      수   NORMAL MEDIUM 보스HP  킬수
    { "NORMAL",  20,   18,    1,     1.0f,  200 },   // HEAVY 1  (90/5/5%)
    { "HARD",    30,   24,    4,     2.0f,  300 },   // HEAVY 2  (80/13/7%)
};

// --- 헬퍼 ---
inline float RandF(float a, float b) { return a + (b - a) * (GetRandomValue(0, 100000) / 100000.0f); }
inline Color Rainbow(float t, float sat = 0.75f) { return ColorFromHSV(fmodf(t * 360.0f, 360.0f), sat, 1.0f); }
inline bool  CircleHit(Vector2 a, float ra, Vector2 b, float rb) {
    float dx = a.x - b.x, dy = a.y - b.y, r = ra + rb;
    return dx * dx + dy * dy <= r * r;
}
