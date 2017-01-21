//
//  CuttingBeam.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 2/8/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "CuttingBeam.h"

#include "GameLevel.h"
#include "PerlinNoise.h"
#include "Player.h"
#include "Terrain.h"


using namespace ci;
using namespace core;

namespace game {

/*
		init _initializer;
		bool _aiming;
		beam_segment_vec _beamSegments;
*/

CuttingBeam::CuttingBeam():
	_aiming(false)
{
	setName( "CuttingBeam" );
	addComponent( new CuttingBeamRenderer() );
	addComponent( new CuttingBeamController() );

	
	//
	//	Configure raycast querys
	//
	
	_setRaycastQueryFilter( defaultQueryFilter() );
}

CuttingBeam::~CuttingBeam()
{}

void CuttingBeam::initialize( const init &i )
{
	BeamWeapon::initialize(i);
	_initializer = i;
	BeamWeapon::setRange( _initializer.maxRange );
}

void CuttingBeam::update( const time_state &time )
{
	// we're skipping BeamWeapon::update
	Weapon::update( time );

	Player *player = static_cast<Player*>( parent() );

	if ( player->dead() )
	{
		setAiming(false);
	}


	_beamSegments.clear();
	
	const bool	
		Firing = this->firing(),
		Aiming = this->aiming();

	if ( Firing || Aiming ) 
	{
		reflective_raycast( 
			_beamSegments, 
			level(), 
			static_cast<CuttingBeam*>(this), 
			this->position(), 
			this->direction(), 
			this->range(), 
			_raycastQueryFilter(), 
			NULL,
			_initializer.maxBounces 
		);
		
		//
		//	Modify first segment origin to match this->emissionPosition()
		//
		
		beam_segment &first = _beamSegments.front();
		first.start = this->emissionPosition();
		first.length = distance( first.start, first.end );
		
		//
		//	Now expand AABB and evaluate cuts against terrain -- note we run whole beam and not just the endpoint(s)
		//
		
		cpBB bounds = cpBBInvalid;
		
		{
			const real Rad = _initializer.width * 0.5;

			for( beam_segment_vec::const_iterator seg(_beamSegments.begin()),end(_beamSegments.end()); seg != end; ++seg )
			{
				if ( Firing )
				{
					_cutTerrain( *seg ); 
				}

				cpBBExpand( bounds, cpv(seg->start), Rad );
				cpBBExpand( bounds, cpv(seg->end), Rad );
			}
		}	
		
		setAabb( bounds );		
	}
		
}

void CuttingBeam::setFiring( bool firing )
{
	if ( firing != this->firing() )
	{
		if ( firing )
		{
			core::InputDispatcher::get()->hideMouse();
		}
		else
		{
			core::InputDispatcher::get()->unhideMouse();
		}
		
		BeamWeapon::setFiring(firing);
	}
}

void CuttingBeam::setRange( real range )
{
	// cutting beam always fires to maxRange
	Weapon::setRange( _initializer.maxRange );
}

void CuttingBeam::setAiming( bool aiming ) 
{ 
	if ( aiming != _aiming )
	{
		if ( aiming )
		{
			core::InputDispatcher::get()->hideMouse();
		}
		else
		{
			core::InputDispatcher::get()->unhideMouse();
		}
		
		_aiming = aiming; 
	}
}


void CuttingBeam::_cutTerrain( const beam_segment &seg )
{
	const Vec2 
		Wiggle = seg.right * _initializer.cutWidth * _initializer.overcut * real(0.25) * std::cos( level()->time().time * 2 * M_PI ),
		Start = seg.start + Wiggle,
		End = seg.end + Wiggle + (seg.dir * _initializer.overcut * _initializer.cutWidth);
		
	((GameLevel*)level())->terrain()->cutLine( 
		Start, 
		End,
		_initializer.cutWidth, 
		_initializer.cutStrength, 
		TerrainCutType::GAMEPLAY );			
}

#pragma mark -

CuttingBeam::raycast_query_filter CuttingBeam::defaultQueryFilter()
{
	raycast_query_filter filter;
	filter.collisionType = 0;
	filter.collisionTypeMask = CollisionType::is::SHOOTABLE;
	filter.layers = CP_ALL_LAYERS;
	filter.group = CP_NO_GROUP;
	return filter;
}
	
void CuttingBeam::reflective_raycast( 
	beam_segment_vec &segments,
	Level *level,
	Weapon *weapon,
	Vec2 start,
	Vec2 dir,
	real range,
	raycast_query_filter filter,
	cpShape *ignore,
	unsigned int maxBounces )
{
	segments.clear();

	Vec2 end = start + ( dir * range );

	raycast_target target;
	target.shape = ignore;
	
	std::size_t bounceCount = 0;
	
	do 
	{	
		// the cutting beam emerges from the player's center, so we want to ignore the player on first bounce
		filter.ignoreCollisionType = bounceCount == 0 ? CollisionType::PLAYER : 0;
	
		// raycast, ignoring the last shape hit
		target = BeamWeapon::raycast( level, weapon, start, end, filter, target.shape );
		end = target.shape ? target.position : end;

		segments.push_back( beam_segment( start, end ));
		range -= segments.back().length;
	
		if ( weapon && weapon->firing() && target.object )
		{
			shot_info sinfo;
			sinfo.weapon = weapon;
			sinfo.target = target.object;
			sinfo.shape = target.shape;
			sinfo.position = target.position;
			sinfo.normal = target.normal;		
			weapon->objectWasHit( sinfo );
		}

		if ( segments.size() >= maxBounces ) 
		{
			break;
		}

		start = end;
		dir = reflect( dir, target.normal );
		end = start + dir * range;
		
		bounceCount++;

	} while( (range > Epsilon) && target.shape && (cpShapeGetCollisionType( target.shape ) == CollisionType::MIRROR));
}


#pragma mark -
#pragma mark CuttingBeamRenderer

/*
		GLuint _vbo;
		ci::gl::GlslProg _shader;
		vertex_vec _vertices;
		std::vector< Vec2 > _normalizedCircle;
		
		core::util::Flicker _innerFlicker, _outerFlicker;
				
		Vec4 _colors[4];
		bool _wasFiring, _wasAiming;
		real _onOffTransition;
*/

CuttingBeamRenderer::CuttingBeamRenderer():
	_vbo(0),
	_normalizedCircle( TRIANGLES_PER_DISC+1 ),
	_innerFlicker( 32, 8, 0.5, 80, 1234 ),
	_outerFlicker( 16, 6, 0.5, 80, 5678 ),
	_onOffTransition(0)
{
	//
	// Generate a normalized circle, starting at 12-oclock, going counter-clockwise
	//

	real 
		r = M_PI_2,
		inc = 2 * M_PI / real(TRIANGLES_PER_DISC);
	  
	for ( int i = 0; i < TRIANGLES_PER_DISC; i++ )
	{
		_normalizedCircle[i] = Vec2( std::cos( r ), std::sin( r ));
		r += inc;
	}	
	
	// we generate one extra so we don't need to use % in loop
	_normalizedCircle.back() = _normalizedCircle.front();
}

CuttingBeamRenderer::~CuttingBeamRenderer()
{
	if ( _vbo )
	{
		glDeleteBuffers( 1, &_vbo );
	}
}

void CuttingBeamRenderer::addedToGameObject( GameObject * )
{
	CuttingBeam *beam = cuttingBeam();
	const CuttingBeam::init &init( beam->initializer());
	_colors[0] = Vec4(init.beamColor);
	_colors[1] = Vec4(init.beamHaloColor);
	_colors[2] = Vec4(init.aimColor);
	_colors[3] = Vec4(init.aimHaloColor);
}


void CuttingBeamRenderer::update( const core::time_state &time )
{
	CuttingBeam *beam = cuttingBeam();
	
	_onOffTransition = lrp<real>( 0.2, _onOffTransition, (beam->firing() || beam->aiming()) ? 1 : 0 );

	if ( beam->aiming() ) 
	{
		_wasAiming = true;
		_wasFiring = false;
	}

	if ( beam->firing() )
	{
		_wasAiming = false;
		_wasFiring = true;
	}
}

void CuttingBeamRenderer::_drawGame( const core::render_state &state )
{
	if ( _onOffTransition < ALPHA_EPSILON ) return;

	CuttingBeam *beam = cuttingBeam();
	
	const CuttingBeam::beam_segment_vec 
		&Segments = beam->beamSegments();
			
	const real 
		Transition = _onOffTransition,
		AlphaTransition = std::sqrt( Transition ),
		
		InnerPhase = (_innerFlicker.update( state.deltaT ) * 2) + 1,
		OuterPhase = (_outerFlicker.update( state.deltaT ) * 2) + 1,


		MinVisibleBeamWidth = state.viewport.reciprocalZoom(),
		
		InnerWidth = _wasFiring 
			? std::max( InnerPhase * Transition * beam->initializer().width * real(0.5), MinVisibleBeamWidth ) 
			: MinVisibleBeamWidth,

		InnerRadius = InnerWidth * 2,

		OuterWidth = _wasFiring 
			? std::max( OuterPhase * Transition * beam->initializer().width * 2, 2 * MinVisibleBeamWidth ) 
			: std::max( (OuterPhase * Transition * real(0.25) ), 2 * MinVisibleBeamWidth ),

		OuterRadius = OuterWidth * 4;

	const Vec4
		AlphaTransitionV4(1,1,1,AlphaTransition),
		InnerColor( (_wasFiring ? _colors[0] : _colors[2]) * AlphaTransitionV4 ),
		OuterColor( (_wasFiring ? _colors[1] : _colors[3]) * AlphaTransitionV4 );

		
	//
	//	Lazily allocate storage - note beam is drawn twice, once bigger and fainter, second time smaller and brighter
	//

	if ( _vertices.empty() )
	{
		const unsigned int
			MaxSegments = beam->maxBeamSegments(),
			MaxDiscs = 1 + MaxSegments;	
					
		_vertices.resize( ((MaxSegments * TRIANGLES_PER_LINE) + (MaxDiscs * TRIANGLES_PER_DISC)) * 2 * 3 );
	}

	//
	//	Update geometry
	//

	vertex_vec::iterator vertex = _vertices.begin();
	const real MaxBeamRange = beam->initializer().maxRange;
	real distanceRemainingAlongBeam = MaxBeamRange;
	Vec4 
		innerColor = InnerColor,
		outerColor = OuterColor,
		nextInnerColor = innerColor,
		nextOuterColor = outerColor;

	_renderCircle( Segments.front().start, OuterRadius, outerColor, vertex );
	_renderCircle( Segments.front().start, InnerRadius, innerColor, vertex );

	for( CuttingBeam::beam_segment_vec::const_iterator seg(Segments.begin()),end(Segments.end()); seg != end; ++seg )
	{
		// sanity check - avoid degenerate segments
		if ( seg->length < Epsilon ) continue;

		real nextDistanceRemainingAlongBeam = distanceRemainingAlongBeam - seg->length;
		real nextDistanceAlpha = nextDistanceRemainingAlongBeam / MaxBeamRange;

		nextInnerColor.w = innerColor.w * nextDistanceAlpha;
		nextOuterColor.w = outerColor.w * nextDistanceAlpha;

		_renderCircle( seg->end, OuterRadius, nextOuterColor, vertex );
		_renderCircle( seg->end, InnerRadius, nextInnerColor, vertex );

		_renderLine( seg->start, seg->dir, seg->length, OuterWidth, outerColor, nextOuterColor, vertex );
		_renderLine( seg->start, seg->dir, seg->length, InnerWidth, innerColor, nextInnerColor, vertex );
		
		distanceRemainingAlongBeam = nextDistanceRemainingAlongBeam;		
		innerColor.w = nextInnerColor.w;
		outerColor.w = nextOuterColor.w;
	}
	
	
	
	//
	//	Update VBO
	//

	const GLsizei
		VertexCount = vertex - _vertices.begin(),
		Stride = sizeof( CuttingBeamRenderer::vertex ),
		PositionOffset = 0,
		ColorOffset = sizeof(Vec2);
		
	if ( !_shader )
	{
		_shader = level()->resourceManager()->getShader( "CuttingBeamShader" );
	}

	_shader.bind();
	_shader.uniform( "PaletteSize", 16.0f );
	_shader.uniform( "rPaletteSize", 0.0625f );

	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE );


