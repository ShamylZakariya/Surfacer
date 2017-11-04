# Precariously Assured Destruction

This is going to be done in steps; since I need to learn new Cinder APIs (primarily the new vector/matrix/etc stuff), and I need to learn the modern OpenGL stuff.

## PRESENTLY

UniversalParticleSystem
	- compact() is disabled


## BUGS PRIORITY 0

- Make the terrain::detail boost geometry / cinder methods parameterized on double/float
- For my sanity I really must make zooming about mouse cursor work again
- Consider rewriting DrawDispatcher using raw pointers and not shared_ptr<> ?

## BUGS PRIORITY 1

CameraControllerComponent is acting oddly - it no longer allows camera re-centering when alt key is pressed.
	- this is not a bug but the architecture acting as designed. The input is gobbled.
	- is this an ordering issue? who's gobbling the input?
	- is there a robust way to respond to this from a design standpoint?

## NOTES
Contour where the last vertex == the first cause tons of problems. util::simplify automatically removes that last vertex for me in the process of simplification.
