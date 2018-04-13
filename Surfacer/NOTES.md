# Precariously Assured Destruction

NEW NAMES:
RoundWorms
https://www.wikiwand.com/en/Kessler_syndrome
When satellite debris gets out of hand

## PRESENTLY
Need a dust particle system for terrain/terrain collision
    - emitBurst doesn't work, at all
    
Need to improve cloud layer response to explosions:
    - My motion away from explosion is garbage... 
    - Could add "dampers" to the perlin noise generator, like a radial blackness at the explosion point (mapped to the ring topology)

Need to make a camera shake
    see GDC Juicing Your Cameras with Math video
    - use perlin noise
    - have a tension value [0,1] which linearly falls to zero, you add values to it on explosion, say, 0.5
    - have a shake value which is tension^2 or ^3
    - compute translate & rotation offsets atop camera actual position which are perlin noise * shake value, modulated by time


## BUGS PRIORITY HIGH
- observe TerrainWorld::417 cpSpaceBBQuery - I might be able to use cpSPace queries to test if attachments are inside the cut volume. might be a lot faster than my approach, since chipmunk is pretty optimized.

## BUGS PRIORITY LOW

Consider rewriting DrawDispatcher using raw pointers and not shared_ptr<> ?

CameraControllerComponent is acting oddly - it no longer allows camera re-centering when alt key is pressed.
	This is not a bug but the architecture acting as designed. The input is gobbled.
	Is this an ordering issue? who's gobbling the input?
	Is there a robust way to respond to this from a design standpoint?

## NOTES
Contour where the last vertex == the first cause tons of problems. util::simplify automatically removes that last vertex for me in the process of simplification.
