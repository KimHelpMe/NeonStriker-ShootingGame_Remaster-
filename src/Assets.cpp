#include "Assets.h"

static Texture2D g_tex[T_COUNT];
static Font      g_font;

// 같은 효과음이 연달아 겹쳐 재생될 수 있도록 효과음마다 '별칭(alias)'을 여러 개 둔다.
// (raylib의 Sound 하나는 동시에 한 번만 재생됨 -> 연사 소리가 끊기는 문제 방지)
constexpr int ALIAS = 6;
static Sound g_snd[S_COUNT][ALIAS];
static int   g_next[S_COUNT];
static bool  g_audioOk = false;   // 오디오 장치가 없으면(스피커 미연결 등) 소리 없이 진행

static const char* TEX_FILES[T_COUNT] = {
    "player.png", "enemy_normal.png", "enemy_medium.png", "enemy_heavy.png",
    "boss1.png", "boss2.png", "boss3.png", "boss4.png",
    "item_normal.png", "item_flame.png", "item_laser.png", "item_bomb.png", "item_fever.png",
    "bullet_player.png", "bullet_enemy.png", "missile.png", "bomb.png", "glow.png", "spark.png", "beam.png",
    "background.png", "logo.png",
};

static const char* SFX_FILES[S_COUNT] = {
    "shot_normal.wav", "shot_flame.wav", "shot_laser.wav",
    "charge.wav", "charge_ready.wav", "charge_shot.wav",
    "bomb_throw.wav", "bomb_explode.wav",
    "hit.wav", "explode_small.wav", "explode_mid.wav", "boss_explode.wav",
    "player_hit.wav", "player_die.wav",
    "item.wav", "powerup.wav", "levelup.wav", "fever.wav",
    "warning.wav", "select.wav", "pause.wav", "tick.wav", "stage_start.wav", "game_over.wav", "clear.wav",
};

void Assets::Load()
{
    for (int i = 0; i < T_COUNT; i++) {
        g_tex[i] = LoadTexture(TextFormat("assets/images/%s", TEX_FILES[i]));
        SetTextureFilter(g_tex[i], TEXTURE_FILTER_BILINEAR);
    }
    g_font = LoadFontEx("assets/fonts/Poppins-Bold.ttf", 72, nullptr, 0);
    SetTextureFilter(g_font.texture, TEXTURE_FILTER_BILINEAR);

    g_audioOk = IsAudioDeviceReady();
    if (!g_audioOk) { TraceLog(LOG_WARNING, "AUDIO: device not ready - sound disabled"); return; }
    for (int i = 0; i < S_COUNT; i++) {
        g_snd[i][0] = LoadSound(TextFormat("assets/sounds/%s", SFX_FILES[i]));
        for (int a = 1; a < ALIAS; a++) g_snd[i][a] = LoadSoundAlias(g_snd[i][0]);
        g_next[i] = 0;
    }
}

void Assets::Unload()
{
    for (int i = 0; i < S_COUNT && g_audioOk; i++) {
        for (int a = 1; a < ALIAS; a++) UnloadSoundAlias(g_snd[i][a]);
        UnloadSound(g_snd[i][0]);   // 별칭을 먼저 해제하고, 원본 Sound는 마지막에
    }
    for (int i = 0; i < T_COUNT; i++) UnloadTexture(g_tex[i]);
    UnloadFont(g_font);
}

const Texture2D& Assets::Tex(TexId id) { return g_tex[id]; }
const Font& Assets::UIFont() { return g_font; }

void Assets::Play(SfxId id, float volume, float pitchVar)
{
    if (!g_audioOk) return;
    Sound& s = g_snd[id][g_next[id]];
    g_next[id] = (g_next[id] + 1) % ALIAS;    // 라운드 로빈으로 다음 별칭 사용
    SetSoundVolume(s, volume);
    SetSoundPitch(s, pitchVar > 0 ? RandF(1.0f - pitchVar, 1.0f + pitchVar) : 1.0f);
    PlaySound(s);
}

void Assets::Stop(SfxId id)
{
    if (!g_audioOk) return;
    for (int a = 0; a < ALIAS; a++) StopSound(g_snd[id][a]);
}

bool Assets::IsPlaying(SfxId id)
{
    if (!g_audioOk) return false;
    for (int a = 0; a < ALIAS; a++) if (IsSoundPlaying(g_snd[id][a])) return true;
    return false;
}

// ------------------------------------------------------------
void DrawSprite(TexId id, Vector2 c, float scale, float rotDeg, Color tint)
{
    const Texture2D& t = g_tex[id];
    Rectangle src = { 0, 0, (float)t.width, (float)t.height };
    Rectangle dst = { c.x, c.y, t.width * scale, t.height * scale };
    DrawTexturePro(t, src, dst, { dst.width / 2, dst.height / 2 }, rotDeg, tint);
}

void DrawGlow(Vector2 c, float radius, Color color)
{
    const Texture2D& t = g_tex[T_GLOW];
    Rectangle src = { 0, 0, (float)t.width, (float)t.height };
    Rectangle dst = { c.x, c.y, radius * 2, radius * 2 };
    DrawTexturePro(t, src, dst, { radius, radius }, 0, color);
}

void DrawTextC(const char* text, float cx, float y, float size, Color color, float spacing)
{
    Vector2 m = MeasureTextEx(g_font, text, size, spacing);
    DrawTextEx(g_font, text, { cx - m.x / 2, y }, size, spacing, color);
}

void DrawTextL(const char* text, float x, float y, float size, Color color, float spacing)
{
    DrawTextEx(g_font, text, { x, y }, size, spacing, color);
}

void DrawTextR(const char* text, float rx, float y, float size, Color color, float spacing)
{
    Vector2 m = MeasureTextEx(g_font, text, size, spacing);
    DrawTextEx(g_font, text, { rx - m.x, y }, size, spacing, color);
}
