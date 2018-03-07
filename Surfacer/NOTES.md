# Precariously Assured Destruction

NEW NAMES:
RoundWorms
https://www.wikiwand.com/en/Kessler_syndrome
When satellite debris gets out of hand

## PRESENTLY

1) DONE ~~partitioning is BROKEN AF (confirm this fixes precariously level load, which is BROKEN AF, showing only core geometry)~~
2) fast path for reparenting attachments not working as expected (shape hint weak ptr is invalid);
    kind of makes sense in as much as static terrain is ONE shape, which when cut, dies. partitioning could help, but an alternate approach
    might be to collect attachments that intersect the actual cut???
    - consider:
        - compute two lists of attachments. The current set of ones which need reparenting, and ANOTHER, those which intersected the cut and are guaranteed to be orphaned.
        - subtract the latter from the former, and the reparenting set can be blindly re-inserted
    
## BUGS PRIORITY HIGH

For my sanity I really must make zooming about mouse cursor work again

## BUGS PRIORITY LOW

Consider rewriting DrawDispatcher using raw pointers and not shared_ptr<> ?

CameraControllerComponent is acting oddly - it no longer allows camera re-centering when alt key is pressed.
	This is not a bug but the architecture acting as designed. The input is gobbled.
	Is this an ordering issue? who's gobbling the input?
	Is there a robust way to respond to this from a design standpoint?

## NOTES
Contour where the last vertex == the first cause tons of problems. util::simplify automatically removes that last vertex for me in the process of simplification.
