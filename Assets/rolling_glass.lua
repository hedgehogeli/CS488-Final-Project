-- Rolling glass sphere using the same glass/camera baseline as mirror-cow.lua.
-- Run the A5 executable with cwd = this directory (Assets/), e.g.:
--   cd Assets && ../A5 rolling_glass.lua
-- Or use ../render_animation.sh from the A5 directory.
--
-- Environment variables:
--   FRAME   frame index (default 0)
--   FPS     frames per second used for time t = FRAME/FPS (default depends on script; driver sets it)
--   QUALITY "low" or "high" (default low) — selects resolution + renderer preset
--   OUTFILE output PNG path (default frame_0000.png)

local function getenv(name, default)
  local v = os.getenv(name)
  if v == nil or v == "" then
    return default
  end
  return v
end

local frame = tonumber(getenv("FRAME", "0"))
local fps = tonumber(getenv("FPS", "24"))
local quality = getenv("QUALITY", "low")
local outfile = getenv("OUTFILE", "frame_0000.png")

local pi = math.pi

local w, h
if quality == "low" then
  w, h = 300, 300
else
  w, h = 500, 500
end

local t = frame / fps
local radius = 1.8
local x0 = -5.0
local speed = 1.5
local x = x0 + speed * t
-- Roll without slip: theta = -arc / r (radians); positive X motion => rotation about +Z is negative.
local theta_deg = -((x - x0) / radius) * (180.0 / pi)

local grass = gr.material({0.1, 0.7, 0.1}, {0.0, 0.0, 0.0}, 0)
local glass = gr.material({0.1, 0.2, 0.25}, {0.4, 0.5, 0.6}, 80, 0.85, 1.5)
local marker = gr.material({1.0, 0.6, 0.1}, {0.5, 0.5, 0.5}, 30)

scene = gr.node("scene")
scene:rotate("X", 23)

plane = gr.mesh("plane", "plane.obj")
scene:add_child(plane)
plane:set_material(grass)
plane:scale(30, 30, 30)

-- A bright marker behind the sphere makes the refraction easier to read.
reference = gr.nh_box("reference", {0, 0, 0}, 1)
scene:add_child(reference)
reference:set_material(marker)
reference:scale(2.0, 3.0, 2.0)
reference:translate(4.0, 1.5, 8.0)

ball = gr.node("ball")
scene:add_child(ball)
sphere = gr.nh_sphere("sphere", {0, 0, 0}, radius)
ball:add_child(sphere)
sphere:set_material(glass)
ball:rotate("Z", theta_deg)
ball:translate(x, radius, 16.0)

key_light = gr.light({200, 202, 430}, {0.8, 0.8, 0.8}, {1, 0, 0})

gr.render(scene, outfile, w, h,
  {0, 0.5, 25}, {0, 0, -1}, {0, 1, 0}, 80,
  {0.4, 0.4, 0.4}, {key_light}, quality)
