-- Macho cows with a reflective/refractive surface under one cow.
-- Based on macho-cows.lua. Uses transparency + refractiveIndex for reflection/refraction.

-- We'll need an extra function that knows how to read Wavefront .OBJ
-- files.

stone = gr.material({0.8, 0.7, 0.7}, {0.0, 0.0, 0.0}, 0)
grass = gr.material({0.1, 0.7, 0.1}, {0.0, 0.0, 0.0}, 0)
hide = gr.material({0.84, 0.6, 0.53}, {0.3, 0.3, 0.3}, 20)
-- Mirror-like surface: transparency (blend with local), refractiveIndex (e.g. glass 1.5)
mirror = gr.material({0.05, 0.05, 0.05}, {0.8, 0.8, 0.8}, 100, 0.05, 100.0)
-- ks, kd, shininess, transparency, ior
-- Glass sphere in front of camera: refractive index 1.5 
glass = gr.material({0.1, 0.2, 0.25}, {0.4, 0.5, 0.6}, 80, 0.85, 1.5)

-- ##############################################
-- the arch
-- ##############################################

inst = gr.node('inst')

arc = gr.node('arc')
inst:add_child(arc)
arc:translate(0, 0, -10)

p1 = gr.nh_box('p1', {0, 0, 0}, 1)
arc:add_child(p1)
p1:set_material(stone)
p1:scale(0.8, 4, 0.8)
p1:translate(-2.4, 0, -0.4)

p2 = gr.nh_box('p2', {0, 0, 0}, 1)
arc:add_child(p2)
p2:set_material(stone)
p2:scale(0.8, 4, 0.8)
p2:translate(1.6, 0, -0.4)

s = gr.nh_sphere('s', {0, 0, 0}, 1)
arc:add_child(s)
s:set_material(stone)
s:scale(4, 0.6, 0.6)
s:translate(0, 4, 0)


-- #############################################
-- Read in the cow model from a separate file.
-- #############################################

cow_poly = gr.mesh('cow', 'cow.obj')
factor = 2.0/(2.76+3.637)

cow_poly:set_material(hide)

cow_poly:translate(0.0, 3.637, 0.0)
cow_poly:scale(factor, factor, factor)
cow_poly:translate(0.0, -1.0, 0.0)

-- ##############################################
-- the scene
-- ##############################################

scene = gr.node('scene')
scene:rotate('X', 23)

-- the floor

plane = gr.mesh('plane', 'plane.obj' )
scene:add_child(plane)
plane:set_material(grass)
plane:scale(30, 30, 30)

-- Mirror under the first cow: flat box at (1, 1.3, 14)
mirror_plane = gr.node('mirror_plane')
scene:add_child(mirror_plane)
mirror_box = gr.nh_box('mirror_box', {0, 0, 0}, 1)
mirror_plane:add_child(mirror_box)
mirror_box:set_material(mirror)
mirror_box:scale(6, 0.02, 20)
mirror_plane:translate(0, 0.05, 0)

-- Refractive sphere in front of camera (eye at 0,2,30 looking -Z; sphere at z=18)
refractive_sphere = gr.nh_sphere('refractive_sphere', {0, 4, 16 }, 1.8)
scene:add_child(refractive_sphere)
refractive_sphere:set_material(glass)

-- Construct a central altar in the shape of a buckyball.  The
-- buckyball at the centre of the real Stonehenge was destroyed
-- in the great fire of 733 AD.

buckyball = gr.mesh( 'buckyball', 'buckyball.obj' )
scene:add_child(buckyball)
buckyball:set_material(stone)
buckyball:scale(1.5, 1.5, 1.5)
buckyball:translate(0, 5, 0)

-- Use the instanced cow model to place some actual cows in the scene.
-- For convenience, do this in a loop.

cow_number = 1

for _, pt in pairs({
		      {{1,1.3,14}, 20},
		      {{5,1.3,-11}, 180},
		      {{-5.5,1.3,-3}, -60}}) do
   cow_instance = gr.node('cow' .. tostring(cow_number))
   scene:add_child(cow_instance)
   cow_instance:add_child(cow_poly)
   cow_instance:scale(1.4, 1.4, 1.4)
   cow_instance:rotate('Y', pt[2])
   cow_instance:translate(table.unpack(pt[1]))
   
   cow_number = cow_number + 1
end

-- Place a ring of arches.

for i = 1, 6 do
   an_arc = gr.node('arc' .. tostring(i))
   an_arc:rotate('Y', (i-1) * 60)
   scene:add_child(an_arc)
   an_arc:add_child(arc)
end

gr.render(scene,
	  'mirror-cow.png', 300, 300,
	  {0, 2, 30}, {0, 0, -1}, {0, 1, 0}, 40,
	  {0.4, 0.4, 0.4}, {gr.light({200, 202, 430}, {0.8, 0.8, 0.8}, {1, 0, 0})})
