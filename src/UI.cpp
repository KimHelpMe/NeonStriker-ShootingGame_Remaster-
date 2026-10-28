#include "UI.h"
#include "Game.h"
#include "Assets.h"

static const Color NEON_CYAN    = { 0, 230, 255, 255 };
static const Color NEON_MAGENTA = { 255, 60, 220, 255 };
static const Color NEON_YELLOW  = { 255, 230, 60, 255 };
static const Color STAGE_COLOR[5] = { WHITE, NEON_CYAN, { 60, 255, 140, 255 }, { 255, 150, 40, 255 }, { 200, 80, 255, 255 } };

// 0~1 사이를 부드럽게 오가는 값 (원본의 sinf 페이드 효과)
static float Pulse(float t, float speed) { return 0.5f + 0.5f * sinf(t * speed); }

// 네온 테두리 박스
static void NeonBox(Rectangle r, Color c, float alpha = 1.0f)
{
    DrawRectangleRec(r, ColorAlpha({ 8, 4, 24, 255 }, 0.85f * alpha));
    DrawRectangleLinesEx(r, 2, ColorAlpha(c, alpha));
    DrawRectangleLinesEx({ r.x - 4, r.y - 4, r.width + 8, r.height + 8 }, 1, ColorAlpha(c, 0.35f * alpha));
}

// ------------------------------------------------------------
void UI::DrawLogo(const Game& g)
{
    float t = g.sceneTime;
    // 테두리 깜빡임 (노랑/흰색)
    Color border = ((int)(t * 2) % 2 == 0) ? NEON_YELLOW : WHITE;
    DrawRectangleLinesEx({ 20, 20, SCREEN_W - 40, SCREEN_H - 40 }, 3, ColorAlpha(border, 0.8f));

    // 로고: 위아래로 살짝 떠다니며 밝기 변화
    float y = 190 + sinf(t * 1.5f) * 8;
    BeginBlendMode(BLEND_ADDITIVE);
    DrawGlow({ SCREEN_W / 2, y + 85 }, 420, ColorAlpha(NEON_CYAN, 0.15f + Pulse(t, 2) * 0.1f));
    EndBlendMode();
    const Texture2D& logo = Assets::Tex(T_LOGO);
    DrawTexture(logo, (SCREEN_W - logo.width) / 2, (int)y, ColorAlpha(WHITE, 0.75f + Pulse(t, 2) * 0.25f));

    DrawTextC("2D NEON SHOOTING", SCREEN_W / 2, y + 180, 26, ColorAlpha(NEON_MAGENTA, 0.9f), 10);

    // 난이도 메뉴 (선택된 항목만 밝게 + 좌우 화살표 깜빡임)
    static const Color DIFF_COLOR[2] = { NEON_CYAN, { 255, 50, 80, 255 } };
    for (int i = 0; i < 2; i++) {
        bool sel = (int)g.difficulty == i;
        float my = 440 + i * 46.0f;
        const char* name = DIFFICULTY_RULES[i].name;
        if (sel) {
            DrawTextC(TextFormat(">   %s   <", name), SCREEN_W / 2, my, 34, ColorAlpha(DIFF_COLOR[i], 0.6f + Pulse(t, 6) * 0.4f), 6);
        } else {
            DrawTextC(name, SCREEN_W / 2, my, 28, ColorAlpha(WHITE, 0.35f), 6);
        }
    }
    const DifficultyRules& r = g.Rules();
    DrawTextC(TextFormat("ENEMIES %d   BOSS HP x%.0f   BOSS AFTER %d KILLS", r.monsterCount, r.bossHpScale, r.killRequirement),
              SCREEN_W / 2, 538, 18, ColorAlpha(WHITE, 0.7f), 3);
    DrawTextC("UP / DOWN : SELECT      ENTER : START", SCREEN_W / 2, 566, 18, ColorAlpha(WHITE, Pulse(t, 3) * 0.5f + 0.5f), 3);

    // 조작법
    const char* help = "MOVE: ARROWS    SHOOT: SPACE    CHARGE (Lv5): X    BOMB: C    PAUSE: ESC";
    DrawTextC(help, SCREEN_W / 2, 600, 18, ColorAlpha(WHITE, 0.6f), 2);
    DrawTextC("TEST KEYS  1: Weapon Lv+   2/3/4: Normal/Flame/Laser   5: Fever   6: Boss", SCREEN_W / 2, 630, 15,
              ColorAlpha(WHITE, 0.35f), 2);
}

