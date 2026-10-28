# NEON STRIKER - 효과음 생성기 (합성음, 외부 음원 없음)
# 44.1kHz / 16bit / mono WAV
import numpy as np, wave, os, sys

OUT = sys.argv[1] if len(sys.argv) > 1 else "assets/sounds"
os.makedirs(OUT, exist_ok=True)
SR = 44100
rng = np.random.default_rng(1)

def t_(dur): return np.arange(int(SR * dur)) / SR

def env(n, a=0.005, r=0.1, total=None):
    """간단한 AR 엔벨로프 (attack 후 지수 감쇠)"""
    t = np.arange(n) / SR
    e = np.minimum(1.0, t / max(a, 1e-4))
    e *= np.exp(-np.maximum(0, t - a) / max(r, 1e-4))
    return e

def sweep(f0, f1, dur, wave_="sine"):
    t = t_(dur)
    f = np.geomspace(max(f0, 1), max(f1, 1), len(t))
    ph = 2 * np.pi * np.cumsum(f) / SR
    if wave_ == "square": return np.sign(np.sin(ph))
    if wave_ == "saw":    return 2 * ((ph / (2 * np.pi)) % 1.0) - 1
    if wave_ == "tri":    return 2 * np.abs(2 * ((ph / (2 * np.pi)) % 1.0) - 1) - 1
    return np.sin(ph)

def noise(dur): return rng.uniform(-1, 1, int(SR * dur))

def lowpass(x, k):
    """이동평균 로우패스 (k가 클수록 먹먹)"""
    if k <= 1: return x
    return np.convolve(x, np.ones(k) / k, mode="same")