	//
	//	Bind, update, stream draw VBO
	//

	if ( !_vbo )
	{
		glGenBuffers( 1, &_vbo );
		glBindBuffer( GL_ARRAY_BUFFER, _vbo );
		glBufferData( GL_ARRAY_BUFFER, 
					 _vertices.size() * Stride, 
					 NULL,
					 GL_STREAM_DRAW );
	}
	else 
	{
		glBindBuffer( GL_ARRAY_BUFFER, _vbo );
	}
	
	glBufferSubData( GL_ARRAY_BUFFER, 0, VertexCount * Stride, &_vertices.front() );
		
	glEnableClientState( GL_VERTEX_ARRAY );
	glVertexPointer( 2, GL_REAL, Stride, (void*)PositionOffset );

	glEnableClientState( GL_COLOR_ARRAY );
	glColorPointer( 4, GL_REAL, Stride, (void*)ColorOffset );
	
	glDrawArrays( GL_TRIANGLES, 0, VertexCount );
	
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_COLOR_ARRAY );

	_shader.unbind();
}

void CuttingBeamRenderer::_drawDebug( const core::render_state &state )
{
	CuttingBeam *beam = cuttingBeam();
	const CuttingBeam::beam_segment_vec &Segments = beam->beamSegments();
	const real Width = beam->initializer().width;

	ColorA color(0.5,0.5,1,1);
	real alphaStep = real(1) / real(Segments.size());

	foreach( const CuttingBeam::beam_segment &seg, Segments )
	{
		gl::color( color );
		gl::drawLine( seg.start, seg.end );
		gl::drawSolidCircle( seg.end, Width * 0.5, 12 );
		
		color.a -= alphaStep;
	}
}

