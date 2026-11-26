"""Finding Valentino - synthesizes the game's original sound effects and chiptune menu music as WAV files.
Pure Python (no extra packages). Usage: python make_sounds.py <output folder>
"""
import math
import os
import random
import struct
import sys
import wave

SR = 44100
OUT = sys.argv[1] if len(sys.argv) > 1 else "Sounds"
os.makedirs(OUT, exist_ok=True)
TAU = 2 * math.pi


def write(name, s, peak=0.9):
    m = max(1e-9, max(abs(x) for x in s))
    g = peak / m
    with wave.open(os.path.join(OUT, name + ".wav"), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(b"".join(struct.pack("<h", int(max(-1, min(1, x * g)) * 32767)) for x in s))
    print("wrote", name, f"{len(s) / SR:.2f}s")


def silence(sec):
    return [0.0] * int(sec * SR)


def mix(dst, src, at=0.0, gain=1.0):
    i0 = int(at * SR)
    need = i0 + len(src)
    if need > len(dst):
        dst.extend([0.0] * (need - len(dst)))
    for i, x in enumerate(src):
        dst[i0 + i] += x * gain
    return dst


def env(n, attack=0.005, release=0.05, sustain_curve=None):
    a, r = int(attack * SR), int(release * SR)
    out = []
    for i in range(n):
        v = 1.0
        if i < a:
            v = i / max(1, a)
        if i > n - r:
            v *= max(0.0, (n - i) / max(1, r))
        out.append(v)
    return out


def expdecay(n, tau):
    return [math.exp(-i / (tau * SR)) for i in range(n)]


class Biquad:
    """RBJ band-pass / low-pass / high-pass."""

    def __init__(self, kind, f, q=0.707):
        w0 = TAU * f / SR
        al = math.sin(w0) / (2 * q)
        c = math.cos(w0)
        if kind == "bp":
            b0, b1, b2 = al, 0, -al
        elif kind == "lp":
            b0, b1, b2 = (1 - c) / 2, 1 - c, (1 - c) / 2
        else:  # hp
            b0, b1, b2 = (1 + c) / 2, -(1 + c), (1 + c) / 2
        a0, a1, a2 = 1 + al, -2 * c, 1 - al
        self.b = (b0 / a0, b1 / a0, b2 / a0)
        self.a = (a1 / a0, a2 / a0)
        self.z = [0.0, 0.0, 0.0, 0.0]

    def __call__(self, x):
        b0, b1, b2 = self.b
        a1, a2 = self.a
        x1, x2, y1, y2 = self.z
        y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2
        self.z = [x, x1, y, y1]
        return y


def noise(n, seed=None):
    r = random.Random(seed)
    return [r.uniform(-1, 1) for _ in range(n)]


def sweep_sine(f0, f1, sec, curve=1.0):
    n = int(sec * SR)
    ph, out = 0.0, []
    for i in range(n):
        t = (i / n) ** curve
        f = f0 + (f1 - f0) * t
        ph += TAU * f / SR
        out.append(math.sin(ph))
    return out


def square(f, sec, duty=0.5, vib=0.0):
    n = int(sec * SR)
    out, ph = [], 0.0
    for i in range(n):
        ph += f * (1 + vib * math.sin(TAU * 6 * i / SR)) / SR
        out.append(1.0 if (ph % 1.0) < duty else -1.0)
    return out


def triangle(f, sec):
    n = int(sec * SR)
    return [2 * abs(2 * ((f * i / SR) % 1.0) - 1) - 1 for i in range(n)]


def note_hz(midi):
    return 440.0 * 2 ** ((midi - 69) / 12)


def shaped(s, e):
    return [a * b for a, b in zip(s, e)]


# ------------------------------------------------------------------ menu blips (8-bit)
def blip(freqs, each, duty=0.25, gain=0.5):
    out = []
    for f in freqs:
        s = square(f, each, duty)
        out += shaped(s, env(len(s), 0.002, each * 0.4))
    return [x * gain for x in out]


write("UI_Move", blip([1046.5], 0.045))
write("UI_Select", blip([784, 1046.5, 1318.5], 0.045))
write("UI_Back", blip([784, 523.25], 0.06))
write("UI_Pause", blip([659.3, 987.8], 0.07, duty=0.5))
start = blip([523.25, 659.3, 784, 1046.5, 1318.5, 1568], 0.05)
chord = [0.0] * int(0.45 * SR)
for f in (1046.5, 1318.5, 1568):
    mix(chord, shaped(square(f, 0.45, 0.25), expdecay(int(0.45 * SR), 0.15)), 0, 0.33)
write("UI_Start", start + chord)

# ------------------------------------------------------------------ chiptune menu theme (original, 8 bars, loops)
BPM = 132
E8 = 60 / BPM / 2  # an eighth note
bars = [  # chord root (midi), melody eighths (midi)
    (48, [76, 79, 84, 79, 76, 79, 81, 79]),
    (45, [76, 81, 84, 81, 76, 72, 74, 76]),
    (41, [77, 81, 84, 81, 77, 81, 79, 77]),
    (43, [74, 79, 83, 79, 74, 79, 77, 74]),
    (48, [84, 83, 84, 79, 76, 79, 84, 88]),
    (45, [86, 84, 81, 84, 76, 81, 84, 81]),
    (41, [77, 79, 81, 84, 81, 79, 77, 81]),
    (43, [79, 83, 86, 83, 79, 77, 74, 71]),
]
song = silence(len(bars) * 8 * E8)
for b, (root, mel) in enumerate(bars):
    for k, m in enumerate(mel):
        t = (b * 8 + k) * E8
        s = square(note_hz(m), E8 * 0.92, 0.25, vib=0.004)
        mix(song, shaped(s, env(len(s), 0.004, 0.03)), t, 0.22)
        # bass: root / octave bounce on triangle
        bm = root - 12 if k % 2 == 0 else root
        s = triangle(note_hz(bm), E8 * 0.85)
        mix(song, shaped(s, env(len(s), 0.004, 0.02)), t, 0.45)
        # drums: kick on 1 and 3, snare on 2 and 4, hats on off-beats
        if k in (0, 4):
            kick = sweep_sine(150, 45, 0.12, 0.5)
            mix(song, shaped(kick, expdecay(len(kick), 0.05)), t, 0.6)
        if k in (2, 6):
            sn = noise(int(0.1 * SR), b * 10 + k)
            mix(song, shaped(sn, expdecay(len(sn), 0.03)), t, 0.25)
        if k % 2 == 1:
            hp = Biquad("hp", 7000)
            hat = [hp(x) for x in noise(int(0.03 * SR), b * 100 + k)]
            mix(song, shaped(hat, expdecay(len(hat), 0.008)), t, 0.2)
song = song[: int(len(bars) * 8 * E8 * SR)]
write("Music_Menu", song, peak=0.7)

# ------------------------------------------------------------------ dog barks (cartoony formant bark)
def bark(f_start, f_peak, f_end, sec, seed, rough=0.18):
    n = int(sec * SR)
    f1, f2, f3 = Biquad("bp", 650, 2.5), Biquad("bp", 1250, 3.0), Biquad("bp", 2600, 4.0)
    rn = random.Random(seed)
    ph, out = 0.0, []
    for i in range(n):
        t = i / n
        f = f_start + (f_peak - f_start) * (t / 0.15) if t < 0.15 else f_peak + (f_end - f_peak) * ((t - 0.15) / 0.85)
        ph += f / SR
        saw = 2 * (ph % 1.0) - 1
        src = saw + rough * rn.uniform(-1, 1)
        y = f1(src) * 1.0 + f2(src) * 0.7 + f3(src) * 0.25
        a = min(1.0, i / (0.008 * SR)) * math.exp(-max(0, t - 0.25) * 6)
        out.append(y * a)
    return out


write("Dog_Bark", bark(380, 520, 240, 0.2, 1))
write("Dog_Bark2", bark(420, 560, 300, 0.16, 2, 0.25) + silence(0.07) + bark(400, 540, 280, 0.15, 3, 0.25))

# ------------------------------------------------------------------ footsteps on grass (4 variants)
for v in range(4):
    n = int(0.11 * SR)
    lp, hp = Biquad("lp", 1600 + v * 250), Biquad("hp", 180)
    s = [hp(lp(x)) for x in noise(n, 40 + v)]
    e = [min(1.0, i / (0.004 * SR)) * math.exp(-i / (0.025 * SR)) for i in range(n)]
    crunch = Biquad("bp", 3500, 1.5)
    s2 = [crunch(x) for x in noise(n, 80 + v)]
    write(f"Step_Grass_{v + 1}", [(a + 0.35 * b) * c for a, b, c in zip(s, s2, e)], peak=0.6)

# ------------------------------------------------------------------ carry / feeding / petting
pop = sweep_sine(260, 760, 0.09, 0.7)
write("Pickup", shaped(pop, expdecay(len(pop), 0.04)), peak=0.6)
thud = sweep_sine(150, 55, 0.14, 0.5)
lp = Biquad("lp", 400)
tn = [lp(x) for x in noise(len(thud), 5)]
write("PutDown", [(a + 0.6 * b) * c for a, b, c in zip(thud, tn, expdecay(len(thud), 0.05))], peak=0.6)

munch = []
for k in range(3):
    n = int(0.06 * SR)
    bp = Biquad("bp", 2400 - k * 300, 1.2)
    s = [bp(x) for x in noise(n, 60 + k)]
    mix(munch, shaped(s, [min(1, i / 60) * math.exp(-i / (0.015 * SR)) for i in range(n)]), k * 0.14, 1.0 - k * 0.2)
write("Munch", munch, peak=0.6)

sip = []
for k in range(2):
    g = sweep_sine(320, 640, 0.09, 1.3)
    mix(sip, shaped(g, env(len(g), 0.01, 0.04)), k * 0.24, 1.0)
write("Sip", sip, peak=0.5)

chime = silence(0.6)
for k, f in enumerate((1568.0, 2093.0)):
    n = int(0.5 * SR)
    s = [math.sin(TAU * f * i / SR) + 0.3 * math.sin(TAU * 2 * f * i / SR) for i in range(n)]
    mix(chime, shaped(s, expdecay(n, 0.12)), k * 0.11, 0.5)
write("Pet_Chime", chime, peak=0.45)

sparkle = silence(0.8)
for k, m in enumerate((84, 88, 91, 96, 100)):
    s = triangle(note_hz(m), 0.35)
    mix(sparkle, shaped(s, expdecay(len(s), 0.09)), k * 0.07, 0.5)
write("Sprout_Sparkle", sparkle, peak=0.5)

n = int(0.5 * SR)
boing, ph = [], 0.0
for i in range(n):
    t = i / n
    f = (620 - 330 * t) * (1 + 0.08 * math.sin(TAU * 17 * i / SR))
    ph += TAU * f / SR
    boing.append(math.sin(ph) * math.exp(-t * 3.5))
write("Nibble_Boing", boing, peak=0.55)

# ------------------------------------------------------------------ water
# pouring: band-passed noise with a gentle wobble, crossfaded so it loops seamlessly
L, X = 2.0, 0.35
n = int((L + X) * SR)
bp1, bp2 = Biquad("bp", 1700, 0.8), Biquad("bp", 3800, 1.2)
r = random.Random(9)
raw, wob = [], 0.0
for i, x in enumerate(noise(n, 11)):
    wob += (r.uniform(-1, 1) - wob) * 0.0008
    raw.append((bp1(x) + 0.4 * bp2(x)) * (0.75 + 0.25 * math.sin(TAU * 3.1 * i / SR) + wob))
nl, nx = int(L * SR), int(X * SR)
loop = raw[:nl]
for i in range(nx):
    a = i / nx
    loop[i] = raw[nl + i] * (1 - a) + raw[i] * a
write("Water_Pour_Loop", loop, peak=0.5)

n = int(0.6 * SR)
sp = []
cut = 6000.0
st = [0.0]
for i, x in enumerate(noise(n, 21)):
    c = 300 + 5000 * math.exp(-i / (0.12 * SR))
    a = 1 - math.exp(-TAU * c / SR)
    st[0] += (x - st[0]) * a
    sp.append(st[0] * min(1, i / (0.003 * SR)) * math.exp(-i / (0.18 * SR)))
rd = random.Random(4)
for k in range(7):
    d = sweep_sine(rd.uniform(900, 1800), rd.uniform(1800, 2600), 0.04)
    mix(sp, shaped(d, expdecay(len(d), 0.012)), 0.08 + rd.uniform(0, 0.35), 0.25)
write("Water_Splash", sp, peak=0.6)
