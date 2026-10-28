# NEON STRIKER

콘솔 슈팅 게임(`2026.03.30. ShootingGame`)의 기능을 전부 옮겨서, **raylib 그래픽 / 효과음 / 네온 테마**로 새로 만든 가로 스크롤 슈팅 게임.

## 바로 실행

`Play\NeonStriker.exe` 실행. 같은 폴더에 있는 `assets` 폴더도 꼭 함께 있어야 한다.
- GitHub 저장소에는 `Play` 폴더(exe)를 넣지 않았다. 저장소에서 받았다면 아래 **직접 빌드**대로 빌드하면 `bin\Release\`에 exe와 assets가 함께 생긴다.
- 처음 실행하면 Windows SmartScreen 경고가 뜰 수 있다(서명 없는 exe). 그럴 땐 "추가 정보 → 실행"을 누르면 된다.
- 창 크기는 자유롭게 바꿀 수 있고, F11을 누르면 전체화면으로 바뀐다.

## 난이도

타이틀에서 ↑↓로 고르고 Enter(또는 Space)로 시작한다. 고른 난이도는 재시작해도 유지된다.

| | NORMAL | HARD |
|---|---|---|
| 동시 출현 몬스터 | 20 (NORMAL 18 / MEDIUM 1 / HEAVY 1) | 30 (24 / 4 / 2) |
| 보스 체력 | 기본 (300·600·900·1500) | 2배 |
| 보스 등장 킬수 | 200 | 300 |

값은 `src/Config.h`의 `DIFFICULTY_RULES` 표 한 곳에서 바꿀 수 있다.

## 조작

| 키 | 기능 |
|---|---|
| 방향키 | 이동 |
| Space | 발사 (레이저는 누르는 동안 유지) |
| X (무기 5레벨) | 3초 차지 → 떼면 차지샷 |
| C | 폭탄 |
| ESC | 일시정지 (ESC 재개 / R 타이틀 / E 종료) |
| 테스트 키 | 1 무기 레벨업 · 2/3/4 NORMAL/FLAME/LASER · 5 피버 · 6 보스 즉시 등장 |

## 폴더 구조

```
NeonStriker/
├─ Play/                 바로 실행용 (exe + assets) ※ 저장소에는 미포함
├─ NeonStriker.sln       Visual Studio 솔루션
├─ NeonStriker.vcxproj   raylib 소스를 함께 컴파일하도록 설정되어 있음 (별도 설치 불필요)
├─ src/                  게임 소스
├─ assets/               images / sounds / fonts
├─ resources/            exe 아이콘
├─ raylib/               raylib 5.5 소스 (zlib 라이선스)
└─ tools/                에셋 생성 스크립트 (Python)
```

## 원본 → 새 프로젝트 대응표

| 원본 | 새 파일 | 비고 |
|---|---|---|
| ShootingGame.cpp (main 루프) | main.cpp | 1280x720 내부 해상도로 그린 뒤 창 크기에 맞춰 확대 |
| MainGame | Game + UI | 화면 전환을 while 루프 대신 씬 상태(Scene)로 처리 |
| DoubleBuffering | (raylib가 처리) | RenderTexture → 화면 |
| FPS | GetFrameTime() | dt 상한 1/20초 (순간이동 방지) |
| Background | Background | 3레이어 패럴랙스 + 성운 이미지 |
| Player | Player | 무기 3종 x 5레벨, 차지샷 3종, 폭탄, 피버, EXP, 목숨 |
| Bullet | Bullet | 탄 종류를 **enum으로** 구분 |
| Monster | Monster | 3종 (90/8/2%), 드랍률 동일 |
| Item | Item | 5종 가중치 드랍, 벽 튕김, 15초 수명 |
| Boss | Boss | 난이도별 킬수(200/300, 원본 100)를 채우면 등장, 4단계 패턴, 8자 이동, 폭발 연출 |
| Collision | Game::ResolveCollisions | 모든 충돌 판정을 한 곳에 모음 |
| (없음) | Assets, Effects | 리소스 관리 / 파티클·화면흔들림·섬광 |

## 원본에서 의도적으로 바꾼 것

1. **탄 종류를 속도값(33.33f, 88.88f, 1000.5f …) 대신 `BulletType` enum으로 구분**
   - 원본은 속도를 바꾸면 판정 코드가 깨졌는데, 이제는 속도를 자유롭게 바꿀 수 있다.
2. **레이저 / 기가 레이저 / 화염구를 탄환이 아니라 Player 상태로 관리**
   - 원본은 탄환을 매 프레임 기체 위치로 되돌리고 지우는 방식이었다.
3. **보스 소용돌이탄 판정 버그 수정**
   - 원본은 왼쪽으로 날아가는 적탄(`Direction.x < 0`)만 플레이어와 충돌 판정을 했다. 그래서 오른쪽으로 도는 탄은 맞아도 피해가 없었다.
4. **피격 후 1초 무적 추가**
   - 원본은 몬스터와 몸이 닿아 있는 동안 매 프레임 HP가 깎였다.
5. **오디오 장치가 없으면 소리 없이 실행**
   - 스피커가 연결되지 않은 PC에서도 크래시가 나지 않는다.

수치(HP, 데미지, 드랍률, 보스 체력, 발사 간격 등)는 NORMAL 기준으로 원본 값을 유지하고(보스 등장 킬수만 200으로 변경), 칸(콘솔 글자) 단위만 픽셀 단위로 환산했다. 주요 값은 `src/Config.h`에 모여 있다.

## 직접 빌드

- **Visual Studio:** `NeonStriker.sln`을 열고 x64 / Release로 빌드한다. 빌드가 끝나면 `assets`가 `bin\Release\`로 자동 복사된다. 툴셋은 원본과 같은 v145로 맞췄다. 다른 VS 버전에서 열면 "솔루션 대상 변경"으로 바꾸면 된다.
- **MinGW (명령줄):** VS 프로젝트와 같은 방식으로, raylib 소스 8개를 함께 컴파일한다.
  ```
  gcc -c -O2 -DPLATFORM_DESKTOP -DGRAPHICS_API_OPENGL_33 -Iraylib/src -Iraylib/src/external/glfw/include raylib/src/{rcore,rshapes,rtextures,rtext,rmodels,raudio,utils,rglfw}.c
  g++ -std=c++17 -O2 -Iraylib/src src/*.cpp *.o -lopengl32 -lgdi32 -lwinmm -static -mwindows -o NeonStriker.exe
  ```
- raylib 폴더에는 Windows 빌드에 필요한 파일만 남겨 두었다(macOS·Linux 전용 glfw 파일은 제거).

## 에셋 다시 만들기

이미지와 효과음은 외부 소스 없이 코드로 생성했다.
```
python tools/gen_images.py assets/images   (Pillow 필요)
python tools/gen_sounds.py assets/sounds   (numpy 필요)
```

## 라이선스

- **raylib:** zlib/libpng 라이선스 (`raylib/LICENSE`)
- **Poppins 폰트:** SIL Open Font License 1.1 (`assets/fonts/OFL.txt`)
- **이미지 / 효과음:** 이 프로젝트에서 직접 생성
