//
//  ShubNiggurath.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 12/10/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#include "ShubNiggurath.h"

#include "FilterStack.h"
#include "Spline.h"

#include <cinder/gl/gl.h>
#include <chipmunk/chipmunk_unsafe.h>

using namespace ci;
using namespace core;
namespace game {

#pragma mark - Tentacle

/*
		bool _dead;
		init _initializer;
		cpSpace *_space;
		std::vector< segment > _segments;
		real _fudge;		
*/

Tentacle::Tentacle():
	GameObject( GameObjectType::SHUB_NIGGURATH_TENTACLE ),
	_dead(false),
	_space(NULL),
	_fudge( Rand::randFloat( 0.5, 1.5 ) ),
	_color(1,1,1,1)
{
	setName( "Tentacle" );
	setLayer( RenderLayer::MONSTER + 90 );
	setVisibilityDetermination( VisibilityDetermination::FRUSTUM_CULLING );
	addComponent( new TentacleRenderer() );
}

Tentacle::~Tentacle()
{
	aboutToDestroyPhysics(this);
	_destroySegments( _segments.begin() );
}

void Tentacle::initialize( const init &initializer )
{
	GameObject::initialize(initializer);
	_initializer = initializer;
}

void Tentacle::addedToLevel( Level *level )
{
	ShubNiggurath *shubNiggurath = this->shubNiggurath();

	_space = level->space();
	
	const Vec2r 
		Dir( std::cos( _initializer._angleOffset ), std::sin( _initializer._angleOffset )),
		InitialPosition( shubNiggurath->position() + Dir * _initializer._distance );

	Vec2r
		position(InitialPosition);

	real
		segmentWidth = _initializer.segmentWidth,
		segmentLength = _initializer.segmentLength,
		segmentWidthIncrement = segmentWidth * real(-1) / _initializer.numSegments;

	const real 
		Gravity = cpvlength( cpSpaceGetGravity(_space )),
		TorqueMax = segmentWidth * segmentLength * _initializer.segmentDensity * _initializer.numSegments * Gravity * 2,
		TorqueMin = TorqueMax * 0.25,
		AngularRangeMax = M_PI_2,
		AngularRangeMin = AngularRangeMax * 0.125;

		
	cpBody *previousBody = shubNiggurath->body();
	
	for( int i = 0; i < _initializer.numSegments; i++ )
	{
		real distanceAlongTentacle = real(i) / _initializer.numSegments;
	
		segment seg;
		seg.width = segmentWidth;
		seg.length = segmentLength;
		seg.scale = 1;
		
		//
		//	Create box-shaped body and shape - note, we're using the shubNiggurath
		//	as the shapes' group. 
		//
		
		const real 
			Mass = std::max<real>( seg.width * seg.length * _initializer.segmentDensity, 0.1 ),
			Moment = cpMomentForBox( Mass, seg.width, seg.length );
					
		seg.body = cpBodyNew( Mass, Moment );
		cpSpaceAddBody( _space, seg.body );
		cpBodySetPos( seg.body, cpv(position + Dir * seg.length/2 ));
		cpBodySetAngle( seg.body, _initializer._angleOffset - M_PI_2 );
		cpBodySetUserData( seg.body, shubNiggurath );
		
		seg.shape = cpBoxShapeNew( seg.body, std::max<real>( seg.width, 1 ), seg.length );
		cpSpaceAddShape( _space, seg.shape );
		cpShapeSetUserData( seg.shape, shubNiggurath );
		cpShapeSetCollisionType( seg.shape, CollisionType::MONSTER );
		cpShapeSetLayers( seg.shape, CollisionLayerMask::TENTACLE );
		cpShapeSetGroup( seg.shape, cpGroup(shubNiggurath));
		cpShapeSetFriction( seg.shape, 0 );
		cpShapeSetElasticity( seg.shape, 0 );
		
		//
		//	Create joints
		//

		seg.angularRange = lrp( distanceAlongTentacle, AngularRangeMin, AngularRangeMax );
		seg.torque = lrp( distanceAlongTentacle, TorqueMax, TorqueMin ); 
		
		seg.joint = cpPivotJointNew( previousBody, seg.body, cpv(position));		
		cpSpaceAddConstraint( _space, seg.joint );

		seg.rotation = cpGearJointNew( previousBody, seg.body, 0, 1 );
		cpSpaceAddConstraint( _space, seg.rotation );
		cpConstraintSetMaxForce( seg.rotation, seg.torque );
		cpConstraintSetMaxBias( seg.rotation, M_PI * 0.1 );
		
		real angle = i == 0 ? (_initializer._angleOffset-M_PI_2) : 0;
		seg.angularLimit = cpRotaryLimitJointNew( previousBody, seg.body, angle - seg.angularRange, angle + seg.angularRange );
		cpSpaceAddConstraint( _space, seg.angularLimit );
		cpConstraintSetMaxForce( seg.angularLimit, seg.torque );
		cpConstraintSetMaxBias( seg.angularLimit, M_PI * 0.1 );
		
		
		//
		//	Add this segment
		//
		
		_segments.push_back( seg );
		_segmentIndexForShape[ seg.shape ] = i;
		
		
		//
		//	Move forward to end of this segment, and apply size falloff
		//
		
		position += Dir * segmentLength;
		previousBody = seg.body;

		segmentLength *= 0.9;
		segmentWidth = std::max<real>( segmentWidth + segmentWidthIncrement, 0.05 );
	}
	
	//
	//	Now, we're going to squish the bodies all to the origin point, while setting the 
	//	pivot joint's force to zero. In update() we'll ramp the force to a normal level,
	//	allowing the tentacle to "grow" outwards
	//
	
	for( segmentvec::iterator segment(_segments.begin()),end(_segments.end()); segment!=end; ++segment )
	{
		cpBodySetPos( segment->body, cpv(InitialPosition) );
		cpConstraintSetMaxForce( segment->joint, 0 );
	}
	
	
	_color.x = _color.y = _color.z = lrp<real>( _initializer._z, 0.75, 1 );
	_color.w = 1;
}

void Tentacle::update( const time_state &time )
{
	using std::sin;
	using std::cos;
	using std::acos;
	using std::asin;
	using std::sqrt;
	using std::pow;

	ShubNiggurath *shub = shubNiggurath();
	MonsterController *controller = shub->controller();
	cpBB bounds = cpBBInvalid;

	real 
		factor = 0.75 * _fudge,
		phaseOffset = _fudge * M_PI,
		distanceAlongTentacle = 0,
	    distanceAlongTentacleIncrement = 1.0 / _segments.size(),		
		initialAngleOffset = _initializer._angleOffset - M_PI_2,
		aggression = controller ? controller->aggression() : 0,
		lifecycle = shub->lifecycle();//pow( shub->lifecycle(), static_cast<real>(2) );

	seconds_t 
		timeSinceFirstDisconnection = 0,
		orphanTime = 5,	//	time a severed tentacle lives before withering
		witherTime = 5;	//	time it takes to wither and disappear


	Vec2r playerPosition = shub->controller()->lastKnownPlayerPosition();

	const bool	
		CanSever = shub->TENTACLES_CAN_BE_SEVERED;

	bool 
		connected = true, 
		tentacleDead = this->dead();

	segmentvec::iterator firstDeadSegment = _segments.end();
	std::set<segmentvec::iterator> severedSegments;
		
	for( segmentvec::iterator segment(_segments.begin()),end(_segments.end()); segment!=end; ++segment )
	{
		bounds = cpBBMerge( bounds, cpShapeGetBB( segment->shape ));
		segment->position = v2r(cpBodyLocal2World( segment->body, cpv(0,-segment->length/2)));

		cpVect rot = cpBodyGetRot( segment->body ); // rot is the body's unit +x vector
		segment->dir.x = -rot.y;
		segment->dir.y = rot.x;
		
		if ( !segment->connected() ) 
		{
			connected = false;
			if ( timeSinceFirstDisconnection == 0 )
			{
				timeSinceFirstDisconnection = time.time - segment->disconnectionTime;
			}
		}
				
		//
		//	If this segment has been disconnected, the constraints may be null. If not, we want to apply motion!
		//

		if ( segment->rotation )
		{
			real angle = segment->angularRange * sin( (factor * time.time) + phaseOffset );
			
			//
			//	If the ShubNiggurath is aggressive, the player is near. We want to point the tentacle segment
			//	towards the player to act out the appearance of aggression.
			//

			if ( aggression > Epsilon )
			{
				Vec2r 
					segmentDirLocal(0,1),	// segments all face along +y
					segmentRightLocal(1,0),
					dirToPlayerLocal = normalize( v2r(cpBodyWorld2Local( segment->body, cpv(playerPosition))));
				 
				real deflection = dot( segmentDirLocal, dirToPlayerLocal ),
					 correctedDeflectionAngle = 0;

				if ( dot( segmentRightLocal, dirToPlayerLocal ) > 0 )
				{
					// ccw deflection
					correctedDeflectionAngle = -acos( saturate(deflection));
				}
				else
				{
					// cw deflection
					correctedDeflectionAngle = +acos( saturate(deflection));
				}

				angle += aggression * sqrt(distanceAlongTentacle) * correctedDeflectionAngle;
				angle = clamp(angle, -segment->angularRange, segment->angularRange);
			}
				
			//
			//	Apply the computed angle to this segment
			//

			cpGearJointSetPhase( segment->rotation, initialAngleOffset + angle ); 

			//
			//	Ramp down torque when spawnpoint dies, or if the tentacle has been severed
			//
			
			if ( (!connected || tentacleDead || !shub->alive()) && cpConstraintGetMaxForce( segment->rotation ) > Epsilon )
			{
				cpConstraintSetMaxForce( segment->rotation, cpConstraintGetMaxForce( segment->rotation ) * 0.95 );
			}				
		}

		//
		//	Severed tentacles wither; when the first disconnected segment in the tentacle
		//	has withered away completely, mark that as the first in the dead list, and we'll
		//	free them outside the loop.
		//
		//	The first if ( !connected ) pass acts on all segments downstream of a disconnected segment
		//	The second checks the actual disconnected segment, and kills it and downsteam children
		//	when it has completely withered.
		//
		
		if ( !connected )
		{
			if ( timeSinceFirstDisconnection > orphanTime )
			{
				real fadeOut = saturate<real>( (timeSinceFirstDisconnection - orphanTime) / witherTime );
				segment->scale = 1 - fadeOut;
			}
		}
		else
		{
			//
			//	apply lifecycle scaling
			//
			
			segment->scale = lifecycle;
		}
		
		if ( segment->disconnectionTime > 0 )
		{
			if ( segment->width < Epsilon && firstDeadSegment == _segments.end() )
			{
				firstDeadSegment = segment;
			}
		}
		
		//
		//	If a segment has reached maximum heat we either sever segment, or kill whole tentacle
		//	depending on ShubNiggurath::TENTACLES_CAN_BE_SEVERED
		//
		
		if ( segment->heat >= 1 - Epsilon )
		{
			if ( CanSever )
			{
				severedSegments.insert( segment );
			}
			else
			{
				tentacleDead = true;
			}
		}

		if ( segment->isBeingShot )
		{
			segment->isBeingShot = false;
		}
		else
		{
			// "cool down"
			segment->heat *= 0.995;
		}
		
					
		//
		//	Move on for next segment
		//

		initialAngleOffset = 0;		
		phaseOffset += M_PI_4;
		distanceAlongTentacle += distanceAlongTentacleIncrement;
	}
	
	//
	//	Apply Shub's lifecycle to segment joint force - this allows the initially compacted tentacle joints
	//	to "spring" out as shub is born
	//
	
	if ( shub->alive() && (lifecycle < 1))
	{		
		for( segmentvec::iterator segment(_segments.begin()),end(_segments.end()); segment!=end; ++segment )
		{
			cpConstraintSetMaxForce( segment->joint, segment->torque * 10 * lifecycle );
		}
	}
	
	

	//
	//	If any segments overheated and need to be severed, do so
	//

	if ( CanSever )
	{
		if ( !severedSegments.empty() )
		{
			for( std::set<segmentvec::iterator>::iterator 
				seg(severedSegments.begin()),end(severedSegments.end()); 
				seg != end; 
				++seg )
			{
				disconnectSegment( *seg - _segments.begin() );
			}
		}

		//
		//	If we found a severed tentacle which has timed out, remove it
		//
		
		if ( firstDeadSegment != _segments.end() )
		{
			_destroySegments( firstDeadSegment );
		}
	}
	else if ( tentacleDead )
	{
		if ( tentacleDead && !this->dead() )
		{
			die();
		}
	}
	
	
	setAabb( bounds );
}

int Tentacle::indexOfSegmentForShape( cpShape *shape ) const
{
	std::map< cpShape*, int >::const_iterator pos = _segmentIndexForShape.find(shape);
	if ( pos != _segmentIndexForShape.end() )
	{
		return pos->second;
	}
	
	return -1;
}

void Tentacle::disconnectSegment( int idx )
{
	if ( idx >= 0 && idx < _segments.size() )
	{
		segment &s = _segments[idx];
		if ( s.joint && s.rotation && s.angularLimit )
		{
			cpSpaceRemoveConstraint( _space, s.joint );
			cpSpaceRemoveConstraint( _space, s.rotation );
			cpSpaceRemoveConstraint( _space, s.angularLimit );
			
			cpConstraintFree( s.joint );
			cpConstraintFree( s.rotation );
			cpConstraintFree( s.angularLimit );
			
			s.joint = s.rotation = s.angularLimit = NULL;		
			s.disconnectionTime = level()->time().time;
			
			shubNiggurath()->_tentacleSevered( this, idx );
		}
		
		for ( segmentvec::iterator segment( _segments.begin() + idx ), end( _segments.end() ); segment != end; ++segment )
		{
			cpShapeSetLayers( segment->shape, CollisionLayerMask::Layers::TERRAIN_BIT );
		}
	}
}

real Tentacle::length() const
{
	real l = 0;
	foreach( const segment &s, _segments ) 
	{
		l += s.length;
	}
	
	return l;
}

void Tentacle::_destroySegments( segmentvec::iterator firstToDestroy )
{
	if ( !_space ) return;

	//
	//	We have to disconnect joints, then free bodies - could use a reverse_iterator but I don't
	//	know how to initialize one from a forward iterator.
	//
	
	for( segmentvec::iterator segment(firstToDestroy),end(_segments.end()); segment!=end; ++segment )
	{
		if ( segment->joint )
		{
			cpSpaceRemoveConstraint( _space, segment->joint );
			cpConstraintFree( segment->joint );
			segment->joint = NULL;
		}

		if ( segment->rotation )
		{
			cpSpaceRemoveConstraint( _space, segment->rotation );
			cpConstraintFree( segment->rotation );
			segment->rotation = NULL;
		}

		if ( segment->angularLimit )
		{
			cpSpaceRemoveConstraint( _space, segment->angularLimit );
			cpConstraintFree( segment->angularLimit );
			segment->angularLimit = NULL;
		}
	}

	for( segmentvec::iterator segment(firstToDestroy),end(_segments.end()); segment!=end; ++segment )
	{
		if ( segment->shape )
		{
			_segmentIndexForShape.erase( segment->shape );

			cpSpaceRemoveShape( _space, segment->shape );
			cpShapeFree( segment->shape );
			segment->shape = NULL;
		}
	
		if ( segment->body )
		{
			cpSpaceRemoveBody( _space, segment->body );
			cpBodyFree( segment->body );
			segment->body = NULL;
		}
	}

	
	_segments.erase( firstToDestroy, _segments.end());
}


#pragma mark - ShubNiggurath

CLASSLOAD(ShubNiggurath);

/*
		init _initializer;
		bool _firing;
		std::vector< Tentacle* > _tentacles;
		real _aggressionDistance, _grubs;
		seconds_t _lastGrubEmissionTime;
		cpShape *_tentacleAttachmentShape;
*/

ShubNiggurath::ShubNiggurath():
	MotileFluidMonster( GameObjectType::SHUB_NIGGURATH ),
	_firing(false),
	_aggressionDistance(0),
	_grubs(0),
	_lastGrubEmissionTime(0),
	_tentacleAttachmentShape(NULL)
{
	setName( "ShubNiggurath" );
	fluid()->setLayer( RenderLayer::MONSTER + 100 );
	setController( new ShubNiggurathController() );
}

ShubNiggurath::~ShubNiggurath()
{
	if ( space() )
	{
		//
		//	Detach and destroy all child tentacles
		//

		foreach( Tentacle *t, _tentacles )
		{
			removeChild(t);

			if ( t->level() ) 
			{
				t->level()->removeObject( t );
			}
						
			delete t;
		}
		
		_tentacles.clear();
		
		if ( _tentacleAttachmentShape )
		{
			cpSpaceRemoveShape(space(), _tentacleAttachmentShape );
			cpShapeFree( _tentacleAttachmentShape );
			_tentacleAttachmentShape = NULL;
		}
	}
}

void ShubNiggurath::initialize( const init &initializer )
{
	MotileFluidMonster::initialize( initializer );
	_initializer = initializer;
	_grubs = _initializer.maxGrubs;
	
	if ( _initializer.tentacles.empty() )
	{
		Tentacle::init tentacleInitDefault;		
		tentacleInitDefault.numSegments = 8;
		tentacleInitDefault.segmentLength = 2;
		tentacleInitDefault.segmentWidth = 1;

		for ( int i = 0; i < 6; i++ )
		{
			Tentacle::init tentacleInit = tentacleInitDefault;
			tentacleInit.numSegments += Rand::randInt(0,4);
			tentacleInit.segmentLength *= Rand::randFloat(0.7,1.25);
			tentacleInit.segmentWidth *= Rand::randFloat(0.7,1.25);
						
			_initializer.tentacles.push_back( tentacleInit );
		}
	}
	
}

void ShubNiggurath::addedToLevel( Level *level )
{
	MotileFluidMonster::addedToLevel(level);
	
	if ( false )
	{
		_tentacleAttachmentShape = cpCircleShapeNew( body(), _initializer.fluidInit.radius, cpvzero );
		cpSpaceAddShape( space(), _tentacleAttachmentShape );
		cpShapeSetUserData( _tentacleAttachmentShape, this );
		cpShapeSetGroup( _tentacleAttachmentShape, cpGroup(this) );
		cpShapeSetFriction( _tentacleAttachmentShape, 0 );
		cpShapeSetCollisionType( _tentacleAttachmentShape, CollisionType::MONSTER );
		cpShapeSetLayers( _tentacleAttachmentShape, CollisionLayerMask::TENTACLE );
	}
	
	//
	//	Create tentacles
	//

	int tick = 0;
	std::random_shuffle( _initializer.tentacles.begin(), _initializer.tentacles.end());
	real 
		arcWidth = toRadians( real(45) ),
		arcStart = toRadians( real(90) ) - arcWidth/2,
		arcIncrement = arcWidth / _initializer.tentacles.size(),
		arcAngle = arcStart,
		z = 0,
		zIncrement = real(1) / (_initializer.tentacles.size()-1),
		averageTentacleLength = 0;
		
	app::console()
		<< "arcStart: " << toDegrees( arcStart )
		<< " arcWidth: " << toDegrees( arcWidth )
		<< std::endl;
	
	foreach( Tentacle::init ti, _initializer.tentacles )
	{
		ti._distance = _initializer.fluidInit.radius * 0.5;
		ti._angleOffset = arcAngle + Rand::randFloat( -arcIncrement/2, arcIncrement/2 );
		ti._z = z;
		ti._idx = tick++;

		arcAngle += arcIncrement;
		z += zIncrement;
	
		Tentacle *tentacle = new Tentacle();
		tentacle->initialize( ti );
		addChild( tentacle );
		
		_tentacles.push_back( tentacle );
		
		averageTentacleLength += tentacle->length();
	}	
	
	//
	//	The ShubNiggurath gets aggressive when the player comes within two times the avg tentacle length
	//

	averageTentacleLength /= _tentacles.size();
	_aggressionDistance = 2 * averageTentacleLength;	
}

void ShubNiggurath::draw( const render_state &state )
{
	MotileFluidMonster::draw(state);
	
	if ( state.mode == RenderMode::DEBUG && controller() && controller()->playerVisible() )
	{
		real vel = 10;
		Vec2r dir;
		int tries = 10;

		if ( _aim( controller()->lastKnownPlayerPosition(), vel, dir, tries ))
		{
			gl::color( 1,1,0,1 );
			gl::drawLine( eyePosition(), eyePosition() + vel * dir );
		}
	}
}

void ShubNiggurath::update( const time_state &time )
{
	MotileFluidMonster::update( time );

	//
	//	I'm looking for a vomit kind of effect. When attack starts, we can only fire if we have more than 75%
	//	of our ammo available. Once firing, we fire until attack stops or ammo is depleted.
	//
	
	if ( attacking() )
	{
		const real 
			numGrubsToStartFiring = _initializer.maxGrubs * 0.75;
	
		if ( !_firing && _grubs >= numGrubsToStartFiring )
		{
			_firing = true;
		}
	}
	else
	{
		_firing = false;
	}
	
	if ( _firing )
	{
		seconds_t timeSinceLastEmission = time.time - _lastGrubEmissionTime;
		if ( timeSinceLastEmission > (1/_initializer.fireGrubsPerSecond) && _grubs >= 1 )
		{
			//
			// emit a grub
			//

			_emitGrub();			
			_lastGrubEmissionTime = time.time;
			_grubs--;
			
			if ( _grubs < 1 )
			{
				_firing = false;
			}			
		}	
	}
	else
	{
		//
		//	Replenish ammo
		//

		_grubs = std::min<real>( _grubs + _initializer.growGrubsPerSecond * time.deltaT, _initializer.maxGrubs );
	}
	
}

void ShubNiggurath::injured( const HealthComponent::injury_info &info )
{
	if ( info.shape )
	{
		if ( fluid()->fluidShapes().count( info.shape ))
		{
			_handleBodyInjury( info );
		}
		else
		{
			_handleTentacleInjury( info );
		}
	}
	
	real hue = 1 - (health()->health() / health()->maxHealth());	
	fluid()->setFluidToneMapHue( std::min<real>( hue, 1 - ALPHA_EPSILON ));

	MotileFluidMonster::injured( info );
}

void ShubNiggurath::died( const HealthComponent::injury_info &info )
{
	//
	// after death, I want the spawnpoint to collide only with terrain. 
	//

	foreach( Tentacle *tentacle, _tentacles )
	{
		foreach( Tentacle::segment &seg, tentacle->segments() )
		{
			cpShapeSetLayers( seg.shape, CollisionLayerMask::DEAD_MONSTER );
			cpShapeSetGroup( seg.shape, cpGroup(this));
		}
	}
	
	foreach( cpShape *fluidShape, fluid()->fluidShapes() )
	{
		cpShapeSetLayers( fluidShape, CollisionLayerMask::DEAD_MONSTER );
		cpShapeSetGroup( fluidShape, cpGroup(this));
	}
	
	MotileFluidMonster::died( info );
}

bool ShubNiggurath::allowWeaponHit( Weapon *weapon, cpShape *shape ) const
{
	if ( alive() )
	{
		//
		//	If the weapon is hitting a tentacle, only allow hits to living tentacles.
		//

		for( std::vector< Tentacle* >::const_iterator 
			tentacle( _tentacles.begin() ),
			end( _tentacles.end());
			tentacle != end;
			++tentacle )
		{
			int idx = (*tentacle)->indexOfSegmentForShape( shape );
			if ( idx >= 0 )
			{
				return !(*tentacle)->dead();
			}			
		}
	
		//
		// Otherwise, we're hitting the body, so allow it
		//

		return true;
	}
	
	return false;
}

void ShubNiggurath::_handleBodyInjury( const HealthComponent::injury_info &info )
{
}

void ShubNiggurath::_handleTentacleInjury( const HealthComponent::injury_info &info )
{
	Tentacle *shotTentacle = NULL;
	int shotTentacleSegment = -1;

	for( std::vector< Tentacle* >::iterator 
		tentacle( _tentacles.begin() ),
		end( _tentacles.end());
		tentacle != end;
		++tentacle )
	{
		int idx = (*tentacle)->indexOfSegmentForShape( info.shape );
		if ( idx >= 0 )
		{
			shotTentacle = *tentacle;
			
			//
			//	If tentacle-severing is supported, find the segment which was hit
			//
			
			if ( TENTACLES_CAN_BE_SEVERED )
			{
				if ( idx < shotTentacle->segments().size() - 1 )
				{
					Vec2r a( shotTentacle->segments()[idx].position ),
						  b( shotTentacle->segments()[idx+1].position );
						  
					if ( info.position.distanceSquared(a) < info.position.distanceSquared(b) )
					{
						shotTentacleSegment = idx;
					}
					else
					{
						shotTentacleSegment = idx+1;
					}							  
				}
				else
				{			
					shotTentacleSegment = idx;
				}
			}

			break;
		}
	}
	
	if ( shotTentacle )
	{
		//
		//	We want to "heat up" the tentacle where it was shot or the whole tentacle if we don't support severing.
		//	Resistance is the mass of the first tentacle segment -- this applies to all segments, since
		//	shooting end segments would be too easy since their resistance falls to zero.
		//

		const real 
			heatResistance = shotTentacle->initializer().segmentWidth * shotTentacle->initializer().segmentLength * shotTentacle->initializer().segmentDensity,
			deltaHeat = info.grievousness / heatResistance;
		
		if ( TENTACLES_CAN_BE_SEVERED )
		{
			Tentacle::segment &segment = shotTentacle->segments()[shotTentacleSegment];
			
			segment.heat = saturate( segment.heat + deltaHeat );
			segment.isBeingShot = true;
		}
		else
		{
			for ( Tentacle::segmentvec::iterator 
				segment( shotTentacle->segments().begin()), end( shotTentacle->segments().end());
				segment != end; 
				++segment )
			{
				segment->heat = saturate(segment->heat + deltaHeat );
				segment->isBeingShot = true;
			}
		}
	}	
}

void ShubNiggurath::_tentacleSevered( Tentacle *tentacle, int idx )
{
	assert( !TENTACLES_CAN_BE_SEVERED );

	if ( idx >= 0 && idx < tentacle->segments().size() )
	{
		//
		//	Create acid burst!
		//

		Tentacle::segment &segment = tentacle->segments()[idx];
		real vel = cpvlength( cpSpaceGetGravity( space() ));		
		
		for ( int i = 0; i < 8; i++ )
		{
			((GameLevel*)level())->emitAcid( 
				segment.position, 
				(segment.dir * vel) + Vec2r( Rand::randFloat(-0.25,0.25),Rand::randFloat(-0.25,0.25)));
		}
		
		//
		//	Propel severed tentacle
		//
		
		cpBodyApplyImpulse( segment.body, cpv(segment.dir * 2 * vel * level()->time().deltaT ), cpvzero );
		
		//
		//	Now, reduce spawnpoint's resistance -- if all tentacles are severed the resistance goes to zero
		//
		
		real initialResistance = _initializer.health.resistance,
		     amountSevered = 1 - ( real(idx) / tentacle->initializer().numSegments ),
			 reduction = initialResistance * amountSevered / _tentacles.size();
				
		health()->setResistance( health()->resistance() - reduction );		
	}
}

void ShubNiggurath::_emitGrub()
{
	Grub::init init = _initializer.grubInit;
	init.position = this->eyePosition();

	Grub *grub = new Grub();
	grub->setLayer(fluid()->layer() + 1 );
	grub->initialize( init );
	
	level()->addObject( grub );

	real launchVel = 10;
	Vec2r dir;
	int tries = 10;
	
	if ( !(controller() && controller()->playerVisible() && _aim( controller()->lastKnownPlayerPosition(), launchVel, dir, tries )))
	{
		// upwards, and a bit left or right
		dir = normalize( Vec2r( (Rand::randBool() ? -1:1) * Rand::randFloat(0.25, 0.50), 1 ));
		launchVel = cpvlength( cpSpaceGetGravity( space() ));
	}
	
	//
	//	Launch, with a little extra oomph. We'll reduce applied velocity by each segment of grub
	//	to cause the grub to orient a little. This is highly fudgy, but it prevents the grub stream
	//	from being too narrow.
	//

	cpVect lv = cpv( dir * launchVel * Rand::randFloat(0.95, 1.25) );
	foreach( cpBody *b, grub->bodies() )
	{
		cpBodySetVel( b, lv );
		lv = cpvmult(lv, 0.985);
	}		

}

bool ShubNiggurath::_aim( const Vec2r &targetPosition, real &linearVel, Vec2r &dir, int triesRemaining )
{
	if ( triesRemaining <= 0 ) return false;

	/*
		Implementation adapted from:
		http://en.wikipedia.org/wiki/Trajectory_of_a_projectile#Angle_.CE.B8_required_to_hit_coordinate_.28x.2Cy.29
	*/
	
	const Vec2r 
		dp = targetPosition - eyePosition();

	const real
		x2 = dp.x * dp.x,
		//y2 = dp.y * dp.y,
		g = cpvlength(cpSpaceGetGravity(space())),
		gx = g * dp.x,
		lv2 = linearVel * linearVel,
		lv4 = lv2 * lv2,
		t2 = lv4 - ( g * ( (g * x2) + (2 * dp.y * lv2 )) );
		
	//
	//	If t2 is < 0, that means there's no way to hit the target with the given linear velocity,
	//	so we'll give a shot with greater and lesser vels. We'll preferentially take the lesser path if possible.
	//

	if ( t2 < 0 )
	{
		real 
			lesserVel = linearVel * 0.8,
			greaterVel = linearVel * 1.2;
			
		if ( _aim( targetPosition, lesserVel, dir, triesRemaining - 1 ) )
		{
			linearVel = lesserVel;
			return true;
		}
		else if ( _aim( targetPosition, greaterVel, dir, triesRemaining - 1 ) )
		{
			linearVel = greaterVel;
			return true;
		}
		
		return false;
	}
		
	const real t = std::sqrt( t2 );
		
	const real 
		theta0 = std::atan2( (lv2 + t),gx ),
		theta1 = std::atan2( (lv2 - t),gx );
		
	Vec2r 
		aim0( std::cos( theta0 ), std::sin( theta0 )),
		aim1( std::cos( theta1 ), std::sin( theta1 ));

	//
	//	We're taking the arc with a lower height, arbitrarily why not.
	//
	
	dir = aim0.y < aim1.y ? aim0 : aim1;
	return true;
}

#pragma mark - ShubNiggurathController

ShubNiggurathController::ShubNiggurathController()
{
	setName( "ShubNiggurathController" );
}

void ShubNiggurathController::update( const time_state &time )
{
	MotileFluidMonsterController::update( time );
	
	ShubNiggurath *shub = shubNiggurath();
	bool canAttack = false;
	
	if ( playerVisible() )
	{
		real d2 = distanceSquared(lastKnownPlayerPosition(), shub->eyePosition()),
		     threshold = shub->initializer().maxGrubAttackDistance;

		if ( d2 <= (threshold * threshold))
		{
			canAttack = true;
		}	
	}
	
	shub->attack( canAttack );
}



#pragma mark - TentacleRenderer

/*
		GLuint _vbo;
		ci::gl::GlslProg _shader;
		GLint _heatAttrLoc, _opacityAttrLoc;

		spline_rep_vec _splineReps;
		vertex_vec _quads;
*/

TentacleRenderer::TentacleRenderer():
	_vbo(0),
	_heatAttrLoc(0),
	_opacityAttrLoc(0)
{
	setName( "TentacleRenderer" );
}

TentacleRenderer::~TentacleRenderer()
{
	if ( _vbo )
	{
		glDeleteBuffers( 1, &_vbo );
	}
}

void TentacleRenderer::draw( const render_state &state )
{
	_updateSplineReps();
	_decompose();

	DrawComponent::draw( state );
}

void TentacleRenderer::_drawGame( const render_state &state )
{
	if ( _quads.empty() )
	{
		return;
	}

	Tentacle *tentacle = this->tentacle();
	ShubNiggurath *shubNiggurath = tentacle->shubNiggurath();
	
	const GLsizei 
		stride = sizeof( vertex ),
		positionOffset = 0,
		texCoordOffset = sizeof( Vec2f ),
		heatOffset = sizeof( Vec2f ) + sizeof( Vec2f ),
		opacityOffset = sizeof( Vec2f ) + sizeof( Vec2f ) + sizeof( float );
	
	if ( !_shader )
	{
		_shader = level()->resourceManager()->getShader( "TentacleShader" );
		_heatAttrLoc = _shader.getAttribLocation( "Heat" );
		_opacityAttrLoc = _shader.getAttribLocation( "Opacity" );
	}
	
	if ( !_tentacleTexture )
	{
		_tentacleTexture = level()->resourceManager()->getTexture( shubNiggurath->initializer().tentacleTexture );
	}
	
	if ( !_toneMapTexture )
	{
		_toneMapTexture = level()->resourceManager()->getTexture( shubNiggurath->initializer().fluidInit.fluidToneMapTexture );
	}
		 
	_tentacleTexture.bind(0);
	_tentacleTexture.setWrap( GL_REPEAT, GL_REPEAT );
	
	_toneMapTexture.bind(1);
	glActiveTexture( GL_TEXTURE1 );
	_toneMapTexture.setWrap( GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE );
	
	float injuryLevel = 1 - ( shubNiggurath->health()->health() / shubNiggurath->health()->maxHealth() );
					
	_shader.bind();
	_shader.uniform( "ModulationTexture", 0 );
	_shader.uniform( "ColorTexture", 1 );
	_shader.uniform( "ColorTextureCoord", Vec2f( 0, injuryLevel ) );
	_shader.uniform( "Color", tentacle->color() );

							
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	
	if ( !_vbo )
	{
		glGenBuffers( 1, &_vbo );
		glBindBuffer( GL_ARRAY_BUFFER, _vbo );
		glBufferData( GL_ARRAY_BUFFER, 
					  _quads.size() * stride, 
					  NULL,
					  GL_STREAM_DRAW );
	}
	
	glBindBuffer( GL_ARRAY_BUFFER, _vbo );
	glBufferSubData( GL_ARRAY_BUFFER, 0, _quads.size() * stride, &(_quads.front()));

	glEnableClientState( GL_VERTEX_ARRAY );
	glVertexPointer( 2, GL_REAL, stride, (void*)positionOffset );

	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glTexCoordPointer( 2, GL_REAL, stride, (void*)texCoordOffset );
	
	glEnableVertexAttribArray( _heatAttrLoc );
	glVertexAttribPointer( _heatAttrLoc, 1, GL_REAL, GL_FALSE, stride, (void*) heatOffset );

	glEnableVertexAttribArray( _opacityAttrLoc );
	glVertexAttribPointer( _opacityAttrLoc, 1, GL_REAL, GL_FALSE, stride, (void*) opacityOffset );

	glDrawArrays( GL_QUADS, 0, _quads.size() );

	//
	//	Clean up
	//

	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );	
	glDisableVertexAttribArray( _heatAttrLoc );
	glDisableVertexAttribArray( _opacityAttrLoc );
	
