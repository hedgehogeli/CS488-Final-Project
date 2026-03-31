"""
Animate Newton's Cradle from CSV output of newtons_cradle.py.

Reads ball positions from the CSV and draws pendulum rods + balls
using matplotlib animation. Imports geometry constants (pivots, radius, etc.)
from the simulation module.

Usage:
    python animate_cradle.py                    # uses cradle_output.csv
    python animate_cradle.py my_run.csv         # custom CSV file
"""

import sys
import csv
import matplotlib.pyplot as plt
import matplotlib.patches as patches
from matplotlib.animation import FuncAnimation

from newtons_cradle import PIVOTS, BALL_RADIUS, NUM_BALLS, DT

# ── Load CSV ──────────────────────────────────────────────────────────────────

def load_csv(path):
    times = []
    positions = []  # list of [(x1,y1), (x2,y2), ... (x5,y5)]
    with open(path) as f:
        reader = csv.reader(f)
        next(reader)  # skip header
        for row in reader:
            times.append(float(row[0]))
            frame = []
            for i in range(NUM_BALLS):
                x = float(row[1 + 2 * i])
                y = float(row[2 + 2 * i])
                frame.append((x, y))
            positions.append(frame)
    return times, positions

# ── Animation ─────────────────────────────────────────────────────────────────

def animate(csv_path):
    times, positions = load_csv(csv_path)

    fig, ax = plt.subplots(figsize=(10, 7))
    ax.set_aspect("equal")
    ax.set_xlim(-7, 7)
    ax.set_ylim(-4, 4)
    ax.set_facecolor("#1a1a2e")
    fig.patch.set_facecolor("#1a1a2e")
    ax.set_title("Newton's Cradle", color="white", fontsize=16, pad=12)
    ax.tick_params(colors="white")
    for spine in ax.spines.values():
        spine.set_color("#444")

    # Draw the top bar (static)
    bar_y = PIVOTS[0][1]
    bar_x0 = PIVOTS[0][0] - BALL_RADIUS * 1.5
    bar_x1 = PIVOTS[-1][0] + BALL_RADIUS * 1.5
    ax.plot([bar_x0, bar_x1], [bar_y, bar_y], color="#888", linewidth=4,
            solid_capstyle="round")

    # Create artists: rods (lines) and balls (circles)
    colors = ["#e94560", "#f5a623", "#f7e04b", "#50c878", "#4da6ff"]
    rods = []
    balls = []
    for k in range(NUM_BALLS):
        rod, = ax.plot([], [], color="#aaa", linewidth=1.5)
        rods.append(rod)
        circle = patches.Circle((0, 0), BALL_RADIUS, fc=colors[k], ec="white",
                                linewidth=0.8, zorder=5)
        ax.add_patch(circle)
        balls.append(circle)

    time_text = ax.text(0.02, 0.95, "", transform=ax.transAxes, color="white",
                        fontsize=11, family="monospace")

    def init():
        for rod in rods:
            rod.set_data([], [])
        for ball in balls:
            ball.center = (0, 0)
        time_text.set_text("")
        return rods + balls + [time_text]

    def update(frame_idx):
        frame = positions[frame_idx]
        for k in range(NUM_BALLS):
            bx, by = frame[k]
            px, py = PIVOTS[k]
            rods[k].set_data([px, bx], [py, by])
            balls[k].center = (bx, by)
        time_text.set_text(f"t = {times[frame_idx]:.3f} s")
        return rods + balls + [time_text]

    # Skip frames to target ~60 fps playback at real-time speed
    playback_fps = 60
    skip = max(1, int(1 / (playback_fps * DT)))
    frame_indices = list(range(0, len(times), skip))

    anim = FuncAnimation(fig, update, frames=frame_indices, init_func=init,
                         interval=1000 / playback_fps, blit=True)
    plt.tight_layout()
    plt.show()

# ── Main ──────────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    path = sys.argv[1] if len(sys.argv) > 1 else "cradle_output.csv"
    animate(path)