#include "Boss.h"
#include "Game.h"
#include "Assets.h"
#include <initializer_list>

static const char*  BOSS_NAME[5]  = { "", "VIOLET CRUISER", "BIO HYDRA", "IRON BASTION", "CRIMSON CORE" };
static const Color  BOSS_COLOR[5] = { WHITE, { 190, 90, 255, 255 }, { 60, 255, 140, 255 }, { 255, 150, 40, 255 }, { 255, 40, 80, 255 } };
static const TexId  BOSS_TEX[5]   = { T_BOSS1, T_BOSS1, T_BOSS2, T_BOSS3, T_BOSS4 };
static const float  EXPLODE_TIME  = 3.0f;

void Boss::Init(int stage)
{
    state = BossState::NONE;
    defeated = false;
    killCount = 0;
    step = stage < 1 ? 1 : stage > 4 ? 4 : stage;
    warningTimer = explodeTimer = moveAcc = fireTimer = 0;
    invincibleTimer = flash = 0;
    burstCount = 0;
}

void Boss::AddKillCount(Game& g)
{
    if (Present()) return;
    if (++killCount >= g.Rules().killRequirement) ForceAppear(g);
}

void Boss::ForceAppear(Game& g)
{
    if (Present()) return;
    state = BossState::WARNING;
    warningTimer = 3.0f;
    // 보스 등장: 일반 몬스터와 탄환 정리
    g.monsters.enabled = false;
    g.monsters.Reset();
    g.bullets.ClearEnemyBullets(g, true);

    // ★보스 체력★ 1~3단계: 300 x 단계 / 4단계: 1500 (원본과 동일) x 난이도 배율
    int baseHp = (step == 4) ? 1500 : 300 * step;
    maxHp = hp = (int)(baseHp * g.Rules().bossHpScale);
    pos = { SCREEN_W + 200.0f, (PLAY_TOP + PLAY_BOTTOM) / 2 };
    fireTimer = 0;
    Assets::Play(S_WARNING);
}

Rectangle Boss::Hitbox() const
{
    const Texture2D& t = Assets::Tex(BOSS_TEX[step]);
    float w = t.width * 0.8f, h = t.height * 0.75f;   // 이미지 가장자리 글로우 영역은 제외
    return { pos.x - w / 2, pos.y - h / 2, w, h };
}

bool Boss::Damage(Game& g, int damage, float invincible)
{
    if (state != BossState::FIGHT || invincibleTimer > 0) return false;
    hp -= damage;
    flash = 0.06f;
    invincibleTimer = invincible;     // 공격 종류별 연속 피격 간격 (원본 invincibleTimer 값)
    if (GetRandomValue(0, 3) == 0) Assets::Play(S_HIT, 0.4f, 0.1f);

    if (hp <= 0) {                    // 격파 -> 폭발 연출 시작
        hp = 0;
        state = BossState::EXPLODING;
        explodeTimer = EXPLODE_TIME;
        g.bullets.ClearEnemyBullets(g, true);
        Assets::Play(S_BOSS_EXPLODE);
        g.fx.Shake(10, EXPLODE_TIME);
    }
    return true;
}