	_shader.unbind();
	_tentacleTexture.unbind(0);
	_toneMapTexture.unbind(1);
}

void TentacleRenderer::_drawDebug( const render_state &state )
{
	Tentacle *tentacle = this->tentacle();
	Mat4r R;
	real 
		rcpZoom = state.viewport.reciprocalZoom(),
		unitVectorLength = 10 * rcpZoom;
	
	ColorA 
		color = tentacle->shubNiggurath()->debugColor(),
		heatColor( 1,1,1,1 );

	color.a = 0.25;
	
	gl::enableAlphaBlending();
	gl::disableWireframe();
	gl::color( color );
	
	foreach( const Tentacle::segment &segment, tentacle->segments() )
	{
		cpBody_ToMatrix( segment.body, R );
		gl::pushModelView();
		gl::multModelView( R );
		
		gl::color( color.lerp( segment.heat, heatColor ) );
		gl::drawSolidRect( Rectf( -segment.width/2, -segment.length/2, segment.width/2, segment.length/2 ));

		if ( segment.joint )
		{
			gl::drawSolidCircle( v2r( cpPivotJointGetAnchr2( segment.joint )), 1.1 * segment.width/2, 16 );
		}
		
		gl::color( 1,0,0,1 );
		gl::drawLine( Vec2f(0,0), Vec2f(unitVectorLength,0) );
		gl::color( 0,1,0,1 );
		gl::drawLine( Vec2f(0,0), Vec2f(0, unitVectorLength) );
		
		gl::popModelView();

	}
	
	gl::color( color );
	for( vertex_vec::const_iterator v(_quads.begin()),end(_quads.end()); v != end; v+=4 )
	{
		gl::drawLine( (v+0)->position, (v+1)->position);
		gl::drawLine( (v+1)->position, (v+2)->position);
		gl::drawLine( (v+2)->position, (v+3)->position);
		gl::drawLine( (v+3)->position, (v+0)->position);
	}
	
}

