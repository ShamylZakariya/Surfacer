//
//  IslandRendering.cpp
//  PhysicsIntegration4
//
//  Created by Shamyl Zakariya on 5/10/11.
//  Copyright 2011 Shamyl Zakariya. All rights reserved.
//

#include "TerrainRendering.h"
#include "Terrain.h"

#include <cinder/app/App.h>

using namespace ci;
using namespace core;
namespace game { namespace terrain {

namespace {

	void FreeVbo( GLuint &id )
	{
		glDeleteBuffers( 1, &id );
		id = 0;
	}

}

#pragma mark -
#pragma mark IslandRenderer

/*
		GLuint _geometryVbo, _geometryVboVertexCount, _greeblingVbo, _greeblingVboVertexCount;
*/

IslandRenderer::IslandRenderer():
	_geometryVbo( 0 ),
	_geometryVboVertexCount(0),
	_greeblingVbo(0),
	_greeblingVboVertexCount(0)
{}

IslandRenderer::~IslandRenderer()
{
	FreeVbo( _geometryVbo );
	FreeVbo( _greeblingVbo );
}

void IslandRenderer::draw( const render_state &state )
{
	Island *island = (Island*) owner();
	
	gl::pushModelView();
	gl::multModelView( island->group()->modelview() );
	
	switch( state.pass )
	{
		case SOLID_GEOMETRY_PASS:
		{
			switch( state.mode )
			{
				case RenderMode::GAME:
				{
					_render_Geometry_Game( island, state );
					break;
				}

				case RenderMode::DEVELOPMENT:
				{
					_render_Geometry_Development( island, state );
					break;
				}

				case RenderMode::DEBUG:
				{
					_render_Geometry_Debug( island, state );
					break;
				}
				
				case RenderMode::COUNT: break;
			}

			break;
		}
		
		case GREEBLING_PASS:
		{
			switch( state.mode )
			{
				case RenderMode::GAME:
				{
					_render_Greebling_Game(island, state);
					break;
				}
				
				case RenderMode::DEVELOPMENT:
				{
					_render_Greebling_Development(island, state);
					break;
				}
				case RenderMode::DEBUG:
				{
					_render_Greebling_Debug( island, state );
					break;
				}
				
				case RenderMode::COUNT: break;
			}
		
			break;
		}
	}
	
	gl::popModelView();
}

void IslandRenderer::reset()
{
	FreeVbo( _geometryVbo );
	FreeVbo( _greeblingVbo );
	_geometryVboVertexCount = _greeblingVboVertexCount = 0;
}

#pragma mark -
#pragma mark Solid Geometry Rendering

void IslandRenderer::_render_Geometry_Game( Island *island, const render_state &state )
{
	const GLsizei stride = sizeof( triangle::vertex ),
		vertexOffset = 0,
		texCoordOffset = sizeof(ci::Vec2f);

	if ( !_geometryVbo )
	{
		const std::vector< triangle > &triangulation = island->triangulation();

		glGenBuffers( 1, &_geometryVbo );
		glBindBuffer( GL_ARRAY_BUFFER, _geometryVbo );
		glBufferData( GL_ARRAY_BUFFER, 
					  triangulation.size() * 3 * sizeof( triangle::vertex ), 
					  &( triangulation.front() ),
					  GL_STATIC_DRAW );

		_geometryVboVertexCount = triangulation.size() * 3;
	}


	glBindBuffer( GL_ARRAY_BUFFER, _geometryVbo );

		glEnableClientState( GL_VERTEX_ARRAY );
		glVertexPointer( 2, GL_REAL, stride, (void*) vertexOffset );

		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		glTexCoordPointer( 2, GL_REAL, stride, (void*) texCoordOffset );

			glDrawArrays( GL_TRIANGLES, 0, _geometryVboVertexCount );

		glDisableClientState( GL_VERTEX_ARRAY );
		glDisableClientState( GL_TEXTURE_COORD_ARRAY );

	glBindBuffer( GL_ARRAY_BUFFER, 0 );
}

void IslandRenderer::_render_Geometry_Development( Island *island, const render_state &state )
{
	_render_Geometry_Game( island, state );
}

void IslandRenderer::_render_Geometry_Debug( Island *island, const render_state &state )
{
	gl::color(1, 1, 1, cpBodyIsSleeping( island->group()->body() ) ? 0.75 : 0.25 );
	gl::disableWireframe();
	_render_Geometry_Game( island, state );
	if ( island->group()->terrain()->renderVoxelsInDebug() ) _render_DebugVoxels( island, state );

	gl::color(1, 1, 1, 1);
	_render_DebugOverlay( island, state );
}

void IslandRenderer::_render_DebugVoxels( Island *island, const render_state &state )
{
	const real scale = island->voxelStore()->scale(),
	           offset = 0.25 * scale;
	

	const Vec2r offsets[4] = {
		Vec2r(-offset,-offset),
		Vec2r(+offset,-offset),
		Vec2r(+offset,+offset),
		Vec2r(-offset,+offset)
	};

	ColorA 
		dc = island->group()->debugColor(),
		voxelColor( dc.r, dc.g, dc.b, 0.375 ),
		fixedVoxelColor( Color::white(), 0.75 );

	// draw all voxels
	foreach( Voxel *v, island->voxels() )
	{
		gl::color( v->fixed() ? fixedVoxelColor : voxelColor );

		if ( v->numIslands == 1 ) 
		{
			gl::drawSolidCircle( v->centroidRelativePosition, v->volume() * 0.5f * scale, 8 );
		}
		else			
		{
			int ii = v->islandIndex(island);				
			Vec2r offset = offsets[ ii % 4 ];
			gl::drawStrokedCircle( v->centroidRelativePosition + offset, v->volume() * 0.125f * scale, 8 );
		}
	}
}

void IslandRenderer::_render_DebugOverlay( Island *island, const render_state &state )
{
	//
	//	Draw wireframes
	//

	gl::enableWireframe();
	gl::color( ColorA(1,1,1,0.25) );
	_render_Geometry_Game( island, state );
	gl::disableWireframe();	
}

#pragma mark -
#pragma mark Greeble Rendering

void IslandRenderer::_render_Greebling_Game( Island *island, const render_state &state )
{
	const GLsizei stride = sizeof( perimeter_greeble_vertex ),
		vertexOffset = 0,
		texCoordOffset = 1 * sizeof(ci::Vec2f),
		maskTexCoordOffset = 2 * sizeof(ci::Vec2f),
		colorOffset = 3 * sizeof(ci::Vec2f);
		
	if ( !_greeblingVbo )
	{
		const std::vector< perimeter_greeble_vertex > &vertices = island->perimeterGreebleVertices();

		glGenBuffers( 1, &_greeblingVbo );
		glBindBuffer( GL_ARRAY_BUFFER, _greeblingVbo );
		glBufferData( GL_ARRAY_BUFFER, 
					  vertices.size() * sizeof( perimeter_greeble_vertex ), 
					  &( vertices.front() ),
					  GL_STATIC_DRAW );

		_greeblingVboVertexCount = vertices.size();
	}
		

	glBindBuffer( GL_ARRAY_BUFFER, _greeblingVbo );

	glEnableClientState( GL_VERTEX_ARRAY );
	glVertexPointer( 2, GL_REAL, stride, (void*) vertexOffset );

	glClientActiveTexture( GL_TEXTURE0 );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glTexCoordPointer( 2, GL_REAL, stride, (void*) texCoordOffset );

	glClientActiveTexture( GL_TEXTURE1 );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glTexCoordPointer( 2, GL_REAL, stride, (void*) maskTexCoordOffset );

	glEnableClientState( GL_COLOR_ARRAY );
	glColorPointer( 4, GL_REAL, stride, (void*) colorOffset );

	glDrawArrays( GL_QUADS, 0, _greeblingVboVertexCount );

	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glDisableClientState( GL_VERTEX_ARRAY );
	glClientActiveTexture( GL_TEXTURE0 );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	glClientActiveTexture( GL_TEXTURE1 );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	glDisableClientState( GL_COLOR_ARRAY );

	glClientActiveTexture( GL_TEXTURE0 );
}

void IslandRenderer::_render_Greebling_Development( Island *island, const render_state &state )
{
	const real alpha = 0.75;
	ci::ColorA color( island->debugColor() ),
	           xAxis( 1,0,0,alpha ),
	           yAxis( 0,1,0,alpha );

	color.a = alpha;
	
	const std::vector< perimeter_greeble_vertex > &vertices(island->perimeterGreebleVertices());
	for ( int i = 0, N = vertices.size(); i < N; i+=4 )
	{
		Vec2f a = vertices[i+0].position,
		      b = vertices[i+1].position,
		      c = vertices[i+2].position,
		      d = vertices[i+3].position;
			
		gl::color( color );
		gl::drawLine( a,b );
		gl::drawLine( b,c );
		gl::drawLine( c,d );
		gl::drawLine( d,a );
		
		gl::color( xAxis );
		gl::drawLine( mid(a,c),mid(b,c));

		gl::color( yAxis );
		gl::drawLine( mid(a,c),mid(c,d));
	}
}

void IslandRenderer::_render_Greebling_Debug( Island *island, const render_state &state )
{}

}} // end namespace game::terrain

