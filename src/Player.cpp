#include "Player.h"
#include "Game.h"
#include "Assets.h"

static const float SHOT_DELAY = 0.12f;  // NORMAL 차지 미사일 연사 간격
static const Color C_NORMAL = { 0, 230, 255, 255 };
static const Color C_FLAME  = { 255, 120, 30, 255 };
static const Color C_LASER  = { 255, 240, 60, 255 };

static Color WeaponColor(WeaponType w)
{
    return w == WeaponType::NORMAL ? C_NORMAL : w == WeaponType::FLAME ? C_FLAME : C_LASER;
}

void Player::Reset()
{
    pos = { 160, (PLAY_TOP + PLAY_BOTTOM) / 2 };
    hp = maxHp = 5;
    level = 1; exp = 0; maxExp = level * 10;
    bombs = 3; lives = 2; kills = 0;
    weapon = WeaponType::NORMAL; weaponLevel = 1;
    fever = false; feverTimer = 0;
    charging = false; chargeTimer = 0; chargeReadyPlayed = false;
    dead = false; respawnTimer = 0;
    invincibleTimer = 2.0f;    // 시작하자마자 죽지 않도록
    hurtTimer = 0; fireTimer = 0; laserOn = false;
    missileQueue = 0; gigaLaserTimer = 0; flameOrbTimer = 0;
}

