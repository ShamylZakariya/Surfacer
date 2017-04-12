#Surfacer Modernization Rewrite

This is going to be done in steps; since I need to learn new Cinder APIs (primarily the new vector/matrix/etc stuff), and I need to learn the modern OpenGL stuff.

##Currently

✓ Need to prevent accidental stitching
	- this can happen if two shapes touch and their edges align perfectly

✓ Need to make Anchors physical enties

Need to verify stitching of a large environment works. 
	✓ Use ring generator to make a large "world" of discrete contours.
	✓ Intersect these contours with a checkerboard type pattern to divide it into a grid.
	✓ Let that grid stitch back together as needed.
	✓ Start with synthetic/parametric geometry, work towards a proper SVG level loader which vends anchors as well as contours.

✓ Then I need to design a basic component system. Can adopt some of my old Surfacer code here.


## TODO

SVG Groups need a way to recursively compute their BB

SVG can specify a fill on a parent group, and then child SVG shapes will inherit the fill. Who knew?

Rewrite the cartesian coordinate renderer using a shader. Make it handle rotation.
	- CPU can compute the origin and step, pass that to shader. Shader uses gl_FragCoord - or shader does a line distance calculation with some kind of modulo

When a dynamic shape has stopped moving, if I zoom in really close, it disappears, as if the spatial index culling alg stops working. If I move it a smidge, culling starts working again.
- REPRO: Cut a shape, move it, let it stop moving on its own. Zoom in real close. Pop, it's gone. Zoom back out, give it a wee toss, culling works again.
	This might only happen for items FAR from the origin. And this would be the same bug as we saw with the camera controller. Doing a length squared test to see if I need to call moved() - might be smarter to just do taxi dist abs() on the dx dy

## NOTES
Contour where the last vertex == the first cause tons of problems. util::simplify will automatically remove that last vertex for me in the process of simplification.
