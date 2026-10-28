# NEON STRIKER - 이미지 에셋 생성기
# 외부 이미지 없이 코드로 네온 스타일 스프라이트를 그린다.
# 방식: 4배 크기로 그린 뒤(슈퍼샘플링) 축소 -> 블러로 만든 글로우를 아래에 합성
import math, random, os, sys
from PIL import Image, ImageDraw, ImageFilter, ImageChops, ImageFont

OUT = sys.argv[1] if len(sys.argv) > 1 else "assets/images"
os.makedirs(OUT, exist_ok=True)
SS = 4  # supersample

def neon(w, h, draw_fn, glow=6, glow_strength=1.6):
    """draw_fn(d, s) : s=스케일. 선명한 레이어를 그리고 그 아래에 글로우를 깐다."""
    big = Image.new("RGBA", (w * SS, h * SS), (0, 0, 0, 0))
    d = ImageDraw.Draw(big)
    draw_fn(d, SS)
    sharp = big.resize((w, h), Image.LANCZOS)
    g = sharp.filter(ImageFilter.GaussianBlur(glow))
    # 글로우 알파 증폭
    r, gg, b, a = g.split()
    a = a.point(lambda v: min(255, int(v * glow_strength)))
    g = Image.merge("RGBA", (r, gg, b, a))
    out = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    out = Image.alpha_composite(out, g)
    out = Image.alpha_composite(out, sharp)
    return out

def P(pts, s, ox=0, oy=0):
    return [((x + ox) * s, (y + oy) * s) for x, y in pts]

def save(img, name):
    img.save(os.path.join(OUT, name))

WHITE = (255, 255, 255, 255)

# ---------------- 플레이어 (오른쪽을 향함) ----------------
def player(d, s):
    cy = 30
    body = [(10, cy - 6), (52, cy - 3), (70, cy), (52, cy + 3), (10, cy + 6), (16, cy)]
    wing_t = [(18, cy - 5), (34, cy - 22), (44, cy - 22), (38, cy - 4)]
    wing_b = [(18, cy + 5), (34, cy + 22), (44, cy + 22), (38, cy + 4)]
    for wpts in (wing_t, wing_b):
        d.polygon(P(wpts, s), fill=(0, 60, 90, 255), outline=(0, 230, 255, 255), width=2 * s)
    d.polygon(P(body, s), fill=(10, 30, 60, 255), outline=(120, 250, 255, 255), width=2 * s)
    d.ellipse(P([(40, cy - 3), (52, cy + 3)], s), fill=(255, 255, 255, 255))   # 캐노피
    d.polygon(P([(4, cy - 4), (14, cy - 2), (14, cy + 2), (4, cy + 4)], s), fill=(255, 120, 230, 255))  # 엔진
save(neon(76, 60, player, glow=4), "player.png")

# ---------------- 적 3종 (왼쪽을 향함) ----------------
def enemy_normal(d, s):
    cy = 20
    pts = [(4, cy), (30, cy - 14), (24, cy), (30, cy + 14)]
    d.polygon(P(pts, s), fill=(60, 0, 50, 255), outline=(255, 60, 220, 255), width=2 * s)
    d.polygon(P([(30, cy - 14), (40, cy - 8), (32, cy)], s), outline=(255, 60, 220, 255), width=2 * s)
    d.polygon(P([(30, cy + 14), (40, cy + 8), (32, cy)], s), outline=(255, 60, 220, 255), width=2 * s)
    d.ellipse(P([(14, cy - 3), (20, cy + 3)], s), fill=WHITE)
save(neon(46, 40, enemy_normal, glow=3), "enemy_normal.png")

def enemy_medium(d, s):
    cy = 26
    hexa = [(6, cy), (18, cy - 18), (42, cy - 18), (54, cy), (42, cy + 18), (18, cy + 18)]
    d.polygon(P(hexa, s), fill=(70, 30, 0, 255), outline=(255, 160, 30, 255), width=2 * s)
    d.polygon(P([(18, cy), (30, cy - 9), (42, cy), (30, cy + 9)], s), outline=(255, 220, 120, 255), width=2 * s)
    d.ellipse(P([(25, cy - 4), (35, cy + 4)], s), fill=WHITE)
    d.line(P([(0, cy), (8, cy)], s), fill=(255, 160, 30, 255), width=3 * s)
save(neon(62, 54, enemy_medium, glow=4), "enemy_medium.png")

def enemy_heavy(d, s):
    cy = 38
    d.rounded_rectangle(P([(14, cy - 26), (70, cy + 26)], s), radius=10 * s, fill=(70, 0, 10, 255),
                        outline=(255, 50, 60, 255), width=3 * s)
    for yy in (-16, 16):
        d.rectangle(P([(2, cy + yy - 4), (18, cy + yy + 4)], s), fill=(255, 90, 90, 255))
    d.rectangle(P([(70, cy - 14), (80, cy + 14)], s), outline=(255, 50, 60, 255), width=2 * s)
    d.ellipse(P([(30, cy - 12), (54, cy + 12)], s), outline=(255, 200, 200, 255), width=3 * s)
    d.ellipse(P([(37, cy - 5), (47, cy + 5)], s), fill=WHITE)
