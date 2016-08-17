//
//  Background.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 7/8/11.
//  Copyright 2011 Shamyl Zakariya. All rights reserved.
//

#include "Background.h"
#include "GameConstants.h"

#include <cinder/app/App.h>

using namespace ci;
using namespace core;

namespace game {

CLASSLOAD(Background);

namespace {

	Vec2r upscaleIfNecessary( Vec2r size, const Vec2r &targetSize )
	{
		if ( size.x < targetSize.x )
		{
			size *= targetSize.x / size.x;
		}
		
		if ( size.y < targetSize.y )
		{
			size *= targetSize.y / size.y;
		}

		return size;
	}

}


void Background::init::addLayer( const ci::fs::path &t, real s, const ColorA &c )
{
	layers.push_back( layer( t, s, c )); 
}

Background::Background():
	GameObject( GameObjectType::BACKGROUND )
{
	setVisibilityDetermination( VisibilityDetermination::ALWAYS_DRAW );
	setName( "Background" );
	setLayer( RenderLayer::BACKGROUND );	

	addComponent( new BackgroundRenderer() );
}

Background::~Background()
{}

void Background::initialize( const Background::init &initializer )
{
	GameObject::initialize(initializer);
	_initializer = initializer;
}

#pragma mark - Foreground

CLASSLOAD(Foreground);

Foreground::Foreground()
{
	setName( "Foreground" );
	setLayer( RenderLayer::FOREGROUND );
}

Foreground::~Foreground()
{}


#pragma mark - BackgroundRenderer

/*
		gl::GlslProg _shader;
		Vec3f _vertices[4];
		std::vector< ci::gl::Texture > _layerTextures;
*/

BackgroundRenderer::BackgroundRenderer()
{
	for ( int i = 0; i < 4; i++ ) _vertices[i] = Vec3f(0,0,0);
}

BackgroundRenderer::~BackgroundRenderer()
{}

void BackgroundRenderer::draw( const render_state &state )
{
	Background *background = (Background*) owner();
	const bool isBackground = ((GameObject*)background)->layer() == RenderLayer::BACKGROUND;
	const Viewport &vp = state.viewport;
	Vec2r bottomLeft = vp.screenToWorld( Vec2r(0,0)),
		  topRight = vp.screenToWorld( Vec2r( vp.size()) );
		  
	real outset = real(1) / vp.zoom();

	//
	//	Create the viewport in world space, with a small outset to 
	//	make certain the background covers the whole screen (in case of
	//	rounding errors )
	//

	Rectf viewportWorld( bottomLeft.x - outset, bottomLeft.y - outset, topRight.x + outset, topRight.y + outset );
	
	_vertices[0].x = bottomLeft.x;
	_vertices[0].y = bottomLeft.y;
	_vertices[1].x = topRight.x;
	_vertices[1].y = bottomLeft.y;
	_vertices[2].x = topRight.x;
	_vertices[2].y = topRight.y;
	_vertices[3].x = bottomLeft.x;
	_vertices[3].y = topRight.y;

	switch( state.mode )
	{
		case RenderMode::GAME:
		{
			_drawParallaxTexturedBackground( state, viewportWorld );
			break;
		}

		case RenderMode::DEVELOPMENT:
		{
			_drawParallaxTexturedBackground( state, viewportWorld );
			if ( isBackground )
			{
				_drawSectorOverlay( state, viewportWorld );
			}
			break;
		}

		case RenderMode::DEBUG:
		{	
			if ( isBackground )
			{
				gl::clear( background->initializer().clearColor );			
				_drawSectorOverlay( state, viewportWorld );
			}
			break;
		}
		
		case RenderMode::COUNT: break;
	}
}

void BackgroundRenderer::_drawParallaxTexturedBackground( const render_state &state, const Rectf &viewportWorld )
{
	Background *background = (Background*) owner();

	const std::vector< Background::layer > &layers = background->initializer().layers;
	const std::size_t numLayers = layers.size();
	
	//
	//	Load textures and shader
	//

	if ( _layerTextures.empty() )
	{			
		ResourceManager *rm = level()->resourceManager();
		foreach( const Background::layer &layer, layers )
		{
			_layerTextures.push_back( rm->getTexture( layer.texture ));
		}

		_shader = rm->getShader( "BackgroundShader.vert", "BackgroundShader" + str(numLayers) + ".frag" );
	}

	const cpBB 
		frustum = state.viewport.frustum(),
		bounds = background->level()->bounds();

	//
	//	Compute normalizedTextureOffset, which goes from 0->1 in x and y to position texture. 0 means bottom/left and 1 means top-right
	//

	Vec2r 
		vpSize( state.viewport.size() ),
		frustumSize( frustum.r - frustum.l, frustum.t - frustum.b ),
		boundsSize( bounds.r - bounds.l, bounds.t - bounds.b ),
		boundsCenter((bounds.l + bounds.r) * 0.5, (bounds.b + bounds.t) * 0.5 ),
		frustumCenter((frustum.l + frustum.r) * 0.5, (frustum.b + frustum.t) * 0.5 ),
		dir( (frustumCenter - boundsCenter) / ((boundsSize-frustumSize) * 0.5)),
		normalizedTextureOffset( dir * 0.5 + Vec2r(0.5,0.5));

//	Vec2r
//		vpSize( state.viewport.size() ),
//		frustumSize( frustum.r - frustum.l, frustum.t - frustum.b ),
//		boundsSize( bounds.r - bounds.l, bounds.t - bounds.b ),
//		window( boundsSize.x - frustumSize.x, boundsSize.y - frustumSize.y ),
//		normalizedTextureOffset( frustum.l / window.x, frustum.b / window.y ); 


	_shader.bind();
	
	for ( int i = 0; i < numLayers; i++ )
	{
		Vec2f size = layers[i].scale * vpSize;
		float max = std::max( size.x, size.y );
		
		Vec2f
			textureSize(max, max),
			textureSizeReciprocal( 1.0f / textureSize.x, 1.0f / textureSize.y ),
			textureOffset = normalizedTextureOffset * (textureSize - vpSize );

		std::string layerIdx = str(i);
		_shader.uniform( "Texture" + layerIdx, i );
		_shader.uniform( "Texture" + layerIdx + "Size", textureSize );
		_shader.uniform( "Texture" + layerIdx + "SizeReciprocal", textureSizeReciprocal );
		_shader.uniform( "Texture" + layerIdx + "Offset", textureOffset );
		_shader.uniform( "Texture" + layerIdx + "Color", layers[i].color );

		_layerTextures[i].bind(i);
	}

	//
	//	Backgrounds doesn't need blending, but foreground does
	//

	if ( ((GameObject*)background)->layer() == RenderLayer::BACKGROUND )
	{
		glDisable( GL_BLEND );
	}
	else
	{
		glEnable( GL_BLEND );
		glBlendFuncSeparate( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE );
	}
	
	//
	//	Draw
	//	

	glEnableClientState( GL_VERTEX_ARRAY );

	glVertexPointer( 3, GL_REAL, 0, _vertices );
	glDrawArrays( GL_QUADS, 0, 4 );

	glDisableClientState( GL_VERTEX_ARRAY );

	_shader.unbind();

	//
	//	Clean up
	//

	for ( std::size_t i = 0; i < numLayers; i++ )
	{
		_layerTextures[i].unbind(i);
	}
}

void BackgroundRenderer::_drawSectorOverlay( const render_state &state, const Rectf &viewportWorld )
{
	real left = viewportWorld.x1,
	     top = viewportWorld.y2,
		 right = viewportWorld.x2,
		 bottom = viewportWorld.y1;

	Background *background = (Background*) owner();
	int sectorSize = background->_initializer.sectorSize;

	gl::enableAlphaBlending();
	
	int colStart = sectorSize * ( int(left) / sectorSize), 
	    colEnd = int(right),
		rowStart = sectorSize * ( int(top) / sectorSize), 
		rowEnd = int( bottom );
		
	for ( int col = colStart; col <= colEnd; col += sectorSize )
	{
		if ( col == 0 )
		{
			gl::color( ColorA(1,0,0,0.75));
			gl::drawLine( Vec2f( real(col), 0 ), Vec2f( real(col), top ));
			gl::color( ColorA(0.25,0,0,0.75));
			gl::drawLine( Vec2f( real(col), 0 ), Vec2f( real(col), bottom ));
		}
		else
		{
			gl::color( ColorA(1,1,1,0.75));
			gl::drawLine( Vec2f( real(col), bottom ), Vec2f( real(col), top ));
		}
	}	

	for ( int row = rowStart; row >= rowEnd; row -= sectorSize )
	{
		if ( row == 0 )
		{
			gl::color( ColorA(0,1,0,0.75));
			gl::drawLine( Vec2f( 0, real(row) ), Vec2f( right, real(row) ));
			gl::color( ColorA(0,0.25,0,0.75));
			gl::drawLine( Vec2f( 0, real(row) ), Vec2f( left, real(row) ));
		}
		else
		{
			gl::color( ColorA(1,1,1,0.75));
			gl::drawLine( Vec2f( left, real(row) ), Vec2f( right, real(row) ));
		}
	}	

	gl::disableAlphaBlending();
}


}