// ------------------------------------------------------------
void Player::Update(Game& g, float dt)
{
    time += dt;
    laserOn = false;

    // ---- 사망 연출 중 ----
    if (dead) {
        respawnTimer -= dt;
        if (respawnTimer <= 0 && lives >= 0) {      // 목숨이 남아 있으면 부활
            dead = false;
            hp = maxHp;
            pos = { 160, (PLAY_TOP + PLAY_BOTTOM) / 2 };
            invincibleTimer = 2.0f;
        }
        return;                                     // 사망 중엔 이동/공격 없음
    }

    if (invincibleTimer > 0) invincibleTimer -= dt;
    if (hurtTimer > 0) hurtTimer -= dt;

    // ---- 피버 타임 ----
    if (fever) {
        feverTimer -= dt;
        if (feverTimer <= 0) { fever = false; feverTimer = 0; }
        if (GetRandomValue(0, 2) == 0)   // 무지개 잔상
            g.fx.Spawn({ pos.x - 30, pos.y + RandF(-8, 8) }, { -200, 0 }, 0.4f, 10, 0, Rainbow(time * 2), BLANK);
    }

    // ===================== TEST KEY (원본과 동일) =====================
    if (IsKeyPressed(KEY_ONE)   && weaponLevel < MAX_WEAPON_LEVEL) { weaponLevel++; Assets::Play(S_POWERUP); }
    if (IsKeyPressed(KEY_TWO))   { weapon = WeaponType::NORMAL; Assets::Play(S_SELECT); }
    if (IsKeyPressed(KEY_THREE)) { weapon = WeaponType::FLAME;  Assets::Play(S_SELECT); }
    if (IsKeyPressed(KEY_FOUR))  { weapon = WeaponType::LASER;  Assets::Play(S_SELECT); }
    if (IsKeyPressed(KEY_FIVE))  { ApplyItem(g, ItemType::FEVER); }
    // =================================================================

    // ---- 1. 이동 (DeltaTime 기반) ----
    float moveSpeed = 360.0f;
    if (charging) moveSpeed *= 0.4f;               // 차징 중 이동속도 감소
    Vector2 dir = { 0, 0 };
    if (IsKeyDown(KEY_LEFT))  dir.x -= 1;
    if (IsKeyDown(KEY_RIGHT)) dir.x += 1;
    if (IsKeyDown(KEY_UP))    dir.y -= 1;
    if (IsKeyDown(KEY_DOWN))  dir.y += 1;
    if (dir.x != 0 && dir.y != 0) dir = Vector2Scale(dir, 0.7071f);  // 대각선이 더 빠르지 않도록 정규화
    pos.x = Clamp(pos.x + dir.x * moveSpeed * dt, 30, SCREEN_W - 40);
    pos.y = Clamp(pos.y + dir.y * moveSpeed * dt, PLAY_TOP + 20, PLAY_BOTTOM - 20);

    // 엔진 불꽃
    if (GetRandomValue(0, 1) == 0)
        g.fx.Spawn({ pos.x - 34, pos.y + RandF(-3, 3) }, { RandF(-260, -160), RandF(-15, 15) }, 0.18f, 7, 0,
                   { 255, 120, 230, 220 }, BLANK);

    // ---- 2. 공격 ----
    Fire(g, dt);

    // ---- 3. 폭탄 (C) : 누른 순간 한 번만 ----
    if (IsKeyPressed(KEY_C)) {
        if (fever || bombs > 0) {
            if (!fever) bombs--;                    // 피버 중엔 개수 차감 없음
            g.bullets.ClearEnemyBullets(g, true);   // 1차 적탄 제거
            g.bullets.Spawn(BulletType::P_BOMB, pos, { 520, 0 }, 0, WHITE, 0.55f);
            Assets::Play(S_BOMB_THROW);
        }
    }

    // ---- 4. 차지 (무기 5레벨 전용, X 키) ----
    if (weaponLevel >= MAX_WEAPON_LEVEL) {
        if (IsKeyDown(KEY_X)) {
            if (!charging) Assets::Play(S_CHARGE, 0.8f);
            charging = true;
            chargeTimer += dt;
            if (chargeTimer >= MAX_CHARGE_TIME) {
                chargeTimer = MAX_CHARGE_TIME;
                if (!chargeReadyPlayed) { Assets::Play(S_CHARGE_READY); chargeReadyPlayed = true; }
            }
            // 기 모으기: 바깥에서 기체로 빨려 들어오는 입자
            float ratio = chargeTimer / MAX_CHARGE_TIME;
            for (int i = 0; i < 1 + (int)(ratio * 3); i++) {
                float a = RandF(0, 2 * PI), r = RandF(70, 120) * (1.2f - ratio * 0.5f);
                Vector2 from = { pos.x + cosf(a) * r, pos.y + sinf(a) * r };
                Vector2 v = Vector2Scale(Vector2Subtract(pos, from), 3.5f);
                g.fx.Spawn(from, v, 0.28f, 5, 2, WeaponColor(weapon), WHITE);
            }
        } else if (charging) {                      // 키를 뗐을 때
            Assets::Stop(S_CHARGE);
            if (chargeTimer >= MAX_CHARGE_TIME) ReleaseCharge(g);
            charging = false;
            chargeTimer = 0;
            chargeReadyPlayed = false;
        }
    } else if (charging) {                          // 레벨이 떨어지면 차지 취소
        Assets::Stop(S_CHARGE);
        charging = false; chargeTimer = 0; chargeReadyPlayed = false;
    }

    // ---- 5. 차지 공격 진행 ----
    if (missileQueue > 0) {                         // NORMAL 차지: 기가 미사일 10연사
        missileTimer += dt;
        if (missileTimer >= SHOT_DELAY) {
            g.bullets.Spawn(BulletType::P_MISSILE, { pos.x + 30, pos.y }, { 620, 0 }, 18, C_NORMAL, 4.0f);
            Assets::Play(S_SHOT_NORMAL, 0.9f, 0.05f);
            missileQueue--;
            missileTimer = 0;
        }
    }
    if (gigaLaserTimer > 0) {
        gigaLaserTimer -= dt;
        g.fx.Shake(3, 0.05f);
    }
    if (flameOrbTimer > 0) {
        flameOrbTimer -= dt;
        orbAngle += dt * 3.0f;
        Vector2 c[4];
        FlameOrbCenters(c);
        for (auto& p : c)
            g.fx.Spawn(Vector2Add(p, { RandF(-20, 20), RandF(-20, 20) }), { RandF(-30, 30), RandF(-60, -20) },
                       0.35f, 16, 2, { 255, 200, 80, 200 }, { 255, 30, 0, 0 });
    }
}

// ------------------------------------------------------------
void Player::Fire(Game& g, float dt)
{
    fireTimer += dt;
    flameOrbit += dt * 3.0f;
    if (!IsKeyDown(KEY_SPACE)) return;

    switch (weapon) {
    case WeaponType::NORMAL:
        if (fireTimer >= 0.15f) { FireNormal(g); fireTimer = 0; }
        break;
    case WeaponType::FLAME:
        if (fireTimer >= 0.03f) { FireFlame(g); fireTimer = 0; }
        break;
    case WeaponType::LASER:
        laserOn = true;                         // 키를 누르고 있는 동안만 유지
        laserSoundTimer -= dt;
        if (laserSoundTimer <= 0) { Assets::Play(S_SHOT_LASER, 0.6f, 0.03f); laserSoundTimer = 0.1f; }
        if (GetRandomValue(0, 1) == 0)
            g.fx.Spawn({ laserEndX, pos.y + RandF(-6, 6) }, { RandF(-150, -40), RandF(-120, 120) }, 0.2f, 6, 0,
                       C_LASER, BLANK);
        break;
    }
}

