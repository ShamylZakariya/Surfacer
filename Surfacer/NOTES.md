# Precariously Assured Destruction

NEW NAMES:
RoundWorms
https://www.wikiwand.com/en/Kessler_syndrome
When satellite debris gets out of hand

## PRESENTLY

Making the "sway" shader for greebling. 
    - works
    - add a uniform 'period' instead of hardcoded 3.14159
    - make a .glsl file format which has vertex and fragment separated by some kind of token, e.g., \n======\n or 
    fragment:
    fragment shader    
    vertex:
    vertex shader

- observe TerrainWorld::417 cpSpaceBBQuery - I might be able to use cpSPace queries to test if attachments are inside the cut volume
    
## BUGS PRIORITY HIGH

## BUGS PRIORITY LOW

Consider rewriting DrawDispatcher using raw pointers and not shared_ptr<> ?

CameraControllerComponent is acting oddly - it no longer allows camera re-centering when alt key is pressed.
	This is not a bug but the architecture acting as designed. The input is gobbled.
	Is this an ordering issue? who's gobbling the input?
	Is there a robust way to respond to this from a design standpoint?

## NOTES
Contour where the last vertex == the first cause tons of problems. util::simplify automatically removes that last vertex for me in the process of simplification.
