#include "Game.h"
#include "Assets.h"
#include "UI.h"

void Game::Init()
{
    background.Init();
    fx.Reset();
    boss.Init(1);
    ChangeScene(Scene::LOGO);
}

void Game::ChangeScene(Scene s)
{
    scene = s;
    sceneTime = 0;
}

void Game::NewGame()
{
    // 모든 객체 완전 초기화 (원본의 R키 재시작 처리)
    player.Reset();
    fx.Reset();
    StartStage(1);
}

void Game::StartStage(int s)
{
    stage = s;
    bullets.Reset();
    items.Reset();
    monsters.Reset();          // 스테이지 연출 동안 몬스터가 미리 나오지 않도록
    boss.Init(stage);
    stageEndDelay = 0;
    ChangeScene(Scene::STAGE_INTRO);
    Assets::Play(S_STAGE_START);
}

// ============================================================
//  메인 프레임
// ============================================================
void Game::Update(float dt)
{
    sceneTime += dt;
    fps = GetFPS();

    // ---------------- Update ----------------
    switch (scene) {
    case Scene::LOGO:
        background.Update(dt, 0.6f);
        fx.Update(dt);
        if (sceneTime < 0.5f) break;         // 이전 화면에서 누르던 키가 바로 먹히지 않도록
        // 난이도 선택: ↑↓로 이동, Enter/Space로 시작
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN)) {
            difficulty = (difficulty == Difficulty::NORMAL) ? Difficulty::HARD : Difficulty::NORMAL;
            Assets::Play(S_TICK);
        }
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            Assets::Play(S_SELECT);
            fx.Flash(WHITE, 0.5f);           // 화이트 아웃 전환
            NewGame();
        }
        break;

    case Scene::STAGE_INTRO:
        background.Update(dt, 2.0f);         // 워프하는 느낌으로 배경 가속
        fx.Update(dt);
        if (sceneTime > 2.5f || (sceneTime > 1.0f && IsKeyPressed(KEY_SPACE))) {
            monsters.Start(Rules());
            ChangeScene(Scene::PLAYING);
        }
        break;

    case Scene::PLAYING:
        UpdatePlaying(dt);
        break;

    case Scene::PAUSED:
        if (IsKeyPressed(KEY_ESCAPE)) { Assets::Play(S_PAUSE); ChangeScene(Scene::PLAYING); }
        else if (IsKeyPressed(KEY_R)) { Assets::Play(S_SELECT); Assets::Stop(S_CHARGE); Assets::Stop(S_GIGA_LASER); Assets::Stop(S_FLAME_ORB); ChangeScene(Scene::LOGO); }
        else if (IsKeyPressed(KEY_E)) quit = true;
        break;

    case Scene::GAME_OVER: {
        background.Update(dt, 0.3f);
        fx.Update(dt);
        if (sceneTime < 2.0f) break;         // MISSION FAILED 화면 2초 유지
        // CONTINUE 카운트다운: 1초에 1씩 감소, 아무 키나 누르면 0.5초 즉시 차감
        countdown -= dt;
        int key = GetKeyPressed();
        if (key == KEY_R) { Assets::Play(S_SELECT); NewGame(); break; }
        if (key == KEY_E) { quit = true; break; }
        if (key != 0) countdown -= 0.5f;
        int num = (int)countdown;
        if (num != lastCountdownNum && countdown >= 0) { Assets::Play(S_TICK); lastCountdownNum = num; }
        if (countdown < 0) ChangeScene(Scene::LOGO);
        break;
    }

    case Scene::CLEAR:
        background.Update(dt, 1.0f);
        fx.Update(dt);
        if (GetRandomValue(0, 8) == 0)       // 축하 불꽃놀이
            fx.Explosion({ RandF(100, SCREEN_W - 100), RandF(80, 500) }, 1.2f, Rainbow(RandF(0, 1)));
        if (sceneTime > 5.0f || (sceneTime > 1.5f && GetKeyPressed() != 0)) {
            allClear = true;
            countdown = 9.99f; lastCountdownNum = 9;
            ChangeScene(Scene::GAME_OVER);
            sceneTime = 2.0f;                // 결과 화면은 바로 CONTINUE부터
        }
        break;
    }

}

