-- -- Default scene for ../render_animation.sh (first argument defaults to sample.lua).
-- -- Forwards to the rolling glass sphere test.
-- dofile("rolling_glass.lua")

-- Sample scene: double shadows, glass door, mirror strip, reduced stonehenge.
-- Based on mirror-cow.lua.

stone = gr.material({0.8, 0.7, 0.7}, {0.0, 0.0, 0.0}, 0)
-- Grass with procedural bump mapping for a more realistic uneven ground
grass = gr.material({0.08, 0.6, 0.08}, {0.0, 0.0, 0.0}, 0, 0.0, 0.0, 0.2, 0.7)
hide = gr.material({0.84, 0.55, 0.53}, {0.3, 0.3, 0.3}, 20)
-- Mirror-like surface
mirror = gr.material({0.05, 0.05, 0.05}, {0.8, 0.8, 0.8}, 100, 0.05, 100.0)
-- Glass: refractive index 1.5. tuning down cuz looks better
glass = gr.material({0.1, 0.2, 0.25}, {0.4, 0.5, 0.6}, 80, 0.85, 1.3)

-- translucent... ice? this is not physically accurate to ice. 
ice = gr.material({0.1, 0.2, 0.25}, {0.4, 0.5, 0.6}, 80, 0.7, 1.8)


-- Orange for dodecahedrons (at light positions)
orange = gr.material({1.0, 0.6, 0.1}, {0.5, 0.7, 0.5}, 25)

-- ##############################################
-- the arch (reused for stonehenge)
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
-- Cow model
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

-- Mirror strip on the ground (kept)
mirror_plane = gr.node('mirror_plane')
scene:add_child(mirror_plane)
mirror_box = gr.nh_box('mirror_box', {0, 0, 0}, 1)
mirror_plane:add_child(mirror_box)
mirror_box:set_material(mirror)
mirror_box:scale(6, 0.02, 20)
mirror_plane:translate(0, 0.05, 0)

-- Glass sphere (kept)
refractive_sphere = gr.nh_sphere('refractive_sphere', {0, 4, 16 }, 1.8)
scene:add_child(refractive_sphere)
refractive_sphere:set_material(glass)

-- Buckyball: moved to the right slightly
buckyball = gr.mesh( 'buckyball', 'buckyball.obj' )
scene:add_child(buckyball)
buckyball:set_material(glass)
buckyball:scale(1.5, 1.5, 1.5)
buckyball:translate(2.5, 5, 0)

-- Glass door / window pane (tall thin box) to the left of the scene
glass_door = gr.node('glass_door')
scene:add_child(glass_door)
glass_door:translate(-7, 0, 8)
-- Door shape: thin in depth, tall, narrow width
door_pane = gr.nh_box('door_pane', {0, 0, 0}, 1)
glass_door:add_child(door_pane)
door_pane:set_material(glass)
door_pane:scale(2.0, 6.5, 0.15)

-- Cow semi-obscured by the glass door 
cow_door_mesh = gr.mesh('cow_door', 'cow.obj')
cow_door_mesh:set_material(hide)
cow_door_mesh:translate(0.0, 3.637, 0.0)
cow_door_mesh:scale(factor, factor, factor)
cow_door_mesh:translate(0.0, -1.0, 0.0)

cow_behind_door = gr.node('cow_behind_door')
scene:add_child(cow_behind_door)
cow_behind_door:add_child(cow_door_mesh)
cow_behind_door:scale(1.2, 1.2, 1.2)
cow_behind_door:translate(-6.5, 1.3, 6)

-- Two orange dodecahedrons at the light positions 

dodec_left_node = gr.node('dodec_left_node')
scene:add_child(dodec_left_node)
dodec_left_node:translate(150, -200, 65)
dodec_left_node:scale(0.02, 0.02, 0.02)
dodec_left_node:translate(-9, 15.0, 6)

dodec_left = gr.mesh('dodec_left', 'smstdodeca.obj')
dodec_left:set_material(orange)
dodec_left_node:add_child(dodec_left)

dodec_right_node = gr.node('dodec_right_node')
scene:add_child(dodec_right_node)
dodec_right_node:translate(150, -200, 65)
dodec_right_node:scale(0.02, 0.02, 0.02)
dodec_right_node:translate(9, 15.0, 6)

dodec_right = gr.mesh('dodec_right', 'smstdodeca.obj')
dodec_right:set_material(orange)
dodec_right_node:add_child(dodec_right)

-- cow demonstrate double shadow
cow_moo = gr.mesh('moo', 'cow.obj')
cow_moo:set_material(hide)
cow_moo:translate(0, 3.637, 0)
cow_moo:scale(factor, factor, factor)
cow_moo:scale(2, 2, 2)
cow_moo:translate(-5.0, 0.0, 17.0)
scene:add_child(cow_moo)

-- block sanity check
-- box_moo_node = gr.node('box_moo')
-- box_moo_cube = gr.cube('box_cube')
-- box_moo_cube:set_material(stone)
-- box_moo_node:add_child(box_moo_cube)
-- box_moo_node:scale(2, 2, 2) -- centers the 2×2×2 cube at (-5, 0, 17)
-- box_moo_node:translate(-6, 1.7, 16) -- y translation >= 0.8 will make shadows disappear entirely 
-- scene:add_child(box_moo_node)

translucent_door = gr.node('translucent_door')
scene:add_child(translucent_door)
translucent_door:translate(12, 0, 4)
door_pane = gr.nh_box('door_pane', {0, 0, 0}, 1)
translucent_door:add_child(door_pane)
door_pane:set_material(ice)
door_pane:scale(0.5, 10.0, 10.0)

orange_cow_node = gr.node('orange_cow_node')
scene:add_child(orange_cow_node)
orange_cow_node:translate(18, 5, 6)
orange_cow = gr.mesh('orange_cow', 'cow.obj')
orange_cow_node:add_child(orange_cow)
orange_cow:set_material(orange)


-- Use the instanced cow model to place some actual cows in the scene.
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

-- Place a reduced ring of arches (fewer stonehenge elements).
for i = 1, 2 do
   an_arc = gr.node('arc' .. tostring(i))
   an_arc:rotate('Y', (i-1) * 60)
   scene:add_child(an_arc)
   an_arc:add_child(arc)
end

-- Two lights (left and right) for double shadows
-- (-9, 15.0, 6)
light_left = gr.light({18, 30, 12}, {1.0, 0.8, 0.8}, {1.5, 1e-3, 1e-6})
light_right = gr.light({-18, 30, 12}, {0.8, 0.8, 1.0}, {1.5, 1e-3, 1e-6})

gr.render(scene,
	  'sample.png', 1200, 1200,
	  {0, 2, 30}, {0, 0, -1}, {0, 1, 0}, 75,
	  {0.4, 0.4, 0.4}, {light_left, light_right})
	--   {0.4, 0.4, 0.4}, {light_left })

