local unpack = table.unpack or unpack

local function getenv(name, default)
  local v = os.getenv(name)
  if v == nil or v == "" then
    return default
  end
  return v
end

local function split_csv_line(line)
  local fields = {}
  for field in string.gmatch(line, "([^,]+)") do
    fields[#fields + 1] = field
  end
  return fields
end

local function clamp(v, lo, hi)
  if v < lo then return lo end
  if v > hi then return hi end
  return v
end

local function normalize3(v)
  local len = math.sqrt(v[1] * v[1] + v[2] * v[2] + v[3] * v[3])
  if len < 1e-8 then
    return {0.0, 0.0, -1.0}
  end
  return {v[1] / len, v[2] / len, v[3] / len}
end

local function load_ball_frames(path, y_offset)
  local frames = {}
  local max_frame = -1
  local file = assert(io.open(path, "r"), "Could not open ball CSV: " .. path)

  local is_header = true
  for line in file:lines() do
    if is_header then
      is_header = false
    elseif line ~= "" then
      local fields = split_csv_line(line)
      local frame_idx = tonumber(fields[1])
      local balls = {}

      for i = 1, 5 do
        local x = tonumber(fields[2 * i + 1])
        local y = tonumber(fields[2 * i + 2])
        balls[i] = {x, y + y_offset, 0.0}
      end

      frames[frame_idx] = balls
      if frame_idx > max_frame then
        max_frame = frame_idx
      end
    end
  end

  file:close()
  return frames, max_frame
end

local function load_camera_frames(path)
  local frames = {}
  local max_frame = -1
  local file = assert(io.open(path, "r"), "Could not open camera CSV: " .. path)

  local is_header = true
  for line in file:lines() do
    if is_header then
      is_header = false
    elseif line ~= "" then
      local fields = split_csv_line(line)
      local frame_idx = tonumber(fields[1])
      frames[frame_idx] = {
        eye = {tonumber(fields[3]), tonumber(fields[4]), tonumber(fields[5])},
        view = normalize3({tonumber(fields[6]), tonumber(fields[7]), tonumber(fields[8])}),
        fov = tonumber(fields[9]),
      }

      if frame_idx > max_frame then
        max_frame = frame_idx
      end
    end
  end

  file:close()
  return frames, max_frame
end

local frame = tonumber(getenv("FRAME", "0")) or 0
local quality = getenv("QUALITY", "low")
local outfile = getenv("OUTFILE", "cradle.png")

local w, h
if quality == "high" then
  w, h = 1920, 1080
else
  w, h = 960, 540
  quality = "low"
end

local cradle_y_offset = 11
local ball_y_offset = 6.52 + cradle_y_offset

local ball_frames, max_ball_frame = load_ball_frames("../CradleSim/try3/out.csv", ball_y_offset)
local camera_frames, max_camera_frame = load_camera_frames("../CameraSpline/camera_path.csv")
local max_frame = math.min(max_ball_frame, max_camera_frame)
local frame_idx = clamp(frame, 0, max_frame)

local balls = assert(ball_frames[frame_idx], "Missing ball data for frame " .. tostring(frame_idx))
local camera = assert(camera_frames[frame_idx], "Missing camera data for frame " .. tostring(frame_idx))

local gold = gr.material({0.9, 0.8, 0.4}, {0.8, 0.8, 0.4}, 25)
local grass = gr.material({0.1, 0.7, 0.1}, {0.0, 0.0, 0.0}, 0)
local glass = gr.material({0.1, 0.2, 0.25}, {0.4, 0.5, 0.6}, 80, 0.85, 1.5)
local chrome = gr.material({0.5, 0.5, 0.6}, {0.85, 0.85, 0.855}, 20)
local matte_black_plastic = gr.material({0.06, 0.06, 0.06}, {0.05, 0.05, 0.05}, 8)

local function make_cloud_medium()
  return gr.medium_material(
    {1.0, 1.0, 1.0},
    {0.0001, 0.0001, 0.0001},
    {0.6, 0.6, 0.6},
    0.1,
    0.8
  )
end

local mat_wood = gr.textured_material(
  {1.0, 1.0, 1.0}, {0.2, 0.2, 0.2}, 25,
  "wood_table_worn_diff_4k.png",
  "wood_table_worn_nor_gl_4k.png"
)

local mat_floor = gr.textured_material(
  {1.0, 1.0, 1.0}, {0.2, 0.2, 0.2}, 25,
  "gravelly_sand_diff_4k.png",
  "gravelly_sand_nor_gl_4k.png",
  {5, 5}
)

local scene = gr.node("scene")

-- TABLE
local wood_table_parent = gr.node("wood_table_parent")
scene:add_child(wood_table_parent)
wood_table_parent:translate(0, -0.5, 0)

local table_width = 20
local table_length = 20
local table_leg_radius = 1.0
local table_leg_height = cradle_y_offset - 2

local wood_table = gr.cube("wood_table")
wood_table_parent:add_child(wood_table)
wood_table:set_material(mat_wood)
wood_table:scale(table_width, 2, table_length)
wood_table:translate(-table_width / 2, cradle_y_offset - 2, -table_length / 2)

local function add_table_leg(parent, name, cx, cz)
  local leg = gr.cube(name)
  parent:add_child(leg)
  leg:set_material(mat_wood)
  local leg_w = 2 * table_leg_radius
  leg:scale(leg_w, table_leg_height, leg_w)
  leg:translate(cx - table_leg_radius, 0, cz - table_leg_radius)
end

local inset = table_leg_radius
add_table_leg(wood_table_parent, "table_leg_pp",  table_width / 2 - inset,  table_length / 2 - inset)
add_table_leg(wood_table_parent, "table_leg_np", -table_width / 2 + inset,  table_length / 2 - inset)
add_table_leg(wood_table_parent, "table_leg_pn",  table_width / 2 - inset, -table_length / 2 + inset)
add_table_leg(wood_table_parent, "table_leg_nn", -table_width / 2 + inset, -table_length / 2 + inset)

-- FRAME
local post_height = 8.5
local post_diameter = 0.25
local post_spacing = 8.5
local half_post_spacing = post_spacing / 2
local halves_spacing = 4.5

local cradle = gr.node("cradle")
scene:add_child(cradle)
cradle:translate(0, cradle_y_offset, 0)

local side_pos_z = gr.node("side_pos_z")
cradle:add_child(side_pos_z)
side_pos_z:translate(0, 0, halves_spacing / 2)

local side_neg_z = gr.node("side_neg_z")
cradle:add_child(side_neg_z)
side_neg_z:translate(0, 0, -halves_spacing / 2)

local function add_post(side, name, x)
  local p = gr.cylinder(name)
  side:add_child(p)
  p:set_material(chrome)
  p:scale(post_diameter, post_height, post_diameter)
  p:translate(x, 0, 0)
end

local function add_cap_sphere(side, name, x)
  local s = gr.sphere(name)
  side:add_child(s)
  s:set_material(chrome)
  s:scale(post_diameter, post_diameter, post_diameter)
  s:translate(x, post_height, 0)
end

local function add_beam(side, name)
  local beam = gr.cylinder(name)
  side:add_child(beam)
  beam:set_material(chrome)
  beam:scale(post_diameter, post_height, post_diameter)
  beam:rotate("Z", -90)
  beam:translate(-half_post_spacing, post_height, 0)
end

add_post(side_pos_z, "post_pos_z_l", -half_post_spacing)
add_post(side_pos_z, "post_pos_z_r", half_post_spacing)
add_cap_sphere(side_pos_z, "cap_pos_z_l", -half_post_spacing)
add_cap_sphere(side_pos_z, "cap_pos_z_r", half_post_spacing)
add_beam(side_pos_z, "beam_pos_z")

add_post(side_neg_z, "post_neg_z_l", -half_post_spacing)
add_post(side_neg_z, "post_neg_z_r", half_post_spacing)
add_cap_sphere(side_neg_z, "cap_neg_z_l", -half_post_spacing)
add_cap_sphere(side_neg_z, "cap_neg_z_r", half_post_spacing)
add_beam(side_neg_z, "beam_neg_z")

local base_thickness = 1.0
local base_width = post_spacing + 2.5
local base_depth = halves_spacing + 2.5
local base_corner_radius = 0.5

local base_core_width = base_width - 2 * base_corner_radius
local base_core_depth = base_depth - 2 * base_corner_radius

local cradle_base = gr.node("cradle_base")
cradle:add_child(cradle_base)
cradle_base:translate(0, -1.0, 0)

local base_core = gr.cube("base_core")
cradle_base:add_child(base_core)
base_core:set_material(matte_black_plastic)
base_core:scale(base_core_width, base_thickness, base_core_depth)
base_core:translate(-base_core_width / 2, 0, -base_core_depth / 2)

local function add_base_edge_x(parent, name, z_pos)
  local edge = gr.cylinder(name)
  parent:add_child(edge)
  edge:set_material(matte_black_plastic)
  edge:scale(base_corner_radius, base_core_width, base_corner_radius)
  edge:rotate("Z", -90)
  edge:translate(-base_core_width / 2, base_thickness / 2, z_pos)
end

local function add_base_edge_z(parent, name, x_pos)
  local edge = gr.cylinder(name)
  parent:add_child(edge)
  edge:set_material(matte_black_plastic)
  edge:scale(base_corner_radius, base_core_depth, base_corner_radius)
  edge:rotate("X", 90)
  edge:translate(x_pos, base_thickness / 2, -base_core_depth / 2)
end

local function add_base_corner(parent, name, x_pos, z_pos)
  local corner = gr.sphere(name)
  parent:add_child(corner)
  corner:set_material(matte_black_plastic)
  corner:scale(base_corner_radius, base_corner_radius, base_corner_radius)
  corner:translate(x_pos, base_thickness / 2, z_pos)
end

local edge_z = base_depth / 2 - base_corner_radius
local edge_x = base_width / 2 - base_corner_radius

add_base_edge_x(cradle_base, "base_edge_x_pos", edge_z)
add_base_edge_x(cradle_base, "base_edge_x_neg", -edge_z)
add_base_edge_z(cradle_base, "base_edge_z_pos", edge_x)
add_base_edge_z(cradle_base, "base_edge_z_neg", -edge_x)

add_base_corner(cradle_base, "base_corner_pp", edge_x, edge_z)
add_base_corner(cradle_base, "base_corner_pn", edge_x, -edge_z)
add_base_corner(cradle_base, "base_corner_np", -edge_x, edge_z)
add_base_corner(cradle_base, "base_corner_nn", -edge_x, -edge_z)

-- BALLS AND WIRES
local ball_radius = 0.608739
local ball_connector_radius = 0.06
local ball_connector_length = 0.18

local beam_y = post_height - post_diameter * math.sqrt(2) / 2 + cradle_y_offset
local beam_z = halves_spacing / 2 - post_diameter * math.sqrt(2) / 2
local beam_x = {-2.426638, -1.217478, 0, 1.217478, 2.426638}

local wire_radius = 0.03
local pi = math.pi

local function atan2(y, x)
  if math.abs(x) < 1e-8 then
    if y > 0 then return pi / 2 end
    if y < 0 then return -pi / 2 end
    return 0
  end

  local a = math.atan(y / x)
  if x < 0 then
    if y >= 0 then
      a = a + pi
    else
      a = a - pi
    end
  end
  return a
end

local function add_wire(parent, name, start_pt, end_pt)
  local dx = end_pt[1] - start_pt[1]
  local dy = end_pt[2] - start_pt[2]
  local dz = end_pt[3] - start_pt[3]
  local len = math.sqrt(dx * dx + dy * dy + dz * dz)
  if len < 1e-6 then
    return
  end

  local nx, ny, nz = dx / len, dy / len, dz / len
  local z_angle = math.deg(-math.asin(clamp(nx, -1.0, 1.0)))
  local x_angle = math.deg(atan2(nz, ny))

  local wire = gr.cylinder(name)
  parent:add_child(wire)
  wire:set_material(glass)
  wire:scale(wire_radius, len, wire_radius)
  wire:rotate("Z", z_angle)
  wire:rotate("X", x_angle)
  wire:translate(start_pt[1], start_pt[2], start_pt[3])
end

for i = 1, #balls do
  local bx, by, bz = unpack(balls[i])
  local dx_to_beam = beam_x[i] - bx
  local dy_to_beam = beam_y - by
  local swing_theta_rad = atan2(-dx_to_beam, dy_to_beam)
  local swing_theta_deg = math.deg(swing_theta_rad)

  local grp = gr.node("ball" .. tostring(i))
  scene:add_child(grp)
  grp:rotate("Z", swing_theta_deg)
  grp:translate(bx, by, bz)

  local b = gr.sphere("ball" .. tostring(i) .. "_sphere")
  grp:add_child(b)
  b:set_material(glass)
  b:scale(ball_radius, ball_radius, ball_radius)

  local w_connector = gr.cylinder("ball" .. tostring(i) .. "_connector")
  grp:add_child(w_connector)
  w_connector:set_material(glass)
  w_connector:scale(ball_connector_radius, ball_connector_length, ball_connector_radius)
  w_connector:rotate("X", 90)
  w_connector:translate(0, ball_radius, -ball_connector_length / 2)

  local connector_p_neg = {0, ball_radius, -ball_connector_length / 2}
  local connector_p_pos = {0, ball_radius,  ball_connector_length / 2}

  local s = math.sin(swing_theta_rad)
  local c = math.cos(swing_theta_rad)
  local function to_world(local_pt)
    return {
      bx + (-s) * local_pt[2],
      by + c * local_pt[2],
      bz + local_pt[3]
    }
  end

  local world_p_neg = to_world(connector_p_neg)
  local world_p_pos = to_world(connector_p_pos)

  local beam_neg = {beam_x[i], beam_y, -beam_z}
  local beam_pos = {beam_x[i], beam_y, beam_z}

  add_wire(scene, "ball" .. tostring(i) .. "_wire_neg", world_p_neg, beam_neg)
  add_wire(scene, "ball" .. tostring(i) .. "_wire_pos", world_p_pos, beam_pos)
end

-- ENVIRONMENT
local ground = gr.cube("ground")
scene:add_child(ground)
ground:set_material(mat_floor)
ground:scale(250, 1, 250)
ground:translate(-125, -1.5, -125)



local function add_cloud(name, sx, sy, sz, tx, ty, tz)
  local c = gr.cube(name)
  scene:add_child(c)
  c:set_material(make_cloud_medium())
  c:scale(sx, sy, sz)
  c:translate(tx, ty, tz)
end

-- add_cloud("cloud1", 25, 5, 25, -30, 35, -60)
-- add_cloud("cloud2", 40, 5, 25, -40, 45, -60)
-- add_cloud("cloud3",  12.0, 2.0, 9.0, -47.0, 30.5, -34.2)
-- add_cloud("cloud4",  13.5, 2.6, 12.5, -34.2, 30.9, -33.6)
-- add_cloud("cloud5",  14.5, 1.8, 35.5, -14.9, 28.6, -45.0)
-- add_cloud("cloud6",  14.8, 2.2, 8.0,  -5.3, 31.1, -32.9)
-- add_cloud("cloud7",  16.0, 2.1, 14.5, -44.5, 34.2, -31.8)
-- add_cloud("cloud8",  15.0, 1.8, 13.0, -27.7, 34.6, -34.6)
-- add_cloud("cloud9",  14.2, 1.6, 12.2, -11.9, 34.4, -32.4)
-- add_cloud("cloud10", 18.0, 2.0, 13.5, -37.2, 36.9, -34.4)
-- add_cloud("cloud11", 18.4, 2.7, 15.0, -18.4, 37.1, -33.1)
-- add_cloud("cloud12", 20.0, 2.8, 14.0, -32.0, 40.4, -34.3)
add_cloud("cloud1", 25, 5, 25, 30, 35, 60)
add_cloud("cloud2", 40, 5, 25, 40, 45, 60)
add_cloud("cloud3", 12.0, 2.0, 9.0, 47.8, 30.5, 34.2)
add_cloud("cloud4", 13.5, 2.6, 12.5, 34.2, 30.9, 33.6)
add_cloud("cloud5", 14.5, 1.8, 35.5, 14.9, 28.6, 45.0)
add_cloud("cloud6", 14.8, 2.2, 8.0, 5.3, 31.1, 32.9)
add_cloud("cloud7", 16.0, 2.1, 14.5, 44.5, 34.2, 31.8)
add_cloud("cloud8", 15.0, 1.8, 13.0, 27.7, 34.6, 34.6)
add_cloud("cloud9", 14.2, 1.6, 12.2, 11.9, 34.4, 32.4)
add_cloud("cloud10", 18.0, 2.0, 13.5, 37.2, 36.9, 34.4)
add_cloud("cloud11", 18.4, 2.7, 15.0, 18.4, 37.1, 33.1)
add_cloud("cloud12", 20.0, 2.8, 14.0, 32.0, 40.4, 34.3)

local l1 = gr.light({200, 200, 400}, {1.0, 1.0, 1.0}, {1, 0, 0})

gr.render(
  scene,
  outfile,
  w, h,
  camera.eye, camera.view, {0, 1, 0}, camera.fov,
  {0.8, 0.8, 0.8},
  {l1},
  quality
)
