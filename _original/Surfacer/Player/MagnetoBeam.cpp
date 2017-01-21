//
//  MagnetoBeam.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 2/8/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "MagnetoBeam.h"

#include "FastTrig.h"
#include "GameConstants.h"
#include "GameLevel.h"
#include "Mirror.h"
#include "PerlinNoise.h"
#include "Player.h"
#include "PowerUp.h"
#include "PowerCell.h"
#include "Terrain.h"

#include <cinder/Surface.h>
#include <cinder/ImageIo.h>

#include <set>
#include <queue>

using namespace ci;
using namespace core;
namespace game {

namespace {

	struct floodfill_state
	{
		cpBodySet visited;
		std::queue<cpBody*> queue;
		
		inline void add( cpBody *body )
		{
			if ( !visited.count(body) ) 
			{
				queue.push(body);
			}
		}		
	};

	void collect_touching_bodies(cpBody *body, cpArbiter *arbiter, void *data)
	{
		cpBody *a = NULL, *b = NULL;
		cpArbiterGetBodies(arbiter, &a, &b);

		((floodfill_state*) data)->add(a);
		((floodfill_state*) data)->add(b);
	}

	/**
		Determine if we're going to allow towing of a body.
		We have higher-level filters like whether the body is the right type, light enough, etc. But
		I want to prevent "magic carpet" behavior (known as "MC" here), where the player stands on a rock, and then uses the gun to
		fly the rock around. That would make the game too easy.
		
		So here's the conceptual idea: 
			If touching static geoemtry, then we're good.
				This is because so long as player touches static geometry the rock can't be a MC. Hopefully!
	
			If no contact-graph path exists from player to target, we're good.
				This means the target isn't touching player, directly or indirectly, so it can't be used for MC
				
			If the whole contact graph is examined, and no static geometry is found, and the target is
			found, then disallow towing. this means there's a chance the target can transfer inertia to player.
						
	*/
	bool allow_towing( cpBody *playerBody, cpBody *towBody )
	{
		floodfill_state state;
		
		state.add( playerBody );
		while( !state.queue.empty() )
		{
			//
			//	Get the next body to process
			//

			cpBody *body = state.queue.front();
			state.queue.pop();
			state.visited.insert(body);

			//
			//	If this body is static, we're good
			//
			if ( cpBodyIsStatic( body )) 
			{
				//app::console() << "allow_towing - static geometry contact: ALLOW" << std::endl;
				return true;
			}
			
				
			cpBodyEachArbiter( body, collect_touching_bodies, &state );
		}
		
		//
		//	The flood fill finished. If we found towBody, we didn't encounter any static geometry,
		//	so we cannot allow this tow to occur.
		//
		
		if ( state.visited.count(towBody) )
		{
			//app::console() << "allow_towing - finished floodfill without static contact, found target: DISALLOW" << std::endl;
			return false;
		}		
		
		//
		//	If we didn't find the tow body that means there's no path from the player to the tow body,
		//	so towing is just fine.
		//
		
		//app::console() << "allow_towing - finished floodfill without target: ALLOW" << std::endl;

		return true;		
	}


}


#pragma mark - MagnetoBeam::tow_handle

void MagnetoBeam::tow_handle::release()
{
	cpConstraintCleanupAndFree( &positionConstraint );
	cpConstraintCleanupAndFree( &rotationConstraint );
	
	if ( target )
	{
		cpBodyActivate( target.body );
	}
		
	target.reset();
	contracting = false;
	towable = false;
	untowableReason = NONE;
}

bool MagnetoBeam::tow_handle::allowTowing( Player *player ) const
{
	if ( towable )
	{
		return allow_towing( player->footBody(), target.body );
	}
	
	return false;
}


#pragma mark - MagnetoBeam

/*
		init _initializer;
		MagnetoBeamController *_controller;
		
		real _towAngle, _maxConstraintForce;		
		tow_handle _towing;
		cpBody *_towBody;	
		Vec2 _towPosition;
		
		seconds_t _towSeverTime, _minTimeUntilNextTow;
*/

MagnetoBeam::MagnetoBeam():
	_controller(new MagnetoBeamController()),
	_towAngle(0),
	_towBody(NULL),
	_towSeverTime(-1000),
	_minTimeUntilNextTow(1)
{
	setLayer( RenderLayer::EFFECTS );
	setName( "MagnetoBeam" );
	addComponent(new MagnetoBeamRenderer());
	addComponent( _controller );
	
	//
	//	Configure raycast querys
	//
	
	raycast_query_filter filter;
	filter.collisionType = 0;
	filter.collisionTypeMask = CollisionType::is::TOWABLE;
	filter.layers = CP_ALL_LAYERS;
	filter.group = CP_NO_GROUP;
	_setRaycastQueryFilter( filter );
}

MagnetoBeam::~MagnetoBeam()
{
	aboutToDestroyPhysics(this);

	if ( _towBody )
	{
		cpBodyFree( _towBody );
	}
}

// Initialization
void MagnetoBeam::initialize( const init &i )
{
	BeamWeapon::initialize(i);
	_initializer = i;
}

void MagnetoBeam::addedToLevel( Level *level )
{
	BeamWeapon::addedToLevel(level);

	_maxConstraintForce = 2 * cpvlength(cpvmult(cpSpaceGetGravity(level->space()), _initializer.maxTowMass));

	//
	//	Create tow body - note we don't add to space - we'll simulate this body manually in ::update
	//

	const Vec2 
		MousePosScreen = InputDispatcher::get()->mousePosition(),
		MousePosWorld = level->camera().screenToWorld( MousePosScreen );

	_towBody = cpBodyNew(INFINITY, INFINITY);
	cpBodySetPos( _towBody, cpv( MousePosWorld ));
}

void MagnetoBeam::removedFromLevel( Level *level )
{
	BeamWeapon::removedFromLevel(level);
	setFiring( false );
}

void MagnetoBeam::update( const time_state &time )
{
	Player *player = static_cast<Player*>(parent());

	//
	//	Move and rotate tow body - update linear/angular vel accordingly
	//
	
	{
		const cpVect 
			LastTowBodyPos = cpBodyGetPos( _towBody ),
			NewTowBodyPos = cpv( _towPosition ),
			TowBodyVel = cpvmult(cpvsub(NewTowBodyPos, LastTowBodyPos), time.deltaT );
		
		const real
			LastTowBodyAngle = cpBodyGetAngle( _towBody ),
			NewTowBodyAngle = _towAngle,
			TowBodyAngularVel = (NewTowBodyAngle - LastTowBodyAngle) * time.deltaT;
		
		cpBodySetPos( _towBody, NewTowBodyPos );
		cpBodySetVel( _towBody, TowBodyVel );
		cpBodySetAngle( _towBody, NewTowBodyAngle );
		cpBodySetAngVel( _towBody, TowBodyAngularVel );					
	}
	
	//
	//	Update the BeamWeapon's dir and range
	//

	{
		
		const Vec2	
			Dir = v2(cpBodyGetPos(_towBody)) - player->position();

		setDirection( Dir );
		setRange( Dir.length() );
	}

	BeamWeapon::update(time);	

	//
	//	If towing, verify we're still aimed at what we're towing. if not, stop towing that thing. This
	//		can happen when an occluder breaks the towing beam. Say trying to drag something beyond a wall. 
	//
	//	If NOT towing, see if we can tow what we're aimed at and tow it
	//

	if ( firing() )
	{
		tow_handle currentlyAimedAt = towable( beamTarget() );
		

		if ( _towing )
		{		

			const bool 
				TargetIsStaticOrTooHeavy = 
					(currentlyAimedAt.untowableReason==UNTOWABLE_BECAUSE_STATIC) || 
					(currentlyAimedAt.untowableReason==UNTOWABLE_BECAUSE_TOO_HEAVY),

				TargetChanged = currentlyAimedAt.towable ? (currentlyAimedAt.target.object != _towing.target.object) : false,
				TargetIsDisallowed = currentlyAimedAt.towable ? !currentlyAimedAt.allowTowing( player ) : false;
				
			if ( TargetIsStaticOrTooHeavy || TargetChanged || TargetIsDisallowed )
			{
//				app::console() << "TargetIsStaticOrTooHeavy: " << TargetIsStaticOrTooHeavy 
//					<< " TargetChanged: " << TargetChanged 
//					<< " TargetIsDisallowed: " << TargetIsDisallowed
//					<< std::endl; 
			
				//
				//	The beam has been interrupted, and we need to notify the towing session
				//	was involuntarily ended, and untow the current _towing target
				//

				towSevered(this, _towing );
				
				if ( TargetIsDisallowed )
				{
					_towSeverTime = level()->time().time;
				}

				_untow( _towing.target.object );
			}
			else
			{
				//
				//	When we start the tow process, we're using a pin joint. We gradually contract
				//	the pin joint until the towed body's center of mass is atop the _towBody, then
				//	kill the pin joint and replace it with a pivot joint.
				//

				if ( _towing.contracting )
				{
					const real
						Dist = cpPinJointGetDist( _towing.positionConstraint ) * 0.9;
					
					_towing.contraction = 1 - (Dist / _towing.initialDistance);
																										
					if ( Dist < Epsilon )
					{
						cpSpace *space = level()->space();
						cpSpaceRemoveConstraint( space, _towing.positionConstraint);
						cpConstraintFree(_towing.positionConstraint);
						
						_towing.positionConstraint = cpPivotJointNew2(_towBody, _towing.target.body, cpvzero, cpvzero );
						cpConstraintSetMaxForce( _towing.positionConstraint, _maxConstraintForce );
						cpSpaceAddConstraint( space, _towing.positionConstraint );
						_towing.contracting = false;
					}
					else
					{
						cpPinJointSetDist( _towing.positionConstraint, Dist );
					}
				}
			}
		}
		
		if ( !_towing )
		{
			//
			//	Begin new towing session, and notify any listeners
			//
			
			const bool EnoughTimeElapsed = (level()->time().time - _towSeverTime) >= _minTimeUntilNextTow;

			if ( currentlyAimedAt.allowTowing( ((GameLevel*)level())->player() ) && EnoughTimeElapsed )
			{
				_tow(currentlyAimedAt);

				if ( _towing )
				{
					towBegun(this, _towing );
				}
			}
		}
	}
	else
	{
		//
		//	A towing session has voluntarily ended, let listeners know and break tow connection
		//

		if ( _towing )
		{
			towReleased( this, _towing );
			_untow( _towing.target.object );
		}
	}
}

void MagnetoBeam::setFiring( bool firing )
{
	if ( firing != this->firing() )
	{
		BeamWeapon::setFiring( firing );
		
		if ( !firing && _towing )
		{
			towReleased( this, _towing );
			_untow( _towing.target.object );
		}	
	}
}

void MagnetoBeam::wasPutAway()
{
	if ( _towing ) _towing.release();
}

real MagnetoBeam::drawingPowerPerSecond() const
{
	const tow_handle &Towing = this->towing();
	if ( Towing )
	{
		const real Load = (cpBodyGetMass(Towing.target.body) / initializer().maxTowMass);
		return BeamWeapon::drawingPowerPerSecond() * Load;
	}
	
	return 0;
}

MagnetoBeam::tow_handle 
MagnetoBeam::towable( const raycast_target &target ) const
{
	tow_handle handle;
	if ( target )
	{
		//
		//	If target object type is in "approved list" copy over. note that ISLAND gets
		//	up-promoted to the island's parent IslandGroup since that's the actual owner
		//	of the physics rep.
		//

		switch( target.object->type() )
		{
			case GameObjectType::ISLAND:
				handle.target = target;
				handle.target.object = ((terrain::Island*) target.object)->group();
				handle.towable = true;
				handle.untowableReason = NONE;
				break;
			
			case GameObjectType::DECORATION:
			case GameObjectType::POWERUP:
			case GameObjectType::POWERCELL:
				handle.target = target;
				handle.towable = true;
				handle.untowableReason = NONE;
				break;
				
			default:
				handle.untowableReason = UNTOWABLE_BECAUSE_WRONG_MATERIAL;
				break;
		}

		if ( cpBodyIsStatic( target.body )) 
		{
			handle.towable = false;
			handle.untowableReason = UNTOWABLE_BECAUSE_STATIC;		
			return handle;	
		}
		
		if ( cpBodyGetMass( target.body ) > _initializer.maxTowMass ) 
		{
			handle.towable = false;
			handle.untowableReason = UNTOWABLE_BECAUSE_TOO_HEAVY;
			return handle;
		}

	}
	else
	{
		handle.untowableReason = UNTOWABLE_BECAUSE_NULL;
	}
	
	return handle;
}

void MagnetoBeam::setTowPosition( const Vec2 &worldPos )
{
	_towPosition = worldPos;
}

void MagnetoBeam::rotateTowBy( real deltaRadians )
{
	_towAngle += deltaRadians;
}

void MagnetoBeam::_tow( const tow_handle &towing )
{
	if ( _towing.target ) _untow( _towing.target.object );

	cpSpace *space = level()->space();

	_towing = towing;
	_towing.target.object->aboutToDestroyPhysics.connect( this, &MagnetoBeam::_untow );

	//
	//	If a powerup is being eaten by a power platform, we need to let go!
	//
	if ( _towing.target.object->type() == GameObjectType::POWERCELL )
	{
		PowerCell *powercell = static_cast<PowerCell*>(_towing.target.object);
		powercell->onInjestedByPowerPlatform.connect(this, &MagnetoBeam::_untow );
	}

	_towing.positionConstraint = cpPinJointNew(_towBody, _towing.target.body, cpvzero, cpvzero );
	cpConstraintSetMaxForce( _towing.positionConstraint, _maxConstraintForce );
	cpSpaceAddConstraint( space, _towing.positionConstraint );

	_towing.rotationConstraint = cpGearJointNew( _towBody, _towing.target.body, 0, 1 );
	cpConstraintSetMaxForce( _towing.rotationConstraint, _maxConstraintForce );
	cpGearJointSetPhase( _towing.rotationConstraint, cpBodyGetAngle(_towing.target.body ) - cpBodyGetAngle(_towBody));
	cpSpaceAddConstraint( space, _towing.rotationConstraint );
	
	//
	//	Now that we've connected, mark that contraction phase has begun. During contraction, the
	//	pin joint we just created will be brought in over time to zero length, and then replaced with a pivot joint.
	//
	
	_towing.contracting = true;
	_towing.initialDistance = cpvdist(cpBodyGetPos(_towBody), cpBodyGetPos(_towing.target.body));
	
	TowListener *towListener = dynamic_cast<TowListener*>(_towing.target.object);
	if ( towListener )
	{
		towListener->aboutToBeTowed(this);
	}
	
	//
	//	Hide the mouse cursor
	//

	core::InputDispatcher::get()->hideMouse();
	
}

void MagnetoBeam::_untow( GameObject *object )
{
	if ( _towing && object == _towing.target.object )
	{
		TowListener *towListener = dynamic_cast<TowListener*>(_towing.target.object);
		if ( towListener )
		{
			towListener->aboutToBeUnTowed(this);
		}

		_towing.release();
	}
	
	//
	//	Unhide the mouse cursor
	//

	core::InputDispatcher::get()->unhideMouse();	
}

#pragma mark - MagnetoBeamRenderer

namespace {

