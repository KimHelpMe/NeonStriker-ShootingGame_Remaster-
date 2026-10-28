#pragma once
// ============================================================
//  게임 전체 관리 (원본 MainGame 역할)
//
//  [원본과의 구조 차이]
//  원본은 로고/게임오버/퍼즈 같은 화면을 함수 안의 while 루프로 만들었다.
//  (그 동안 메인 루프가 멈추고, 끝나면 dt가 크게 튀어서 FPS::FPSUpdate()로 보정했음)
//  여기서는 '씬 상태(Scene)'를 두고 메인 루프 한 개가 현재 상태만 업데이트/그린다.
//  -> 화면 전환 중에도 배경·효과·사운드가 계속 자연스럽게 돌아간다.
// ============================================================
#include "Config.h"
#include "Background.h"
#include "Effects.h"
#include "Bullet.h"
#include "Player.h"
#include "Monster.h"
#include "Item.h"
#include "Boss.h"

enum class Scene { LOGO, STAGE_INTRO, PLAYING, PAUSED, GAME_OVER, CLEAR };

class Game {
public:
    void Init();
    void Update(float dt);    // 현재 씬 업데이트
    void Draw() const;        // 현재 씬 그리기
    void DrawWorld() const;   // 배경 + 게임 오브젝트
    bool quit = false;

    // --- 월드 객체 (각 시스템이 서로를 참조할 수 있도록 public) ---
    Background    background;
    Effects       fx;
    BulletSystem  bullets;
    Player        player;
    MonsterSystem monsters;
    ItemSystem    items;
    Boss          boss;

    // --- UI에서 읽는 상태 ---
    Scene scene = Scene::LOGO;
    float sceneTime = 0;      // 현재 씬에 들어온 뒤 경과 시간
    int   stage = 1;
    float countdown = 9.99f;  // CONTINUE 카운트다운
    bool  allClear = false;   // 4스테이지까지 클리어 후 CONTINUE 화면인지
    int   fps = 0;
    Difficulty difficulty = Difficulty::NORMAL;   // 타이틀에서 선택 (재시작해도 유지)
    const DifficultyRules& Rules() const { return DIFFICULTY_RULES[(int)difficulty]; }

private:
    void ChangeScene(Scene s);
    void NewGame();
    void StartStage(int s);

    void UpdatePlaying(float dt);
    void ResolveCollisions();
    void UpdateLaserBlocking();

    float stageEndDelay = 0;  // 보스 격파 후 다음 스테이지까지 잠깐 대기
    int   lastCountdownNum = 9;
};