// ============================================================
//  그리기 (main의 RenderTexture 안에서 호출됨)
// ============================================================
void Game::Draw() const
{
    Vector2 shake = fx.ShakeOffset();
    Camera2D cam = { { 0, 0 }, { -shake.x, -shake.y }, 0, 1 };   // 화면 흔들림은 카메라 이동으로 구현

    switch (scene) {
    case Scene::LOGO:
        background.Draw();
        fx.Draw();
        UI::DrawLogo(*this);
        break;
    case Scene::STAGE_INTRO:
        background.Draw();
        fx.Draw();
        UI::DrawHUD(*this);
        UI::DrawStageIntro(*this);
        break;
    case Scene::PLAYING:
    case Scene::PAUSED:
        BeginMode2D(cam);
        DrawWorld();
        EndMode2D();
        boss.DrawUI();
        UI::DrawHUD(*this);
        if (scene == Scene::PAUSED) UI::DrawPause(*this);
        break;
    case Scene::GAME_OVER:
        background.Draw();
        fx.Draw();
        UI::DrawGameOver(*this);
        break;
    case Scene::CLEAR:
        background.Draw();
        fx.Draw();
        UI::DrawClear(*this);
        break;
    }
    fx.DrawOverlay();
}

// ============================================================
//  플레이 중 업데이트 (원본 Tick + PhysicsUpdate)
// ============================================================
void Game::UpdatePlaying(float dt)
{
    if (IsKeyPressed(KEY_ESCAPE)) {          // 일시정지
        Assets::Play(S_PAUSE);
        ChangeScene(Scene::PAUSED);
        return;
    }
    if (IsKeyPressed(KEY_SIX)) boss.ForceAppear(*this);   // 테스트 키: 보스 즉시 등장

    background.Update(dt, boss.Present() ? 1.6f : 1.0f);
    UpdateLaserBlocking();
    player.Update(*this, dt);
    bullets.Update(*this, dt);
    if (monsters.enabled) monsters.Update(*this, dt);
    items.Update(*this, dt);
    boss.Update(*this, dt);
    ResolveCollisions();
    UpdateChargeImpacts(dt);
    fx.Update(dt);

    // 게임 오버: 목숨을 다 쓰고 폭발 연출까지 끝났을 때
    if (player.GameOverReady()) {
        allClear = false;
        countdown = 9.99f; lastCountdownNum = 9;
        Assets::Stop(S_CHARGE);
        Assets::Play(S_GAME_OVER);
        ChangeScene(Scene::GAME_OVER);
        return;
    }

    // 보스 격파 -> 0.8초 뒤 다음 스테이지 또는 클리어
    if (boss.defeated) {
        stageEndDelay += dt;
        if (stageEndDelay > 0.8f) {
            boss.defeated = false;
            if (stage < LAST_STAGE) StartStage(stage + 1);
            else {
                Assets::Play(S_CLEAR);
                ChangeScene(Scene::CLEAR);
            }
        }
    }
}

