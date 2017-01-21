//
//  Mirror.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/11/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Mirror.h"

#include "GameLevel.h"
#include "GameConstants.h"
#include "PerlinNoise.h"
#include "Player.h"

#include <cinder/gl/gl.h>


using namespace ci;
using namespace core;
namespace game {

CLASSLOAD(Mirror);

/*
		init _initializer;
		cpBody *_body;
		cpShape *_caseShape, *_mirrorShape;
		cpConstraint *_rotationConstraint;
		bool _beingTowed;
		MirrorRotationOverlay *_rotationOverlay;
*/

Mirror::Mirror():
	GameObject( GameObjectType::DECORATION ),
	_body(NULL),
	_caseShape(NULL),
	_mirrorShape(NULL),
	_rotationConstraint(NULL),
	_beingTowed(false),
	_rotationOverlay( new MirrorRotationOverlay(this))
{
	setName( "Mirror" );
	setLayer(RenderLayer::CHARACTER - 1 );
	addComponent( new MirrorRenderer());
	setVisibilityDetermination( VisibilityDetermination::FRUSTUM_CULLING );

	addChild( _rotationOverlay );	
}

Mirror::~Mirror()
{
	aboutToDestroyPhysics(this);
	cpConstraintCleanupAndFree( &_rotationConstraint );
	cpShapeCleanupAndFree( &_caseShape );
	cpShapeCleanupAndFree( &_mirrorShape );
	cpBodyCleanupAndFree( &_body );
}

void Mirror::initialize( const init &i )
{
	GameObject::initialize(i);
	_initializer = i;
}

void Mirror::addedToLevel( core::Level *level )
{
	cpSpace *space = level->space();

	const real 
		Radius = _initializer.size * 0.5,
		Density = 100, 
		Mass = 2 * M_PI * Radius * Density,
		Moment = cpMomentForCircle( Mass, 0, Radius, cpvzero );
		
	_body = cpBodyNew(Mass, Moment);
	cpBodySetPos( _body, cpv(_initializer.position) );	
	
	_caseShape = cpCircleShapeNew( _body, Radius, cpvzero );
	cpShapeSetUserData( _caseShape, this );	
	cpShapeSetCollisionType( _caseShape, CollisionType::MIRROR_CASING );
	cpShapeSetLayers( _caseShape, CollisionLayerMask::MIRROR );
	cpShapeSetFriction( _caseShape, 10 );
	cpShapeSetElasticity( _caseShape, 0 );
	
	_mirrorShape = cpSegmentShapeNew( _body, cpv(0,-Radius*0.9), cpv(0,+Radius*0.9), Radius * 0.1 );
	cpShapeSetUserData( _mirrorShape, this );	
	cpShapeSetCollisionType( _mirrorShape, CollisionType::MIRROR );
	cpShapeSetLayers( _mirrorShape, CollisionLayerMask::MIRROR );
	
	_rotationConstraint = cpGearJointNew(cpSpaceGetStaticBody( space ), _body, 0, 1 );

	cpSpaceAddBody( space, _body );
	cpSpaceAddShape( space, _caseShape );
	cpSpaceAddShape( space, _mirrorShape );
	cpSpaceAddConstraint( space, _rotationConstraint );
	
}

void Mirror::update( const core::time_state &time )
{
	setAabb( cpShapeGetBB( _caseShape ));
}

void Mirror::aboutToBeTowed( MagnetoBeam *beam )
{
	cpConstraintSetMaxForce( _rotationConstraint, 0 );
	_beingTowed = true;
}

void Mirror::aboutToBeUnTowed( MagnetoBeam *beam )
{
	// towing ends, sync our internal gear joint angle to what the user set via MagnetoBeam
	cpGearJointSetPhase( _rotationConstraint, cpBodyGetAngle( _body ));
	cpConstraintSetMaxForce( _rotationConstraint, INFINITY );
	_beingTowed = false;
}


#pragma mark - MirrorRenderer

/*
		ci::gl::GlslProg _svgShader;
		core::SvgObject _svg, _halo;
		Vec2 _svgScale;
		core::util::Flicker _flicker;
*/

MirrorRenderer::MirrorRenderer():
	_svgScale(1,1),
	_flicker( 32 )
{
	setName( "MirrorRenderer" );
}

MirrorRenderer::~MirrorRenderer()
{}

void MirrorRenderer::_drawGame( const core::render_state &state )
{
	Mirror *mirror = this->mirror();

	if ( !_svg )
	{
		_svg = level()->resourceManager()->getSvg( "Mirror.svg" );
		_halo = _svg.find( "halo" );
		assert( _halo );
		_halo.setBlendMode( BlendMode( GL_SRC_ALPHA, GL_ONE ));

		// mirror is circular, so we can treat x&y as the same
		Vec2 documentSize = _svg.documentSize();
		_svgScale.x = _svgScale.y = mirror->initializer().size / documentSize.x;

		_svgShader = level()->resourceManager()->getShader( "SvgPassthroughShader" );
	}
	
	//
	// Update noise value for flickering
	//

	real flickerValue = _flicker.update( state.deltaT );
	//flickerValue = (flickerValue*flickerValue) * sign(flickerValue);
	_halo.setOpacity( flickerValue > 0 ? 1 : 0 );
	

	_svgShader.bind();
	
	_svg.setPosition( mirror->position() );
	_svg.setScale( _svgScale );
	_svg.setAngle( cpBodyGetAngle( mirror->body() ));

	_svg.draw( state );
	
	_svgShader.unbind();
}

void MirrorRenderer::_drawDebug( const core::render_state &state )
{
	Mirror *mirror = this->mirror();
	ci::ColorA 
		mirrorColor = mirror->debugColor(),
		caseColor = mirrorColor;
		
	caseColor.a *= 0.25;
		
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	_debugDrawChipmunkBody( state, mirror->body() );
	_debugDrawChipmunkShape( state, mirror->caseShape(), caseColor );
	_debugDrawChipmunkShape( state, mirror->mirrorShape(), mirrorColor );
}

#pragma mark - MirrorRotationOverlay

/*
		Mirror *_mirror;
		vertex_vec _planeVertices;
		ci::gl::GlslProg _shader;		
		GLint _distanceFromOriginAttrLoc;
		GLuint _planeVbo;
		CuttingBeam::beam_segment_vec _beamPathSegments;
		CuttingBeam::raycast_query_filter _rqf;
*/

MirrorRotationOverlay::MirrorRotationOverlay( Mirror *mirror ):
	GameObject( GameObjectType::DECORATION ),
	_mirror( mirror ),
	_planeVertices(6), // two quads in quadstrip
	_distanceFromOriginAttrLoc(0),
	_planeVbo(0)
{
	setName( "MirrorRotationOverlay" );
	setLayer(RenderLayer::EFFECTS );
	setVisibilityDetermination( VisibilityDetermination::ALWAYS_DRAW );
	
	_rqf = CuttingBeam::defaultQueryFilter();	
}
	
MirrorRotationOverlay::~MirrorRotationOverlay()
{
	if ( _planeVbo )
	{
		glDeleteBuffers(1, &_planeVbo );
	}
}

void MirrorRotationOverlay::draw( const core::render_state &state )
{
	if ( _mirror->beingTowed() )
	{
		if ( !_shader )
		{
			_shader = level()->resourceManager()->getShader( "MirrorRotationOverlayShader" );
			_distanceFromOriginAttrLoc = _shader.getAttribLocation( "DistanceFromOriginAttr" );
		}
		
	
		//
		//	Draw a line across the screen, through the center of the mirror
		//

		const ColorA 
			ReflectionPlaneColor(1,1,1,0.5),
			ReflectedBeamColor(1,0,1,0.75);
			
		//
		//	Bind shader - we're drawing a dashed line
		//

		_shader.bind();
		_shader.uniform( "DashLength", float(5));

//		_shader.uniform( "Color", ReflectionPlaneColor );
//		_renderReflectionPlane( state );

		_shader.uniform( "Color", ReflectedBeamColor );
		_renderBeamPath( state );
				
		_shader.unbind();
	}
}

void MirrorRotationOverlay::_renderReflectionPlane( const core::render_state &state )
{
	{
		const Vec2i	
			ScreenSize = state.viewport.size();

		const real
			RZoom = state.viewport.reciprocalZoom(),
			LineLengthScreen = std::max( ScreenSize.x, ScreenSize.y ) * real(1.414213562),
			LineLengthWorld = LineLengthScreen * RZoom,
			LineHalfWidth = 0.5,
			LineHalfWidthWorld = LineHalfWidth * RZoom;


		//
		//	We're rendering a quadstrip, two quads - 6 vertices
		//

		vertex_vec::iterator vertex = _planeVertices.begin();

		vertex->position.x = +LineHalfWidthWorld;
		vertex->position.y = -LineLengthWorld;
		vertex->distanceFromOrigin = -LineLengthScreen;
		vertex++;

		vertex->position.x = -LineHalfWidthWorld;
		vertex->position.y = -LineLengthWorld;
		vertex->distanceFromOrigin = -LineLengthScreen;
		vertex++;

		vertex->position.x = +LineHalfWidthWorld;
		vertex->position.y = 0;
		vertex->distanceFromOrigin = 0;
		vertex++;

		vertex->position.x = -LineHalfWidthWorld;
		vertex->position.y = 0;
		vertex->distanceFromOrigin = 0;
		vertex++;
		
		vertex->position.x = +LineHalfWidthWorld;
		vertex->position.y = +LineLengthWorld;
		vertex->distanceFromOrigin = +LineLengthScreen;
		vertex++;

		vertex->position.x = -LineHalfWidthWorld;
		vertex->position.y = +LineLengthWorld;
		vertex->distanceFromOrigin = +LineLengthScreen;
		vertex++;
	}

	const Vec2
		Position = _mirror->position(),
		Rotation = v2(cpBodyGetRot( _mirror->body() ));
		
		
	//
	//	Create and rotation matrix - note Rotation is the mirror's X axis, and
	//	we draw the line as a vertical line along +- Y.
	//

	Mat4 R;
	Mat4WithPositionAndRotation( R, Position, Rotation );
	
	gl::pushModelView();
	gl::multModelView( R );

	const GLsizei 
		Stride = sizeof( MirrorRotationOverlay::vertex ),
		PositionOffset = 0,
		DistanceFromOriginAttrOffset = sizeof( Vec2 );
		
	if ( !_planeVbo )
	{
		glGenBuffers( 1, &_planeVbo );
		glBindBuffer( GL_ARRAY_BUFFER, _planeVbo );
		glBufferData( GL_ARRAY_BUFFER, 
					 _planeVertices.size() * Stride, 
					 &(_planeVertices.front()),
					 GL_STREAM_DRAW );
	}
	else 
	{
		glBindBuffer( GL_ARRAY_BUFFER, _planeVbo );
		glBufferSubData( GL_ARRAY_BUFFER, 0, _planeVertices.size() * Stride, &(_planeVertices.front()) );
	}
	
	glEnableClientState( GL_VERTEX_ARRAY );
	glVertexPointer( 2, GL_REAL, Stride, (void*)PositionOffset );	

	glEnableVertexAttribArray( _distanceFromOriginAttrLoc );
	glVertexAttribPointer( _distanceFromOriginAttrLoc, 1, GL_REAL, GL_FALSE, Stride, (void*) DistanceFromOriginAttrOffset );

	glDrawArrays( GL_QUAD_STRIP, 0, _planeVertices.size() );		
	glBindBuffer( GL_ARRAY_BUFFER, 0 );	

	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableVertexAttribArray( _distanceFromOriginAttrLoc );
	
	gl::popModelView();	
}

void MirrorRotationOverlay::_renderBeamPath( const core::render_state &state )
{
	const bool RenderFirstSegment = true;

	{	
		//
		//	Compute reflected beam segments, then update the beam path vertices
		//

		Weapon *weapon = ((GameLevel*)level())->player()->weapon();
		
		CuttingBeam::reflective_raycast( 
			_beamPathSegments, 
			level(), 
			NULL, 
			weapon->position(), 
			weapon->direction(), 
			weapon->initializer().maxRange,
			_rqf );
			

		const real
			Zoom = state.viewport.zoom(),
			RZoom = state.viewport.reciprocalZoom(),
			LineHalfWidth = 0.5,
			LineHalfWidthWorld = LineHalfWidth * RZoom;

		real
			dist = 0;

		if ( _beamPathVertices.size() != _beamPathSegments.size() * 4 )
		{
			_beamPathVertices.resize( _beamPathSegments.size() * 4 );
		}	

		vertex_vec::iterator vertex = _beamPathVertices.begin();
		
		for( CuttingBeam::beam_segment_vec::iterator 
			segment( _beamPathSegments.begin() ),
			end( _beamPathSegments.end()); 
			segment != end; 
			++segment )
		{
			const Vec2 Right = segment->right * LineHalfWidthWorld;

			vertex->position = segment->start + Right;
			vertex->distanceFromOrigin = dist;
			vertex++;

			vertex->position = segment->start - Right;
			vertex->distanceFromOrigin = dist;
			vertex++;
			
			dist += segment->length * Zoom;

			vertex->position = segment->end - Right;
			vertex->distanceFromOrigin = dist;
			vertex++;

			vertex->position = segment->end + Right;
			vertex->distanceFromOrigin = dist;
			vertex++;
		}					
	}
	
	//
	//	Stream
	//
	
	const GLsizei Stride = sizeof( MirrorRotationOverlay::vertex );
	const unsigned int First = RenderFirstSegment ? 0 : 4;
	
	if ( _beamPathSegments.size() - First == 0 ) return;
		
	glEnableClientState( GL_VERTEX_ARRAY );
	glVertexPointer( 2, GL_REAL, Stride, &(_beamPathVertices[First].position) );	

	glEnableVertexAttribArray( _distanceFromOriginAttrLoc );
	glVertexAttribPointer( _distanceFromOriginAttrLoc, 1, GL_REAL, GL_FALSE, Stride, &(_beamPathVertices[First].distanceFromOrigin) );

	glDrawArrays( GL_QUADS, 0, _beamPathVertices.size() );		

	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableVertexAttribArray( _distanceFromOriginAttrLoc );
}



}
