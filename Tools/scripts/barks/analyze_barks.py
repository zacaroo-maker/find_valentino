# Blender: decode OGGs with aud, find bark onsets by energy envelope.
import aud, sys, os, numpy as np
args = sys.argv[sys.argv.index("--")+1:]
src, out = args[0], args[1]
os.makedirs(out, exist_ok=True)
for f in sorted(os.listdir(src)):
    if not f.endswith(".ogg"): continue
    snd = aud.Sound(os.path.join(src, f))
    rate = int(snd.specs[0])
    data = snd.data()  # (samples, channels) float
    mono = data.mean(axis=1) if data.ndim > 1 else data
    np.save(os.path.join(out, f[:-4] + ".npy"), mono.astype(np.float32))
    hop = int(rate * 0.01)
    env = np.array([np.sqrt(np.mean(mono[i:i+hop]**2)) for i in range(0, len(mono)-hop, hop)])
    thr = max(env.max() * 0.12, np.median(env) * 3)
    on = env > thr
    segs = []; i = 0
    while i < len(on):
        if on[i]:
            j = i
            while j < len(on) and (on[j] or np.any(on[j:j+6])): j += 1
            segs.append((i*0.01, j*0.01, float(env[i:j].max())))
            i = j
        else: i += 1
    print(f"{f}: rate={rate} len={len(mono)/rate:.2f}s peak={np.abs(mono).max():.2f} noise={np.median(env):.4f}")
    for s in segs: print(f"   {s[0]:.2f}-{s[1]:.2f}  ({s[1]-s[0]:.2f}s) peak_rms={s[2]:.3f}")
    open(os.path.join(out, f[:-4] + ".rate"), "w").write(str(rate))
