#include "Monster.h"
#include "Game.h"
#include "Assets.h"

static const Color MON_COLOR[3] = { { 255, 60, 220, 255 }, { 255, 160, 30, 255 }, { 255, 50, 60, 255 } };

void MonsterSystem::Init(int i)
{
    Monster& o = m[i];
    o.active = true;
    o.pos = { SCREEN_W + RandF(20, 200), RandF(PLAY_TOP + 30, PLAY_BOTTOM - 30) };
    o.dir = { -1.0f, (float)GetRandomValue(-1, 1) };
    o.hitCooldown = 0;
    o.flash = 0;

    // 인덱스 기준으로 타입 배정 (원본 방식). 개수는 난이도 규칙에서 받아옴
    o.type = i < normalCount ? MonsterType::NORMAL
           : i < normalCount + mediumCount ? MonsterType::MEDIUM : MonsterType::HEAVY;

    switch (o.type) {   // 속도 / 체력 / 첫 발사 시간
    case MonsterType::NORMAL: o.speed = RandF(60, 160); o.hp = 3;  o.fireTimer = RandF(6.0f, 9.0f); break;
    case MonsterType::MEDIUM: o.speed = RandF(45, 90);  o.hp = 15; o.fireTimer = RandF(5.0f, 7.0f); break;
    case MonsterType::HEAVY:  o.speed = RandF(25, 50);  o.hp = 30; o.fireTimer = RandF(4.0f, 6.0f); break;
    }
    o.maxHp = o.hp;
}

void MonsterSystem::Start(const DifficultyRules& rules)
{
    enabled = true;
    activeCount = rules.monsterCount;
    normalCount = rules.normalCount;
    mediumCount = rules.mediumCount;
    for (int i = 0; i < MONSTER_COUNT; i++) m[i].active = false;
    for (int i = 0; i < activeCount; i++) Init(i);   // 쓰지 않는 슬롯은 비활성으로 남김
}

void MonsterSystem::Reset()
{
    for (auto& o : m) o.active = false;
}

bool MonsterSystem::Hit(Game& g, int idx, int damage, float cooldown)
{
    Monster& o = m[idx];
    if (!o.active || o.pos.x > SCREEN_W) return false;   // 화면 안에 있을 때만 피격
    if (cooldown > 0) {
        if (o.hitCooldown > 0) return false;
        o.hitCooldown = cooldown;
    }
    o.hp -= damage;
    o.flash = 0.08f;
    if (o.hp <= 0) { Kill(g, idx, false); return true; }
    if (GetRandomValue(0, 2) == 0) Assets::Play(S_HIT, 0.35f, 0.15f);
    return true;
}

void MonsterSystem::Kill(Game& g, int i, bool rammed)
{
    Monster& o = m[i];
    int t = (int)o.type;

    // 드랍률: 총알로 처치 3/15/30%, 몸통 박치기로 처치 10/30/50% (원본과 동일)
    int chance = rammed ? (t == 0 ? 10 : t == 1 ? 30 : 50) : (t == 0 ? 3 : t == 1 ? 15 : 30);
    if (GetRandomValue(0, 99) < chance) g.items.Drop(o.pos);

    g.fx.Explosion(o.pos, t == 0 ? 0.8f : t == 1 ? 1.3f : 2.0f, MON_COLOR[t]);
    if (t > 0) g.fx.Shake(t * 4.0f, 0.2f);
    Assets::Play(t == 0 ? S_EXPLODE_SMALL : S_EXPLODE_MID, 0.7f, 0.1f);

    if (!rammed) {                  // 총알로 잡았을 때만 경험치/킬 카운트
        g.player.AddExp(g);
        g.player.kills++;
        g.boss.AddKillCount(g);
    }
    if (enabled) Init(i); else o.active = false;   // 보스 등장 중이면 재생성하지 않음
}

