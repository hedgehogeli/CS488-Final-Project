"""
Newton's Cradle simulation — minimal Python port inspired by myphysicslab.

Physics model:
  - Each ball is a rigid pendulum (massless rod + point-mass ball).
  - State per pendulum: angle θ (from vertical) and angular velocity ω.
  - Equation of motion: θ'' = -(g/L)*sin(θ) - damping*ω
  - Integration: 4th-order Runge-Kutta with adaptive sub-stepping near collisions.
  - Collisions: detected when ball-to-ball distance ≤ 2*radius.
    Resolved with impulse exchange using coefficient of restitution (elasticity).

Outputs CSV: frame, time, x1, y1, x2, y2, x3, y3, x4, y4, x5, y5
  where frame is the output index (0, 1, 2, …) and (xi, yi) is the center of ball i in world coordinates.
"""

import math
import csv
import sys

# ── Parameters (hardcoded magic values matching myphysicslab defaults) ────────

NUM_BALLS      = 5
PENDULUM_LEN   = 7.52       # rod length (meters)
BALL_RADIUS    = 1.217478/2        # ball radius
ELASTICITY     = 0.9        # coefficient of restitution (1 = perfectly elastic)
DAMPING        = 0.1        # angular damping coefficient
GRAVITY        = 32.0       # gravitational acceleration (myphysicslab uses 32)
GAP_DISTANCE   = 0.0        # extra gap between resting balls
DIST_TOL       = 0.01       # distance tolerance (used for tiny gap like myphysicslab)

# Initial condition: first ball pulled back by 45 degrees
START_ANGLE    = -math.pi / 4  # radians (negative = pulled to the left)

# Simulation timing
DT             = 1.0 / 240  # output time-step (seconds)
T_END          = 24.0       # total simulation time
COLLISION_DT   = 1e-6       # tiny step used to advance past collision instant

# ── Derived geometry ──────────────────────────────────────────────────────────

def build_pivots(n, radius, gap_distance, dist_tol):
    """Return list of (px, py) pivot positions, evenly spaced horizontally.

    Matches myphysicslab: each ball is 2*radius wide, with a small gap
    of dist_tol/2 + gap_distance between neighbours.  Pivots sit at y=2.
    """
    between = dist_tol / 2 + gap_distance
    total_width = n * 2 * radius + (n - 1) * between
    x = -total_width / 2 + radius
    pivots = []
    for _ in range(n):
        pivots.append((x, 2.0))  # myphysicslab places pivots at y=2
        x += 2 * radius + between
    return pivots


PIVOTS = build_pivots(NUM_BALLS, BALL_RADIUS, GAP_DISTANCE, DIST_TOL)

# ── State helpers ─────────────────────────────────────────────────────────────

def ball_pos(pivot, theta, length):
    """World position of ball center given pivot, angle, rod length."""
    px, py = pivot
    x = px + length * math.sin(theta)
    y = py - length * math.cos(theta)
    return x, y


def ball_velocity(pivot, theta, omega, length):
    """World velocity of ball center."""
    vx = length * omega * math.cos(theta)
    vy = length * omega * math.sin(theta)
    return vx, vy

# ── Equations of motion ──────────────────────────────────────────────────────

def deriv(theta, omega):
    """Return (dtheta/dt, domega/dt) for a single pendulum."""
    dtheta = omega
    domega = -(GRAVITY / PENDULUM_LEN) * math.sin(theta) - DAMPING * omega
    return dtheta, domega


def rk4_step(theta, omega, dt):
    """One RK4 step for a single pendulum."""
    k1t, k1o = deriv(theta, omega)
    k2t, k2o = deriv(theta + 0.5*dt*k1t, omega + 0.5*dt*k1o)
    k3t, k3o = deriv(theta + 0.5*dt*k2t, omega + 0.5*dt*k2o)
    k4t, k4o = deriv(theta + dt*k3t, omega + dt*k3o)
    new_theta = theta + (dt/6)*(k1t + 2*k2t + 2*k3t + k4t)
    new_omega = omega + (dt/6)*(k1o + 2*k2o + 2*k3o + k4o)
    return new_theta, new_omega

# ── Collision detection & resolution ─────────────────────────────────────────

def detect_collisions(thetas):
    """Return list of colliding pairs (i, i+1) where balls overlap."""
    pairs = []
    for i in range(NUM_BALLS - 1):
        xi, yi = ball_pos(PIVOTS[i], thetas[i], PENDULUM_LEN)
        xj, yj = ball_pos(PIVOTS[i+1], thetas[i+1], PENDULUM_LEN)
        dist = math.hypot(xj - xi, yj - yi)
        if dist <= 2 * BALL_RADIUS:
            pairs.append(i)
    return pairs