void TentacleRenderer::_updateSplineReps()
{
	Tentacle *tentacle = this->tentacle();
	
	//
	//	To minimize heap allocations, I'm pre-allocating the splineReps to a worst-case scenario, and
	//	I'm clearing the referred-to vectors, not the splineReps vector itself.	
	//
		
	if ( _splineReps.empty() )
	{
		_splineReps.resize( tentacle->segments().size() + 1 );
	}
	
	for( spline_rep_vec::iterator 
		splineRep(_splineReps.begin()),
		end( _splineReps.end());
		splineRep != end; 
		++splineRep )
	{
		splineRep->segmentVertices.clear();
		splineRep->splineVertices.clear();
	}
	
	if ( tentacle->segments().empty() ) return;
	
	//
	//	Walk down the tentacle segments, bumping to the next spline rep on each disconnection
	//

	int splineRepIdx = 0;

	for( Tentacle::segmentvec::const_iterator 
		segment( tentacle->segments().begin()), 
		end( tentacle->segments().end()); 
		segment != end; 
		++segment )
	{
		spline_rep &spline = _splineReps[splineRepIdx];
		
		if ( segment->connected() )
		{
			if ( spline.segmentVertices.empty() ) 
			{
				spline.startWidth = segment->scale * segment->width;
				spline.startHeat = segment->heat;
				spline.startScale = segment->scale;
			}

			spline.segmentVertices.push_back( segment->position );
		}
		else
		{
			if ( segment != tentacle->segments().begin() )
			{
				const Tentacle::segment &previousSegment = *(segment-1);
				spline.segmentVertices.push_back(v2r(cpBodyLocal2World( previousSegment.body, cpv( 0, previousSegment.length/2 ))));
				spline.endWidth = previousSegment.scale * previousSegment.width;
				spline.endHeat = previousSegment.heat;
				spline.endScale = previousSegment.scale;
			}
		
			splineRepIdx++;
			spline_rep &nextSpline = _splineReps[splineRepIdx];
			nextSpline.segmentVertices.push_back( segment->position );	
			nextSpline.startWidth = segment->scale * segment->width;
			nextSpline.startHeat = segment->heat;
			nextSpline.startScale = segment->scale;
		}
	}

	//
	//	add tail
	//

	const Tentacle::segment &lastSegment(tentacle->segments().back());
	spline_rep &lastSplineRep = _splineReps[splineRepIdx];
	lastSplineRep.segmentVertices.push_back( v2r(cpBodyLocal2World( lastSegment.body, cpv( 0, lastSegment.length/2 ))) );
	lastSplineRep.endWidth = lastSegment.scale * lastSegment.width;
	lastSplineRep.endHeat = lastSegment.heat;
	lastSplineRep.endScale = lastSegment.scale;

	//
	//	compute splines
	//

	for( spline_rep_vec::iterator 
		splineRep(_splineReps.begin()),
		end( _splineReps.end());
		splineRep != end; 
		++splineRep )
	{
		if ( !splineRep->segmentVertices.empty() )
		{
			util::spline::spline<float>( splineRep->segmentVertices, 0.5, splineRep->segmentVertices.size() * 5, false, splineRep->splineVertices );
		}
		else
		{
			// we've hit the end
			break;
		}
	}
}