void MonsterSystem::Update(Game& g, float dt)
{
    Vector2 pp = g.player.pos;
    for (int i = 0; i < MONSTER_COUNT; i++) {
        Monster& o = m[i];
        if (!o.active) continue;
        if (o.hitCooldown > 0) o.hitCooldown -= dt;
        if (o.flash > 0) o.flash -= dt;

        // 이동 (세로 속도는 가로의 절반)
        o.pos.x += o.dir.x * o.speed * dt;
        o.pos.y += o.dir.y * o.speed * 0.5f * dt;

        // 상/하단 경계에서 반사
        if (o.pos.y >= PLAY_BOTTOM - o.Radius()) { o.pos.y = PLAY_BOTTOM - o.Radius(); if (o.dir.y > 0) o.dir.y = -o.dir.y; }
        if (o.pos.y <= PLAY_TOP + o.Radius())    { o.pos.y = PLAY_TOP + o.Radius();    if (o.dir.y < 0) o.dir.y = -o.dir.y; }

        // ---- 발사 (화면 안에 들어왔고 플레이어가 살아 있을 때) ----
        o.fireTimer -= dt;
        if (o.fireTimer <= 0 && o.pos.x < SCREEN_W - 20 && g.player.Alive()) {
            Vector2 to = Vector2Normalize(Vector2Subtract(pp, o.pos));
            float base = atan2f(to.y, to.x);
            const float bs = 230.0f;   // 몬스터 탄 속도
            Color c = MON_COLOR[(int)o.type];
            auto Shot = [&](float a, float spd) {
                g.bullets.Spawn(BulletType::E_SMALL, o.pos, { cosf(a) * spd, sinf(a) * spd }, 1, c, 10);
            };
            switch (o.type) {
            case MonsterType::NORMAL:   // 조준 1발 + 좌우 2방향 (가운데는 비워 회피 공간 확보)
                Shot(base, bs);
                Shot(base - 0.2f, bs); Shot(base + 0.2f, bs);
                o.fireTimer = RandF(2.5f, 5.0f);
                break;
            case MonsterType::MEDIUM:   // 70% 확률로 조준탄 + 확산탄 3발
                if (GetRandomValue(0, 99) < 70) {
                    Shot(base, bs * 1.2f);
                    for (int k = -1; k <= 1; k++) Shot(PI + k * 0.3f, bs);
                }
                o.fireTimer = RandF(1.5f, 3.5f);
                break;
            case MonsterType::HEAVY:    // 조준탄 + 확산탄 4발
                Shot(base, bs * 1.3f);
                for (int k = 0; k < 4; k++) Shot(PI - 0.45f + k * 0.3f, bs);
                o.fireTimer = RandF(1.0f, 2.5f);
                break;
            }
        }

        // 화면 왼쪽 밖으로 나가면 재배치
        if (o.pos.x < -60) Init(i);
    }

    // ---- 플레이어 몸통 충돌 ----
    if (g.player.Alive()) {
        for (int i = 0; i < MONSTER_COUNT; i++) {
            Monster& o = m[i];
            if (!o.active || !CircleHit(o.pos, o.Radius(), pp, Player::HIT_RADIUS + 6)) continue;
            bool wasInvincible = g.player.Invincible();
            g.player.Damage(g, 1);
            if (!wasInvincible && --o.hp <= 0) Kill(g, i, true);   // 부딪힌 몬스터도 1 피해
        }
    }
}

void MonsterSystem::Draw() const
{
    for (const auto& o : m) {
        if (!o.active || o.pos.x > SCREEN_W + 60) continue;
        TexId tex = o.type == MonsterType::NORMAL ? T_EN_NORMAL : o.type == MonsterType::MEDIUM ? T_EN_MEDIUM : T_EN_HEAVY;
        float bob = sinf(GetTime() * 4 + o.pos.y) * 2;
        DrawSprite(tex, { o.pos.x, o.pos.y + bob }, 1.0f, 0, o.flash > 0 ? Color{ 255, 255, 255, 255 } : WHITE);
        if (o.flash > 0) {
            BeginBlendMode(BLEND_ADDITIVE);
            DrawGlow(o.pos, o.Radius() * 1.6f, { 255, 255, 255, 180 });
            EndBlendMode();
        }
        // 중/대형은 HP 바 표시
        if (o.type != MonsterType::NORMAL && o.hp < o.maxHp) {
            float w = o.Radius() * 2;
            DrawRectangle((int)(o.pos.x - w / 2), (int)(o.pos.y - o.Radius() - 10), (int)w, 4, { 60, 20, 30, 200 });
            DrawRectangle((int)(o.pos.x - w / 2), (int)(o.pos.y - o.Radius() - 10), (int)(w * o.hp / o.maxHp), 4, MON_COLOR[(int)o.type]);
        }
    }
}
