"""
Camera Spline Tool
==================
Evaluates camera position along a cubic spline path, with:
  - Timing remap spline (slow-in / slow-out)
  - 3D position spline (Catmull-Rom, centripetal)
  - FOV spline (Catmull-Rom)
  - Derived look-at direction toward a fixed target

Outputs a CSV: time, px, py, pz, dx, dy, dz, fov
"""

import numpy as np
import csv
import sys

# ─────────────────────────────────────────────
# MAGIC VALUES — edit these
# ─────────────────────────────────────────────


FPS = 48
TOTAL_FRAMES = FPS * 24

y_offset = 11

LOOKAT = np.array([0.0, 1.25 + y_offset, 0.0])

# Camera path control points (x, y, z)
PATH_POINTS = np.array([
    [ 16.0,  5.0+y_offset,  30],
    [ 8.0,  5.0+y_offset,  17.0],
    [ 0.0,  5.0+y_offset,  15.0],
    [ -7.0,  5.0+y_offset, 13.0],

    [-17.0,  5.0+y_offset, 6.0],
    [-17.0,  5.0+y_offset, -6.0],

    [-8.0,  3.0+y_offset,  -12.0],
    [0.0,  1.4+y_offset,  -10.0],
    # [0.0,  1.0+y_offset,  -8.0],
    [0.0,  1.0+y_offset,  -4.0],
])

# FOV control values (one per path point, or independent)
FOV_POINTS = np.array([50, 60.0, 61.0, 62.0, 61.0, 55.0, 50.0, 46.0, 45.0])

# Timing remap: cubic bezier handles for a single ease segment
# These are the two interior control points of a cubic bezier on [0,1]→[0,1]
# (same semantics as CSS cubic-bezier)
# Example: (0.25, 0.0, 0.25, 1.0) = ease,  (0.42, 0, 0.58, 1) = ease-in-out
EASE_P1 = (0.20, 0.0)   # (t, s) of first handle
EASE_P2 = (0.7, 1.0)   # (t, s) of second handle

OUTPUT_FILE = "camera_path.csv"


# ─────────────────────────────────────────────
# Cubic Bezier timing remap
# ─────────────────────────────────────────────

def _cubic_bezier_sample(p1x, p1y, p2x, p2y, t_target, iterations=16):
    """Solve cubic bezier for y given x = t_target, using Newton's method.
    Bezier defined by (0,0), (p1x,p1y), (p2x,p2y), (1,1)."""
    # Find bezier parameter u such that x(u) = t_target
    u = t_target  # initial guess
    for _ in range(iterations):
        # x(u) = 3(1-u)^2 u p1x + 3(1-u) u^2 p2x + u^3
        x = 3 * (1 - u) ** 2 * u * p1x + 3 * (1 - u) * u ** 2 * p2x + u ** 3
        dx = 3 * (1 - u) ** 2 * p1x + 6 * (1 - u) * u * (p2x - p1x) + 3 * u ** 2 * (1 - p2x)
        if abs(dx) < 1e-12:
            break
        u -= (x - t_target) / dx
        u = np.clip(u, 0.0, 1.0)
    # y(u)
    y = 3 * (1 - u) ** 2 * u * p1y + 3 * (1 - u) * u ** 2 * p2y + u ** 3
    return float(np.clip(y, 0.0, 1.0))


def evaluate_timing(t, p1=EASE_P1, p2=EASE_P2):
    """Remap t ∈ [0,1] through cubic bezier ease curve → s ∈ [0,1]."""
    return _cubic_bezier_sample(p1[0], p1[1], p2[0], p2[1], t)


# ─────────────────────────────────────────────
# Centripetal Catmull-Rom spline
# ─────────────────────────────────────────────

def _catmull_rom_segment(P0, P1, P2, P3, t, alpha=0.5):
    """Evaluate one Catmull-Rom segment at parameter t ∈ [0,1].
    alpha=0.5 → centripetal."""
    def tj(ti, Pi, Pj):
        d = Pj - Pi
        return (np.sum(d ** 2) ** 0.5) ** alpha + ti

    t0 = 0.0
    t1 = tj(t0, P0, P1)
    t2 = tj(t1, P1, P2)
    t3 = tj(t2, P2, P3)

    # remap t from [0,1] to [t1, t2]
    t = t1 + t * (t2 - t1)

    A1 = (t1 - t) / (t1 - t0) * P0 + (t - t0) / (t1 - t0) * P1
    A2 = (t2 - t) / (t2 - t1) * P1 + (t - t1) / (t2 - t1) * P2
    A3 = (t3 - t) / (t3 - t2) * P2 + (t - t2) / (t3 - t2) * P3

    B1 = (t2 - t) / (t2 - t0) * A1 + (t - t0) / (t2 - t0) * A2
    B2 = (t3 - t) / (t3 - t1) * A2 + (t - t1) / (t3 - t1) * A3

    C = (t2 - t) / (t2 - t1) * B1 + (t - t1) / (t2 - t1) * B2
    return C


