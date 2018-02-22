# Precariously Assured Destruction

NEW NAMES:
RoundWorms
https://www.wikiwand.com/en/Kessler_syndrome
When satellite debris gets out of hand

## PRESENTLY

OPTIMIZATIONS:
- core::ip can be threaded
    - http://en.cppreference.com/w/cpp/thread/thread/hardware_concurrency
    algs like split blur, etc can be chunked, giving each thread a subset of the image to work on, say, rows 0->255, 256->512 on a 2-core machine. Just reimplement the operations to take a first and last row, and then if > 1 CPU, spawn N threads, and join.

Explosion effect looks totally wonky in PrecariouslyStage
	Need to figure out how to make initialDirection variance work better. my variance algo isn't cutting it.
		Consider creating some kind of emission source object. can be a circle, a cone, etc. it vends initial position and direction

## BUGS PRIORITY HIGH

WorldCartesianGridComponent - color of grid not working
For my sanity I really must make zooming about mouse cursor work again

## BUGS PRIORITY LOW

Consider rewriting DrawDispatcher using raw pointers and not shared_ptr<> ?

CameraControllerComponent is acting oddly - it no longer allows camera re-centering when alt key is pressed.
	This is not a bug but the architecture acting as designed. The input is gobbled.
	Is this an ordering issue? who's gobbling the input?
	Is there a robust way to respond to this from a design standpoint?

## NOTES
Contour where the last vertex == the first cause tons of problems. util::simplify automatically removes that last vertex for me in the process of simplification.