namespace {

	float segment_extents( const Vec2f &segStart, const Vec2f &segEnd, float halfWidth, Vec2f &a, Vec2f &b )
	{
		Vec2f dir = segEnd - segStart;
		float length = dir.length();
		dir /= length;
			
		a = Vec2f( halfWidth * dir.y, halfWidth * -dir.x );
		b.x = -a.x;
		b.y = -a.y;

		return length;
	}
	
}


void TentacleRenderer::_decompose()
{
	Tentacle *tentacle = this->tentacle();
	
	bool odd = tentacle->initializer()._idx % 2;
	
	float 
		distanceAlongTentacle = 0,
		ty = odd ? 0 : 1,
		ty2 = odd ? 1 : 0,
		
		// texture dx and dy scale
		//dys = 0.5 * (odd ? -1 : 1),
		dxs = 0.5;
		
		//ty += 0.5;
		//ty2 += 0.5;
	
	int nQuads = _quads.size();				
	_quads.clear();

	for( spline_rep_vec::const_iterator 
		splineRep(_splineReps.begin()),
		end( _splineReps.end());
		splineRep != end; 
		++splineRep )
	{
		const std::vector< Vec2f > &spline = splineRep->splineVertices;
		
		//
		//	We've hit the end of tesselation
		//
		
		if ( spline.empty() ) break;

		float 
			incrementScale = float(1) / ( spline.size()-1),
			halfWidth = splineRep->startWidth / 2,
			halfWidthIncrement = ((splineRep->endWidth/2) - halfWidth) * incrementScale,
			heat = splineRep->startHeat,
			heatIncrement = ( splineRep->endHeat - heat ) * incrementScale,
			opacity = splineRep->startScale,
			opacityIncrement = ( splineRep->endScale - opacity ) * incrementScale;
			

	
		Vec2f a,b,c,d;	
		segment_extents( spline.front(), spline[1], halfWidth, a, d );
		a += spline.front();
		d += spline.front();

		//
		//	generate quadstrip from spline
		//

		for( std::vector< Vec2f >::const_iterator 
			s0(spline.begin()),
			s1(spline.begin()+1),
			end(spline.end());
			s1 != end; 
			++s0, ++s1 )
		{
			float length = segment_extents( *s0, *s1, halfWidth+ halfWidthIncrement, b, c );
			b += *s1;
			c += *s1;
			
			float nextDistanceAlongTentacle = distanceAlongTentacle + dxs * length;
				
			_quads.push_back(vertex(a, Vec2r( distanceAlongTentacle, ty ), heat, opacity ));
			_quads.push_back(vertex(b, Vec2r( nextDistanceAlongTentacle, ty ), heat + heatIncrement, opacity + opacityIncrement ));
			_quads.push_back(vertex(c, Vec2r( nextDistanceAlongTentacle, ty2 ), heat + heatIncrement, opacity + opacityIncrement ));
			_quads.push_back(vertex(d, Vec2r( distanceAlongTentacle, ty2 ), heat, opacity ));

			distanceAlongTentacle = nextDistanceAlongTentacle;
			halfWidth += halfWidthIncrement;
			heat += heatIncrement;
			opacity += opacityIncrement;
			
			a = b;
			d = c;
			
		}		
	}
	
	//
	//	If number of quads changed, we need to recreate the VBO
	//

	if ( _quads.size() != nQuads )
	{
		glDeleteBuffers( 1, &_vbo );
		_vbo = 0;
	}
	
}


} // end namespace game