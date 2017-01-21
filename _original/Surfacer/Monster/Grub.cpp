//
//  Grub.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 1/28/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Grub.h"

#include "ChipmunkDebugDraw.h"
#include "GameConstants.h"
#include "Spline.h"

#include <chipmunk/chipmunk_unsafe.h>

using namespace ci;
using namespace core;

namespace game {

namespace {
	
	const real AngularLimit = M_PI_4;

}

CLASSLOAD(Grub);

/*
		cpShapeVec _shapes;
		cpConstraintVec _constraints;
		cpBodyVec _bodies;
		cpBody _headBody;
		std::vector< real > _radii;
		real _speed;
*/


Grub::Grub():
	Monster( GameObjectType::GRUB ),
	_headBody( NULL ),
	_speed(0)
{
	setName( "Grub" );
	addComponent( new GrubRenderer() );
	setController( new GrubController() );
}

Grub::~Grub()
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
void Grub::initialize( const init &i )
{
	Monster::initialize( i );
	_initializer = i;
}

// GameObject
void Grub::addedToLevel( Level *level )
{
	Monster::addedToLevel( level );

	//
	//	A grub is a chain of balls, nothing more. thin at ends, fat in the middle.
	//	We'll treat the "head" as the topmost, and initialize going down.
	//

	const int 
		count = 5;

	const real 
		size = _initializer.size,
		gravity = cpvlength( cpSpaceGetGravity(space() )),
		radii[count] = {
			size * 0.1 / 2,
			size * 0.2 / 2,
			size * 0.4 / 2,
			size * 0.2 / 2,
			size * 0.1 / 2
		};

	cpVect pos = cpv( _initializer.position.x, _initializer.position.y );
	cpBody *lastBody = NULL;

	for ( int i = 0; i < count; i++ )
	{
		const real 
			radius = radii[i],
			padding = radius * 0.02,
			shapeRadius = radius - padding,
			mass = 2 * M_PI * radius * _initializer.density,
			moment = cpMomentForCircle(mass, 0, radius, cpvzero );

		pos.y -= radius;
			
		cpBody *body = cpBodyNew( mass, moment );
		cpBodySetPos( body, pos );
		_bodies.push_back( body );
		
		// shape will have radius scaled to follow lifecycle
		cpShape *shape = cpCircleShapeNew( body, 0.01, cpvzero );
		_shapes.push_back( shape );
		
		if ( lastBody )
		{
			cpConstraint *constraint = cpPivotJointNew(lastBody, body, 
				cpvmult( cpvadd( cpBodyGetPos( lastBody ), cpBodyGetPos( body )), 0.5 ));

			_constraints.push_back( constraint );
			
			constraint = cpGearJointNew( lastBody, body, 0, 1 );
			cpConstraintSetMaxForce( constraint, (0.5 * cpBodyGetMass(lastBody) + cpBodyGetMass(body)) * gravity * 0.25 );
			_constraints.push_back( constraint );
			_gearJoints.push_back( constraint );
			
			constraint = cpRotaryLimitJointNew( lastBody, body, -AngularLimit, AngularLimit );
			_constraints.push_back( constraint );
		}
		
		pos.y -= radius;
		lastBody = body;
		_radii.push_back( shapeRadius );
	}
	
	// alias 'head' to the first body -- though grub doesn't really have a front or back
	_headBody = _bodies.front();

	foreach( cpBody *body, _bodies )
	{
		cpBodySetUserData( body, this );
		cpSpaceAddBody( space(), body );
	}

	foreach( cpShape *shape, _shapes )
	{
		cpShapeSetUserData( shape, this );
		cpShapeSetCollisionType( shape, CollisionType::MONSTER );
		cpShapeSetLayers( shape, CollisionLayerMask::MONSTER );
		cpShapeSetGroup( shape, cpGroup(this));
		cpShapeSetFriction( shape, 1 );
		cpShapeSetElasticity( shape, 0 );
		cpSpaceAddShape( space(), shape );
	}
	
	foreach( cpConstraint *constraint, _constraints )
	{
		cpConstraintSetUserData( constraint, this );
		cpSpaceAddConstraint( space(), constraint );
	}	


}

void Grub::update( const time_state &time )
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
	
	if ( dead() ) direction = 0;
	_speed = lrp<real>( 0.1, _speed, direction * _initializer.speed );
			
	//
	//	Animate lifecycle
	//
		
	if ( lifecycle < 1 )
	{
		std::vector<real>::iterator radius = _radii.begin();
		for( cpShapeVec::iterator shape( _shapes.begin() ), end( _shapes.end() ); shape != end; ++shape, ++radius )
		{
			cpCircleShapeSetRadius( *shape, lifecycle * (*radius) );
		}
	}
	