save(neon(86, 78, enemy_heavy, glow=5), "enemy_heavy.png")

# ---------------- 보스 4종 ----------------
def boss1(d, s):  # 보라 크루저
    cx, cy = 100, 62
    hull = [(14, cy), (60, cy - 40), (170, cy - 30), (190, cy), (170, cy + 30), (60, cy + 40)]
    d.polygon(P(hull, s), fill=(40, 0, 70, 255), outline=(190, 90, 255, 255), width=3 * s)
    d.polygon(P([(50, cy), (90, cy - 18), (150, cy), (90, cy + 18)], s), outline=(240, 180, 255, 255), width=2 * s)
    d.ellipse(P([(78, cy - 10), (98, cy + 10)], s), fill=WHITE)
    for yy in (-24, 24):
        d.rectangle(P([(130, cy + yy - 4), (196, cy + yy + 4)], s), fill=(120, 40, 200, 255))
save(neon(210, 126, boss1, glow=6), "boss1.png")

def boss2(d, s):  # 녹색 바이오 메카
    cy = 76
    d.ellipse(P([(40, cy - 50), (190, cy + 50)], s), fill=(0, 50, 30, 255), outline=(60, 255, 140, 255), width=3 * s)
    for i in range(6):
        a = -math.pi / 2 + i * math.pi / 5 + math.pi / 10 + math.pi / 2
        x1, y1 = 60 + math.cos(a) * 30, cy + math.sin(a) * 60
        d.line(P([(60, cy), (x1 - 40, y1)], s), fill=(60, 255, 140, 255), width=3 * s)
    d.ellipse(P([(70, cy - 22), (114, cy + 22)], s), outline=(200, 255, 220, 255), width=3 * s)
    d.ellipse(P([(84, cy - 8), (100, cy + 8)], s), fill=WHITE)
    for yy in (-30, 0, 30):
        d.ellipse(P([(140, cy + yy - 7), (154, cy + yy + 7)], s), fill=(120, 255, 180, 255))
save(neon(226, 156, boss2, glow=6), "boss2.png")

def boss3(d, s):  # 주황 요새
    cy = 92
    d.polygon(P([(20, cy), (70, cy - 70), (190, cy - 70), (210, cy), (190, cy + 70), (70, cy + 70)], s),
              fill=(60, 25, 0, 255), outline=(255, 150, 40, 255), width=3 * s)
    for k in range(3):
        y = cy - 44 + k * 44
        d.rectangle(P([(4, y - 6), (60, y + 6)], s), fill=(255, 190, 80, 255))
    d.rectangle(P([(90, cy - 40), (170, cy + 40)], s), outline=(255, 220, 140, 255), width=3 * s)
    d.ellipse(P([(110, cy - 18), (146, cy + 18)], s), fill=WHITE)
save(neon(226, 186, boss3, glow=6), "boss3.png")

def boss4(d, s):  # 최종 보스: 붉은 별 코어
    cx, cy = 150, 104
    pts = []
    for i in range(16):
        r = 96 if i % 2 == 0 else 54
        a = i * math.pi / 8
        pts.append((cx + math.cos(a) * r * 1.3, cy + math.sin(a) * r))
    d.polygon(P(pts, s), fill=(60, 0, 15, 255), outline=(255, 40, 80, 255), width=3 * s)
    d.ellipse(P([(cx - 50, cy - 40), (cx + 50, cy + 40)], s), outline=(255, 200, 220, 255), width=4 * s)
    d.ellipse(P([(cx - 22, cy - 22), (cx + 22, cy + 22)], s), fill=(255, 255, 255, 255))
    d.ellipse(P([(cx - 30, cy - 30), (cx + 30, cy + 30)], s), outline=(255, 80, 120, 255), width=3 * s)
save(neon(300, 210, boss4, glow=8), "boss4.png")

# ---------------- 아이템 (흰 글자 + 색 테두리) ----------------
try:
    FONT = ImageFont.truetype("/usr/share/fonts/truetype/google-fonts/Poppins-Bold.ttf", 20 * SS)
except Exception:
    FONT = ImageFont.load_default()

def item(letter, col):
    def fn(d, s):
        d.rounded_rectangle(P([(4, 4), (36, 36)], s), radius=9 * s, fill=(10, 10, 30, 230), outline=col, width=3 * s)
        bb = d.textbbox((0, 0), letter, font=FONT)
        tw, th = bb[2] - bb[0], bb[3] - bb[1]
        d.text((20 * s - tw / 2 - bb[0], 20 * s - th / 2 - bb[1]), letter, font=FONT, fill=WHITE)
    return neon(40, 40, fn, glow=3)

save(item("N", (0, 230, 255, 255)), "item_normal.png")
save(item("F", (255, 110, 30, 255)), "item_flame.png")
save(item("L", (255, 240, 60, 255)), "item_laser.png")
save(item("B", (120, 255, 120, 255)), "item_bomb.png")
save(item("S", (255, 255, 255, 255)), "item_fever.png")   # 색은 코드에서 무지개로 tint

