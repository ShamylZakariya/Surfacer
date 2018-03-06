# Precariously Assured Destruction

NEW NAMES:
RoundWorms
https://www.wikiwand.com/en/Kessler_syndrome
When satellite debris gets out of hand

## PRESENTLY

Why is cutting the static planet geometry so slow? Shape hints don't help enough - maybe we need to be smarter about not doing inside tests for non-modified static geometry?
    - consider, instead of having each Attachment have a shapehint, what if the shape also got a list of attachments? Bidirectional. WHen a cut is performed, we collect the attachments which were associated with the cut shapes and only reparent those. Leave the rest alone, completely.
    - this has ramifications for dynamic group situations. But I think we re-use shapes with DynamicGroup as well, so we can reuse the shape assignments too?

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
