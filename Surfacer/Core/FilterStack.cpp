//
//  FilterStack.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 11/14/11.
//  Copyright (c) 2011 TomorrowPlusX. All rights reserved.
//

#include "FilterStack.h"
#include <cinder/app/AppBasic.h>

using namespace ci;
namespace core {

#pragma mark - Filter

/*
		real _strength;
		std::string _name;
		FilterStack *_stack;
		Vec2r _vertices[4], _texCoords[4];
*/

Filter::Filter():
	_strength(1),
	_stack(NULL)
{
	_texCoords[0] = Vec2r(0,0);
	_texCoords[1] = Vec2r(1,0);
	_texCoords[2] = Vec2r(1,1);
	_texCoords[3] = Vec2r(0,1);
}

Filter::~Filter()
{}

ResourceManager *Filter::resourceManager() const 
{ 
	return _stack->resourceManager(); 
}

bool Filter::execute( ci::gl::Fbo input, ci::gl::Fbo output, const render_state &state, const ci::ColorA &clearColor )
{
	output.bindFramebuffer();	
	gl::setViewport( _bounds );
	gl::setMatricesWindow( _bounds.getWidth(), _bounds.getHeight(), false );

	gl::clear( clearColor );
	_render( input, state );
	return true;
}

void Filter::_blit()
{
	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );

	glVertexPointer( 2, GL_REAL, 0, _vertices );
	glTexCoordPointer( 2, GL_REAL, 0, _texCoords);
	glDrawArrays( GL_QUADS, 0, 4 );

	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );
}

void Filter::_resize( const Vec2i &newSize )
{
	_bounds.set( 0,0, newSize.x, newSize.y );	

	_vertices[0].x = 0;
	_vertices[0].y = 0;

	_vertices[1].x = newSize.x;
	_vertices[1].y = 0;

	_vertices[2].x = newSize.x;
	_vertices[2].y = newSize.y;

	_vertices[3].x = 0;
	_vertices[3].y = newSize.y;
	
	resize( newSize );
}



#pragma mark - FilterStack

/*
		ci::gl::Fbo _a, _b;
		std::vector< Filter* > _filters;
		
		ResourceManager *_resourceManager;

		ci::Area _bounds;
		Vec2r _vertices[4], _texCoords[4];

		ci::gl::GlslProg _blitAlphaCompositor;
		BlendMode _blendMode;
		ci::ColorA _clearColor, _compositeColor;
*/

FilterStack::FilterStack( ResourceManager *rm ):
	_resourceManager( rm ),
	_bounds(0,0,100,100),
	_clearColor(0,0,0,1),
	_compositeColor(1,1,1,1)
{
	_texCoords[0] = Vec2r(0,0);
	_texCoords[1] = Vec2r(1,0);
	_texCoords[2] = Vec2r(1,1);
	_texCoords[3] = Vec2r(0,1);
}

FilterStack::~FilterStack()
{
	foreach( Filter *f, _filters ) delete f;
}

void FilterStack::push( Filter *f )
{
	if ( f->stack() )
	{
		f->stack()->remove( f );
	}

	_filters.push_back(f);
	f->_setStack( this );
	f->_resize( _bounds.getSize() );
}

void FilterStack::pop()
{
	if ( !_filters.empty() )
	{
		Filter *f = _filters.back();
		if ( f && f->stack() == this )
		{
			f->_setStack( NULL );
		}
		
		_filters.pop_back();
	}
}

Filter *FilterStack::top() const
{
	return _filters.empty() ? NULL : _filters.back();
}

void FilterStack::remove( Filter *f )
{
	if ( f->stack() == this )
	{
		_filters.erase( std::remove( _filters.begin(), _filters.end(), f ), _filters.end() );
		f->_setStack( NULL );
	}
}

void FilterStack::beginCapture()
{
	_a.bindFramebuffer();
}

void FilterStack::beginCapture( const ci::ColorA &clearColor )
{
	_a.bindFramebuffer();
	_clearColor = clearColor;
	
	glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
	glClearColor( clearColor.r, clearColor.g, clearColor.b, clearColor.a );
	glClear( GL_COLOR_BUFFER_BIT );
}

