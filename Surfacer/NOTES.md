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

	NOTES:
	Right now, this works when my geometry is simple, like a big box. But when running on a ring, it weirds out when cutting starts, as if Shape transforms are wrong.
		- maybe try making edge snapping more aggressive? snap any edge where vertices are within 2,4,8 units?


Then I need to design a basic component system. Can adopt some of my old Surfacer code here. But:
	- Needs a collect() method where components can deliver their draw components to be passed through visibility determination/culling. 
	This needs to be flexible enough that World can vend a list of island::Shape instances to draw and the culler can efficiently pick the visible subset 
 
