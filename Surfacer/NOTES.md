# Precariously Assured Destruction

NEW NAMES:
RoundWorms
https://www.wikiwand.com/en/Kessler_syndrome
When satellite debris gets out of hand

## PRESENTLY

attachments are ostensibly not orphaned, but don't track with cut geometry
    - build a "cut recorder" which persists cuts to something (JSON?)
    - build a "cut replayer" which cen execute those cuts
    - emit ONE attachment, find the one or two cuts which leave it orphaned
    - VERIFY

fast path for reparenting attachments not working as expected (shape hint weak ptr is invalid);
    kind of makes sense in as much as static terrain is ONE shape, which when cut, dies. partitioning could help, but an alternate approach
    might be to collect attachments that intersect the actual cut???
    consider:
        - compute two lists of attachments. The current set of ones which need reparenting, and ANOTHER, those which intersected the cut and are guaranteed to be orphaned.
        - subtract the latter from the former, and the reparenting set can be blindly re-inserted
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
