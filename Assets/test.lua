
gold = gr.material({0.9, 0.8, 0.4}, {0.8, 0.8, 0.4}, 25)
grass = gr.material({0.1, 0.7, 0.1}, {0.0, 0.0, 0.0}, 0)
glass = gr.material({0.1, 0.2, 0.25}, {0.4, 0.5, 0.6}, 80, 0.85, 1.5)
-- chrome = gr.material({0.95, 0.95, 0.95}, {0.05, 0.05, 0.055}, 20, 0.05, 100.0)
chrome = gr.material({0.5, 0.5, 0.6}, {0.85, 0.85, 0.855}, 100)
matte_black_plastic = gr.material({0.06, 0.06, 0.06}, {0.05, 0.05, 0.05}, 8)

cylinder_mat = gr.material({0.2, 0.45, 0.85}, {0.2, 0.2, 0.3}, 35)
-- gr.medium_material(scatteringColour, sigmaA, sigmaS [, densityScale [, stepSize]])
smoke_medium = gr.medium_material(
	{0.3, 0.3, 0.3},
	{0.015, 0.015, 0.02},
	{0.06, 0.06, 0.08},
	1.0,
	0.1
)
-- cloud_medium = gr.medium_material(
-- 	{1.0, 1.0, 1.0},       -- scattering colour (white)
-- 	{0.0001, 0.0001, 0.0001},  -- sigmaA (very low absorption for a white cloud)
-- 	{0.6, 0.6, 0.6},        -- sigmaS (much higher scattering)
-- 	0.1,                     -- densityScale
-- 	0.5                      -- stepSize
-- )
local function make_cloud_medium()
	return gr.medium_material(
		{1.0, 1.0, 1.0},              -- scattering colour (white)
		{0.0001, 0.0001, 0.0001},      -- sigmaA (near-zero absorption)
		{0.6, 0.6, 0.6},              -- sigmaS (heavy scattering)
		0.1,                           -- densityScale
		0.8                            -- stepSize
	)
end
mat_wood = gr.textured_material(
  {1.0, 1.0, 1.0}, {0.2, 0.2, 0.2}, 25,
  "wood_table_worn_diff_4k.png",
  "wood_table_worn_nor_gl_4k.png"
)

mat_floor = gr.textured_material(
	{1.0, 1.0, 1.0}, {0.2, 0.2, 0.2}, 25,
	"gravelly_sand_diff_4k.png",
	"gravelly_sand_nor_gl_4k.png",
	{10, 10}
)

-------------------------------------------------------------------------

cradle_y_offset = 11

scene = gr.node('scene')
-- scene:rotate('X', 10)
-- scene:rotate('Y', 180)

-- scene:translate(0, -4, -10)

-- raw csv ball position data: 
-- frame,time,x1,y1,x2,y2,x3,y3,x4,y4,x5,y5
-- 0,0.000000,-7.762399,-3.317443,-6.539921,-3.317443,-5.317443,-3.317443,1.222478,-5.520000,2.444956,-5.520000

-- y is translated up by 6.52
local y_offset = 6.52 + cradle_y_offset  -- = PENDULUM_LENGTH - 1 + offset_within_scene
-- local balls = {
-- 	{ -5.238427, -0.828427 + y_offset, 0.0 },
-- 	{ -4.029268, -0.828427 + y_offset, 0.0 },
-- 	{ -2.820108, -0.828427 + y_offset, 0.0 },
-- 	{ 1.217478, -2.000000 + y_offset, 0.0 },
-- 	{ 2.426638, -2.000000 + y_offset, 0.0 },
-- }
local balls = {
	{ -7.762399,-3.317443 + y_offset, 0.0 },
	{ -6.539921,-3.317443 + y_offset, 0.0 },
	{ -5.317443,-3.317443 + y_offset, 0.0 },
	{ 1.222478,-5.520000 + y_offset, 0.0 },
	{ 2.444956,-5.520000 + y_offset, 0.0 },
}

-- TABLE STUFF

wood_table_parent = gr.node('wood_table_parent')
scene:add_child(wood_table_parent)
wood_table_parent:translate(0, -0.5, 0)

local table_width = 20
local table_length = 20
local table_leg_radius = 1.0 -- use 1.0 if want ikea lack look
local table_leg_height = cradle_y_offset - 2