// 차지샷(기가 레이저 / 화염구 / 기가 미사일)에 맞고 있는 적의 피격 위치에 강렬한 이펙트를 계속 뿌림
// 프레임마다 뿌리면 144fps에서 60fps보다 2.4배 많아지므로, 1/60초 간격으로 고정
void Game::UpdateChargeImpacts(float dt)
{
    bool anyMissile = false;
    Bullet* bp = bullets.Data();
    for (int i = 0; i < BULLET_COUNT && !anyMissile; i++)
        anyMissile = bp[i].active && bp[i].type == BulletType::P_MISSILE;
    if (!player.Alive() || !(player.GigaLaserOn() || player.FlameOrbsOn() || anyMissile)) { impactTimer = 0; return; }

    impactTimer += dt;
    const float INTERVAL = 1.0f / 60.0f;
    if (impactTimer < INTERVAL) return;
    impactTimer -= INTERVAL;
    if (impactTimer > INTERVAL) impactTimer = 0;   // 프레임이 크게 밀렸을 때 몰아서 뿜지 않도록

    // dir: 불꽃이 튀는 기준 각도 / hot: 중심색 / cool: 사라질 때 색
    auto Impact = [&](Vector2 p, float power, float dir, Color hot, Color cool) {
        fx.Spawn(p, { 0, 0 }, 0.08f, 34 * power, 14 * power, WHITE, ColorAlpha(hot, 0));             // 중심 섬광
        for (int i = 0; i < 6; i++) {                                                               // 불꽃
            float a = dir + RandF(-1.3f, 1.3f), sp = RandF(250, 750) * power;
            fx.Spawn(p, { cosf(a) * sp, sinf(a) * sp }, RandF(0.15f, 0.35f), 5, 1, hot, ColorAlpha(cool, 0), 4.0f);
        }
        fx.Spawn(Vector2Add(p, { RandF(-8, 8), RandF(-8, 8) }), { RandF(-60, 60), RandF(-60, 60) }, 0.25f,
                 18 * power, 4, ColorAlpha(cool, 0.8f), BLANK, 2.0f);                                 // 잔광
        if (GetRandomValue(0, 9) == 0) fx.Explosion(p, 0.5f * power, hot);                          // 가끔 작은 폭발
    };
    Monster* mp = monsters.Data();
    bool bossOn = boss.Fighting();
    Rectangle hb = bossOn ? boss.Hitbox() : Rectangle{ 0, 0, 0, 0 };

    // 1) 기가 레이저: 노랑·청록 전기 불꽃이 레이저 반대쪽(왼쪽)으로 튐
    if (player.GigaLaserOn()) {
        const Color hot = { 255, 255, 200, 255 }, cool = { 80, 255, 255, 255 };
        Rectangle r = player.GigaLaserRect();
        for (int k = 0; k < MONSTER_COUNT; k++) {
            if (!mp[k].active || mp[k].pos.x > SCREEN_W) continue;
            if (!CheckCollisionCircleRec(mp[k].pos, mp[k].Radius(), r)) continue;
            Impact({ mp[k].pos.x - mp[k].Radius() * 0.6f, mp[k].pos.y + RandF(-6, 6) }, 1.0f, PI, hot, cool);
        }
        if (bossOn) {
            float top = fmaxf(r.y, hb.y), bottom = fminf(r.y + r.height, hb.y + hb.height);
            if (top < bottom && r.x < player.gigaLaserEndX) {
                for (int i = 0; i < 2; i++)   // 보스는 면적이 크니 두 군데
                    Impact({ player.gigaLaserEndX + RandF(-10, 10), RandF(top, bottom) }, 1.5f, PI, hot, cool);
                fx.Shake(5, 0.05f);
            }
        }
    }

    // 2) 화염구: 닿은 지점에서 사방으로 불꽃
    if (player.FlameOrbsOn()) {
        const Color hot = { 255, 220, 120, 255 }, cool = { 255, 40, 0, 255 };
        Vector2 c[4];
        player.FlameOrbCenters(c);
        for (auto& o : c) {
            for (int k = 0; k < MONSTER_COUNT; k++) {
                if (!mp[k].active || mp[k].pos.x > SCREEN_W) continue;
                if (!CircleHit(o, Player::FLAME_ORB_RADIUS, mp[k].pos, mp[k].Radius())) continue;
                Vector2 dir = Vector2Normalize(Vector2Subtract(mp[k].pos, o));
                Vector2 contact = Vector2Subtract(mp[k].pos, Vector2Scale(dir, mp[k].Radius()));
                Impact(contact, 1.1f, atan2f(dir.y, dir.x), hot, cool);
            }
            if (bossOn && CheckCollisionCircleRec(o, Player::FLAME_ORB_RADIUS, hb)) {
                Vector2 contact = { Clamp(o.x, hb.x, hb.x + hb.width), Clamp(o.y, hb.y, hb.y + hb.height) };
                Impact(contact, 1.6f, atan2f(contact.y - o.y, contact.x - o.x), hot, cool);
                fx.Shake(4, 0.05f);
            }
        }
    }

    // 3) 기가 미사일: 미사일 벽이 닿은 지점에서 폭발
    if (anyMissile) {
        const Color hot = { 220, 250, 255, 255 }, cool = { 40, 160, 255, 255 };
        bool hitAny = false;
        for (int i = 0; i < BULLET_COUNT; i++) {
            Bullet& b = bp[i];
            if (!b.active || b.type != BulletType::P_MISSILE) continue;
            float h = b.MissileHalfHeight();
            Rectangle r = { b.pos.x - 20, b.pos.y - h, 50, h * 2 };
            for (int k = 0; k < MONSTER_COUNT; k++) {
                if (!mp[k].active || mp[k].pos.x > SCREEN_W) continue;
                if (!CheckCollisionCircleRec(mp[k].pos, mp[k].Radius(), r)) continue;
                Impact({ mp[k].pos.x - mp[k].Radius() * 0.5f, mp[k].pos.y }, 1.0f, 0, hot, cool);   // 진행 방향(오른쪽)으로 튐
                hitAny = true;
            }
            if (bossOn && CheckCollisionRecs(r, hb)) {
                float top = fmaxf(r.y, hb.y), bottom = fminf(r.y + r.height, hb.y + hb.height);
                Impact({ hb.x + RandF(0, 20), RandF(top, bottom) }, 1.4f, PI, hot, cool);
                hitAny = true;
            }
        }
        if (hitAny && GetRandomValue(0, 7) == 0) Assets::Play(S_EXPLODE_SMALL, 0.5f, 0.2f);
    }
}