void FilterStack::endCapture()
{
	_a.unbindFramebuffer();
}

void FilterStack::execute( const render_state &state, bool blitToScreen, bool blitRespectsAlpha )
{
	gl::Fbo result = execute( state, _filters );

	if ( blitToScreen )
	{
		_blitToScreen( result, blitRespectsAlpha );
	}
}

void FilterStack::blitToScreen( bool blitRespectsAlpha )
{
	_blitToScreen( _a, blitRespectsAlpha );
}	

gl::Fbo FilterStack::execute( const render_state &state, const std::vector<Filter*> &filters )
{
	gl::SaveFramebufferBinding bindingSaver;
	gl::pushMatrices();

	if ( !filters.empty() ) 
	{
		gl::disableWireframe();
		gl::enableAlphaBlending();
		
		foreach( Filter *f, filters )
		{
			if ( f->strength() > ALPHA_EPSILON )
			{
				//
				//	filters write from input to output, but some might write a second pass back into input,
				//	and so on. If the final result is in output the filter returns false and we swap. Otherwise
				//	we don't swap.
				//

				if (f->execute( _a, _b, state, _clearColor ))
				{
					std::swap( _a, _b );
				}
			}
		}
	}
	
	gl::popMatrices();
	
	return _a;
}


void FilterStack::resize( const Vec2i &newSize )
{
	Area newArea( 0,0,newSize.x, newSize.y );
	if ( !(newArea == _bounds))
	{
		_bounds = newArea;
		
		//
		//	update vertices for blitting
		//

		_vertices[0].x = 0;
		_vertices[0].y = 0;

		_vertices[1].x = newSize.x;
		_vertices[1].y = 0;

		_vertices[2].x = newSize.x;
		_vertices[2].y = newSize.y;

		_vertices[3].x = 0;
		_vertices[3].y = newSize.y;

		//
		//	recreate FBOs
		//

		gl::Fbo::Format format;
		format.setTarget(GL_TEXTURE_RECTANGLE_ARB);
		format.setColorInternalFormat( GL_RGBA8 );
		format.enableColorBuffer( true );
		format.enableDepthBuffer( false );
		format.enableMipmapping( false );

		_a = gl::Fbo( newSize.x, newSize.y, format );
		_b = gl::Fbo( newSize.x, newSize.y, format );

		foreach( Filter *f, _filters ) 
		{
			f->_resize( newSize );
		}
	}
}

void FilterStack::update( const time_state &time )
{
	foreach( Filter *f, _filters ) f->update(time);
}

void FilterStack::draw( gl::Fbo fbo )
{
	Area bounds = fbo.getBounds();
					
	gl::pushMatrices();

		gl::setViewport( bounds );
		gl::setMatricesWindow( bounds.getWidth(), bounds.getHeight(), false );
		
		_blendMode.bind();
		
		glEnableClientState( GL_VERTEX_ARRAY );
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );

		glVertexPointer( 2, GL_REAL, 0, _vertices );
		glTexCoordPointer( 2, GL_REAL, 0, _texCoords);

		fbo.bindTexture( 0 );
		glDrawArrays( GL_QUADS, 0, 4 );
		fbo.unbindTexture();

		glDisableClientState( GL_VERTEX_ARRAY );
		glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	
	gl::popMatrices();

}

void FilterStack::_blitToScreen( const ci::gl::Fbo &fbo, bool respectingAlpha )
{
	if ( respectingAlpha )
	{
		if ( !_blitAlphaCompositor )
		{
			_blitAlphaCompositor = resourceManager()->getShader( "FilterPassthrough" );
		}
		
		_blitAlphaCompositor.bind();
		_blitAlphaCompositor.uniform( "InputTex", 0 );
		_blitAlphaCompositor.uniform( "Color", _compositeColor );
				
		draw( fbo );
		
		_blitAlphaCompositor.unbind();
	}
	else
	{
		fbo.blitToScreen( _bounds, _bounds );
	}
}


} // end namespace core
