import librosa
import librosa.display
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np

y, sr = librosa.load("collision.m4a", sr=None)
duration = len(y) / sr

fig, ax = plt.subplots(figsize=(18, 4))
times = np.linspace(0, duration, len(y))
ax.plot(times, y, linewidth=0.3)

# Major ticks every second, minor ticks every 50ms
ax.xaxis.set_major_locator(ticker.MultipleLocator(1.0))
ax.xaxis.set_minor_locator(ticker.MultipleLocator(0.05))
ax.xaxis.set_major_formatter(ticker.FuncFormatter(
    lambda x, _: f"{int(x)}:{int((x % 1) * 1000):03d}" if x != int(x) else f"{int(x)}s"
))
ax.tick_params(axis='x', which='minor', length=3)
ax.tick_params(axis='x', which='major', length=6)

ax.set_xlabel("Time (s)")
ax.set_ylabel("Amplitude")
ax.set_title("collision.m4a waveform")
ax.grid(True, which='major', alpha=0.4)
ax.grid(True, which='minor', alpha=0.15)
ax.set_xlim(0, duration)
plt.tight_layout()
plt.savefig("waveform.png", dpi=200)
plt.show()
print(f"Duration: {duration:.3f}s | Sample rate: {sr} Hz")