wood_table = gr.cube('wood_table')
wood_table_parent:add_child(wood_table)
wood_table:set_material(mat_wood)
wood_table:scale(table_width, 2, table_length)
wood_table:translate(-table_width/2, cradle_y_offset-2, -table_length/2) -- -2 from the scale ^


local function add_table_leg(parent, name, cx, cz)
	local leg = gr.cube(name)
	parent:add_child(leg)
	leg:set_material(mat_wood)
	local leg_w = 2 * table_leg_radius
	leg:scale(leg_w, table_leg_height, leg_w)
	-- unit cube [0,1]^3: bottom at y=0, leg center in xz at (cx, cz)
	leg:translate(cx - table_leg_radius, 0, cz - table_leg_radius)
end
local inset = table_leg_radius
add_table_leg(wood_table_parent, 'table_leg_pp',  table_width / 2 - inset,  table_length / 2 - inset)
add_table_leg(wood_table_parent, 'table_leg_np', -table_width / 2 + inset,  table_length / 2 - inset)
add_table_leg(wood_table_parent, 'table_leg_pn',  table_width / 2 - inset, -table_length / 2 + inset)
add_table_leg(wood_table_parent, 'table_leg_nn', -table_width / 2 + inset, -table_length / 2 + inset)



-- FRAME STUFF 

local post_height = 8.5
local post_diameter = 0.25
local post_spacing = 8.5
local half_post_spacing = post_spacing / 2
local halves_spacing = 4.5 -- distance between the two U's 

-- Frame: two rigid sides (pos/neg z), each with two posts, a beam, and cap spheres.
-- Top-level cradle holds both sides.
cradle = gr.node('cradle')
scene:add_child(cradle)
cradle:translate(0, cradle_y_offset, 0)

side_pos_z = gr.node('side_pos_z')
cradle:add_child(side_pos_z)
side_pos_z:translate(0, 0, halves_spacing/2)

side_neg_z = gr.node('side_neg_z')
cradle:add_child(side_neg_z)
side_neg_z:translate(0, 0, -halves_spacing/2)

-- Post centres at x = ±half_post_spacing in each side's local frame; canonical cylinder y in [0,1] scaled to height post_height.
local function add_post(side, name, x)
	local p = gr.cylinder(name)
	side:add_child(p)
	p:set_material(chrome)
	p:scale(post_diameter, post_height, post_diameter)
	p:translate(x, 0, 0)
end

-- Unit sphere centred at top of post (y = post_height).
local function add_cap_sphere(side, name, x)
	local s = gr.sphere(name)
	side:add_child(s)
	s:set_material(chrome)
	s:scale(post_diameter, post_diameter, post_diameter)
	s:translate(x, post_height, 0)
end

-- Horizontal beam along x between the two posts; same radius as posts (post_diameter).
local function add_beam(side, name)
	local beam = gr.cylinder(name)
	side:add_child(beam)
	beam:set_material(chrome)
	beam:scale(post_diameter, post_height  , post_diameter)
	beam:rotate('Z', -90)
	beam:translate(-half_post_spacing , post_height , 0)
end

add_post(side_pos_z, 'post_pos_z_l', -half_post_spacing)
add_post(side_pos_z, 'post_pos_z_r', half_post_spacing)
add_cap_sphere(side_pos_z, 'cap_pos_z_l', -half_post_spacing)
add_cap_sphere(side_pos_z, 'cap_pos_z_r', half_post_spacing)
add_beam(side_pos_z, 'beam_pos_z')

add_post(side_neg_z, 'post_neg_z_l', -half_post_spacing)
add_post(side_neg_z, 'post_neg_z_r', half_post_spacing)
add_cap_sphere(side_neg_z, 'cap_neg_z_l', -half_post_spacing)
add_cap_sphere(side_neg_z, 'cap_neg_z_r', half_post_spacing)
add_beam(side_neg_z, 'beam_neg_z')

-- Rounded rectangular base (1 unit thick): cube core + 4 edge cylinders + 4 corner spheres.
local base_thickness = 1.0
local base_width = post_spacing + 2.5
local base_depth = halves_spacing + 2.5
local base_corner_radius = 0.5

local base_core_width = base_width - 2 * base_corner_radius
local base_core_depth = base_depth - 2 * base_corner_radius

local cradle_base = gr.node('cradle_base')
cradle:add_child(cradle_base)
-- Sink half of the base into the table (0.5) and drop the base by 0.5.
cradle_base:translate(0, -1.0, 0)

