//
//  Tongue.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 7/12/12.
//
//

#include "Tongue.h"

#include "ChipmunkDebugDraw.h"
#include "GameConstants.h"
#include "GameNotifications.h"
#include "Player.h"
#include "Spline.h"

#include <chipmunk/chipmunk_unsafe.h>

using namespace ci;
using namespace core;
namespace game {

namespace {


}

#pragma mark - Tongue

CLASSLOAD(Tongue);

/*
		init _initializer;
		segment_vec _segments;
		real _extension, _contractionExtension, _currentExtension, _pivotForce, _segmentLength;
		int _playerContactCount;
		bool _initContraction, _grabbingPlayer, _releasingPlayer;
		seconds_t _birthTime, _detachTime, _timeTouchingPlayer, _timeReleasedPlayer;
		cpConstraintSet _grabConstraints;
*/

Tongue::Tongue():
	Monster( GameObjectType::TONGUE ),
	_extension(0),
	_contractionExtension(0),
	_currentExtension(0),
	_pivotForce(0),
	_segmentLength(0),
	_playerContactCount(0),
	_initContraction(false),
	_grabbingPlayer(false),
	_releasingPlayer(false),
	_birthTime(std::numeric_limits<seconds_t>::max()),
	_detachTime(std::numeric_limits<seconds_t>::max()),
	_timeTouchingPlayer(0),
	_timeReleasedPlayer(0)
{
	setName( "Tongue" );
	setLayer(RenderLayer::MONSTER );
	addComponent( new TongueRenderer());
	setVisibilityDetermination( VisibilityDetermination::FRUSTUM_CULLING );
}
	
Tongue::~Tongue()
{
	assert( !attached() );
	assert( !playerBeingHeld());
	
	foreach( const segment &seg, _segments )
	{
		cpCleanupAndFree( seg.pivot );
		cpCleanupAndFree( seg.pin );
		cpCleanupAndFree( seg.angularLimit );
		cpCleanupAndFree( seg.shape );
		cpCleanupAndFree( seg.body );
	}
	
	_segments.clear();
}

void Tongue::initialize( const init &i )
{
	Monster::initialize( i );
	_initializer = i;
}

// GameObject
void Tongue::addedToLevel( core::Level *level )
{
	Monster::addedToLevel(level);

	assert( _initializer.position.lengthSquared() > Epsilon );
	assert( _segments.empty() );

	cpSpace *space = level->space();
	cpBody *lastBody = NULL;
	
	const real
		Density = _initializer.density,
		SegmentLength = _initializer.length / _initializer.segmentCount,
		SegmentRadius = _initializer.thickness,
		SegmentEndRadius = _initializer.thickness * 0.5,
		SegmentMass = SegmentLength * SegmentRadius,
		SegmentMoment = cpMomentForBox(SegmentMass, SegmentLength, SegmentRadius ),
		TotalMass = _initializer.segmentCount * SegmentMass,
		Gravity = cpvlength( cpSpaceGetGravity( space ));
		
	_pivotForce = TotalMass * Gravity * 100;
	_segmentLength = SegmentLength;
		
	cpVect
		A = cpv(_initializer.position);

	for ( int i = 0; i < _initializer.segmentCount; i++ )
	{
		const real
			Position = static_cast<real>(i) / (_initializer.segmentCount-1);
	
		segment seg;
		seg.length = SegmentLength;
		seg.radius = lrp( Position, SegmentRadius, SegmentEndRadius );
		seg.distance = static_cast<real>(i) / _initializer.segmentCount;
		
		seg.body = cpBodyNew( SegmentMass, SegmentMoment );
		cpBodySetPos( seg.body, A );


		if ( lastBody )
		{
			
			const cpVect
				LastBodyPos = cpBodyGetPos( lastBody );
				
			const real
				Dist = cpvlength( cpvsub( LastBodyPos, A ));
				
			seg.pin = cpPinJointNew(lastBody, seg.body, cpvzero, cpvzero );
			seg.pivot = cpPivotJointNew( lastBody, seg.body, LastBodyPos );
			seg.angularLimit = cpRotaryLimitJointNew( lastBody, seg.body, -M_PI_4, M_PI_4 );
		}
			

		cpBodySetUserData( seg.body, this );
		cpSpaceAddBody( space, seg.body );
		
		if ( seg.pivot )
		{
			cpSpaceAddConstraint( space, seg.pivot );
			cpConstraintSetMaxForce( seg.pivot, _pivotForce );
		}

		if ( seg.pin )
		{
			cpSpaceAddConstraint( space, seg.pin );
		}

		if ( seg.angularLimit )
		{
			cpSpaceAddConstraint( space, seg.angularLimit );
			cpConstraintSetMaxForce( seg.pivot, _pivotForce );
		}
				
		_segments.push_back(seg);

		lastBody = seg.body;
		A = cpvadd( A, cpv(0,-SegmentLength));
	}

	//
	// mark that we are to begin contraction - when contraction's done we'll add the tongue shapes
	//

	_initContraction = true;
	_contractionExtension = 1;
	_extension = 1;
}

void Tongue::removedFromLevel( core::Level *removedFrom )
{
	detach();	
}

void Tongue::ready()
{
	Monster::ready();
}

void Tongue::update( const core::time_state &time )
{
	Monster::update( time );

	_currentExtension = lrp<real>(0.1, _currentExtension, _extension * _contractionExtension);
	
	const real
		CurrentExtension = _currentExtension,
		CurrentContraction = 1 - CurrentExtension,
		SegmentLength = std::max<real>(CurrentExtension * _segmentLength, 0.01);	

	//
	//	We create the tongue shapes when the tongue is fully contracted
	//

	if( _initContraction )
	{
		_contractionExtension = _contractionExtension * 0.9;
		if ( _contractionExtension < Epsilon )
		{
			_initContraction = false;
			_birthTime = time.time;
			_createShapes( SegmentLength );
		}
	}
	else
	{
		_contractionExtension = std::min<real>( _contractionExtension + 0.25*time.deltaT, 1 );
	}
	
	const real
		PivotForce = CurrentExtension * CurrentExtension * CurrentExtension * CurrentExtension * _pivotForce,
		Damping = lrp (CurrentContraction * CurrentContraction * CurrentContraction * CurrentContraction, 0.99, 0.9 ),
		AgeRadiusScale = saturate((time.time - _birthTime)/_initializer.extroTime),
		ExtensionRadiusScale = lrp<real>(CurrentContraction,0.5,1.25),
		WhitherScale = 1-saturate(( time.time - _detachTime ) / _initializer.extroTime),
		RadiusScale = AgeRadiusScale * ExtensionRadiusScale * WhitherScale;

	cpBB
		bounds = cpBBInvalid;
		
	for( segment_vec::iterator begin(_segments.begin()), seg(begin),end(_segments.end()); seg != end; ++seg )
	{
		if ( seg != begin )
		{
			if ( seg->pivot )
			{
				cpConstraintSetMaxForce( seg->pivot, PivotForce );
			}
			
			if ( seg->pin )
			{
				cpPinJointSetDist( seg->pin, SegmentLength );
			}
		}

		if ( seg->shape )
		{
			cpSegmentShapeSetEndpoints( seg->shape, cpv(0,SegmentLength), cpvzero );
			cpSegmentShapeSetRadius( seg->shape, seg->radius * RadiusScale );

			bounds = cpBBMerge(bounds, cpShapeGetBB(seg->shape));
		}
		else
		{
			cpBBExpand(bounds, cpBodyGetPos(seg->body), _initializer.thickness );
		}
		
		cpBodySetVel( seg->body, cpvmult( cpBodyGetVel(seg->body), Damping ));
		cpBodySetAngVel( seg->body, cpBodyGetAngVel( seg->body) * Damping );
	}
	
	setAabb(bounds);
	
	//
	//	Handle player contact. Dispatch injury, and grabbing
	//

	if ( !playerBeingHeld() && ( time.time - _timeReleasedPlayer > _initializer.timeBetweenPlayerGrabs ) )
	{
		if ( _playerContactCount > 0 )
		{
			_timeTouchingPlayer += time.deltaT;
		}
		else
		{
			_timeTouchingPlayer *= 0.5;
		}
	}
	else
	{
		_timeTouchingPlayer = 0;
	}

	//
	//	Grabbing goes through a cycle. First we set _grabbingPlayer to true, then in the next postSolve we establish pivot joints
	//	Here we're going into grabbingPlayer phase, iff tonggue has touched long enough and we're not already grabbing
	//

	if ( alive() )
	{
		if ( _timeTouchingPlayer > _initializer.timeToGrabPlayer && !_grabbingPlayer && _grabConstraints.empty() )
		{
			app::console() << description() << "::update - timeTouchingPlayer: " << _timeTouchingPlayer << " will set _grabbingPlayer=true" << std::endl;
			_grabbingPlayer = true;
		}
		
		if ( !_grabConstraints.empty())
		{
			//
			//	The grab constraints are created during the postSolve callback, but we can't modify the
			//	space then, so we're lazily adding them here in the next step.
			//

			if( !cpConstraintGetSpace( *_grabConstraints.begin() ))
			{
				cpSpace *space = this->space();
				for ( cpConstraintSet::iterator c(_grabConstraints.begin()),end(_grabConstraints.end()); c != end; ++c )
				{
					cpSpaceAddConstraint( space, *c );
				}
				
				// now the grabbing process is complete!
				notificationDispatcher()->broadcast(Notification(Notifications::BARNACLE_GRABBED_PLAYER ));
			}
		}
	}
	
	//
	//	We attempted to detach constraints while space was locked, so we had to postpone it until later
	//

	if ( _releasingPlayer )
	{
		cpCleanupAndFree( _grabConstraints );
		_releasingPlayer = false;
	}
		
	//
	//	Now, if the tongue has whithered to nothingness, it's time to auto-terminate
	//
	
	if ( WhitherScale < Epsilon )
	{
		setFinished(true);
	}
	
}

void Tongue::injured( const HealthComponent::injury_info &info )
{
	Monster::injured(info);
}

void Tongue::died( const HealthComponent::injury_info &info )
{
	detach();
	Monster::died(info);
}

// Monster

Vec2r Tongue::position() const
{
	return v2r(cpBodyGetPos(body()));
}

Vec2r Tongue::up() const
{
	return Vec2r(0,1);
}

Vec2r Tongue::eyePosition() const
{
	return position();
}

cpBody *Tongue::body() const
{
	return _segments.front().body;
}

void Tongue::restrainPlayer()
{
	Monster::restrainPlayer();
}

void Tongue::releasePlayer()
{
	Monster::releasePlayer();
	
	_timeReleasedPlayer = level() ? level()->time().time : 0;

	if ( !_grabConstraints.empty() )
	{
		_grabbingPlayer = false;
		_releasingPlayer = true;
		_timeTouchingPlayer = 0;
		
		// if space is locked, grab constraints will be cleared at the next step()
		if ( !level()->spaceIsLocked() )
		{
			cpCleanupAndFree( _grabConstraints );
		}
	
		if ( NotificationDispatcher *disp = notificationDispatcher() )
		{
			disp->broadcast(Notification(Notifications::BARNACLE_RELEASED_PLAYER ));
		}
	}
}

// Tongue
void Tongue::attachTo( Monster *parent, cpBody *body, const Vec2r &positionWorld )
{
	assert( !attached() );
	assert( !this->parent() );

	parent->addChild( this );

	assert( !_segments.empty() );
	
	const cpVect anchorDelta = cpvsub( cpv(positionWorld), cpBodyGetPos(_segments.front().body));
	foreach( segment &seg, _segments )
	{
		cpBodySetPos( seg.body, cpvadd( cpBodyGetPos(seg.body), anchorDelta ));
		cpBodySetVel( seg.body, cpBodyGetVel( body ));
		cpBodySetAngVel( seg.body, cpBodyGetAngVel( body ));
	}
	
	
	segment &front = _segments.front();
	front.pivot = cpPivotJointNew(body, _segments.front().body, cpv(positionWorld) );
	front.angularLimit = cpRotaryLimitJointNew( body, _segments.front().body, -M_PI * 0.125, M_PI * 0.125 );
	
	cpSpaceAddConstraint( cpBodyGetSpace(body), front.pivot );
	cpSpaceAddConstraint( cpBodyGetSpace(body), front.angularLimit );

	//
	//	We need to aggressively detach from Parent when parent gives up the ghost
	//
	parent->health()->died.connect( this, &Tongue::_parentDied );
	parent->aboutToDestroyPhysics.connect( this, &Tongue::_anchorAboutToDestroyPhysics );
	parent->willBeRemovedFromLevel.connect( this, &Tongue::_parentWillBeRemovedFromLevel );
}

void Tongue::attach( const Vec2r &positionWorld )
{
	assert( !attached() );
	assert( !_segments.empty() );
	
	cpSpace *space = level()->space();
	const cpVect anchorDelta = cpvsub( cpv(positionWorld), cpBodyGetPos(_segments.front().body));

	foreach( segment &seg, _segments )
	{
		cpBodySetPos( seg.body, cpvadd( cpBodyGetPos(seg.body), anchorDelta ));
	}

	segment &front = _segments.front();
	front.pivot = cpPivotJointNew( cpSpaceGetStaticBody(space), _segments.front().body, cpv(positionWorld) );
	front.angularLimit = cpRotaryLimitJointNew( cpSpaceGetStaticBody(space), _segments.front().body, 0, 0 );

	cpSpaceAddConstraint( space, front.pivot );
	cpSpaceAddConstraint( space, front.angularLimit );
}

bool Tongue::attached() const
{
	return !_segments.empty() && (_segments.front().pivot != NULL);
}

cpBody *Tongue::attachedTo() const
{
	if ( attached() )
	{
		return cpConstraintGetA( _segments.front().pivot );
	}
	
	return NULL;
}

void Tongue::detach()
{
	releasePlayer();
	
	if ( parent() )
	{
		//app::console() << description() << "::detach - ORPHANING" << std::endl;
		parent()->orphanChild( this );
	}

	if ( attached() )
	{
		//app::console() << description() << "::detach - DETACHING" << std::endl;

		segment &front = _segments.front();
		cpCleanupAndFree( front.pivot );
		cpCleanupAndFree( front.pin );
		cpCleanupAndFree( front.angularLimit );
		front.pivot = front.pin = front.angularLimit = NULL;
	
		if ( level() )
		{
			_detachTime = level()->time().time;
		}
	}
}

void Tongue::_createShapes( real segmentLength )
{
	cpSpace *space = level()->space();
	for( segment_vec::iterator seg(_segments.begin()+1),end(_segments.end()); seg != end; ++seg )
	{
		seg->shape = cpSegmentShapeNew( seg->body, cpv(0, segmentLength), cpvzero, seg->radius );

		cpShapeSetUserData( seg->shape, this );
		cpShapeSetGroup( seg->shape, reinterpret_cast<cpGroup>(this));
		cpShapeSetCollisionType( seg->shape, CollisionType::MONSTER );
		cpShapeSetLayers( seg->shape, CollisionLayerMask::MONSTER );
		cpSpaceAddShape( space, seg->shape );
	}
}

bool Tongue::_hasShapes() const
{
	return !_initContraction;
}

void Tongue::_anchorAboutToDestroyPhysics( GameObject * )
{
	detach();
}

void Tongue::_parentWillBeRemovedFromLevel( GameObject * )
{
	detach();
}

void Tongue::_parentDied( const HealthComponent::injury_info & )
{
	detach();
	health()->kill();
}

void Tongue::_monsterPlayerContact( const core::collision_info &info, bool &discard )
{
	Monster::_monsterPlayerContact(info, discard);

	assert( cpBodyGetUserData(info.bodyA) == this );
	_playerContactCount++;
}

void Tongue::_monsterPlayerPostSolve( const core::collision_info &info )
{
	Monster::_monsterPlayerPostSolve(info);

	assert( cpBodyGetUserData(info.bodyA) == this );
	if ( alive() && _grabbingPlayer && _grabConstraints.empty() )
	{
		app::console() << description() << "::_monsterPlayerPostSolve - establishing grab constraints to player" << std::endl;
		for ( int i = 0, N = info.contacts(); i < N; i++ )
		{
			_grabConstraints.insert(cpPivotJointNew( info.bodyA, info.bodyB, cpv(info.position(i))));
		}
		
		// keep on grabbing until we have some constraints
		_grabbingPlayer = _grabConstraints.empty();
		
		restrainPlayer();
	}
}

void Tongue::_monsterPlayerSeparate( const core::collision_info &info )
{
	Monster::_monsterPlayerSeparate(info);

	assert( cpBodyGetUserData(info.bodyA) == this );
	if( _playerContactCount > 0 ) _playerContactCount--;
}



#pragma mark - TongueRenderer

/*
		GLuint _vbo;
		ci::gl::GlslProg _shader;

		spline_rep _splineRep;
		vertex_vec _triangleStrip;
*/

TongueRenderer::TongueRenderer():
	_vbo(0)
{}

TongueRenderer::~TongueRenderer()
{
	if ( _vbo )
	{
		glDeleteBuffers( 1, &_vbo );
	}
}

void TongueRenderer::draw( const core::render_state &state )
{
	DrawComponent::draw(state);
}

void TongueRenderer::_drawGame( const core::render_state &state )
{
	Tongue *tongue = this->tongue();
	if ( tongue->_hasShapes() )
	{
		if ( !_shader )
		{
			_shader = level()->resourceManager()->getShader( "SvgPassthroughShader" );
		}
		
		_shader.bind();


		_updateSpline( state );
		_updateTrianglulation( state );
			
		const GLsizei 
			stride = sizeof( TongueRenderer::vertex ),
			positionOffset = 0;
		
		if ( !_vbo )
		{
			glGenBuffers( 1, &_vbo );
			glBindBuffer( GL_ARRAY_BUFFER, _vbo );
			glBufferData( GL_ARRAY_BUFFER, 
						 _triangleStrip.size() * stride, 
						 NULL,
						 GL_STREAM_DRAW );
		}
		
		
		ci::ColorA color = tongue->initializer().color;
		color.a *= tongue->lifecycle();

		glEnable( GL_BLEND );
		glColor4f( color );

		glBindBuffer( GL_ARRAY_BUFFER, _vbo );
		glBufferSubData( GL_ARRAY_BUFFER, 0, _triangleStrip.size() * stride, &(_triangleStrip.front()));
		
		glEnableClientState( GL_VERTEX_ARRAY );
		glVertexPointer( 2, GL_REAL, stride, (void*)positionOffset );
		
		glDrawArrays( GL_TRIANGLE_STRIP, 0, _triangleStrip.size() );
		
		glBindBuffer( GL_ARRAY_BUFFER, 0 );
		glDisableClientState( GL_VERTEX_ARRAY );

		
		_shader.unbind();
	}
}

void TongueRenderer::_drawDebug( const core::render_state &state )
{
	const ci::ColorA
		ConstraintColor(0,0.5,1,0.5),
		ShapeColor(1,0.5,1,0.5);
	
	foreach( const Tongue::segment &seg, tongue()->segments() )
	{
		if ( seg.pivot ) _debugDrawChipmunkConstraint( state, seg.pivot, ConstraintColor );
		if ( seg.shape ) _debugDrawChipmunkShape(state, seg.shape, ShapeColor );
		if ( seg.body ) _debugDrawChipmunkBody( state, seg.body );
	}
}

void TongueRenderer::_updateSpline( const core::render_state &state )
{
	Tongue *tongue = this->tongue();
	const Tongue::segment_vec &segments = tongue->segments();
	
	if ( _splineRep.positions.size() != segments.size() )
	{
		_splineRep.positions.resize( segments.size() );
		_splineRep.dirs.resize( segments.size() );
	}
	
	Vec2fVec::iterator
		bodyPositions = _splineRep.positions.begin(),
		bodyDirs = _splineRep.dirs.begin();

	for( Tongue::segment_vec::const_iterator
		segment( segments.begin()), end( segments.end());
		segment != end;
		++segment, ++bodyPositions )
	{
		const cpVect
			Pos = cpBodyGetPos( segment->body ),
			Dir = cpBodyGetRot( segment->body );

		bodyPositions->x = Pos.x;
		bodyPositions->y = Pos.y;

		// rotate 90 deg
		bodyDirs->x = Dir.y;
		bodyDirs->y = -Dir.x;
	}
	
	_splineRep.splinePositions.clear();
	util::spline::spline<float>( _splineRep.positions, 0.5, _splineRep.positions.size() * 5, false, _splineRep.splinePositions );
}

void TongueRenderer::_updateTrianglulation( const core::render_state &state )
{
	Tongue *tongue = this->tongue();

	const Tongue::segment_vec &segments = tongue->segments();

	real
		halfWidth = cpSegmentShapeGetRadius( segments[1].shape ), // seg[0] has no shape attached
		halfWidthEnd = cpSegmentShapeGetRadius( segments.back().shape ),
		halfWidthIncrement = (halfWidthEnd - halfWidth) / _splineRep.splinePositions.size(),
		phase = state.time * 2 * M_PI * tongue->fudge(),
		phaseIncrement = tongue->fudge() * 8 * M_PI / _splineRep.splinePositions.size();
	
	if ( _triangleStrip.size() != _splineRep.splinePositions.size() * 2 )
	{
		_triangleStrip.resize( _splineRep.splinePositions.size() * 2 );
	}
	
	vertex_vec::iterator vertex = _triangleStrip.begin();
	Vec2fVec::iterator 
		splineVertex = _splineRep.splinePositions.begin(),
		splineEnd = _splineRep.splinePositions.end();
	
	Vec2f dir = rotateCW( v2f( cpBodyGetRot( tongue->body() )) );
	
	while( splineVertex != splineEnd )
	{
		Vec2f right = rotateCW( dir );
		real	
			waveCycle = (0.25 * std::cos( phase )),
			waveOffsetA = waveCycle + 1,
			waveOffsetB = 1 - waveCycle;
		
		*vertex = *splineVertex + (right * halfWidth * waveOffsetA);
		vertex++;
		
		*vertex = *splineVertex - (right * halfWidth * waveOffsetB);
		vertex++;

		if ( splineVertex + 1 != splineEnd )
		{
			dir = normalize( *(splineVertex + 1) - *splineVertex );
		}
	
		splineVertex++;
		halfWidth += halfWidthIncrement;
		phase += phaseIncrement;

		if ( splineVertex + 1 == splineEnd ) halfWidth = 0;
	}
}
		
}