void UI::DrawStageIntro(const Game& g)
{
    float t = g.sceneTime;
    Color c = STAGE_COLOR[g.stage];
    // 글자가 왼쪽에서 미끄러져 들어와서 멈춤
    float slide = t < 0.4f ? (1 - t / 0.4f) * -600 : 0;
    float alpha = t > 2.1f ? (2.5f - t) / 0.4f : 1.0f;
    if (alpha < 0) alpha = 0;

    DrawRectangle(0, 250, SCREEN_W, 200, ColorAlpha(BLACK, 0.5f * alpha));
    DrawLineEx({ 0, 250 }, { SCREEN_W, 250 }, 2, ColorAlpha(c, alpha));
    DrawLineEx({ 0, 450 }, { SCREEN_W, 450 }, 2, ColorAlpha(c, alpha));
    BeginBlendMode(BLEND_ADDITIVE);
    DrawGlow({ SCREEN_W / 2 + slide, 340 }, 300, ColorAlpha(c, 0.25f * alpha));
    EndBlendMode();
    DrawTextC(TextFormat("STAGE %d", g.stage), SCREEN_W / 2 + slide, 270, 120, ColorAlpha(c, alpha), 14);
    const char* sub = g.stage == LAST_STAGE ? "FINAL BATTLE" : "GET READY";
    if ((int)(t * 4) % 2 == 0 || t < 1.0f)
        DrawTextC(sub, SCREEN_W / 2, 400, 26, ColorAlpha(WHITE, alpha), 8);
}

// ------------------------------------------------------------
//  하단 HUD
// ------------------------------------------------------------
static void Bar(float x, float y, float w, float h, float ratio, Color c)
{
    if (ratio < 0) ratio = 0;
    if (ratio > 1) ratio = 1;
    DrawRectangle((int)x, (int)y, (int)w, (int)h, { 25, 15, 45, 255 });
    DrawRectangle((int)x, (int)y, (int)(w * ratio), (int)h, c);
    DrawRectangleLinesEx({ x, y, w, h }, 1, ColorAlpha(WHITE, 0.4f));
}

