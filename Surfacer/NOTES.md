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
	- Start with synthetic/parametric geometry, work towards a proper SVG level loader which vends anchors as well as contours.

✓ Then I need to design a basic component system. Can adopt some of my old Surfacer code here.


## Trouble

When a dynamic shape has stopped moving, if I zoom in really close, it disappears, as if the spatial index culling alg stops working. If I move it a smidge, culling starts working again.
- REPRO: Cut a shape, move it, let it stop moving on its own. Zoom in real close. Pop, it's gone. Zoom back out, give it a wee toss, culling works again.
 
Do I need to switch to a double prec representation? This affects trimesh generation, because trimesh must be single precision. My guess is I can be entirely double prec, except for making a single-prec copy of the contours for making the trimesh?