void Player::FireNormal(Game& g)
{
    const float spd = 900.0f;
    Vector2 p = { pos.x + 30, pos.y };
    int dmg = weaponLevel;                      // 레벨당 데미지 1씩 증가 (원본과 동일)
    auto Shot = [&](float oy, float angle) {
        g.bullets.Spawn(BulletType::P_NORMAL, { p.x, p.y + oy }, { cosf(angle) * spd, sinf(angle) * spd }, dmg, C_NORMAL, 3.0f);
    };

    switch (weaponLevel) {
    case 1: Shot(0, 0); break;                                         // 정면 1발
    case 2: Shot(-10, 0); Shot(0, 0); Shot(10, 0); break;              // 정면 3발
    case 3: Shot(-10, 0); Shot(0, 0); Shot(10, 0);                     // 정면 3발 + 대각 2발
            Shot(0, -0.25f); Shot(0, 0.25f); break;
    case 4: Shot(-10, 0); Shot(0, 0); Shot(10, 0);                     // 정면 3발 + 부채꼴 5방향
            for (int i = 0; i < 5; i++) Shot(0, -0.4f + i * 0.2f);
            break;
    default:                                                           // 정면 기준 8방향 부채꼴 (-70도 ~ +70도)
            for (int i = 0; i < 8; i++) Shot(0, -1.2f + i / 7.0f * 2.4f);
            Shot(-10, 0); Shot(10, 0); break;
    }
    g.fx.Spawn(p, { 0, 0 }, 0.08f, 18, 6, C_NORMAL, BLANK);           // 총구 섬광
    Assets::Play(S_SHOT_NORMAL, 0.5f, 0.08f);
}

void Player::FireFlame(Game& g)
{
    // 레벨이 오를수록 사거리(수명)가 길어짐
    float life = 0.28f + weaponLevel * 0.06f;
    int dmg = weaponLevel <= 2 ? 1 : weaponLevel <= 4 ? 2 : 3;
    Vector2 p = { pos.x + 34, pos.y };
    auto Puff = [&](Vector2 from, float angle, float spd) {
        g.bullets.Spawn(BulletType::P_FLAME, from, { cosf(angle) * spd, sinf(angle) * spd }, dmg, C_FLAME, life);
    };

    if (weaponLevel <= 3) {
        float spread = weaponLevel == 1 ? 0.04f : weaponLevel == 2 ? 0.09f : 0.14f;  // 레벨별 퍼짐 각
        for (int i = 0; i < 2; i++) Puff(p, RandF(-spread, spread), RandF(520, 640));
    } else if (weaponLevel == 4) {
        const float centers[] = { -0.25f, 0.0f, 0.25f };                // 3갈래 화염
        for (float c : centers) Puff(p, c + RandF(-0.05f, 0.05f), RandF(520, 640));
    } else {
        // 5레벨: 기체 주변 12갈래 회전 화염 (전방위)
        for (int i = 0; i < 12; i++) {
            float a = flameOrbit + i * (2 * PI / 12);
            Vector2 from = { pos.x + cosf(a) * 26, pos.y + sinf(a) * 26 };
            Puff(from, a, 420);
        }
        Puff(p, RandF(-0.05f, 0.05f), 640);
    }
    static float snd = 0;
    snd -= 0.03f;
    if (snd <= 0) { Assets::Play(S_SHOT_FLAME, 0.5f, 0.1f); snd = 0.09f; }
}

void Player::ReleaseCharge(Game& g)
{
    switch (weapon) {
    case WeaponType::NORMAL: missileQueue = 10; missileTimer = SHOT_DELAY; break; // 첫 발은 즉시
    case WeaponType::FLAME:  flameOrbTimer = 4.0f; break;
    case WeaponType::LASER:  gigaLaserTimer = 5.0f; break;
    }
    Assets::Play(S_CHARGE_SHOT);
    g.fx.Flash(ColorAlpha(WeaponColor(weapon), 0.5f), 0.3f);
    g.fx.Shake(10, 0.4f);
    g.fx.PopupText("CHARGE SHOT!", { pos.x, pos.y - 70 }, WeaponColor(weapon), 30);
}