	const real
		TransitionAbruptness = 0.15;

	const unsigned int 
		MaxLassoSegments = 128,
		MaxBeamSegments = 384;
		
//	template< typename T >
//	void TRANSIT( T &currentValue, const T &newValue )
//	{
//		currentValue = lrp<T>( TransitionAbruptness, currentValue, newValue );
//	}

	// specialize on real
	void TRANSIT( real &currentValue, real newValue )
	{
		currentValue = lrp<real>( TransitionAbruptness, currentValue, newValue );
	}	
	
	const Vec4
		TowableColor( 0.5,1,0.5,1 ),
		UntowableColor( 1,0.2,0,1 );

	inline void TowabilityColorRamp( real towability, Vec4 &color )
	{
		color.x = lrp( towability, TowableColor.x, UntowableColor.x );
		color.y = lrp( towability * towability, TowableColor.y, UntowableColor.y );
		color.z = lrp( towability, TowableColor.z, UntowableColor.z );
		color.w = lrp( towability, TowableColor.w, UntowableColor.w );
	}
	
	inline real FUDGE( real range )
	{
		real f = Rand::randFloat(range);
		return Rand::randBool() ? f : -f;
	}
	
}

MagnetoBeamRenderer::beam_state::~beam_state()
{
	if ( vbo )
	{
		glDeleteBuffers( 1, &vbo );
	}
}

/*
		ci::gl::GlslProg _towOverlayShader, _queryOverlayShader, _beamShader;		
		beam_state_vec _beamStates;
		GLint _beamShaderEdgeAttrLoc;
*/

MagnetoBeamRenderer::MagnetoBeamRenderer():
	_beamShaderEdgeAttrLoc(0)
{
	setName( "MagnetoBeamRenderer" );
	
	const Vec4
		Color0( 0,0.5,1,0.9 ),
		Color1( 0,0.7,1,0.9 ),
		Color2( 0.9,0.2,0.9,0.6 );

	const real 
		NoiseOffsetRateScale = 4,
		Width = 4;
	
	beam_state bs;

	bs.color = Color0;
	bs.width = Width;
	bs.perlin.octaves = 6;
	bs.perlin.falloff = 1;
	bs.perlin.scale = 6;
	bs.perlin.seed = 1111;
	bs.noiseScale = 2;
	bs.noiseOffset.set(0, 0);
	bs.noiseOffsetRate = Vec2( 4,8 ) * NoiseOffsetRateScale;
	_beamStates.push_back( bs );

	bs.color = Color1;
	bs.width = Width;
	bs.perlin.octaves = 5;
	bs.perlin.falloff = 1;
	bs.perlin.scale = 5;
	bs.perlin.seed = 2222;
	bs.noiseScale = 3;
	bs.noiseOffset.set(0, 0);
	bs.noiseOffsetRate = Vec2( 8,16 ) * NoiseOffsetRateScale;
	_beamStates.push_back( bs );

	bs.color = Color2;
	bs.width = Width;
	bs.perlin.octaves = 4;
	bs.perlin.falloff = 1;
	bs.perlin.scale = 4;
	bs.perlin.seed = 3333;
	bs.noiseScale = 4;
	bs.noiseOffset.set(0, 0);
	bs.noiseOffsetRate = Vec2( 16,32 ) * NoiseOffsetRateScale;
	_beamStates.push_back( bs );

	//
	//	Generate a nice single-shannel perlin noise image - note we're talling fill() to create
	//	normalized values, running from [-1,+1] so we don't have to normalize in the draw loop.
	//
	
	foreach( beam_state &beamState, _beamStates )
	{
		util::PerlinNoise pNoise( beamState.perlin.octaves, beamState.perlin.falloff, beamState.perlin.scale, beamState.perlin.seed );
		beamState.noise = fill( pNoise, Vec2i(128,128), Vec2i::zero(), true );
	}
}

MagnetoBeamRenderer::~MagnetoBeamRenderer()
{}

void MagnetoBeamRenderer::draw( const render_state &state )
{
	switch( state.mode )
	{
		case RenderMode::GAME:
		case RenderMode::DEVELOPMENT:
		{
			//
			//	Update high-level animation & transition states for beams
			//
		
			MagnetoBeam *beam = this->magnetoBeam();
			const MagnetoBeam::tow_handle &CurrentlyTowing = beam->towing();
			const MagnetoBeam::tow_handle &CurrentlyAimedAt = beam->towable(beam->beamTarget());

			if ( CurrentlyTowing )
			{
				const cpBB Bounds = CurrentlyTowing.target.object->aabb();
				const real Radius = cpBBRadius( Bounds );
				const Vec2 Position = v2( cpBBCenter(Bounds) );

				for ( beam_state_vec::iterator beamState( _beamStates.begin()), end( _beamStates.end()); beamState != end; ++beamState )
				{
					TRANSIT( beamState->transition, 1 );
					beamState->start = beam->emissionPosition();
					beamState->end = Position;
					beamState->radius = Radius;
				}				
			}
			else
			{		
				for ( beam_state_vec::iterator beamState( _beamStates.begin()), end( _beamStates.end()); beamState != end; ++beamState )
				{
					TRANSIT( beamState->transition, 0 );
				}
			}		

			break;
		}
		
		default: break;
	}

	DrawComponent::draw( state );
}
		
void MagnetoBeamRenderer::_drawGame( const render_state &state )
{

	if ( !_beamShader )
	{
		_beamShader = level()->resourceManager()->getShader( "MagnetoBeamShader" );
		_beamShaderEdgeAttrLoc = _beamShader.getAttribLocation( "Edge" );			
	}

	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE );

	_beamShader.bind();
	_beamShader.uniform( "PaletteSize", 3.0f );
	_beamShader.uniform( "rPaletteSize", 0.33333f );
	
	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableVertexAttribArray( _beamShaderEdgeAttrLoc );
	
	for ( beam_state_vec::iterator beamState( _beamStates.begin()), end( _beamStates.end()); beamState != end; ++beamState )
	{
		if ( beamState->transition > ALPHA_EPSILON )
		{
			_updateBeam( state, *beamState );
			_drawBeam( state, *beamState, _beamShader );
		}
	}
	
	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableVertexAttribArray( _beamShaderEdgeAttrLoc );

	_beamShader.unbind();
}

void MagnetoBeamRenderer::_drawDevelopment( const render_state &state )
{
	for ( beam_state_vec::iterator beamState( _beamStates.begin()), end( _beamStates.end()); beamState != end; ++beamState )
	{
		if ( beamState->transition > ALPHA_EPSILON )
		{
			_updateBeam( state, *beamState );
			_debugDrawBeam( state, *beamState );
		}
	}
}


void MagnetoBeamRenderer::_drawDebug( const render_state &state )
{
	_debugDrawChipmunkBody( state, this->magnetoBeam()->towBody() );	
}

void MagnetoBeamRenderer::_updateBeam( const render_state &state, beam_state &beamState )
{
	using util::fasttrig::sin;
	using util::fasttrig::cos;
	using util::fasttrig::atan;
	
	MagnetoBeam *beam = this->magnetoBeam();

	const real 
		InitialSegLength = 0.5;

	const Vec2 
		BeamStart = beamState.start,
		BeamEnd = beamState.end - normalize( beamState.end - beamState.start ) * beamState.radius,
		BeamDir = BeamEnd - BeamStart;

	const real 
		BeamLength = BeamDir.length();
	
	const unsigned int 
		NLS = std::ceil((2 * M_PI * beamState.radius) / InitialSegLength),
		NBS = std::ceil( BeamLength / InitialSegLength ),
		NumLassoSegments = clamp< unsigned int >( NLS, 32u, MaxLassoSegments ),
		NumBeamSegments = clamp< unsigned int >( NBS, 0u, MaxBeamSegments ),
		NumSegmentsToDraw = NumLassoSegments + NumBeamSegments;


	const real 
		Transition = beamState.transition,
		SegLength = BeamLength / (NumBeamSegments),
		MinVisibleBeamWidth = state.viewport.reciprocalZoom(),
		BeamTemplateWidth = std::max( Transition * beamState.width * beam->initializer().width, MinVisibleBeamWidth );

	const Vec2 
		NormalizedBeamDir = normalize( BeamDir ),
		BeamPositionStep = beamState.transition * NormalizedBeamDir * SegLength,
		BeamRight = rotateCW(NormalizedBeamDir),
		BeamRightWidth = BeamTemplateWidth * BeamRight;
		
	const Vec2i		
		NoiseSize( beamState.noise.getSize() );

	const real 
		BeamDistanceStep = 1 / real(NumBeamSegments);
	
	//
	//	Now resize the storage to fit need
	//

	if (beamState.segments.empty() )
	{
		beamState.segments.resize( MaxLassoSegments + MaxBeamSegments );
		beamState.vertices.resize( MaxLassoSegments + MaxBeamSegments * 2 );
	}
	else if ( NumSegmentsToDraw > beamState.segmentsDrawn )
	{	
		//
		//	If we're drawing more segments this frame than the last, we need to reset tension
		//	of the new segments to 1 so lrping them will not lrp from possibly wonky values.
		//
		for ( beam_segment_vec::iterator 
			segment(beamState.segments.begin() + beamState.segmentsDrawn),
			end(beamState.segments.begin() + NumSegmentsToDraw);
			segment != end; 
			++segment )				
		{
			segment->tension = 1;
			segment->firstFrame = true;
		}		
	}
	
	
	Vec2 positionAlongBeam = beamState.start;
	real distanceAlongBeam = 0;

	int noiseX = std::floor( beamState.noiseOffset.x );
	const int NoiseY = std::floor( beamState.noiseOffset.y );
	const real 
		BeamNoiseScale = Transition * beamState.noiseScale,
		LassoNoiseScale = BeamNoiseScale;
	
	
	beam_vertex_vec::iterator vertex = beamState.vertices.begin();
	beam_segment_vec::iterator segment = beamState.segments.begin();

	//
	//	First, render the beam segments from the gun to the perimeter of the lasso circle
	//	

	for ( unsigned int i = 0; i < NumBeamSegments; ++i, ++segment )
	{
	
		//
		//	Compute tighter tension at endpoints, go sloppy in the middle
		//

		real 
			tension = 1 - ( 0.5 * sin( distanceAlongBeam * M_PI ) );
			
		tension = tension*tension*tension*tension;
		tension = lrp<real>( 1 - Transition, tension, 1 );

		if ( !segment->firstFrame )
		{
			segment->tension = lrp( 0.1, segment->tension, tension );
		}

		const real NoisePhase = *beamState.noise.getData( noiseX, NoiseY );

		const Vec2 
			WaveOffset = (1-segment->tension) * BeamRight * NoisePhase * BeamNoiseScale,
			BeamRightWidthScaled = (1-tension) * BeamRightWidth * 0.75;
			
		segment->position = lrp( segment->tension, segment->position, positionAlongBeam + WaveOffset );	
		segment->firstFrame = false;		
		
		vertex->position = segment->position + BeamRightWidthScaled;
		vertex->edge = -1;
		++vertex;

		vertex->position = segment->position - BeamRightWidthScaled;
		vertex->edge = 1;
		++vertex;
	
		distanceAlongBeam += BeamDistanceStep;
		positionAlongBeam += BeamPositionStep;
		noiseX = (noiseX + 1) % NoiseSize.x;
	}
	
	//
	//	Second, render beam segments around perimeter of the lasso circle;
	//	NOTE: I tried fasttrig atan2 for lasso origin and fasttrig::sin/cos for
	//	lasso vertices and it was NOT good.
	//
	
	const Vec2 
		LassoDir = normalize( beamState.end - beamState.start );
			
	const real 
		LassoStartAngle = std::atan2( -LassoDir.y, -LassoDir.x ),
		LassoAngleIncrement = 2 * M_PI / NumLassoSegments,
		LassoRadius = beamState.radius,
		LassoTension = 0.25;

	const Vec2
		LassoCenter = beamState.end;
		
	real lassoAngle = LassoStartAngle;
	
	beam_vertex_vec::iterator firstLassoVertex = vertex;
	beam_segment_vec::iterator firstLassoSegment = segment;
	
	noiseX = 0;

	for ( unsigned int i = 0, N = (NumLassoSegments-1); i < N; ++i, ++segment )
	{
		const Vec2 
			RadialDir( std::cos( lassoAngle ), std::sin( lassoAngle )),
			LassoSegmentPosition( LassoCenter + LassoRadius*RadialDir ),
			LassoRightWidth = BeamTemplateWidth * 0.5 * RadialDir;
	
		const real 
			NoisePhase = *beamState.noise.getData( noiseX, NoiseY );

		const Vec2 
			WaveOffset = RadialDir * NoisePhase * LassoNoiseScale;

		if ( !segment->firstFrame )
		{	
			segment->tension = lrp( 0.1, segment->tension, LassoTension );
		}

		//
		//	tighten tension for when beam is initially fired; 
		//

		segment->tension = lrp<real>( 1 - Transition, segment->tension, 1 );
		segment->position = lrp( segment->tension, segment->position, LassoSegmentPosition + WaveOffset );
		segment->firstFrame = false;

		vertex->position = segment->position + LassoRightWidth;
		vertex->edge = -1;
		++vertex;

		vertex->position = segment->position - LassoRightWidth;
		vertex->edge = 1;
		++vertex;
	
		noiseX = (noiseX + 1) % NoiseSize.x;
		lassoAngle += LassoAngleIncrement;
	}
	
	//
	//	Update last segment to meet the first
	//
	
	segment->tension = 1;
	segment->position = firstLassoSegment->position;

	vertex->position = firstLassoVertex->position;
	++vertex;
	++firstLassoVertex;

	vertex->position = firstLassoVertex->position;
	
	//
	//	Update noise offsets for animation
	//

	beamState.noiseOffset.x = std::fmod( real(beamState.noiseOffset.x + beamState.noiseOffsetRate.x * state.deltaT), real(NoiseSize.x) );
	beamState.noiseOffset.y = std::fmod( real(beamState.noiseOffset.y + beamState.noiseOffsetRate.y * state.deltaT), real(NoiseSize.y) );

	beamState.segmentsDrawn = NumSegmentsToDraw;
}

