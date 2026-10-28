#pragma once
// ============================================================
//  화면 UI (원본 MainGame의 Logo / ShowStageStart / ShowPause / GameOver / GameClear
//           + Player의 하단 HUD 부분)
//  UI는 상태를 '읽기만' 하고 바꾸지 않는다 -> 모두 const Game&
// ============================================================
class Game;

namespace UI {
    void DrawLogo(const Game& g);
    void DrawStageIntro(const Game& g);
    void DrawHUD(const Game& g);
    void DrawPause(const Game& g);
    void DrawGameOver(const Game& g);
    void DrawClear(const Game& g);
}