def save(name, x, vol=0.8):
    x = np.asarray(x, dtype=np.float64)
    peak = np.max(np.abs(x)) or 1.0
    x = x / peak * vol
    fade = min(len(x), 200)          # 끝 클릭 방지
    x[-fade:] *= np.linspace(1, 0, fade)
    data = (x * 32767).astype(np.int16)
    with wave.open(os.path.join(OUT, name), "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
        w.writeframes(data.tobytes())

def mix(*parts):
    n = max(len(p) for p in parts)
    out = np.zeros(n)
    for p in parts: out[:len(p)] += p
    return out

# --- 플레이어 무기 ---
s = sweep(1800, 600, 0.08, "square"); save("shot_normal.wav", s * env(len(s), r=0.03), 0.35)
n = lowpass(noise(0.12), 6);            save("shot_flame.wav", n * env(len(n), a=0.01, r=0.05), 0.35)
l = sweep(900, 880, 0.12, "saw") + 0.5 * sweep(1800, 1760, 0.12, "sine")
save("shot_laser.wav", l * env(len(l), a=0.01, r=0.08), 0.25)

# 차지 (3초간 상승) / 차지 완료 / 차지샷 발사
c = sweep(120, 1200, 3.0, "saw") * np.linspace(0.2, 1, int(SR * 3.0))
c = lowpass(c, 3); save("charge.wav", c, 0.35)
r = mix(sweep(1500, 3000, 0.25, "sine") * env(int(SR * .25), r=.15), sweep(2000, 4000, 0.25, "sine") * env(int(SR * .25), r=.1) * .5)
save("charge_ready.wav", r, 0.4)
cs = mix(sweep(1200, 60, 0.9, "saw") * env(int(SR * .9), a=.005, r=.4), lowpass(noise(0.9), 4) * env(int(SR * .9), r=.3) * .8)
save("charge_shot.wav", cs, 0.8)

# 폭탄 투척 / 폭탄 폭발
b = sweep(300, 900, 0.3, "tri"); save("bomb_throw.wav", b * env(len(b), a=.02, r=.2), 0.5)
bx = mix(lowpass(noise(1.6), 20) * env(int(SR * 1.6), a=.002, r=.5), sweep(160, 30, 1.6, "sine") * env(int(SR * 1.6), r=.6))
save("bomb_explode.wav", bx, 0.9)

# --- 피격 / 폭발 ---
h = mix(sweep(900, 300, 0.05, "square") * .4, noise(0.05) * .5) * env(int(SR * .05), r=.02)
save("hit.wav", h, 0.3)
e = mix(lowpass(noise(0.45), 8) * env(int(SR * .45), r=.12), sweep(400, 60, 0.45, "square") * env(int(SR * .45), r=.08) * .4)
save("explode_small.wav", e, 0.55)
e2 = mix(lowpass(noise(0.9), 14) * env(int(SR * .9), r=.3), sweep(220, 40, 0.9, "saw") * env(int(SR * .9), r=.25) * .5)
save("explode_mid.wav", e2, 0.7)
bb = mix(lowpass(noise(3.0), 25) * env(int(SR * 3.0), a=.01, r=1.0),
         sweep(120, 25, 3.0, "sine") * env(int(SR * 3.0), r=1.2),
         lowpass(noise(3.0), 3) * env(int(SR * 3.0), r=.2) * .4)
save("boss_explode.wav", bb, 0.95)
pd = mix(sweep(800, 50, 1.2, "saw") * env(int(SR * 1.2), r=.5), lowpass(noise(1.2), 10) * env(int(SR * 1.2), r=.4))
save("player_die.wav", pd, 0.8)
ph = sweep(500, 200, 0.18, "square") * env(int(SR * .18), r=.08); save("player_hit.wav", ph, 0.5)

# --- 아이템 / 성장 ---
def arp(notes, step=0.06, w="square", r=0.08):
    out = []
    for f in notes:
        s = sweep(f, f, step, w) * env(int(SR * step), r=r)
        out.append(s)
    return np.concatenate(out)
save("item.wav", arp([880, 1320, 1760], 0.05), 0.35)
save("powerup.wav", arp([660, 880, 1100, 1320, 1760], 0.05), 0.35)
save("levelup.wav", arp([523, 659, 784, 1047, 784, 1047], 0.07, "tri", .2), 0.45)
fv = np.concatenate([arp([523, 659, 784, 1047], 0.05, "square"), arp([587, 740, 880, 1175], 0.05, "square"), arp([659, 831, 988, 1319, 1568], 0.06, "square", .3)])
save("fever.wav", fv, 0.4)

# --- 보스 / 시스템 ---
wn = []
for i in range(3):
    seg = sweep(440, 880, 0.45, "saw") * env(int(SR * .45), a=.02, r=.6)
    wn.append(np.concatenate([seg, np.zeros(int(SR * .1))]))
save("warning.wav", lowpass(np.concatenate(wn), 2), 0.55)
save("select.wav", arp([1046, 1568], 0.06, "square", .06), 0.35)
save("pause.wav", arp([784, 523], 0.08, "tri", .1), 0.4)
save("tick.wav", sweep(1200, 1200, 0.06, "square") * env(int(SR * .06), r=.02), 0.3)
st = np.concatenate([arp([392], .15, "square", .1), arp([523], .15, "square", .1), arp([784], .4, "square", .3)])
save("stage_start.wav", st, 0.4)
go = np.concatenate([arp([392], .3, "tri", .3), arp([330], .3, "tri", .3), arp([262], .9, "tri", .6)])
save("game_over.wav", go, 0.5)
cl = np.concatenate([arp([523, 659, 784], .12, "square", .15), arp([1047], .2, "square", .2), arp([784, 1047], .12, "square", .15), arp([1319], .8, "square", .6)])
save("clear.wav", cl, 0.45)

# --- 기가 레이저 지속음 (파지지지직) ---
# 1초짜리를 만들어 레이저가 켜져 있는 동안 반복 재생한다 (일시정지 시 소리가 따로 놀지 않도록)
n = SR
t = np.arange(n) / SR
hum = sweep(110, 110, 1.0, "saw") * 0.35 + sweep(220, 220, 1.0, "square") * 0.15   # 전기 웅웅거림
am = 0.6 + 0.4 * np.sign(np.sin(2 * np.pi * 23 * t))                                  # 초당 23번 떨림 = "지지지"
crackle = np.zeros(n)                                                                  # 무작위 방전 = "파직"
for i in rng.integers(0, n - 300, 450):
    L = int(rng.integers(20, 250))
    crackle[i:i + L] += rng.uniform(-1, 1, L) * np.exp(-np.arange(L) / 40)
buzz = lowpass(noise(1.0), 2) * 0.4 * (0.5 + 0.5 * np.sin(2 * np.pi * 7 * t))         # 지글거리는 잡음
gl = hum * am + crackle * 0.9 + buzz
gl[:150] *= np.linspace(0, 1, 150)                                                     # 반복 이음새 클릭 방지
save("giga_laser.wav", gl, 0.7)

# --- 화염구 지속음 (화르르륵): 1초 루프 ---
n = SR
t = np.arange(n) / SR
roar = lowpass(noise(1.0), 18) * (0.7 + 0.3 * np.sin(2 * np.pi * 3 * t))      # 낮게 웅웅거리는 불길
hiss = lowpass(noise(1.0), 3) * 0.35 * (0.6 + 0.4 * np.sin(2 * np.pi * 9 * t))  # 쉬익
pops = np.zeros(n)                                                               # 타닥타닥 튀는 소리
for i in rng.integers(0, n - 400, 60):
    L = int(rng.integers(80, 300))
    pops[i:i + L] += rng.uniform(-1, 1, L) * np.exp(-np.arange(L) / 60)
rumble = sweep(55, 55, 1.0, "sine") * 0.4 * (0.6 + 0.4 * np.sin(2 * np.pi * 2 * t))
fo = roar * 1.4 + hiss + pops * 0.6 + rumble
fo[:150] *= np.linspace(0, 1, 150)
save("flame_orb.wav", fo, 0.75)

# --- 기가 미사일 발사 (슈웅 + 쿵) ---
dur = 0.45
w = noise(dur) - lowpass(noise(dur), 6)                                          # 고음 위주의 바람 소리
w *= np.concatenate([np.linspace(0, 1, int(SR * 0.05)), np.exp(-np.arange(int(SR * dur) - int(SR * 0.05)) / (SR * 0.12))])
thump = sweep(140, 45, dur, "sine") * env(int(SR * dur), a=0.002, r=0.08)
zap = sweep(1600, 400, dur, "saw") * env(int(SR * dur), a=0.002, r=0.05) * 0.3
save("missile_launch.wav", mix(w * 0.8, thump, zap), 0.7)

print("sounds ok:", sorted(os.listdir(OUT)))