local base_core = gr.cube('base_core')
cradle_base:add_child(base_core)
base_core:set_material(matte_black_plastic)
base_core:scale(base_core_width, base_thickness, base_core_depth)
base_core:translate(-base_core_width / 2, 0, -base_core_depth / 2)

local function add_base_edge_x(parent, name, z_pos)
	local edge = gr.cylinder(name)
	parent:add_child(edge)
	edge:set_material(matte_black_plastic)
	edge:scale(base_corner_radius, base_core_width, base_corner_radius)
	edge:rotate('Z', -90)
	edge:translate(-base_core_width / 2, base_thickness / 2, z_pos)
end

local function add_base_edge_z(parent, name, x_pos)
	local edge = gr.cylinder(name)
	parent:add_child(edge)
	edge:set_material(matte_black_plastic)
	edge:scale(base_corner_radius, base_core_depth, base_corner_radius)
	edge:rotate('X', 90)
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

add_base_edge_x(cradle_base, 'base_edge_x_pos', edge_z)
add_base_edge_x(cradle_base, 'base_edge_x_neg', -edge_z)
add_base_edge_z(cradle_base, 'base_edge_z_pos', edge_x)
add_base_edge_z(cradle_base, 'base_edge_z_neg', -edge_x)

add_base_corner(cradle_base, 'base_corner_pp', edge_x, edge_z)
add_base_corner(cradle_base, 'base_corner_pn', edge_x, -edge_z)
add_base_corner(cradle_base, 'base_corner_np', -edge_x, edge_z)
add_base_corner(cradle_base, 'base_corner_nn', -edge_x, -edge_z)

-- FRAME STUFF DONE

-- BALL STUFF + CONNECTING WIRE TO FRAME
-- Each ball is a group: gold sphere + vertical cylinder from sphere top to the frame beam.

local ball_radius = 0.608739

local ball_connector_radius = 0.06
local ball_connector_length = 0.18

-- wire attaches to the beam at its 45 degrees
-- Beam centre is at y = post_height + post_diameter/2 (from add_beam translate),
-- and side centres are at z = +/- halves_spacing/2.
-- Using the inward/downward 45deg point on the beam cross-section.
local beam_y = post_height - post_diameter * math.sqrt(2)/2 + cradle_y_offset
local beam_z = halves_spacing/2 - post_diameter * math.sqrt(2)/2
local beam_x = {-2.426638, -1.217478, 0, 1.217478, 2.426638}
-- beam attachment points are (beam_x, beam_y, +beam_z) and (beam_x, beam_y, -beam_z)

local wire_radius = 0.03

local pi = math.pi

local function clamp(v, lo, hi)
	if v < lo then return lo end
	if v > hi then return hi end
	return v
end

local function atan2(y, x)
	if math.abs(x) < 1e-8 then
		if y > 0 then return pi/2 end
		if y < 0 then return -pi/2 end
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
	-- Rotate +Y (canonical cylinder axis) to the target direction using Z then X.
	local z_angle = math.deg(-math.asin(clamp(nx, -1.0, 1.0)))
	local x_angle = math.deg(atan2(nz, ny))

	local wire = gr.cylinder(name)
	parent:add_child(wire)
	wire:set_material(glass)
	wire:scale(wire_radius, len, wire_radius)
	wire:rotate('Z', z_angle)
	wire:rotate('X', x_angle)
	wire:translate(start_pt[1], start_pt[2], start_pt[3])
end

