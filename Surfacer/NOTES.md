# Precariously Assured Destruction

NEW NAMES:
RoundWorms
https://www.wikiwand.com/en/Kessler_syndrome
When satellite debris gets out of hand

## PRESENTLY

Planet Attachments
- attachment is just a transform relative to a group
- an attachment must be *inside* a shape
- when a shape is cut, all that shape's attachments are tested to be inside the generated new shapes. They are reparented or orphaned accordingly.
- when orphaned send a signal
- no update() or draw(). just functions for querying local and world space transforms

STEPS:
DONE:
    - attachment @confirmed
    - basic tests for if attachment is in a group/shape @confirmed
    - draw attachments in debug mode @ confirmed
    - add attachments to world @confirmed
TODO:
    - migrate attachments to new groups (see World::build ~:766 - this is where new shapes are configured)
        - make orphans of those which couldn't be attached
    
THOUGHTS:
    - make certain I'm setting localTransform right when adding
    - don't forget to set _group ptr
    


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