// 레이저가 보스에 막히는 지점 계산 (원본: maxDist = 보스 x)
void Game::UpdateLaserBlocking()
{
    player.laserEndX = SCREEN_W;
    player.gigaLaserEndX = SCREEN_W;
    if (!boss.Fighting()) return;
    Rectangle hb = boss.Hitbox();
    Rectangle l = player.LaserRect();
    if (l.y < hb.y + hb.height && l.y + l.height > hb.y && hb.x > player.pos.x)
        player.laserEndX = hb.x + 20;
    Rectangle gl = player.GigaLaserRect();
    if (gl.y < hb.y + hb.height && gl.y + gl.height > hb.y && hb.x > player.pos.x)
        player.gigaLaserEndX = hb.x + hb.width * 0.5f;
}

// ============================================================
//  충돌 처리 (원본의 ClashAndBullet / ClashAndBoss / ClashAndBulletToPlayer)
//  모든 판정을 이 한 함수에 모아서 '누가 누구를 때리는지'를 한눈에 볼 수 있게 함
// ============================================================
void Game::ResolveCollisions()
{
    Bullet*  bp = bullets.Data();
    Monster* mp = monsters.Data();
    bool bossOn = boss.Fighting();
    Rectangle bossBox = bossOn ? boss.Hitbox() : Rectangle{ 0, 0, 0, 0 };
    int wl = player.weaponLevel;
    int pierceDmg = wl <= 2 ? 1 : wl <= 4 ? 2 : 3;   // 화염/레이저 데미지 (lv1~2:1, 3~4:2, 5:3)

    // ---------- 1. 플레이어 탄환 -> 몬스터 / 보스 ----------
    for (int i = 0; i < BULLET_COUNT; i++) {
        Bullet& b = bp[i];
        if (!b.active || !b.IsPlayerSide()) continue;

        if (b.type == BulletType::P_MISSILE) {
            // 기가 미사일: 세로로 넓은 직사각형 판정 + 적탄 상쇄
            float h = b.MissileHalfHeight();
            Rectangle r = { b.pos.x - 20, b.pos.y - h, 50, h * 2 };
            for (int k = 0; k < MONSTER_COUNT; k++)
                if (mp[k].active && CheckCollisionCircleRec(mp[k].pos, mp[k].Radius(), r))
                    monsters.Hit(*this, k, 18, 0.15f);
            if (bossOn && CheckCollisionRecs(r, bossBox)) boss.Damage(*this, 18, 0.10f);
            for (int j = 0; j < BULLET_COUNT; j++)
                if (bp[j].active && !bp[j].IsPlayerSide() && CheckCollisionPointRec(bp[j].pos, r)) {
                    fx.Spawn(bp[j].pos, { 0, 0 }, 0.2f, 10, 0, WHITE, BLANK);
                    bp[j].active = false;
                }
            continue;
        }
        if (b.type == BulletType::P_BOMB) {
            // 폭탄 본체: 적이나 보스에 닿으면 그 자리에서 폭발
            bool touch = bossOn && CheckCollisionCircleRec(b.pos, b.radius, bossBox);
            for (int k = 0; k < MONSTER_COUNT && !touch; k++)
                touch = mp[k].active && mp[k].pos.x < SCREEN_W && CircleHit(b.pos, b.radius, mp[k].pos, mp[k].Radius());
            if (touch) { b.active = false; bullets.ShatterBomb(*this, b.pos); }
            continue;
        }

        // 일반탄 / 화염 / 폭탄 파편
        for (int k = 0; k < MONSTER_COUNT && b.active; k++) {
            if (!mp[k].active || !CircleHit(b.pos, b.radius, mp[k].pos, mp[k].Radius())) continue;
            if (b.type == BulletType::P_NORMAL)        { if (monsters.Hit(*this, k, b.damage, 0)) b.active = false; }
            else if (b.type == BulletType::P_FLAME)    { monsters.Hit(*this, k, b.damage, 0.06f); }   // 관통
            else if (b.type == BulletType::P_SHRAPNEL) { if (monsters.Hit(*this, k, 5, 0)) b.active = false; }
            if (!b.active) fx.Burst(b.pos, 4, 160, 4, 0.2f, b.color, BLANK);
        }
        if (b.active && bossOn && CheckCollisionCircleRec(b.pos, b.radius, bossBox)) {
            // 공격 종류별 보스 무적 간격은 원본 수치 (일반 0.08 / 화염 0.03 / 파편 0.01)
            if (b.type == BulletType::P_NORMAL) {
                boss.Damage(*this, b.damage, 0.0f);   // 일반탄은 맞으면 소멸하므로 무적 간격 없이 적용
                fx.Burst(b.pos, 4, 160, 4, 0.2f, b.color, BLANK);
                b.active = false;
            } else if (b.type == BulletType::P_FLAME) {
                boss.Damage(*this, b.damage, 0.03f);
            } else if (b.type == BulletType::P_SHRAPNEL) {
                boss.Damage(*this, 3, 0.0f);
                b.active = false;
            }
        }
    }

    // ---------- 2. 기체 부착형 공격 ----------
    if (player.Alive()) {
        if (player.LaserOn()) {                      // 일반 레이저 (관통, 보스에서 막힘)
            Rectangle r = player.LaserRect();
            for (int k = 0; k < MONSTER_COUNT; k++)
                if (mp[k].active && CheckCollisionCircleRec(mp[k].pos, mp[k].Radius(), r))
                    monsters.Hit(*this, k, pierceDmg, 0.1f);
            if (bossOn && CheckCollisionRecs(r, bossBox)) boss.Damage(*this, pierceDmg, 0.18f);
        }
        if (player.GigaLaserOn()) {                  // 기가 레이저: 적탄 상쇄
            Rectangle r = player.GigaLaserRect();
            for (int k = 0; k < MONSTER_COUNT; k++)
                if (mp[k].active && CheckCollisionCircleRec(mp[k].pos, mp[k].Radius(), r))
                    monsters.Hit(*this, k, 10, 0.2f);
            if (bossOn && CheckCollisionRecs(r, bossBox)) boss.Damage(*this, 10, 0.2f);
            for (int j = 0; j < BULLET_COUNT; j++)
                if (bp[j].active && !bp[j].IsPlayerSide() && CheckCollisionPointRec(bp[j].pos, r)) bp[j].active = false;
        }
        if (player.FlameOrbsOn()) {                  // 화염구 4개: 적탄 상쇄
            Vector2 c[4];
            player.FlameOrbCenters(c);
            for (auto& o : c) {
                for (int k = 0; k < MONSTER_COUNT; k++)
                    if (mp[k].active && CircleHit(o, Player::FLAME_ORB_RADIUS, mp[k].pos, mp[k].Radius()))
                        monsters.Hit(*this, k, 10, 0.2f);
                if (bossOn && CheckCollisionCircleRec(o, Player::FLAME_ORB_RADIUS, bossBox)) boss.Damage(*this, 10, 0.2f);
                for (int j = 0; j < BULLET_COUNT; j++)
                    if (bp[j].active && !bp[j].IsPlayerSide() && CircleHit(o, Player::FLAME_ORB_RADIUS, bp[j].pos, bp[j].radius))
                        bp[j].active = false;
            }
        }
    }

    // ---------- 3. 적 탄환 -> 플레이어 ----------
    // (원본은 '왼쪽으로 날아가는' 적탄만 판정해서, 보스 소용돌이탄 중 오른쪽으로 가는 탄은 맞지 않았음 -> 수정)
    if (player.Alive() && !player.Invincible()) {
        for (int i = 0; i < BULLET_COUNT; i++) {
            Bullet& b = bp[i];
            if (!b.active || b.IsPlayerSide()) continue;
            if (CircleHit(b.pos, b.radius * 0.8f, player.pos, Player::HIT_RADIUS)) {
                b.active = false;
                fx.Burst(b.pos, 10, 200, 6, 0.3f, b.color, BLANK);
                player.Damage(*this, 1);
                break;   // 한 프레임에 한 발만
            }
        }
    }
}

// ============================================================
void Game::DrawWorld() const
{
    background.Draw();
    boss.Draw();
    monsters.Draw();
    items.Draw();
    bullets.Draw();
    player.Draw(*this);
    fx.Draw();
}