void CuttingBeamRenderer::_renderCircle( const Vec2 &position, real radius, const Vec4 &color, vertex_vec::iterator &vertex ) const
{
	const Vec4 OuterColor( color.xyz(), color.w * 0.25 );

	// note: normalized circles has TRIANGLES_PER_DISC+1 vertices with back == front to eliminate modulus in loop
	for ( int i = 0, N = TRIANGLES_PER_DISC; i < N; i++ )
	{
		// inner
		vertex->position = position;
		vertex->color = color; 
		++vertex;

		// outer
		vertex->position = position + _normalizedCircle[i  ]*radius;
		vertex->color = OuterColor; 
		++vertex;

		vertex->position = position + _normalizedCircle[i+1]*radius;;
		vertex->color = OuterColor; 
		++vertex;
	}
}

void CuttingBeamRenderer::_renderLine( 
	const Vec2 &start, 
	const Vec2 &dir, 
	real length, 
	real width, 
	const Vec4 &startColor,
	const Vec4 &endColor,
	vertex_vec::iterator &vertex ) const
{
	/*
		Generate a lozenge with origin at start, rotated to point at end.
		The lozenge has hanf-circles (TRIANGLES_PER_DISC/2 triangles) on left and right, and four triangles
		making up the center.
		
		Rendering Endpoints: Draw a circle with left-half left alone, right half translated 'length along X
		
	*/
	
	const Vec4
		StartOuterColor( startColor.x, startColor.y, startColor.z, startColor.w * 0.25 ),
		EndOuterColor( endColor.x, endColor.y, endColor.z, endColor.w * 0.25 );

	const real Radius = width * 0.5;
	const Vec2 XOrigin(0,0), XEnd(length,0);

	const Mat4 R(
		  dir.x,   dir.y,     0,       0,
		 -dir.y,   dir.x,     0,       0,
		      0,       0,     1,       0,
		start.x, start.y,     0,       1 
	);
		
	//
	//	Render end caps
	//	NOTE: normalized circles has TRIANGLES_PER_DISC+1 vertices with back == front to eliminate modulus in loop
	//
	for ( int i = 0, N = TRIANGLES_PER_DISC/2; i < N; i++ )
	{
		// LEFT HALF
		// inner
		vertex->position = R * XOrigin;
		vertex->color = startColor; 
		++vertex;

		// outer
		vertex->position = R * (_normalizedCircle[i  ]*Radius);
		vertex->color = StartOuterColor; 
		++vertex;

		vertex->position = R * (_normalizedCircle[i+1]*Radius);
		vertex->color = StartOuterColor; 
		++vertex;

		// RIGHT HALF
		// inner
		vertex->position = R*XEnd;
		vertex->color = endColor; 
		++vertex;

		// outer
		vertex->position = R*(XEnd + _normalizedCircle[i+N  ]*Radius);
		vertex->color = EndOuterColor; 
		++vertex;

		vertex->position = R*(XEnd + _normalizedCircle[i+N+1]*Radius);
		vertex->color = EndOuterColor; 
		++vertex;
	}
	
	//
	//	Render beam: four triangles, 
	//	a----------d
	//	b----------e
	//	c----------f
	//
	
	Vec2 
		a = R*Vec2(0,Radius),
		b = R*Vec2(0,0),
		c = R*Vec2(0,-Radius),
		d = R*Vec2(length,Radius),
		e = R*Vec2(length,0),
		f = R*Vec2(length,-Radius);

	// ABE
	vertex->position = a;
	vertex->color = StartOuterColor;
	++vertex;

	vertex->position = b;
	vertex->color = startColor;
	++vertex;

	vertex->position = e;
	vertex->color = endColor;
	++vertex;


	// AED
	vertex->position = a;
	vertex->color = StartOuterColor;
	++vertex;

	vertex->position = e;
	vertex->color = endColor;
	++vertex;

	vertex->position = d;
	vertex->color = EndOuterColor;
	++vertex;


	// CBE
	vertex->position = c;
	vertex->color = StartOuterColor;
	++vertex;

	vertex->position = b;
	vertex->color = startColor;
	++vertex;

	vertex->position = e;
	vertex->color = endColor;
	++vertex;
	

	// CEF
	vertex->position = c;
	vertex->color = StartOuterColor;
	++vertex;

	vertex->position = e;
	vertex->color = endColor;
	++vertex;

	vertex->position = f;
	vertex->color = EndOuterColor;
	++vertex;
	
}