void MagnetoBeamRenderer::_drawBeam( const render_state &state, beam_state &beamState, ci::gl::GlslProg &shader )
{
	//
	//	Now render the beam. We're using stream draw - since we'll never render more than (MaxLassoSegments + MaxBeamSegments)
	//	we can pre-allocate a VBO for that much, and just stream-update the amount we're actually drawing
	//
	
	shader.uniform( "Color", Vec4( beamState.color.xyz(), beamState.color.w * beamState.transition ));

	{
		const GLsizei 
			Stride = sizeof( beam_vertex ),
			PositionOffset = 0,
			EdgeAttrOffset = sizeof( Vec2 );

		const unsigned int 
			Count = beamState.segmentsDrawn * 2;

		if ( !beamState.vbo )
		{
			const unsigned int 
				MaxCount = MaxLassoSegments + MaxBeamSegments * 2;

			glGenBuffers( 1, &beamState.vbo );
			glBindBuffer( GL_ARRAY_BUFFER, beamState.vbo );
			glBufferData( GL_ARRAY_BUFFER, 
						 MaxCount * Stride, 
						 NULL,
						 GL_STREAM_DRAW );
		}
		else 
		{
			glBindBuffer( GL_ARRAY_BUFFER, beamState.vbo );
		}

		glBufferSubData( GL_ARRAY_BUFFER, 0, Count * Stride, &(beamState.vertices.front()) );
		glVertexPointer( 2, GL_REAL, Stride, (void*)PositionOffset );
		glVertexAttribPointer( _beamShaderEdgeAttrLoc, 1, GL_REAL, GL_FALSE, Stride, (void*) EdgeAttrOffset );

		glDrawArrays( GL_QUAD_STRIP, 0, Count );
		
		glBindBuffer( GL_ARRAY_BUFFER, 0 );	
	}
}

