//
//  Fronds.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 6/6/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Fronds.h"

#include "GameConstants.h"
#include "Level.h"
#include "Spline.h"

using namespace ci;
using namespace core;
namespace game {

#pragma mark - FrondEngine

/*
		init _initializer;
		std::vector< frond_state > _fronds;
*/

FrondEngine::FrondEngine():
	GameObject( GameObjectType::DECORATION )
{
	setName( "FrondEngine" );
	setLayer(RenderLayer::MONSTER - 1 );
	setVisibilityDetermination( VisibilityDetermination::FRUSTUM_CULLING );
	addComponent( new FrondEngineRenderer() );
}

FrondEngine::~FrondEngine()
{
	aboutToDestroyPhysics(this);
	_cleanup();	
}

void FrondEngine::initialize( const init &i )
{
	GameObject::initialize(i);
	_initializer = i;
	_color = _initializer.color;
}


void FrondEngine::addedToLevel( core::Level *level )
{
	GameObject::addedToLevel(level);
}

void FrondEngine::update( const core::time_state &state )
{
	//
	//	Update AABB, and update each frond_state.positions vector
	//
	
	const real Damping = 0.99;

	cpBB bounds = cpBBInvalid;
	for( frond_state_vec::iterator frond(_fronds.begin()),end(_fronds.end()); frond != end; ++frond )
	{
		frond->positions.clear();
		const real 
			HalfSegLength = frond->segmentLength * 0.5,
			FrondRadius = frond->radius;
		
		frond->positions.push_back( 
			v2r(cpBodyGetPos(frond->bodies.front() )) + 
			v2r(cpBodyGetRot(frond->bodies.front())) * 
			-HalfSegLength );
		
		for( cpBodyVec::const_iterator body(frond->bodies.begin()),bEnd(frond->bodies.end()); body != bEnd; ++body )
		{
			cpBBExpand(bounds, cpBodyGetPos(*body), FrondRadius );
			frond->positions.push_back( v2r(cpBodyGetPos( *body )) + v2r(cpBodyGetRot(*body )) * HalfSegLength );
			
			cpBodySetVel( *body, cpvmult( cpBodyGetVel( *body ), Damping ));
		}
	}
	
	setAabb(bounds);
}


void FrondEngine::addFrond( const frond_init &frond, cpBody *attachToBody, const Vec2r &positionWorld, const Vec2r &dirWorld )
{
	assert( level() );
	
	cpSpace *space = level()->space();

	frond_state state;
	state.segmentLength = frond.length / frond.count;
	state.radius = frond.thickness * 0.5;

	real 
		radius = state.radius;

	
	const cpVect 
		Dir = cpv(normalize(dirWorld)),
		PosInc = cpvmult( Dir, state.segmentLength );

	const real
		RadiusInc = -radius / frond.count,
		ForceOfGravity = cpvlength(cpSpaceGetGravity(space)),
		AngularLimit = lrp<real>(frond.stiffness, 0.25, 0.0125 ) * M_PI,
		TotalMass = frond.length * frond.thickness * frond.density,
		Angle = std::atan2( Dir.y, Dir.x );

	cpVect
		anchorPos = cpv(positionWorld),
		bodyPos = cpv(positionWorld) + cpvmult(PosInc, 0.5);
		
	
	cpBody *lastBody = attachToBody;
	cpConstraint *constraint = NULL;

	for ( int i = 0; i < frond.count; i++ )
	{
		const real
			DistanceAlongFrond = static_cast<real>(i) / frond.count,
			Mass = 2 * M_PI * radius * frond.density,
			Moment = cpMomentForCircle(Mass, 0, radius, cpvzero );
						
		cpBody *body = cpBodyNew(Mass, Moment);
		cpBodySetAngle( body, Angle );
		cpBodySetPos( body, bodyPos );
		cpSpaceAddBody( space, body );
		state.bodies.push_back(body);
		
		constraint = cpPivotJointNew( lastBody, body, anchorPos );
		state.constraints.push_back(constraint);
		cpSpaceAddConstraint( space, constraint );
			
//		constraint = cpGearJointNew(lastBody, body, 0, 1 );
//		state.constraints.push_back( constraint );
//		cpConstraintSetMaxBias( constraint, M_PI * 0.01 );
//		cpSpaceAddConstraint( space, constraint );
//		cpConstraintSetMaxForce( constraint, 1000 * frond.stiffness * TotalMass * ForceOfGravity/* * (1-DistanceAlongFrond)*/ );

		if ( i==0 )
		{
			constraint = cpRotaryLimitJointNew( lastBody, body, Angle - AngularLimit, Angle + AngularLimit );
		}
		else
		{
			constraint = cpRotaryLimitJointNew( lastBody, body, -AngularLimit, +AngularLimit );
		}
		
		state.constraints.push_back( constraint );
		cpConstraintSetMaxBias( constraint, M_PI * 0.01 );
//		cpConstraintSetMaxForce( constraint, 1000 * frond.stiffness * TotalMass * ForceOfGravity * (1-DistanceAlongFrond) );
		cpSpaceAddConstraint( space, constraint );
			
		// update params for next body
		radius += RadiusInc;
		bodyPos = cpvadd(bodyPos, PosInc);		
		anchorPos = cpvadd(anchorPos, PosInc );
		lastBody = body;
	}
	
	_fronds.push_back( state );
}

void FrondEngine::_setParent( core::GameObject *p ) 
{
	// why would you ever transfer fronds??!
	assert( !parent() );

	GameObject::_setParent(p);
	if (p)
	{
		p->aboutToDestroyPhysics.connect(this, &FrondEngine::_parentWillDestroyPhysics );
		setLayer(p->layer()-1);
	}
}	

void FrondEngine::_parentWillDestroyPhysics( core::GameObject * )
{
	_cleanup();
}

void FrondEngine::_cleanup()
{
	for( frond_state_vec::iterator frond(_fronds.begin()),end(_fronds.end()); frond != end; ++frond )
	{
		cpCleanupAndFree(frond->constraints);
		cpCleanupAndFree(frond->bodies);
	}
	
	_fronds.clear();
}


#pragma mark - FrondEngineRenderer

/*
		spline_rep_vec _splineReps;
		vertex_vec _vertices;
		GLuint _vbo;
		ci::gl::GlslProg _shader;		
*/

FrondEngineRenderer::FrondEngineRenderer():
	_vbo(0)
{}

FrondEngineRenderer::~FrondEngineRenderer()
{
	if ( _vbo )
	{
		glDeleteBuffers( 1, &_vbo );
	}
}

void FrondEngineRenderer::_drawGame( const core::render_state &state )
{
	FrondEngine *frondEngine = this->frondEngine();

	_tesselate(state);
	
	if ( !_shader )
	{
		_shader = frondEngine->level()->resourceManager()->getShader( "SvgPassthroughShader" );
	}
	
	const GLsizei 
		Stride = sizeof( FrondEngineRenderer::vertex ),
		PositionOffset = 0;
	
	if ( !_vbo )
	{
		glGenBuffers( 1, &_vbo );
		glBindBuffer( GL_ARRAY_BUFFER, _vbo );
		glBufferData( GL_ARRAY_BUFFER, 
					 _vertices.size() * Stride, 
					 NULL,
					 GL_STREAM_DRAW );
	}
	
	_shader.bind();

		glEnable( GL_BLEND );
		glColor4f( frondEngine->color() );

		glBindBuffer( GL_ARRAY_BUFFER, _vbo );
		glBufferSubData( GL_ARRAY_BUFFER, 0, _vertices.size() * Stride, &(_vertices.front()));
		
		glEnableClientState( GL_VERTEX_ARRAY );
		glVertexPointer( 2, GL_REAL, Stride, (void*)PositionOffset );
		
			glDrawArrays( GL_QUADS, 0, _vertices.size() );
		
		glBindBuffer( GL_ARRAY_BUFFER, 0 );
		glDisableClientState( GL_VERTEX_ARRAY );
	
	_shader.unbind();
}

void FrondEngineRenderer::_drawDebug( const core::render_state &state )
{
	foreach( const FrondEngine::frond_state &frond, frondEngine()->fronds() )
	{
		_debugDrawChipmunkBodies( state, frond.bodies );
		_debugDrawChipmunkConstraints( state, frond.constraints );
	}

	gl::color(1,1,1);	
	foreach( const FrondEngine::frond_state &frond, frondEngine()->fronds() )
	{
		for( Vec2rVec::const_iterator a(frond.positions.begin()),b(frond.positions.begin()+1),end(frond.positions.end()); b != end; ++a, ++b )
		{
			gl::drawLine(*a, *b);
		}
	}

	if ( false )
	{
		_tesselate(state);
		foreach( const spline_rep &splineRep, _splineReps )
		{
			for( Vec2rVec::const_iterator a(splineRep.positions.begin()),b(splineRep.positions.begin()+1),end(splineRep.positions.end()); b != end; ++a, ++b )
			{
				gl::drawLine(*a, *b);
			}
		}
		
		for( vertex_vec::const_iterator v(_vertices.begin()),end(_vertices.end()); v != end; v+=4 )
		{
			gl::drawLine( (v+0)->position, (v+1)->position );
			gl::drawLine( (v+1)->position, (v+2)->position );
			gl::drawLine( (v+2)->position, (v+3)->position );
			gl::drawLine( (v+3)->position, (v+0)->position );
			gl::drawLine( (v+0)->position, (v+2)->position );
			gl::drawLine( (v+1)->position, (v+3)->position );
		}
	}
}

void FrondEngineRenderer::_tesselate( const core::render_state & )
{
	FrondEngine *frondEngine = this->frondEngine();
	const FrondEngine::frond_state_vec &fronds(frondEngine->fronds());
	
	//
	//	Update spline reps
	//

	if ( fronds.size() != _splineReps.size() )
	{
		_splineReps.resize( fronds.size() );
	}
	
	_vertices.clear();
	
	spline_rep_vec::iterator spline(_splineReps.begin());
	for( FrondEngine::frond_state_vec::const_iterator frond(fronds.begin()),end(fronds.end()); frond != end; ++frond, ++spline )
	{
		spline->positions.clear();
		util::spline::spline<real>( frond->positions, 0.5, frond->positions.size() * 2, false, spline->positions );

		//
		//	Now quadrangulate - we have to use quads not triangle strips because we want to draw all fronds in one batch and they're
		//	not connected to each other
		//
		
		real radius = frond->radius;
		const real RadiusInc = -radius / spline->positions.size();
		std::size_t i = 0;
		
		for ( Vec2fVec::const_iterator a(spline->positions.begin()),b(spline->positions.begin()+1),end(spline->positions.end()); b!=end; ++a,++b, ++i )
		{
			const Vec2f 
				Dir = normalize(*b - *a),
				Right = rotateCW(Dir) * radius;
				
			if ( i==0 )
			{
				_vertices.push_back( *a + Right );
				_vertices.push_back( *a - Right );
			}
			else
			{
				_vertices.push_back( *a - Right );
				_vertices.push_back( *a + Right );

				_vertices.push_back( *a + Right );
				_vertices.push_back( *a - Right );
			}
				
			radius += RadiusInc;
		}
		
		//
		//	Add final collapsed vertices for tip
		//
		
		_vertices.push_back( spline->positions.back() );
		_vertices.push_back( spline->positions.back() );
	}
}


}