# ---------------- 탄/이펙트 (흰색 위주, 코드에서 tint) ----------------
def pbullet(d, s):
    d.rounded_rectangle(P([(2, 4), (26, 8)], s), radius=2 * s, fill=WHITE)
save(neon(28, 12, pbullet, glow=2), "bullet_player.png")

def orb(d, s):
    d.ellipse(P([(4, 4), (16, 16)], s), fill=WHITE)
save(neon(20, 20, orb, glow=3, glow_strength=2.2), "bullet_enemy.png")

def missile(d, s):
    d.polygon(P([(2, 6), (30, 6), (40, 10), (30, 14), (2, 14)], s), fill=WHITE)
    d.polygon(P([(2, 2), (10, 6), (10, 14), (2, 18)], s), fill=WHITE)
save(neon(44, 20, missile, glow=3), "missile.png")

def bomb(d, s):
    d.ellipse(P([(4, 4), (24, 24)], s), fill=(30, 60, 30, 255), outline=(140, 255, 140, 255), width=3 * s)
    d.ellipse(P([(11, 11), (17, 17)], s), fill=WHITE)
save(neon(28, 28, bomb, glow=3), "bomb.png")

# 부드러운 원형 글로우 (파티클/화염/폭발용)
def radial(size, power=2.0):
    img = Image.new("RGBA", (size, size))
    px = img.load()
    c = (size - 1) / 2
    for y in range(size):
        for x in range(size):
            r = math.hypot(x - c, y - c) / c
            a = max(0.0, 1.0 - r) ** power
            px[x, y] = (255, 255, 255, int(255 * a))
    return img
save(radial(64, 1.8), "glow.png")
save(radial(16, 1.0), "spark.png")

# 레이저 빔 단면 (세로 그라데이션, 가로로 늘려서 사용)
beam = Image.new("RGBA", (8, 32))
bp = beam.load()
for y in range(32):
    t = abs(y - 15.5) / 15.5
    a = max(0.0, 1 - t) ** 1.5
    core = max(0.0, 1 - t * 3)
    for x in range(8):
        bp[x, y] = (255, 255, 255, int(255 * min(1, a + core)))
save(beam, "beam.png")

# ---------------- 배경 성운 ----------------
random.seed(7)
W, H = 1280, 720
bg = Image.new("RGB", (W, H), (4, 2, 14))
neb = Image.new("RGB", (W, H), (0, 0, 0))
nd = ImageDraw.Draw(neb)
cols = [(70, 0, 110), (0, 40, 110), (110, 0, 70), (0, 80, 100)]
for i in range(26):
    x, y = random.randint(-100, W + 100), random.randint(-50, H + 50)
    r = random.randint(90, 260)
    c = random.choice(cols)
    for ox in (-W, 0, W):   # 좌우로 한 장씩 더 그려서 가로로 이어 붙여도 경계가 안 보이게(타일링)
        nd.ellipse([x + ox - r, y - r * 0.6, x + ox + r, y + r * 0.6], fill=c)
# 블러가 이미지 끝에서 잘리지 않도록 3배 폭으로 블러 후 가운데만 사용
wide = Image.new("RGB", (W * 3, H)); [wide.paste(neb, (k * W, 0)) for k in range(3)]
neb = wide.filter(ImageFilter.GaussianBlur(70)).crop((W, 0, W * 2, H))
neb = neb.point(lambda v: int(v * 0.45))  # 게임 중 탄환 가독성을 위해 어둡게
bg = ImageChops.add(bg, neb)
# 그리드 라인 (네온 바닥 느낌)
gd = Image.new("RGB", (W, H), (0, 0, 0))
g2 = ImageDraw.Draw(gd)
for x in range(0, W, 64):
    g2.line([(x, 0), (x, H)], fill=(12, 6, 28), width=1)
for y in range(0, H, 64):
    g2.line([(0, y), (W, y)], fill=(12, 6, 28), width=1)
bg = ImageChops.add(bg, gd)
bg.save(os.path.join(OUT, "background.png"))

# ---------------- 타이틀 로고 ----------------
LF = ImageFont.truetype("/usr/share/fonts/truetype/google-fonts/Poppins-Bold.ttf", 110 * SS)
def logo(d, s):
    text = "NEON STRIKER"
    bb = d.textbbox((0, 0), text, font=LF)
    tw = bb[2] - bb[0]
    x = (900 * s - tw) / 2 - bb[0]
    d.text((x, 20 * s - bb[1]), text, font=LF, fill=(0, 0, 0, 0), stroke_width=3 * s, stroke_fill=(0, 240, 255, 255))
    d.line(P([(60, 150), (840, 150)], s), fill=(255, 60, 220, 255), width=3 * s)
save(neon(900, 170, logo, glow=8, glow_strength=2.0), "logo.png")

print("images ok:", sorted(os.listdir(OUT)))