// ------------------------------------------------------------
void Boss::Update(Game& g, float dt)
{
    time += dt;
    if (invincibleTimer > 0) invincibleTimer -= dt;
    if (flash > 0) flash -= dt;

    // ========== [폭발 연출] ==========
    if (state == BossState::EXPLODING) {
        explodeTimer -= dt;
        // 화면 중앙 쪽으로 서서히 끌려가며 몸 전체에서 불꽃
        pos.x = Lerp(pos.x, SCREEN_W * 0.6f, dt * 0.5f);
        Rectangle hb = Hitbox();
        for (int i = 0; i < 4; i++) {
            Vector2 p = { RandF(hb.x, hb.x + hb.width), RandF(hb.y, hb.y + hb.height) };
            g.fx.Spawn(p, { RandF(-60, 60), RandF(-180, -60) }, RandF(0.4f, 0.9f), RandF(10, 26), 2,
                       { 255, 255, 220, 255 }, { 255, 40, 0, 0 });
        }
        if (GetRandomValue(0, 6) == 0) {
            g.fx.Explosion({ RandF(hb.x, hb.x + hb.width), RandF(hb.y, hb.y + hb.height) }, 1.2f, BOSS_COLOR[step]);
            Assets::Play(S_EXPLODE_MID, 0.6f, 0.2f);
        }
        if (explodeTimer <= 0.6f && explodeTimer + dt > 0.6f) {     // 마지막 0.6초: 강력한 백색 폭발
            g.fx.Explosion(pos, 5.0f, WHITE);
            g.fx.Explosion(pos, 3.0f, BOSS_COLOR[step]);
            g.fx.Flash(WHITE, 0.8f);
            g.fx.Shake(24, 0.8f);
        }
        if (explodeTimer <= 0) {
            state = BossState::NONE;
            defeated = true;
        }
        return;
    }

    // ========== [등장 경고 + 진입] ==========
    float baseX = SCREEN_W * 0.7f;
    float baseY = (PLAY_TOP + PLAY_BOTTOM) / 2;
    if (state == BossState::WARNING) {
        warningTimer -= dt;
        pos.x = Lerp(pos.x, baseX, dt * 1.2f);    // 오른쪽에서 전투 위치(화면 70%)로 서서히 진입
        pos.y = baseY;
        if (warningTimer <= 0) { state = BossState::FIGHT; moveAcc = 0; }
        return;
    }
    if (state != BossState::FIGHT) return;

    // ========== [전투 이동 패턴] ==========
    moveAcc += dt;
    if (step == 4) {
        // 4단계: 거대한 8자 + 원형 복합 궤적
        float tri = moveAcc * 1.5f;
        pos.x = baseX - (110 + sinf(tri) * 160);
        pos.y = baseY + cosf(tri * 0.5f) * 150;
    } else {
        // 1~3단계: 단계가 오를수록 빨라지고 넓어지는 8자 패턴
        float speedX = 1.2f + step * 0.2f;
        float speedY = 0.8f + step * 0.1f;
        float rangeX = 140 + step * 25.0f;
        pos.x = baseX - (sinf(moveAcc * speedX) + 1.0f) * rangeX * 0.5f + 60;
        pos.y = baseY + cosf(moveAcc * speedY) * 130;
    }
    if (step >= 2) {                               // 2단계 이상: 과부하 진동
        pos.x += RandF(-2, 2);
        pos.y += RandF(-2, 2);
    }

    // ========== [공격] ==========
    fireTimer += dt;
    float interval = 1.5f / step;                  // ★난이도★ 1단계 1.5초 -> 4단계 0.375초
    if (fireTimer > interval) { Fire(g); fireTimer = 0; }

    // 몸통 충돌
    if (g.player.Alive() && CheckCollisionCircleRec(g.player.pos, Player::HIT_RADIUS, Hitbox()))
        g.player.Damage(g, 1);
}

void Boss::SpiralPattern(Game& g, Vector2 p)
{
    spiralAngle += 0.45f;
    const int streams = 6;
    for (int i = 0; i < streams; i++) {
        float a = spiralAngle + i * (2 * PI / streams);
        g.bullets.Spawn(BulletType::E_BOSS, p, { cosf(a) * 220, sinf(a) * 220 }, 1, { 255, 80, 200, 255 }, 10);
    }
}

void Boss::Fire(Game& g)
{
    Vector2 fp = { pos.x - 40, pos.y };    // 기체 앞쪽 중앙에서 발사
    Vector2 to = Vector2Subtract(g.player.pos, fp);
    float d = Vector2Length(to);
    to = d > 0 ? Vector2Scale(to, 1.0f / d) : Vector2{ -1, 0 };

    const float speed = 260.0f;
    Color c = BOSS_COLOR[step];
    // 원본의 방향벡터 { -1, ay } 를 정규화해서 쏨
    auto Dir = [&](float ay, float spd) {
        Vector2 v = Vector2Normalize({ -1.0f, ay });
        g.bullets.Spawn(BulletType::E_BOSS, fp, Vector2Scale(v, spd), 1, c, 10);
    };
    auto Aim = [&](Vector2 v, float spd) {
        g.bullets.Spawn(BulletType::E_BOSS, fp, Vector2Scale(Vector2Normalize(v), spd), 1, { 255, 255, 255, 255 }, 10);
    };

    if (step == 1) {
        for (float ay = -0.8f; ay <= 0.81f; ay += 0.4f) Dir(ay, speed * 0.9f);   // 5방향 확산탄
        Aim(to, speed * 1.15f);                                                   // 조준탄
        if (GetRandomValue(0, 99) < 10) Dir(RandF(-0.5f, 0.5f), speed);           // 10% 무작위탄
    } else if (step == 2) {
        float wave = sinf(moveAcc * 5.0f) * 0.5f;                                  // 상하 웨이브 탄막
        for (float ay = -0.8f; ay <= 0.81f; ay += 0.4f) Dir(ay + wave, speed);
        for (int i = 0; i < 2; i++) Dir(RandF(-1.0f, 1.0f), speed * 0.9f);        // 무작위 산탄
    } else if (step == 3) {
        burstCount++;
        for (float ay = -1.2f; ay <= 1.21f; ay += 0.3f) Dir(ay, speed);           // 넓은 수직 탄막
        if (burstCount % 2 == 0) Aim(to, speed * 1.3f);                           // 2회마다 조준 강화탄
    } else {
        SpiralPattern(g, pos);                                                    // 소용돌이
        float flicker = cosf(moveAcc * 10.0f) * 0.3f;
        for (float ay = -1.5f; ay <= 1.51f; ay += 0.3f) Dir(ay + flicker, speed); // 상하 깜빡이 탄막
        for (float off : { -0.2f, 0.0f, 0.2f }) Aim({ to.x, to.y + off }, speed * 1.5f);  // 3갈래 정밀 저격
    }
    g.fx.Spawn(fp, { 0, 0 }, 0.15f, 40, 10, c, BLANK);
}