void UI::DrawHUD(const Game& g)
{
    const Player& p = g.player;
    float t = (float)GetTime();
    float top = PLAY_BOTTOM + 6;

    DrawRectangle(0, (int)PLAY_BOTTOM, SCREEN_W, SCREEN_H - (int)PLAY_BOTTOM, { 6, 3, 18, 235 });
    DrawLineEx({ 0, PLAY_BOTTOM }, { SCREEN_W, PLAY_BOTTOM }, 2, ColorAlpha(NEON_CYAN, 0.7f));

    Color feverColor = Rainbow(t * 2);
    // --- 좌측: FPS / 스테이지 / 킬 ---
    DrawTextL(TextFormat("FPS %d", g.fps), 20, top + 6, 18, ColorAlpha(WHITE, 0.6f));
    DrawTextL(TextFormat("STAGE %d", g.stage), 20, top + 30, 26, STAGE_COLOR[g.stage]);
    DrawTextL(TextFormat("KILL %d", p.kills), 150, top + 6, 18, ColorAlpha(WHITE, 0.8f));
    if (!g.boss.Present())
        DrawTextL(TextFormat("BOSS %d / %d", g.boss.killCount, g.Rules().killRequirement), 150, top + 36, 18, ColorAlpha(NEON_MAGENTA, 0.9f));
    DrawTextL(g.Rules().name, 290, top + 6, 18, g.difficulty == Difficulty::HARD ? Color{ 255, 50, 80, 255 } : ColorAlpha(NEON_CYAN, 0.8f));

    // --- 중앙: EXP / HP ---
    float cx = 400;
    DrawTextL(TextFormat("LV.%d", p.level), cx, top + 4, 20, WHITE);
    Bar(cx + 70, top + 9, 300, 12, (float)p.exp / p.maxExp, p.fever ? feverColor : NEON_CYAN);
    DrawTextL(TextFormat("%3d%%", p.exp * 100 / (p.maxExp > 0 ? p.maxExp : 1)), cx + 380, top + 5, 18, ColorAlpha(WHITE, 0.8f));
    DrawTextL("HP", cx, top + 36, 20, WHITE);
    for (int i = 0; i < p.maxHp; i++) {     // HP는 칸 단위 게이지
        Color c = i < p.hp ? (p.fever ? feverColor : NEON_YELLOW) : Color{ 40, 30, 60, 255 };
        DrawRectangle((int)(cx + 70 + i * 61), (int)top + 40, 56, 14, c);
    }

    // --- 우측: 무기 / 폭탄 / 목숨 ---
    const char* wname = p.weapon == WeaponType::NORMAL ? "NORMAL" : p.weapon == WeaponType::FLAME ? "FLAME" : "LASER";
    Color wc = p.weapon == WeaponType::NORMAL ? NEON_CYAN : p.weapon == WeaponType::FLAME ? Color{ 255, 110, 30, 255 } : NEON_YELLOW;
    if (p.weaponLevel >= MAX_WEAPON_LEVEL && fmodf(t, 0.4f) < 0.2f) wc = NEON_YELLOW;   // 5레벨 반짝임 (황금)
    const char* lv = p.weaponLevel >= MAX_WEAPON_LEVEL ? "MAX" : TextFormat("Lv.%d", p.weaponLevel);
    DrawTextR(TextFormat("%s  %s", wname, lv), SCREEN_W - 20, top + 4, 22, wc);

    // 폭탄 아이콘 (피버 중엔 무한)
    float x = SCREEN_W - 20;
    if (p.fever) {
        DrawTextR("BOMB  INF", x, top + 38, 18, feverColor);
    } else {
        int shown = p.bombs > 6 ? 6 : p.bombs;
        for (int i = 0; i < shown; i++) DrawSprite(T_BOMB, { x - 12 - i * 26, top + 48 }, 0.8f);
        if (p.bombs > 6) DrawTextR(TextFormat("x%d", p.bombs), x - 6 * 26 - 4, top + 38, 18, WHITE);
        DrawTextR("BOMB", x - (shown > 0 ? shown : 1) * 26 - (p.bombs > 6 ? 40 : 4), top + 38, 18, ColorAlpha(WHITE, 0.7f));
    }
    // 목숨 아이콘 (남은 추가 목숨)
    float lx = 900;
    DrawTextL("LIFE", lx, top + 38, 18, ColorAlpha(WHITE, 0.7f));
    for (int i = 0; i < (p.lives > 0 ? p.lives : 0); i++) DrawSprite(T_PLAYER, { lx + 72 + i * 40, top + 48 }, 0.45f);

    // 피버 남은 시간
    if (p.fever) {
        DrawTextC(TextFormat("FEVER %.1f", p.feverTimer), SCREEN_W / 2, PLAY_TOP + 10, 30, feverColor, 6);
    }
}

