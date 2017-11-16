# Precariously Assured Destruction
https://www.wikiwand.com/en/Kessler_syndrome
When satellite debris gets out of hand

## PRESENTLY

Explosion effect looks totally wonky in PrecariouslyStage
	need to scale the spark size down to zero when motion approaches zero (flag?)
	Need to figure out how to make initialDirection variance work better. my variance algo isn't cutting it.

## BUGS PRIORITY 0

For my sanity I really must make zooming about mouse cursor work again

## BUGS PRIORITY 1

Consider rewriting DrawDispatcher using raw pointers and not shared_ptr<> ?

CameraControllerComponent is acting oddly - it no longer allows camera re-centering when alt key is pressed.
	This is not a bug but the architecture acting as designed. The input is gobbled.
	Is this an ordering issue? who's gobbling the input?
	Is there a robust way to respond to this from a design standpoint?

## NOTES
Contour where the last vertex == the first cause tons of problems. util::simplify automatically removes that last vertex for me in the process of simplification.