// ------------------------------------------------------------
Rectangle Player::LaserRect() const
{
    float half = 4.0f + weaponLevel * 4.0f;          // 레벨마다 굵어짐
    float x0 = pos.x + 34;
    return { x0, pos.y - half, laserEndX - x0, half * 2 };
}

Rectangle Player::GigaLaserRect() const
{
    float x0 = pos.x + 30;
    return { x0, pos.y - 180, gigaLaserEndX - x0, 360 };   // 원본: 위아래 17칸 (35줄)
}

void Player::FlameOrbCenters(Vector2 out[4]) const
{
    for (int i = 0; i < 4; i++) {                    // 90도 간격 4개, 가로로 조금 긴 궤도
        float a = orbAngle + i * (PI / 2);
        out[i] = { pos.x + cosf(a) * 150, pos.y + sinf(a) * 110 };
    }
}

// ------------------------------------------------------------
void Player::Damage(Game& g, int damage)
{
#ifdef NEON_GODMODE   // 자동 테스트용 무적 빌드 (-DNEON_GODMODE). 일반 빌드에는 영향 없음
    return;
#endif
    if (dead || Invincible()) return;
    hp -= damage;
    g.fx.Shake(6, 0.2f);
    g.fx.Flash({ 255, 0, 60, 90 }, 0.2f);
    if (hp <= 0) { Die(g); return; }
    hurtTimer = 1.0f;
    Assets::Play(S_PLAYER_HIT);
}

void Player::Die(Game& g)
{
    hp = 0;
    dead = true;
    respawnTimer = 2.0f;           // 2초간 폭발 연출
    lives--;

    // 사망 시 무기 및 폭탄 초기화 (원본과 동일)
    weaponLevel = 1;
    weapon = WeaponType::NORMAL;
    bombs = 3;
    charging = false; chargeTimer = 0; missileQueue = 0; gigaLaserTimer = 0; flameOrbTimer = 0;
    Assets::Stop(S_CHARGE);

    g.fx.Explosion(pos, 2.5f, C_NORMAL);
    g.fx.Explosion(pos, 1.5f, { 255, 120, 230, 255 });
    g.fx.Shake(16, 0.8f);
    Assets::Play(S_PLAYER_DIE);
}

void Player::AddExp(Game& g)
{
    if (++exp >= maxExp) {
        level++;
        hp = maxHp;                 // 레벨업 시 HP 회복 (원본과 동일)
        exp = 0;
        maxExp = level * 10;
        g.fx.PopupText("LEVEL UP!", { pos.x, pos.y - 50 }, { 120, 255, 255, 255 });
        g.fx.Burst(pos, 30, 300, 8, 0.6f, { 120, 255, 255, 255 }, BLANK);
        Assets::Play(S_LEVELUP);
    }
}

void Player::ApplyItem(Game& g, ItemType type)
{
    if (type == ItemType::FEVER) {
        fever = true;
        feverTimer = FEVER_TIME;
        g.fx.PopupText("FEVER TIME!", { pos.x, pos.y - 60 }, Rainbow(time), 34);
        g.fx.Flash({ 255, 255, 255, 90 }, 0.25f);
        Assets::Play(S_FEVER);
    } else if (type == ItemType::BOMB) {
        bombs++;
        g.fx.PopupText("+BOMB", { pos.x, pos.y - 50 }, { 140, 255, 140, 255 });
        Assets::Play(S_ITEM);
    } else {
        WeaponType w = (WeaponType)type;   // 아이템 enum의 앞 3개가 무기 순서와 같음
        if (weapon == w) {
            if (weaponLevel < MAX_WEAPON_LEVEL) weaponLevel++;
            g.fx.PopupText(weaponLevel >= MAX_WEAPON_LEVEL ? "MAX POWER!" : "POWER UP", { pos.x, pos.y - 50 }, WeaponColor(w));
        } else {
            weapon = w;
            weaponLevel = 1;
            g.fx.PopupText(w == WeaponType::NORMAL ? "NORMAL" : w == WeaponType::FLAME ? "FLAME" : "LASER",
                           { pos.x, pos.y - 50 }, WeaponColor(w));
        }
        Assets::Play(S_POWERUP);
    }
}