for i = 1, #balls do
	local bx, by, bz = table.unpack(balls[i])
	local dx_to_beam = beam_x[i] - bx
	local dy_to_beam = beam_y - by
	local swing_theta_rad = atan2(-dx_to_beam, dy_to_beam)
	local swing_theta_deg = math.deg(swing_theta_rad)

	local grp = gr.node('ball' .. tostring(i))
	scene:add_child(grp)
	grp:rotate('Z', swing_theta_deg)
	grp:translate(bx, by, bz)

	local b = gr.sphere('ball' .. tostring(i) .. '_sphere')
	grp:add_child(b)
	b:set_material(glass)
	b:scale(ball_radius, ball_radius, ball_radius)

	local w = gr.cylinder('ball' .. tostring(i) .. '_connector')
	grp:add_child(w)
	w:set_material(glass)
	w:scale(ball_connector_radius, ball_connector_length, ball_connector_radius)
	w:rotate('X', 90)
	w:translate(0, ball_radius, -ball_connector_length/2)

	-- Connector endpoints in group-local space (before grp transform).
	local connector_p_neg = {0, ball_radius, -ball_connector_length/2}
	local connector_p_pos = {0, ball_radius,  ball_connector_length/2}

	local s = math.sin(swing_theta_rad)
	local c = math.cos(swing_theta_rad)
	local function to_world(local_pt)
		return {
			bx + (-s) * local_pt[2],
			by + ( c) * local_pt[2],
			bz + local_pt[3]
		}
	end

	local world_p_neg = to_world(connector_p_neg)
	local world_p_pos = to_world(connector_p_pos)

	local beam_neg = {beam_x[i], beam_y, -beam_z}
	local beam_pos = {beam_x[i], beam_y,  beam_z}

	add_wire(scene, 'ball' .. tostring(i) .. '_wire_neg', world_p_neg, beam_neg)
	add_wire(scene, 'ball' .. tostring(i) .. '_wire_pos', world_p_pos, beam_pos)

end



-- plane = gr.mesh('plane', 'plane.obj')
-- scene:add_child(plane)
-- plane:set_material(grass)
-- plane:scale( 50,  50,  50)

ground = gr.cube('ground')
scene:add_child(ground)
ground:set_material(mat_floor)
ground:scale(250, 1, 250)
ground:translate(-125, -1.5 , -125)

cloud1 = gr.cube('cloud1')
scene:add_child(cloud1)
cloud1:set_material(make_cloud_medium())
-- cloud1:set_material(cylinder_mat)
cloud1:scale(25, 5, 25)
cloud1:translate(-30, 35, -60)

cloud2 = gr.cube('cloud2')
scene:add_child(cloud2)
cloud2:set_material(make_cloud_medium())
-- cloud2:set_material(cylinder_mat)
cloud2:scale(40, 5, 25)
cloud2:translate(-40, 45, -60)

local function add_cloud(name, sx, sy, sz, tx, ty, tz)
  local c = gr.cube(name)
  scene:add_child(c)
  c:set_material(make_cloud_medium())
  c:scale(sx, sy, sz)
  c:translate(tx, ty, tz)
end


add_cloud('cloud3',  12.0, 2.0, 9.0, -47.0, 30.5, -34.2)
add_cloud('cloud4',  13.5, 2.6, 12.5, -34.2, 30.9, -33.6)
add_cloud('cloud5',  14.5, 1.8, 35.5, -14.9, 28.6, -45.0)
add_cloud('cloud6',  14.8, 2.2, 8.0,  -5.3, 31.1, -32.9)
add_cloud('cloud7',  16.0, 2.1, 14.5, -44.5, 34.2, -31.8)
add_cloud('cloud8',  15.0, 1.8, 13.0, -27.7, 34.6, -34.6)
add_cloud('cloud9',  14.2, 1.6, 12.2, -11.9, 34.4, -32.4)
add_cloud('cloud10', 18.0, 2.0, 13.5, -37.2, 36.9, -34.4)
add_cloud('cloud11', 18.4, 2.7, 15.0, -18.4, 37.1, -33.1)
add_cloud('cloud12', 20.0, 2.8, 14.0, -32.0, 40.4, -34.3)

l1 = gr.light({200, 200, 400}, {1.0, 1.0, 1.0}, {1, 0, 0})

gr.render(scene, 'test.png', 
	1920, 1080,
	--   {0, 4, 15}, {0, 0, -1}, {0, 1, 0}, 80, -- (eye, view direction, up, fov)
	  {0, cradle_y_offset + 6, 30}, {0,  -0.0, -1}, {0, 1, 0}, 60, -- (eye, view direction, up, fov)
	  {0.4, 0.4, 0.4}, {l1})

-- gr.render(scene, filename, width, height,
--   eye, view, up, fov, ambient, lights [, quality])
--   scene    : root gr.node of the scene graph
--   filename : output PNG path
--   width, height : image resolution in pixels
--   eye      : camera position (world space), vec3
--   view     : view direction, vec3
--   up       : camera up vector, vec3
--   fov      : vertical field of view in degrees
--   ambient  : global ambient light RGB, vec3
--   lights   : table of at least one gr.light, e.g. { l1, l2 }
--   quality  : optional string; render preset if provided