	//
	//	Wobble - both fear and agression make the wobble period shorter
	//
	
	{
		real 
			period = lrp<real>( std::max( aggression, fear ), 1, 0.5 ) * fudge(),
			phase = M_PI * time.time / period,
			phaseOffset = M_PI / _gearJoints.size();
			
		for ( cpConstraintVec::iterator gear( _gearJoints.begin()), end( _gearJoints.end() ); gear != end; ++gear )
		{
			cpGearJointSetPhase( *gear,  0.5 * AngularLimit * std::sin( phase ));
			phase += phaseOffset;
		}	
	}
	
	
	//
	//	Apply motion, update AABB
	//

	{
		cpBB bounds = cpBBInvalid;
	
		real
			oscillationPeriod = fudge() * 2,
			oscillationMagnitude = fudge() * 0.25,
			oscillation = oscillationMagnitude * std::sin( M_PI * time.time / oscillationPeriod ) + 0.25*std::sin( M_PI_2 * time.time / (2*oscillationPeriod) );
			
		cpVect dir = cpv( up() );
		if ( dir.x < 0 ) dir = cpvmult( dir, -1 );
		cpVect sv = cpvmult( dir, (_speed + (direction*oscillation)));
		
		for( cpShapeVec::iterator shape( _shapes.begin()), end( _shapes.end() ); shape != end; ++shape )
		{
			cpShapeSetSurfaceVelocity( *shape, sv );
			cpBBExpand( bounds, cpShapeGetBB( *shape ));
		}
		
		setAabb( bounds );
	}

	
	if ( (_initializer.lifespan > 0 ) && alive() && (age() > _initializer.lifespan) )
	{
		health()->kill(); 
	}
	

}

// Monster
void Grub::died( const HealthComponent::injury_info &info )
{
	Monster::died( info );

	foreach( cpShape *shape, _shapes )
	{
		cpShapeSetLayers( shape, CollisionLayerMask::DEAD_MONSTER );
	}
}

Vec2 Grub::up() const 
{ 
	Vec2 right = v2( cpBodyGetRot( _headBody ));
	Vec2 up = rotateCCW(right);
	
	if ( up.y < 0 ) up *= -1;
	return up;
}

#pragma mark - GrubRenderer

/*
		GLuint _vbo;
		ci::gl::GlslProg _shader;
		Vec2fVec _triangleStrip;
*/
GrubRenderer::GrubRenderer():
	_vbo(0)
{
	setName( "GrubRenderer" );
}

GrubRenderer::~GrubRenderer()
{
	if ( _vbo )
	{
		glDeleteBuffers( 1, &_vbo );
	}
}

void GrubRenderer::draw( const render_state &state )
{
	_tesselate(state);	
	DrawComponent::draw( state );
}

void GrubRenderer::_tesselate( const render_state &state )
{
	/*
		Note, this is all being generated in world coordinate space
	*/

	Grub *grub = this->grub();
	
	const cpShapeVec &circles( grub->shapes() );
	const cpBodyVec &bodies( grub->bodies() );

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

void GrubRenderer::_drawGame( const render_state &state )
{
	Grub *grub = this->grub();


	if ( !_shader )
	{
		_shader = level()->resourceManager()->getShader( "GrubShader" );
	}
	
	if ( !_modulationTexture )
	{
		gl::Texture::Format noMipmappingFormat;
		noMipmappingFormat.enableMipmapping(false);

		_modulationTexture = level()->resourceManager()->getTexture( grub->initializer().modulationTexture, noMipmappingFormat );
	}

	_modulationTexture.bind(0);
	_modulationTexture.setWrap( GL_REPEAT, GL_REPEAT );

	ci::ColorA color = grub->initializer().color;
	color.a *= grub->lifecycle();
	
	_shader.bind();
	_shader.uniform( "ModulationTexture", 0 );
	_shader.uniform( "Color", color );

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
	
	_shader.unbind();
	_modulationTexture.unbind();
}

void GrubRenderer::_drawDebug( const render_state &state )
{
	Grub *grub = this->grub();
	_debugDrawChipmunkShapes( state, grub->shapes() );
	_debugDrawChipmunkConstraints( state, grub->constraints() );
	_debugDrawChipmunkBody( state, grub->body() );
	
	
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

#pragma mark - GrubController

GrubController::GrubController()
{
	setName( "GrubController" );
}

void GrubController::update( const time_state &time )
{
	GroundBasedMonsterController::update( time );
}

void GrubController::_getMotionProbeRays( cpVect &leftOrigin, cpVect &leftEnd, cpVect &rightOrigin, cpVect &rightEnd )
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