// ------------------------------------------------------------
void Player::Draw(const Game&) const
{
    if (dead) return;   // 사망 폭발은 Effects가 그림

    // 부활 무적: 깜빡임
    if (invincibleTimer > 0 && fmodf(invincibleTimer, 0.2f) < 0.1f) return;

    Color tint = WHITE;
    if (fever) tint = Rainbow(time * 2, 0.5f);
    if (hurtTimer > 0 && fmodf(hurtTimer, 0.16f) < 0.08f) tint = { 255, 80, 80, 255 };

    // --- 기체에 붙은 공격: 레이저 / 기가 레이저 / 화염구 ---
    BeginBlendMode(BLEND_ADDITIVE);
    const Texture2D& beam = Assets::Tex(T_BEAM);
    Rectangle src = { 0, 0, (float)beam.width, (float)beam.height };
    if (laserOn) {
        Rectangle r = LaserRect();
        float flick = 1.0f + sinf(time * 60) * 0.15f;
        Rectangle outer = { r.x, r.y - r.height * 0.6f * flick, r.width, r.height * 2.2f * flick };
        DrawTexturePro(beam, src, outer, { 0, 0 }, 0, ColorAlpha(C_LASER, 0.6f));
        DrawTexturePro(beam, src, r, { 0, 0 }, 0, WHITE);
        DrawGlow({ r.x, pos.y }, r.height * 1.5f, C_LASER);
        DrawGlow({ r.x + r.width, pos.y }, r.height * 1.8f, C_LASER);
    }
    if (GigaLaserOn()) {
        Rectangle r = GigaLaserRect();
        float wob = sinf(time * 40) * 10;   // 불안정하게 떨리는 연출
        DrawTexturePro(beam, src, { r.x, r.y - 30 + wob, r.width, r.height + 60 }, { 0, 0 }, 0, { 255, 40, 40, 180 });
        DrawTexturePro(beam, src, { r.x, r.y + 40, r.width, r.height - 80 }, { 0, 0 }, 0, { 255, 220, 60, 220 });
        DrawTexturePro(beam, src, { r.x, r.y + 120 - wob * 0.5f, r.width, r.height - 240 }, { 0, 0 }, 0, { 80, 255, 255, 255 });
        DrawTexturePro(beam, src, { r.x, pos.y - 18, r.width, 36 }, { 0, 0 }, 0, WHITE);
        DrawGlow({ r.x, pos.y }, 140, { 255, 255, 200, 255 });
    }
    if (FlameOrbsOn()) {
        Vector2 c[4];
        FlameOrbCenters(c);
        for (auto& p : c) {
            DrawGlow(p, FLAME_ORB_RADIUS * 1.7f, { 255, 60, 20, 200 });
            DrawGlow(p, FLAME_ORB_RADIUS * 1.1f, { 255, 200, 60, 255 });
            DrawGlow(p, FLAME_ORB_RADIUS * 0.5f, WHITE);
        }
    }

    // --- 차징 상태 표시 ---
    if (charging) {
        float ratio = chargeTimer / MAX_CHARGE_TIME;
        if (ratio >= 1.0f) {
            bool on = fmodf(time, 0.12f) < 0.06f;   // 풀 차징: 강렬한 번쩍임
            DrawGlow(pos, 70, on ? WHITE : WeaponColor(weapon));
            tint = on ? WHITE : WeaponColor(weapon);
        } else {
            DrawGlow(pos, 30 + ratio * 40, ColorAlpha(WeaponColor(weapon), 0.5f + ratio * 0.5f));
        }
    }
    DrawGlow(pos, 40, ColorAlpha(fever ? Rainbow(time * 2) : C_NORMAL, 0.35f));
    EndBlendMode();

    DrawSprite(T_PLAYER, pos, 1.0f, 0, tint);

    // 차지 게이지 (기체 아래)
    if (charging) {
        float ratio = chargeTimer / MAX_CHARGE_TIME;
        DrawRectangle((int)pos.x - 30, (int)pos.y + 30, 60, 5, { 40, 40, 60, 200 });
        DrawRectangle((int)pos.x - 30, (int)pos.y + 30, (int)(60 * ratio), 5, ratio >= 1 ? WHITE : WeaponColor(weapon));
    }
}
