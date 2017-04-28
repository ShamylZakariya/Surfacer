#Surfacer Modernization Rewrite

This is going to be done in steps; since I need to learn new Cinder APIs (primarily the new vector/matrix/etc stuff), and I need to learn the modern OpenGL stuff.

##Currently

Player wigs after transiting to "southern hemisphere" (a bit after _up.y < 0)
Tilt is wrong

## BUGS PRIORITY 0

glm::inverseTranspose exists. I can use it instead of glm::inverse in a lot of places

SVG can specify a fill on a parent group, and then child SVG shapes will inherit the fill. Who knew?
- in fact, there's a lot of valid SVG being exported from Sketch which I don't support :(

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
