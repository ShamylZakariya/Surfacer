#Surfacer Modernization Rewrite

This is going to be done in steps; since I need to learn new Cinder APIs (primarily the new vector/matrix/etc stuff), and I need to learn the modern OpenGL stuff.

##TODO

- Make Terrain more robust
	- make a sequence of line segments around the perimeter with a->b->c->d->a connectivity. This should reduce "slipping through the cracks" errors.
		- mitering the perimeter is HARD. Getting this done right is crazy hard and less important now that boid velocity is better managed
	- ALTERNATELY: Add a triangle "plug" at each vertex. If you have two polys ABC and DEF which share BC and DF as an edge, I should create a triangle at B|D which would be like a shrunken A(B|D)E. This is worth examining on paper. Would require less geometry addition than the line segments approach above, but on the other hand, you'd still get the "bumps" at flush corners unless you push those triangle vertices "down"

## BUGS PRIORITY 0

Consider rewriting DrawDispatcher using raw pointers and not shared_ptr<> ?

## BUGS PRIORITY 1

CameraControllerComponent is acting oddly - it no longer allows camera re-centering when alt key is pressed.
	- this is not a bug but the architecture acting as designed. The input is gobbled.
	- is this an ordering issue? who's gobbling the input?
	- is there a robust way to respond to this from a design standpoint?

When a dynamic shape has stopped moving, if I zoom in really close, it disappears, as if the spatial index culling alg stops working. If I move it a smidge, culling starts working again.
- REPRO: Cut a shape, move it, let it stop moving on its own. Zoom in real close. Pop, it's gone. Zoom back out, give it a wee toss, culling works again.
	This might only happen for items FAR from the origin. And this would be the same bug as we saw with the camera controller. Doing a length squared test to see if I need to call moved() - might be smarter to just do taxi dist abs() on the dx dy

## NOTES
Contour where the last vertex == the first cause tons of problems. util::simplify will automatically remove that last vertex for me in the process of simplification.