void MagnetoBeamRenderer::_debugDrawBeam( const core::render_state &state, beam_state &beamState )
{
	gl::color( Color(beamState.color.x,beamState.color.y,beamState.color.z));

	const GLsizei
		Stride = sizeof( beam_vertex );

	const unsigned int 
		Count = beamState.segmentsDrawn * 2;

	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	glEnableClientState( GL_VERTEX_ARRAY );
	glVertexPointer( 2, GL_REAL, Stride, &(beamState.vertices.front().position ));
	glDrawArrays( GL_QUAD_STRIP, 0, Count );
	glDisableClientState( GL_VERTEX_ARRAY );
	
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
}


#pragma mark - MagnetoBeamController

MagnetoBeamController::MagnetoBeamController():
	_spin(0),
	_deltaSpin(0)
{
	setName( "MagnetoBeamController" );
}

MagnetoBeamController::~MagnetoBeamController()
{}

void MagnetoBeamController::update( const time_state &time )
{
	MagnetoBeam *magnetoBeam = this->magnetoBeam();
	Player *player = static_cast<GameLevel*>(level())->player();


	if ( player->alive() )
	{
		//
		//	Compute aim & range
		//

		const Vec2 
			MousePosScreen = mousePosition(),
			MousePosWorld = level()->camera().screenToWorld( MousePosScreen );

		magnetoBeam->setTowPosition( lrp( 0.5, magnetoBeam->towPosition(), MousePosWorld ) );
		
		//
		//	Now update load rotation
		//

		real newSpin = lrp<real>(0.1, _spin, _spin + _deltaSpin );
		_deltaSpin *= 0.5;
		
		real delta = newSpin - _spin;
		_spin = newSpin;
		
		magnetoBeam->rotateTowBy( delta );
	}
}

bool MagnetoBeamController::mouseDown( const ci::app::MouseEvent &event )
{
	if ( active() ) 
	{
		magnetoBeam()->setFiring(true);
		return true;
	}
	
	return false;
}

bool MagnetoBeamController::mouseUp( const ci::app::MouseEvent &event )
{
	if( active() && event.isLeft() ) 
	{
		magnetoBeam()->setFiring(false);
		return true;
	}
	
	return false;
}

bool MagnetoBeamController::mouseWheel( const ci::app::MouseEvent &event )
{
	if ( active() )
	{
		real wheel = event.getWheelIncrement();
		_deltaSpin += wheel * 0.5;
		return true;
	}
	
	return false;
}

} // end namespace game
