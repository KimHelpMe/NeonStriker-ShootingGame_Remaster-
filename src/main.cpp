// ============================================================
//  NEON STRIKER - 진입점
//  - 게임은 항상 1280x720 '내부 해상도'로 그린 뒤, 창 크기에 맞춰 확대해서 출력한다.
//    (원본이 모니터 해상도에 맞춰 콘솔 폰트 크기를 계산하던 것과 같은 목적)
//  - F11: 전체화면 전환
// ============================================================
#include "Game.h"
#include "Assets.h"
#include "rlgl.h"

int main()
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_W, SCREEN_H, "NEON STRIKER");
    SetWindowMinSize(640, 360);

    // 실행 파일이 있는 폴더를 기준으로 assets를 찾도록 (바탕화면 바로가기 등에서 실행해도 동작)
    ChangeDirectory(GetApplicationDirectory());

    // 시작 창 크기: 모니터의 80% 안에서 16:9 유지
    int mon = GetCurrentMonitor();
    int mw = GetMonitorWidth(mon), mh = GetMonitorHeight(mon);
    if (mw > 0 && mh > 0) {
        float s = fminf(mw * 0.8f / SCREEN_W, mh * 0.8f / SCREEN_H);
        if (s > 1.0f) {
            SetWindowSize((int)(SCREEN_W * s), (int)(SCREEN_H * s));
            SetWindowPosition((mw - (int)(SCREEN_W * s)) / 2, (mh - (int)(SCREEN_H * s)) / 2);
        }
    }

    InitAudioDevice();
    SetExitKey(KEY_NULL);         // raylib 기본값은 ESC = 종료. ESC는 일시정지로 쓰므로 해제
    SetTargetFPS(144);

    Assets::Load();
    RenderTexture2D target = LoadRenderTexture(SCREEN_W, SCREEN_H);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

    Game game;
    game.Init();

    while (!WindowShouldClose() && !game.quit) {
        // dt 상한: 창을 드래그하거나 멈췄다 돌아왔을 때 몬스터가 순간이동하지 않도록
        float dt = GetFrameTime();
        if (dt > 1.0f / 20.0f) dt = 1.0f / 20.0f;

        if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();

        game.Update(dt);

        // 1) 내부 해상도 텍스처에 게임 그리기
        BeginTextureMode(target);
        ClearBackground(BLACK);
        game.Draw();
        EndTextureMode();

        // 2) 창 크기에 맞춰 레터박스로 확대 출력
        BeginDrawing();
        ClearBackground(BLACK);
        float scale = fminf((float)GetScreenWidth() / SCREEN_W, (float)GetScreenHeight() / SCREEN_H);
        Rectangle src = { 0, 0, (float)SCREEN_W, -(float)SCREEN_H };   // RenderTexture는 위아래가 뒤집혀 있음
        Rectangle dst = { (GetScreenWidth() - SCREEN_W * scale) / 2, (GetScreenHeight() - SCREEN_H * scale) / 2,
                          SCREEN_W * scale, SCREEN_H * scale };
        // 블렌딩을 끄고(ONE, ZERO) 그대로 복사한다.
        // 반투명 효과를 그리면서 텍스처의 알파값이 1보다 작아진 곳이 있으면,
        // 일반 블렌딩으로 복사할 때 검은 창 배경이 비쳐 얼룩(세로 띠)이 생기기 때문.
        rlSetBlendFactors(RL_ONE, RL_ZERO, RL_FUNC_ADD);
        BeginBlendMode(BLEND_CUSTOM);
        DrawTexturePro(target.texture, src, dst, { 0, 0 }, 0, WHITE);
        EndBlendMode();
        EndDrawing();
    }

    UnloadRenderTexture(target);
    Assets::Unload();
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
