import sys, os, wave, numpy as np
work, out = sys.argv[1], sys.argv[2]
os.makedirs(out, exist_ok=True)
cuts = {  # file: list of (start, end) seconds of the bark itself
 "Barking_of_a_dog":   [(0.19,0.32),(1.18,1.31),(1.89,2.02)],
 "Barking_of_a_dog_2": [(0.04,0.31),(0.66,0.78),(1.50,1.67),(2.42,2.65),(3.35,3.59)],
 "Perro_ladrando":     [(0.06,0.24),(2.22,2.32),(3.92,4.05)],
 "Sound-of-dog":       [(0.05,0.28),(0.72,0.96)],
}
doubles = {"Sound-of-dog": (0.05, 0.96)}
def save(name, x, rate):
    x = x / max(np.abs(x).max(), 1e-6) * 0.9
    with wave.open(os.path.join(out, name + ".wav"), "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(rate)
        w.writeframes((x * 32767).astype(np.int16).tobytes())
def clip(x, rate, a, b):
    i, j = max(0, int((a - 0.025) * rate)), min(len(x), int((b + 0.22) * rate))
    y = x[i:j].copy()
    fi, fo = int(0.005 * rate), int(0.12 * rate)
    y[:fi] *= np.linspace(0, 1, fi); y[-fo:] *= np.linspace(1, 0, fo) ** 2
    return y - y.mean()
n = 0
for f, segs in cuts.items():
    x = np.load(os.path.join(work, f + ".npy")); rate = int(open(os.path.join(work, f + ".rate")).read())
    for a, b in segs:
        n += 1; save(f"Dog_Bark_Real_{n:02d}", clip(x, rate, a, b), rate)
    if f in doubles:
        save("Dog_Bark_Real_Double", clip(x, rate, *doubles[f]), rate)
print("wrote", n, "singles + 1 double")