// ------------------------------------------------------------
void Boss::Draw() const
{
    if (state == BossState::NONE) return;
    Color c = BOSS_COLOR[step];
    Color tint = flash > 0 ? Color{ 255, 255, 255, 255 } : WHITE;
    if (state == BossState::EXPLODING) {
        float k = explodeTimer / EXPLODE_TIME;
        if (explodeTimer < 0.6f) return;       // 마지막 백색 폭발 때 본체 소멸
        tint = ColorLerp({ 255, 120, 80, 255 }, WHITE, k);
    }
    BeginBlendMode(BLEND_ADDITIVE);
    DrawGlow(pos, 160 + sinf(time * 3) * 10, ColorAlpha(c, 0.35f));
    if (flash > 0) DrawGlow(pos, 140, { 255, 255, 255, 160 });
    EndBlendMode();
    DrawSprite(BOSS_TEX[step], pos, 1.0f, 0, tint);
}

void Boss::DrawUI() const
{
    Color c = BOSS_COLOR[step];
    if (state == BossState::WARNING) {
        // ===== 등장 경고 연출 =====
        bool on = (int)(warningTimer * 6) % 2 == 0;
        Color red = { 255, 30, 60, 255 };
        DrawRectangle(0, 0, SCREEN_W, SCREEN_H, ColorAlpha(red, on ? 0.12f : 0.04f));
        for (int i = 0; i < 2; i++) {                 // 위/아래 경고 띠
            float y = i == 0 ? 210.0f : 470.0f;
            DrawRectangle(0, (int)y, SCREEN_W, 40, ColorAlpha(BLACK, 0.6f));
            float off = fmodf((float)GetTime() * 200, 80);
            for (float x = -80 + (i ? -off : off); x < SCREEN_W; x += 80)
                DrawTriangle({ x, y + 40 }, { x + 40, y }, { x + 20, y + 40 }, ColorAlpha(red, 0.8f));
        }
        DrawTextC("WARNING", SCREEN_W / 2, 270, 110, on ? red : WHITE, 12);
        DrawTextC(TextFormat("BOSS APPROACHING  -  %s", BOSS_NAME[step]), SCREEN_W / 2, 400, 28, WHITE, 4);
        return;
    }
    if (state == BossState::FIGHT || state == BossState::EXPLODING) {
        // ===== 상단 HP 바 =====
        float w = 700, x = (SCREEN_W - w) / 2, y = 14;
        DrawTextL(BOSS_NAME[step], x, y - 4, 22, c);
        float bx = x + 210, bw = w - 210;
        DrawRectangle((int)bx, (int)y + 2, (int)bw, 16, { 30, 10, 40, 220 });
        DrawRectangle((int)bx, (int)y + 2, (int)(bw * hp / maxHp), 16, c);
        DrawRectangleLinesEx({ bx - 2, y, bw + 4, 20 }, 2, ColorAlpha(WHITE, 0.6f));
        DrawTextR(TextFormat("%d / %d", hp, maxHp), x + w, y + 22, 16, ColorAlpha(WHITE, 0.8f));
    }
}