#pragma mark - CuttingBeamController

CuttingBeamController::CuttingBeamController()
{
	setName( "CuttingBeamController" );
	monitorKey( app::KeyEvent::KEY_SPACE );
}

CuttingBeamController::~CuttingBeamController()
{}

void CuttingBeamController::update( const time_state &time )
{
	CuttingBeam *cuttingBeam = this->cuttingBeam();
	Player *player = static_cast<GameLevel*>(level())->player();

	const Vec2 
		mousePosScreen = mousePosition(),
		mousePosWorld = level()->camera().screenToWorld( mousePosScreen ),
		dir = mousePosWorld - player->position();

	const real
		range2 = dir.lengthSquared(),
		range = range2 > Epsilon ? std::sqrt(range2) : 0;

	cuttingBeam->setDirection( range > 0 ? (dir/range) : Vec2(1,0) );
}

bool CuttingBeamController::mouseDown( const ci::app::MouseEvent &event )
{
	if ( active() )
	{
		if ( event.isLeft() ) 
		{
			cuttingBeam()->setFiring(true);
			return true;
		}
		
		
		if ( event.isRight() ) 
		{
			cuttingBeam()->setAiming( true );
			return true;
		}
	}
	
	return false;
}

bool CuttingBeamController::mouseUp( const ci::app::MouseEvent &event )
{
	if ( active() )
	{
		CuttingBeam *cuttingBeam = this->cuttingBeam();

		if ( event.isLeft() ) 
		{
			cuttingBeam->setFiring(false);
			return true;
		}
				
		if ( event.isRight() ) 
		{
			cuttingBeam->setAiming( false );
			return true;
		}
	}
	
	return true;
}

void CuttingBeamController::monitoredKeyDown( int keyCode )
{
	if ( active() )
	{
		switch( keyCode )
		{
			case app::KeyEvent::KEY_SPACE:
				cuttingBeam()->setAiming(true);
				break;
				
			default: break;
		}
	}
}

void CuttingBeamController::monitoredKeyUp( int keyCode )
{
	if ( active() )
	{
		switch( keyCode )
		{
			case app::KeyEvent::KEY_SPACE:
				cuttingBeam()->setAiming(false);
				break;

			default: break;
		}
	}
}



} // end namespace game
