import subprocess

# --- Configuration ---
PEAK_MS = 87
FPS = 48
INPUT_VIDEO = "input.mp4"
INPUT_AUDIO = "collision1.m4a"
OUTPUT_VIDEO = "output.mp4"

# (frame, volume) pairs
HITS = [
    (39,  1.0),
    (115, 0.9),
    (191, 0.85),
    (265, 0.8),
    (340, 0.65),
    (415, 0.6),
    (490, 0.55),
    (565, 0.50),
    (640, 0.45),
    (715, 0.35),
    (790, 0.25),
    (865, 0.20), (870, 0.07),
    (940, 0.15), (945, 0.07),
    (1015, 0.1), (1020, 0.07),
    (1090, 0.1), (1095, 0.07), (1100, 0.07),

]

# --- Build filter_complex ---
n = len(HITS)

parts = [
    f"[1:a]aformat=sample_fmts=fltp:sample_rates=48000:channel_layouts=stereo[a0]",
    f"[a0]asplit={n}" + "".join(f"[c{i}]" for i in range(n)),
]

mix_inputs = ""
for i, (frame, vol) in enumerate(HITS):
    delay_ms = max(0, round(frame * 1000 / FPS - PEAK_MS))
    parts.append(f"[c{i}]adelay={delay_ms}|{delay_ms},volume={vol}[d{i}]")
    mix_inputs += f"[d{i}]"

parts.append(f"{mix_inputs}amix=inputs={n}:duration=longest:normalize=0[mixed]")

filter_complex = ";\n".join(parts)

cmd = [
    "ffmpeg", "-y",
    "-i", INPUT_VIDEO,
    "-i", INPUT_AUDIO,
    "-filter_complex", filter_complex,
    "-map", "0:v",
    "-map", "[mixed]",
    "-c:v", "copy",
    "-c:a", "aac",
    "-b:a", "192k",
    OUTPUT_VIDEO,
]

print("Filter graph:")
print(filter_complex)
print()
print("Command:")
print(" ".join(cmd))
print()

subprocess.run(cmd, check=True)
print(f"Done: {OUTPUT_VIDEO}")