// ------------------------------------------------------------
void UI::DrawPause(const Game& g)
{
    float t = g.sceneTime;
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, ColorAlpha(BLACK, 0.55f));
    Rectangle box = { SCREEN_W / 2 - 300, 200, 600, 300 };
    bool bright = (int)(t * 2) % 2 == 0;     // 흰색 / 회색 깜빡임
    NeonBox(box, bright ? WHITE : Color{ 140, 140, 160, 255 });
    DrawTextC("PAUSE", SCREEN_W / 2, 230, 96, bright ? WHITE : Color{ 160, 160, 180, 255 }, 16);
    DrawLineEx({ box.x + 60, 350 }, { box.x + box.width - 60, 350 }, 2, ColorAlpha(NEON_CYAN, 0.6f));
    DrawTextC("ESC : RESUME", SCREEN_W / 2, 370, 24, ColorAlpha(WHITE, Pulse(t, 4) * 0.5f + 0.5f), 4);
    DrawTextC("R : TITLE        E : EXIT", SCREEN_W / 2, 420, 22, ColorAlpha(WHITE, 0.7f), 4);
}

void UI::DrawGameOver(const Game& g)
{
    float t = g.sceneTime;
    Color red = { 255, 40, 70, 255 };

    if (t < 2.0f) {                           // MISSION FAILED 정지 화면
        float a = t < 0.3f ? t / 0.3f : 1.0f;
        DrawRectangle(0, 0, SCREEN_W, SCREEN_H, ColorAlpha(BLACK, 0.6f * a));
        DrawTextC("GAME OVER", SCREEN_W / 2, 250, 120, ColorAlpha(red, a), 14);
        DrawTextC("MISSION FAILED", SCREEN_W / 2, 400, 30, ColorAlpha(WHITE, a), 10);
        return;
    }

    // --- CONTINUE? + 카운트다운 ---
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, ColorAlpha(BLACK, 0.6f));
    if (g.allClear) DrawTextC("ALL STAGES CLEAR", SCREEN_W / 2, 50, 40, NEON_YELLOW, 8);
    DrawTextC(TextFormat("SCORE  KILL %d   LV.%d", g.player.kills, g.player.level), SCREEN_W / 2, 100, 24, ColorAlpha(WHITE, 0.8f), 4);
    DrawTextC(g.allClear ? "PLAY AGAIN?" : "CONTINUE?", SCREEN_W / 2, 150, 64, red, 10);

    int num = (int)g.countdown;
    if (num < 0) num = 0;
    float blink = sinf(g.countdown * 12.0f);  // 번쩍이며 깜빡이는 숫자
    Color nc = blink > 0 ? NEON_YELLOW : Color{ 200, 170, 40, 255 };
    BeginBlendMode(BLEND_ADDITIVE);
    DrawGlow({ SCREEN_W / 2, 360 }, 200, ColorAlpha(NEON_YELLOW, 0.2f + (blink > 0 ? 0.15f : 0)));
    EndBlendMode();
    DrawTextC(TextFormat("%d", num), SCREEN_W / 2, 240, 240, nc, 0);

    DrawTextC("R : RESTART      E : EXIT", SCREEN_W / 2, 540, 28, ColorAlpha(red, Pulse(t, 3) * 0.6f + 0.4f), 6);
    DrawTextC("(any other key speeds up the countdown)", SCREEN_W / 2, 590, 18, ColorAlpha(WHITE, 0.5f), 2);
}

void UI::DrawClear(const Game& g)
{
    float t = g.sceneTime;
    // 밝은 노랑 / 일반 노랑 반짝임
    Color c = sinf(t * 8) > 0 ? NEON_YELLOW : Color{ 200, 170, 40, 255 };
    DrawTextC("MISSION", SCREEN_W / 2, 180, 120, c, 16);
    DrawTextC("COMPLETE", SCREEN_W / 2, 310, 120, c, 16);
    DrawTextC("CONGRATULATIONS!  YOU SAVED THE NEON GALAXY", SCREEN_W / 2, 470, 26, WHITE, 4);
    if (t > 1.5f) DrawTextC("PRESS ANY KEY", SCREEN_W / 2, 560, 22, ColorAlpha(WHITE, Pulse(t, 4)), 6);
}
