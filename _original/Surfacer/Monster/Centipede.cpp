//
//  Centipede.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 6/2/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Centipede.h"

#include "ChipmunkDebugDraw.h"
#include "Fronds.h"
#include "GameConstants.h"
#include "Spline.h"

#include <chipmunk/chipmunk_unsafe.h>

using namespace ci;
using namespace core;

namespace game {

namespace {
	
	const real AngularLimit = M_PI_4;

}

CLASSLOAD(Centipede);

/*
		init _initializer;
		cpShapeVec _shapes, _circles, _segments;
		cpConstraintVec _constraints, _gearJoints, _pivotJoints;
		cpBodyVec _bodies;
		cpBody *_headBody, *_tailBody;
		std::vector< real > _radii, _thicknesses;
		real _mass, _weight;
		FrondEngine *_whiskers;
*/


Centipede::Centipede():
	Monster( GameObjectType::GRUB ),
	_headBody( NULL ),
	_tailBody( NULL ),
	_mass(0),
	_weight(0),
	_whiskers(NULL)
{
	setName( "Centipede" );
	addComponent( new CentipedeRenderer() );
	setController( new CentipedeController() );
}

Centipede::~Centipede()
{
	aboutToDestroyPhysics(this);

	foreach( cpConstraint *c, _constraints )
	{
		cpSpaceRemoveConstraint( space(), c );
		cpConstraintFree( c );
	}
	
	foreach( cpShape *s, _shapes )
	{
		cpSpaceRemoveShape( space(), s );
		cpShapeFree( s );
	}
	
	foreach( cpBody *b, _bodies )
	{
		cpSpaceRemoveBody( space(), b );
		cpBodyFree( b );
	}
	
	// head body was among _bodies
	_headBody = NULL;
	
}

// Initialization
void Centipede::initialize( const init &i )
{
	Monster::initialize( i );
	_initializer = i;
}

// GameObject
void Centipede::addedToLevel( Level *level )
{
	Monster::addedToLevel( level );

	//
	//	A centipede is a chain of balls, nothing more. thin at ends, fat in the middle.
	//	We're going to have the centipede be rolled up when born, with the 'head' at the center
	//

	const int 
		count = std::ceil( 2*_initializer.length / _initializer.thickness );

	const real 
		CentipedLength = _initializer.length,
		CentipedeThickness = _initializer.thickness,
		gravity = cpvlength( cpSpaceGetGravity(space() ));

	cpVect pos = cpv( _initializer.position.x, _initializer.position.y );
	cpBody *lastBody = NULL;
	
	for ( int i = 0; i < count; i++ )
	{
		const real 
			CircleRadius = 0.5 * CentipedeThickness * ((i==0 || i==(count-1)) ? 0.5 : 1 ),
			Padding = CircleRadius * 0.02,
			ShapeRadius = std::max( CircleRadius - Padding, static_cast<real>(0.125)),
			Mass = 2 * M_PI * CircleRadius * _initializer.density,
			Moment = cpMomentForCircle(Mass, 0, CircleRadius, cpvzero );

		pos.y -= CircleRadius;
			
		cpBody *body = cpBodyNew( Mass, Moment );
		cpBodySetPos( body, pos );
		_bodies.push_back( body );
		
		_mass += Mass;
		
		// shape will have radius scaled to follow lifecycle
		cpShape *circle = cpCircleShapeNew( body, 0.01, cpvzero );
		_shapes.push_back( circle );
		_circles.push_back( circle );
		_radii.push_back( ShapeRadius );
		
		if ( lastBody )
		{
			cpConstraint *constraint = NULL;
		
			constraint = cpPivotJointNew(lastBody, body, 
				cpvmult( cpvadd( cpBodyGetPos( lastBody ), cpBodyGetPos( body )), 0.5 ));

			cpConstraintSetMaxForce( constraint, 0 );
			_constraints.push_back( constraint );
			_pivotJoints.push_back(constraint);
			
			constraint = cpGearJointNew( lastBody, body, 0, 1 );
			cpConstraintSetMaxForce( constraint, cpBodyGetMass(body) * gravity * 3 );
			_constraints.push_back( constraint );
			_gearJoints.push_back( constraint );
			
			constraint = cpRotaryLimitJointNew( lastBody, body, -AngularLimit, AngularLimit );
			_constraints.push_back( constraint );
			
			cpShape *segment = cpSegmentShapeNew( body, cpvzero, cpvzero, 0.01 );
			_shapes.push_back(segment);
			_segments.push_back(segment);
			_thicknesses.push_back(ShapeRadius);
		}
		
		pos.y -= CircleRadius;
		lastBody = body;
	}
	
	// alias head and tail bodies
	_headBody = _bodies.front();
	_tailBody = _bodies.back();


	foreach( cpBody *body, _bodies )
	{
		cpBodySetPos( body, cpv( _initializer.position ));
		cpBodySetUserData( body, this );
		cpSpaceAddBody( space(), body );
	}

	foreach( cpShape *shape, _shapes )
	{
		cpShapeSetUserData( shape, this );
		cpShapeSetCollisionType( shape, CollisionType::MONSTER );
		cpShapeSetLayers( shape, CollisionLayerMask::MONSTER );
		cpShapeSetGroup( shape, cpGroup(this));
		cpShapeSetFriction( shape, 2 );
		cpShapeSetElasticity( shape, 1 );
		cpSpaceAddShape( space(), shape );
	}
	
	foreach( cpConstraint *constraint, _constraints )
	{
		cpConstraintSetUserData( constraint, this );
		cpSpaceAddConstraint( space(), constraint );
	}	

	_weight = _mass * cpvlength( cpSpaceGetGravity(space()));
	
	//
	//	Now, add a FrondEngine to make this thing grosser
	//	
	
	if ( true )
	{
		_whiskers = new FrondEngine();
		FrondEngine::init frondEngineInit;
		frondEngineInit.color = _initializer.color;
		_whiskers->initialize( frondEngineInit );
		addChild( _whiskers );

		

		if ( true )
		{
			const Vec2
				Right( _initializer.thickness * 0.5, 0 );			
				
			FrondEngine::frond_init frondInit;
			frondInit.count = 3;
			frondInit.length = _initializer.thickness * 0.5;
			frondInit.thickness = frondInit.length * 0.5;

			for( cpBodyVec::iterator body(_bodies.begin()+1),end(_bodies.end()-1); body != end; ++body )
			{
				_whiskers->addFrond( frondInit, *body, v2(cpBodyGetPos(*body)) + Right, Right );
				_whiskers->addFrond( frondInit, *body, v2(cpBodyGetPos(*body)) - Right, -Right );
			}
		}
		
		if ( true )
		{
			FrondEngine::frond_init frondInit;
			frondInit.count = 3;
			frondInit.length = _initializer.thickness * 1;
			frondInit.thickness = frondInit.length * 0.1;

			for ( int i = 0, N = 16; i < N; i++ )
			{
				const real Rads = Rand::randFloat(-0.1,0.1) + (M_PI * static_cast<real>(i) / N);
				if ( i > 4 && i < 12 ) continue;
				

				const Vec2 
					HeadDir( std::cos( Rads ), std::sin( Rads )),
					TailDir( -HeadDir );
										
				frondInit.length = _initializer.thickness * Rand::randFloat( 0.9, 1.1 );
				_whiskers->addFrond( frondInit, _headBody, v2(cpBodyGetPos(_headBody)) + HeadDir * 0.1, HeadDir );
				_whiskers->addFrond( frondInit, _tailBody, v2(cpBodyGetPos(_tailBody)) + TailDir * 0.1, TailDir );					
			}
		}
	}
}

void Centipede::update( const time_state &time )
{
	Monster::update( time );
	
	real 
		lifecycle = this->lifecycle(),
		direction = 0,
		aggression = 0,
		fear = 0;
				
	if ( controller() )
	{
		direction = controller()->direction().x;
		aggression = controller()->aggression();
		fear = controller()->fear();
	}
	
	//
	//	Update line segment shapes
	//
	
	if ( !_segments.empty() )
	{
		cpShapeVec::iterator seg = _segments.begin();
		for ( cpBodyVec::iterator a(_bodies.begin()), b(_bodies.begin()+1), end( _bodies.end()); b != end; ++a, ++b, ++seg )
		{
			cpSegmentShapeSetEndpoints( *seg, cpBodyWorld2Local( *b, cpBodyGetPos(*a)), cpvzero );
		}
	}
	
			
	//
	//	Animate lifecycle
	//
		
	if ( lifecycle < 1 )
	{
		std::vector<real>::iterator radius = _radii.begin();
		for( cpShapeVec::iterator circle( _circles.begin() ), end( _circles.end() ); circle != end; ++circle, ++radius )
		{
			cpCircleShapeSetRadius( *circle, lifecycle * (*radius) );
		}
		
		std::vector<real>::iterator thickness = _thicknesses.begin();
		for( cpShapeVec::iterator segment( _segments.begin() ), end( _segments.end() ); segment != end; ++segment, ++radius )
		{
			cpSegmentShapeSetRadius( *segment, lifecycle * (*thickness) );
		}

		for ( cpConstraintVec::iterator gear( _gearJoints.begin()), end( _gearJoints.end() ); gear != end; ++gear )
		{
			cpGearJointSetPhase( *gear, lifecycle * AngularLimit );
		}	
		
		for ( cpConstraintVec::iterator pivot( _pivotJoints.begin()), end( _pivotJoints.end() ); pivot != end; ++pivot )
		{
			cpConstraintSetMaxForce( *pivot, lifecycle * _weight * 100 );
		}	

		if ( _whiskers )
		{
			ci::ColorA color = _initializer.color;
			color.a = lifecycle;
			_whiskers->setColor( color );
		}
	}
	
	
	//
	//	Wobble - both fear and agression make the wobble period shorter
	//
	
	if ( true )
	{
		real 
			scale = lifecycle / _gearJoints.size(),
			period = lrp<real>( std::max( aggression, fear ), 1, 0.5 ) * fudge(),
			magnitude = lrp<real>(std::max( aggression, fear ), 0.5, 1 ) * scale * AngularLimit,
			phase = time.time / period,
			phaseOffset = scale * M_PI_4;
			
		for ( cpConstraintVec::iterator gear( _gearJoints.begin()), end( _gearJoints.end() ); gear != end; ++gear )
		{
			cpGearJointSetPhase( *gear, magnitude * std::sin( phase ));
			phase += phaseOffset;
		}	
	}
	
	
	//
	//	Apply motion, update AABB
	//

	{
		if ( dead() ) direction = 0;	

		cpBB bounds = cpBBInvalid;
	
		real
			speed = lifecycle * _initializer.speed,
			oscillationPeriod = fudge() * 2,
			oscillationMagnitude = fudge() * 0.0625,
			oscillation = 1 + (oscillationMagnitude * std::sin( M_PI * time.time / oscillationPeriod ) + 0.25*std::sin( M_PI_2 * time.time / (2*oscillationPeriod)));
			
		cpVect dir = cpv( up() );
		if ( dir.x < 0 ) dir = cpvmult( dir, -1 );
		cpVect sv = cpvmult( dir, (speed * sign(direction) * oscillation ));
		
//		app::console() << "dir: " << v2(dir) 
//			<< " velocity: " << velocity 
//			<< " oscillation: " << oscillation
//			<< " sv: " << v2( sv )
//			<< std::endl;
		
		for( cpShapeVec::iterator shape( _shapes.begin()), end( _shapes.end() ); shape != end; ++shape )
		{
			cpShapeSetSurfaceVelocity( *shape, sv );
			cpBBExpand( bounds, cpShapeGetBB( *shape ));
		}
		
		setAabb( bounds );
	}

}

// Monster
void Centipede::died( const HealthComponent::injury_info &info )
{
	Monster::died( info );

	foreach( cpShape *shape, _shapes )
	{
		cpShapeSetLayers( shape, CollisionLayerMask::DEAD_MONSTER );
	}
}

Vec2 Centipede::up() const 
{ 
	Vec2 right = v2( cpBodyGetRot( _headBody ));
	Vec2 up = rotateCCW(right);
	
	if ( up.y < 0 ) up *= -1;
	return up;
}

Vec2 Centipede::eyePosition() const
{
	return position() + up() * _initializer.length * 0.5;
}


#pragma mark - CentipedeRenderer

/*
		GLuint _vbo;
		ci::gl::GlslProg _bodyShader, _svgShader;
		ci::gl::Texture _modulationTexture;
		vertex_vec _triangleStrip;
		core::SvgObject _head, _jaw0, _jaw1, _jaw2;
*/
CentipedeRenderer::CentipedeRenderer():
	_vbo(0)
{
	setName( "CentipedeRenderer" );
}

CentipedeRenderer::~CentipedeRenderer()
{
	if ( _vbo )
	{
		glDeleteBuffers( 1, &_vbo );
	}
}

void CentipedeRenderer::draw( const render_state &state )
{
	_tesselate(state);	
	DrawComponent::draw( state );
}

void CentipedeRenderer::_tesselate( const render_state &state )
{
	/*
		Note, this is all being generated in world coordinate space
	*/

	Centipede *centipede = this->centipede();
	
	const cpShapeVec &circles( centipede->circles() );
	const cpBodyVec &bodies( centipede->bodies() );

	cpShapeVec::const_iterator 
		circle = circles.begin(),
		circlesEnd = circles.end();
		
	cpBodyVec::const_iterator
		body = bodies.begin();
	
	int 
		i = 0,
		iLast = circles.size() - 1;
		
	real
		ty = 1,
		tyInc = real(-1) / circles.size(),
		tyInc_2 = tyInc / 2;
	
	_triangleStrip.clear();

	for ( ; circle != circlesEnd; ++circle, ++body, ++i )
	{
		const Vec2 
			pos = v2f( cpBodyGetPos( *body )),
			right = v2f( cpBodyGetRot( *body )),
			up = rotateCCW( right );

		const real 
			width = cpCircleShapeGetRadius( *circle ) * 1.25;
	
		if ( i == 0 )
		{
			// front tip
			_triangleStrip.push_back( vertex( pos + (up * width), ci::Vec2(0.5, ty)));
		}

		{
			// somewhere in the middle
			_triangleStrip.push_back( vertex( pos + (right * width), ci::Vec2( 1, ty + tyInc_2 )));
			_triangleStrip.push_back( vertex( pos - (right * width), ci::Vec2( 0, ty + tyInc_2 )));
		}

		if ( i == iLast )
		{
			// end tip
			_triangleStrip.push_back( vertex( pos - (up * width), ci::Vec2( 0.5, ty + tyInc )));
		}
		
		ty += tyInc;
	}
	
}

void CentipedeRenderer::_drawBody( const core::render_state &state )
{
	Centipede *centipede = this->centipede();


	if ( !_bodyShader )
	{
		_bodyShader = level()->resourceManager()->getShader( "CentipedeShader" );
	}
	
	if ( !_modulationTexture )
	{
		gl::Texture::Format noMipmappingFormat;
		noMipmappingFormat.enableMipmapping(false);

		_modulationTexture = level()->resourceManager()->getTexture( centipede->initializer().modulationTexture, noMipmappingFormat );
	}

	_modulationTexture.bind(0);
	_modulationTexture.setWrap( GL_REPEAT, GL_REPEAT );

	ci::ColorA color = centipede->initializer().color;
	color.a *= centipede->lifecycle();
		
	_bodyShader.bind();
	_bodyShader.uniform( "ModulationTexture", 0 );
	_bodyShader.uniform( "Color", color );

	const GLsizei 
		stride = sizeof( vertex ),
		positionOffset = 0,
		texCoordOffset = sizeof( ci::Vec2 );
	
	if ( !_vbo )
	{
		glGenBuffers( 1, &_vbo );
		glBindBuffer( GL_ARRAY_BUFFER, _vbo );
		glBufferData( GL_ARRAY_BUFFER, 
					 _triangleStrip.size() * stride, 
					 NULL,
					 GL_STREAM_DRAW );
	}
	

	glEnable( GL_BLEND );

	glBindBuffer( GL_ARRAY_BUFFER, _vbo );
	glBufferSubData( GL_ARRAY_BUFFER, 0, _triangleStrip.size() * stride, &(_triangleStrip.front()));
	
	glEnableClientState( GL_VERTEX_ARRAY );
	glVertexPointer( 2, GL_REAL, stride, (void*)positionOffset );

	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glTexCoordPointer( 2, GL_REAL, stride, (void*)texCoordOffset );
	
	glDrawArrays( GL_TRIANGLE_STRIP, 0, _triangleStrip.size() );
	
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	
	_bodyShader.unbind();
	_modulationTexture.unbind();
}

void CentipedeRenderer::_drawHeadAndTail( const core::render_state &state )
{
	Centipede *centipede = this->centipede();
	CentipedeController *controller = static_cast<CentipedeController*>(centipede->controller());

	const real
		Agression = controller->aggression(),
		Fear = controller->fear(),
		Agitation = std::max( Agression, Fear ),
		Thickness = centipede->thickness(),
		HeadOffset = Thickness * 0.125;

	if ( !_svgShader )
	{
		_svgShader = level()->resourceManager()->getShader( "SvgPassthroughShader" );		
		
		_head = level()->resourceManager()->getSvg( "CentipedeJaws.svg" );
		_jaws.push_back(_head.find( "jaw0" ));
		_jaws.push_back(_head.find( "jaw1" ));
		_jaws.push_back(_head.find( "jaw2" ));
		
		assert( _jaws[0] );
		assert( _jaws[1] );
		assert( _jaws[2] );

		Vec2 documentSize = _head.documentSize();
		real documentRadius = std::max( documentSize.x, documentSize.y ) * 0.5;
		_headScale.x = _headScale.y = centipede->fudge() * centipede->thickness() / documentRadius;
	}
	
	_svgShader.bind();
	
	{
		const real 
			BasePhase = state.time * 4 * centipede->fudge(),
			AggressivePhase = BasePhase * 3,
			Cycle = M_PI * 0.3,
			Magnitude = lrp<real>(Agitation, 0.3, 1 ) * M_PI_2 * 0.5;

		real offset = 0;

		for ( std::vector< SvgObject >::iterator jaw(_jaws.begin()),end( _jaws.end()); jaw != end; ++jaw )
		{
			const real
				V0 = std::cos( BasePhase + offset ),
				V1 = std::cos( AggressivePhase + offset ),
				V = lrp< real >(Agitation, V0, V1 );
				
			jaw->setAngle( Magnitude * V );
			offset += Cycle;
		}
	}
	
	cpBody 
		*headBody = centipede->bodies().front(),
		*tailBody = centipede->bodies().back();

	_head.setScale( _headScale * centipede->lifecycle() );
	_head.setOpacity( centipede->dead() ? centipede->lifecycle() : 1 );

	const Vec2
		HeadPosition = v2(cpBodyGetPos( headBody )) + rotateCCW(v2(cpBodyGetRot( headBody ))) * HeadOffset,
		TailPosition = v2(cpBodyGetPos( tailBody )) + rotateCW(v2(cpBodyGetRot( tailBody ))) * HeadOffset;

	_head.setPosition( HeadPosition );
	_head.setAngle( cpBodyGetAngle( headBody ) + M_PI_2 );
	_head.draw( state );
	
	_head.setPosition( TailPosition );
	_head.setAngle( cpBodyGetAngle( tailBody ) - M_PI_2 );
	_head.draw( state );

	_svgShader.unbind();
}

void CentipedeRenderer::_drawGame( const render_state &state )
{
	_drawBody(state);
	_drawHeadAndTail(state);
}

void CentipedeRenderer::_drawDebug( const render_state &state )
{
	Centipede *centipede = this->centipede();
	_debugDrawChipmunkShapes( state, centipede->shapes() );
	_debugDrawChipmunkConstraints( state, centipede->constraints() );
	_debugDrawChipmunkBody( state, centipede->body() );
	
	
	glColor3f(1,1,1);
	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	glBegin( GL_TRIANGLE_STRIP );
	
	for ( vertex_vec::iterator v( _triangleStrip.begin()), end( _triangleStrip.end() ); v != end; ++v )
	{
		glVertex2f( v->position );
	}
	
	glEnd();
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	
}

#pragma mark - CentipedeController

CentipedeController::CentipedeController()
{
	setName( "CentipedeController" );
}

void CentipedeController::update( const time_state &time )
{
	GroundBasedMonsterController::update( time );
}

void CentipedeController::_getMotionProbeRays( cpVect &leftOrigin, cpVect &leftEnd, cpVect &rightOrigin, cpVect &rightEnd )
{
	cpBB bounds = owner()->aabb();

	const real 
		width = cpBBWidth(bounds),
		height = cpBBHeight(bounds),
		extentX = width * 0.75,
		extentY = 2*height;

	cpVect center = cpBBCenter( bounds );


	leftOrigin.x = center.x - extentX;
	leftOrigin.y = center.y + height;

	leftEnd.x = center.x - extentX;
	leftEnd.y = center.y - extentY;
	
	rightOrigin.x = center.x + extentX;
	rightOrigin.y = center.y + height;

	rightEnd.x = center.x + extentX;
	rightEnd.y = center.y - extentY;
}


} // end namespace game