def resolve_collision(i, thetas, omegas):
    """Impulse-based collision between ball i and ball i+1.

    We work in the 1-D tangent-velocity space along the line connecting
    the two ball centres.  For equal-mass balls with coefficient of
    restitution e the post-collision velocities are:

        v1' = v1 + (1+e)/2 * (v2 - v1)   (projected onto normal)
        v2' = v2 - (1+e)/2 * (v2 - v1)

    We convert world-space ball velocity → normal component, apply the
    impulse, then convert back to angular velocity.
    """
    j = i + 1
    # Ball positions
    xi, yi = ball_pos(PIVOTS[i], thetas[i], PENDULUM_LEN)
    xj, yj = ball_pos(PIVOTS[j], thetas[j], PENDULUM_LEN)

    # Collision normal (from i to j)
    dx, dy = xj - xi, yj - yi
    d = math.hypot(dx, dy)
    if d < 1e-12:
        return  # degenerate
    nx, ny = dx / d, dy / d

    # Ball velocities
    vxi, vyi = ball_velocity(PIVOTS[i], thetas[i], omegas[i], PENDULUM_LEN)
    vxj, vyj = ball_velocity(PIVOTS[j], thetas[j], omegas[j], PENDULUM_LEN)

    # Relative velocity along normal
    dvn = (vxj - vxi) * nx + (vyj - vyi) * ny
    if dvn > 0:
        return  # separating — no collision

    # Impulse scalar for equal masses
    impulse = (1 + ELASTICITY) / 2 * dvn  # note: equal mass simplification

    # New world velocities
    new_vxi = vxi + impulse * nx
    new_vyi = vyi + impulse * ny
    new_vxj = vxj - impulse * nx
    new_vyj = vyj - impulse * ny

    # Convert back to angular velocity:
    #   vx = L * ω * cos(θ),  vy = L * ω * sin(θ)
    #   ω  = (vx * cos(θ) + vy * sin(θ)) / L
    ct_i, st_i = math.cos(thetas[i]), math.sin(thetas[i])
    ct_j, st_j = math.cos(thetas[j]), math.sin(thetas[j])

    omegas[i] = (new_vxi * ct_i + new_vyi * st_i) / PENDULUM_LEN
    omegas[j] = (new_vxj * ct_j + new_vyj * st_j) / PENDULUM_LEN

# ── Simulation stepper with sub-stepped collision handling ────────────────────

def advance(thetas, omegas, dt):
    """Advance all pendulums by dt, detecting and resolving collisions.

    Strategy: take the full RK4 step, then check for collisions.  If any
    are found, resolve them simultaneously (handles the chain-impulse
    behaviour of Newton's cradle correctly for equal-mass balls).
    """
    # 1. Integrate each pendulum independently
    new_thetas = list(thetas)
    new_omegas = list(omegas)
    for k in range(NUM_BALLS):
        new_thetas[k], new_omegas[k] = rk4_step(thetas[k], omegas[k], dt)

    # 2. Detect and resolve collisions (iterate to propagate chain impulses)
    max_iters = 2 * NUM_BALLS  # enough for impulse to propagate end-to-end
    for _ in range(max_iters):
        pairs = detect_collisions(new_thetas)
        if not pairs:
            break
        for idx in pairs:
            resolve_collision(idx, new_thetas, new_omegas)

    return new_thetas, new_omegas

# ── Main ──────────────────────────────────────────────────────────────────────

def run(out=sys.stdout):
    # Initial state
    thetas = [0.0] * NUM_BALLS
    omegas = [0.0] * NUM_BALLS
    thetas[0] = START_ANGLE
    thetas[1] = START_ANGLE
    thetas[2] = START_ANGLE

    writer = csv.writer(out)
    writer.writerow(["frame", "time"] + [f"{c}{i+1}" for i in range(NUM_BALLS) for c in ("x", "y")])

    t = 0.0
    steps = int(round(T_END / DT))
    for step in range(steps + 1):
        t = step * DT
        if step % 5 == 0:
            # Write positions
            row = [str(int(step/5)), f"{t:.6f}"]
            for k in range(NUM_BALLS):
                x, y = ball_pos(PIVOTS[k], thetas[k], PENDULUM_LEN)
                row.append(f"{x:.6f}")
                row.append(f"{y:.6f}")
            writer.writerow(row)

        # Advance (sub-step for better collision accuracy)
        sub_steps = 4
        sub_dt = DT / sub_steps
        for _ in range(sub_steps):
            thetas, omegas = advance(thetas, omegas, sub_dt)


if __name__ == "__main__":
    if len(sys.argv) > 1:
        with open(sys.argv[1], "w", newline="") as f:
            run(out=f)
        print(f"Wrote {sys.argv[1]}")
    else:
        run()