def evaluate_catmull_rom(points, s):
    """Evaluate a full Catmull-Rom spline at s ∈ [0,1].
    `points` shape: (N, D) for N control points, D dimensions.
    Clamps to endpoints. Phantom endpoints mirrored."""
    points = np.atleast_2d(points)
    n = len(points)
    if n < 2:
        return points[0]

    # number of segments = n - 1
    num_seg = n - 1
    seg_f = s * num_seg
    seg_i = int(np.floor(seg_f))
    seg_i = min(seg_i, num_seg - 1)
    local_t = seg_f - seg_i

    # get 4 control points, clamping / reflecting at boundaries
    def pt(i):
        if i < 0:
            return 2 * points[0] - points[-i]
        if i >= n:
            return 2 * points[-1] - points[2 * (n - 1) - i]
        return points[i]

    P0 = pt(seg_i - 1)
    P1 = pt(seg_i)
    P2 = pt(seg_i + 1)
    P3 = pt(seg_i + 2)

    return _catmull_rom_segment(P0, P1, P2, P3, local_t)


# ─────────────────────────────────────────────
# Main evaluation
# ─────────────────────────────────────────────

def compute_frames(total_frames=TOTAL_FRAMES, fps=FPS):
    rows = []
    for frame in range(total_frames):
        t = frame / max(total_frames - 1, 1)
        s = evaluate_timing(t)

        pos = evaluate_catmull_rom(PATH_POINTS, s)

        fov_val = evaluate_catmull_rom(FOV_POINTS.reshape(-1, 1), s)[0]

        # look-at direction (normalized)
        direction = LOOKAT - pos
        norm = np.linalg.norm(direction)
        if norm > 1e-9:
            direction = direction / norm

        time_sec = frame / fps

        rows.append({
            "frame": frame,
            "time": round(time_sec, 4),
            "px": round(pos[0], 6),
            "py": round(pos[1], 6),
            "pz": round(pos[2], 6),
            "dx": round(direction[0], 6),
            "dy": round(direction[1], 6),
            "dz": round(direction[2], 6),
            "fov": round(fov_val, 4),
        })
    return rows


def write_csv(rows, filename=OUTPUT_FILE):
    fieldnames = ["frame", "time", "px", "py", "pz", "dx", "dy", "dz", "fov"]
    with open(filename, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)
    print(f"Wrote {len(rows)} frames to {filename}")

def plot_3d(rows):
    import matplotlib.pyplot as plt
    from mpl_toolkits.mplot3d import Axes3D

    fig = plt.figure(figsize=(10, 8))
    ax = fig.add_subplot(111, projection='3d')

    # continuous path
    px = [r['px'] for r in rows]
    py = [r['py'] for r in rows]
    pz = [r['pz'] for r in rows]
    ax.plot(px, pz, py, color='#534ab7', linewidth=1.5, label='Camera path')

    # dots at even time intervals to show speed
    step = 6
    ax.scatter(
        px[::step], pz[::step], py[::step],
        color='#ef9f27', s=30, zorder=5, label=f'Every {step} frames'
    )

    # control points
    cp = PATH_POINTS
    ax.scatter(cp[:, 0], cp[:, 2], cp[:, 1],
               color='#7f77dd', s=80, marker='D', label='Control points')

    # lookat target
    ax.scatter(*LOOKAT[[0, 2, 1]], color='#e24b4a', s=100, marker='x',
               linewidths=3, label='Look-at target')

    # direction arrows (every 12 frames)
    arrow_step = 12
    scale = 0.6
    for i in range(0, len(rows), arrow_step):
        r = rows[i]
        ax.quiver(r['px'], r['pz'], r['py'],
                  r['dx'] * scale, r['dz'] * scale, r['dy'] * scale,
                  color='#ef9f27', arrow_length_ratio=0.2, linewidth=1)

    ax.set_xlabel('X')
    ax.set_ylabel('Z')
    ax.set_zlabel('Y')
    ax.set_title('Camera path (centripetal Catmull-Rom + ease-in-out)')
    ax.legend(loc='upper left', fontsize=8)
    plt.tight_layout()
    plt.savefig('camera_path_3d.png', dpi=150)
    plt.show()
    print("Saved camera_path_3d.png")

if __name__ == "__main__":
    rows = compute_frames()
    write_csv(rows)
    plot_3d(rows)

    # Also write JSON for the visualiser
    # import json
    # data = {
    #     "frames": rows,
    #     "control_points": PATH_POINTS.tolist(),
    #     "lookat": LOOKAT.tolist(),
    #     "fov_points": FOV_POINTS.tolist(),
    #     "ease": {"p1": list(EASE_P1), "p2": list(EASE_P2)},
    # }
    # with open("camera_data.json", "w") as f:
    #     json.dump(data, f)
    # print("Wrote camera_data.json")