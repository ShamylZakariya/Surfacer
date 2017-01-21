//
//  Filters.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 11/14/11.
//  Copyright (c) 2011 TomorrowPlusX. All rights reserved.
//

#include "Filters.h"

#include <cinder/app/App.h>
#include <cinder/Rand.h>
#include <cinder/Surface.h>

using namespace ci;
namespace core { 

namespace {

	/**
		replaces all instances of @a token with @a with in @a str
	*/	
	void replace( std::string &str, const std::string &token, const std::string &with )
	{
		std::string::size_type pos = str.find( token );
		while( pos != std::string::npos )
		{
			str.replace( pos, token.size(), with );		
			pos = str.find( token, pos );
		}		
	} 
	
	std::string bufferToString( Buffer buffer )
	{
		// what a pain in the ass

		boost::shared_ptr<char> sourceBlock( new char[buffer.getSize() + 1], boost::checked_array_deleter<char>() );
		memcpy( sourceBlock.get(), buffer.getData(), buffer.getSize() );
		sourceBlock.get()[buffer.getSize()] = 0; // null terminate

		return std::string( sourceBlock.get() );		
	}

}


#pragma mark - ColorAdjustmentFilter

/*
		gl::GlslProgRef _shader;
		real _saturation, _contrast, _brightness;
*/

ColorAdjustmentFilter::ColorAdjustmentFilter():
	_saturation(0.5),
	_contrast(0.5),
	_brightness(0.5)
{}

ColorAdjustmentFilter::~ColorAdjustmentFilter()
{}

void ColorAdjustmentFilter::_render( gl::Fbo input, const render_state &state )
{
	if ( !_shader )
	{
		_shader = resourceManager()->getShader( "FilterPassthrough.vert", "ColorAdjustmentFilter.frag" );
	}

	_shader->bind();
	_shader->uniform( "InputTex", int(0) );
	_shader->uniform( "Strength", float(strength()));

	//
	//	the shader epxects:
	//		saturation [0,N] with 1 as identity
	//		contrast [0,N] with 1 as identity
	//		brightness [-1,1] with 1 as identity
	//

	float 
		saturation = _saturation * 2,
		contrast = _contrast * 2,
		brightness = (_brightness * 2) - 1;

	_shader->uniform( "Saturation", saturation );
	_shader->uniform( "Contrast", contrast );
	_shader->uniform( "Brightness", brightness );

	
	input.bindTexture( 0 );			
	_blit();	
	input.unbindTexture();
	_shader->unbind();
}

#pragma mark - GradientMapFilter

/*
		gl::GlslProgRef _shader;
		Color _gradient[2];
*/

GradientMapFilter::GradientMapFilter()
{
	setName( "GradientMapFilter" );
	setStart( ColorA(0,0,0,1));
	setEnd( ColorA(1,1,1,1));
}

GradientMapFilter::~GradientMapFilter()
{}

void GradientMapFilter::setStart( const ci::ColorA &c )
{
	_gradient[0] = Vec4( c.r, c.g, c.b, c.a );
}

ci::ColorA GradientMapFilter::start() const
{
	return ci::ColorA( _gradient[0].x, _gradient[0].y, _gradient[0].z, _gradient[0].w );
}

void GradientMapFilter::setEnd( const ci::ColorA &c )
{
	_gradient[1] = Vec4( c.r, c.g, c.b, c.a );
}

ci::ColorA GradientMapFilter::end() const
{
	return ci::ColorA( _gradient[1].x, _gradient[1].y, _gradient[0].z, _gradient[0].w );
}

void GradientMapFilter::_render( gl::Fbo input, const render_state &state )
{
	if ( !_shader )
	{
		_shader = resourceManager()->getShader( "FilterPassthrough.vert", "GradientMapFilter.frag" );
	}

	_shader.bind();
	_shader.uniform( "InputTex", int(0) );
	_shader.uniform( "Strength", float(strength()));
	_shader.uniform( "GradientStart", _gradient[0] );
	_shader.uniform( "GradientEnd", _gradient[1] );
	
	input.bindTexture( 0 );			
	_blit();
	input.unbindTexture();
	_shader.unbind();
}

#pragma mark - PalettizingFilter

/*
		gl::GlslProg _shader;
		color_model _model;
		Vec3 _paletteSizes;
*/

PalettizingFilter::PalettizingFilter( color_model m ):
	_model(m)
{
	setPaletteSize(255, 255, 255);
}

PalettizingFilter::~PalettizingFilter()
{}

void PalettizingFilter::setColorModel( color_model m )
{
	_shader.reset();
	_model = m;
}

void PalettizingFilter::_render( gl::Fbo input, const render_state &state )
{
	if ( !_shader )
	{
		std::string fragExt;
		switch( _model )
		{
			case RGB: fragExt = "RGB"; break;
			case YUV: fragExt = "YUV"; break;
			case HSL: fragExt = "HSL"; break;
		}
	
		_shader = resourceManager()->getShader( "FilterPassthrough.vert", "PalettizingFilter_" + fragExt + ".frag" );
	}

	_shader.bind();
	_shader.uniform( "InputTex", int(0) );
	_shader.uniform( "Strength", float(strength()));
	_shader.uniform( "PaletteSize", _paletteSizes );
	_shader.uniform( "rPaletteSize", _rPaletteSizes );
	
	input.bindTexture( 0 );			
	_blit();
	input.unbindTexture();
	_shader.unbind();
}

#pragma mark - GaussianBlurFilter

/*
		gl::GlslProg _shaders[2];
		std::vector< Vec2 > _kernel;
		int _kernelRadius;
*/

GaussianBlurFilter::GaussianBlurFilter( int kernelRadius ):
	_kernelRadius(-1)
{
	setKernelRadius(kernelRadius);
}

GaussianBlurFilter::~GaussianBlurFilter()
{}

void GaussianBlurFilter::setStrength( real s )
{
	Filter::setStrength(s);
	_resetShaders();
}

void GaussianBlurFilter::setKernelRadius( int kr )
{
	if ( kr != _kernelRadius )
	{
		_kernelRadius = kr;
		_resetShaders();
	}
}

bool GaussianBlurFilter::execute( gl::Fbo input, gl::Fbo output, const render_state &state, const ci::ColorA &clearColor )
{
	_clearColor = clearColor;

	if ( !_shaders[0] )
	{
		_createKernel( int(std::floor(_kernelRadius * strength())), _kernel );	
	
		std::string 
			passthroughVertBuffer = bufferToString(resourceManager()->loadResource( "FilterPassthrough.vert" )->getBuffer()),
			horizontalFragBuffer = bufferToString(resourceManager()->loadResource( "GaussianBlurFilter_Horizontal.frag" )->getBuffer()),
			verticalFragBuffer = bufferToString(resourceManager()->loadResource( "GaussianBlurFilter_Vertical.frag" )->getBuffer());
	
		replace( horizontalFragBuffer, "__SIZE__", str(_kernel.size()) );
		replace( verticalFragBuffer, "__SIZE__", str(_kernel.size()) );
				
		try {
			_shaders[0] = gl::GlslProg( passthroughVertBuffer.c_str(), horizontalFragBuffer.c_str() );
		}
		catch( const gl::GlslProgCompileExc e )
		{
			app::console() << "GaussianBlurFilter - horizontal pass - error:\n\t" << e.what() << std::endl;
		}

		try {
			_shaders[1] = gl::GlslProg( passthroughVertBuffer.c_str(), verticalFragBuffer.c_str() );
		}
		catch( const gl::GlslProgCompileExc e )
		{
			app::console() << "GaussianBlurFilter - vertical pass - error:\n\t" << e.what() << std::endl;
		}
	}
	
	// horizontal blur
	_render( input, output, _shaders[0] );

	// swap input/output buffers
	std::swap( input, output );

	// vertical blur
	_render( input, output, _shaders[1] );

	
	//
	//	Return false to indicate final output is in the input FBO, not output
	//
	
	return false;
}

void GaussianBlurFilter::_resetShaders()
{
	_shaders[0].reset();
	_shaders[1].reset();
}


void GaussianBlurFilter::_render( gl::Fbo input, gl::Fbo output, gl::GlslProg shader )
{
	gl::SaveFramebufferBinding bindingSaver;

	output.bindFramebuffer();	
	gl::setViewport( bounds() );
	gl::setMatricesWindow( bounds().getWidth(), bounds().getHeight(), false );
	gl::clear( _clearColor );

	shader.bind();
	shader.uniform( "Image", 0 );
	shader.uniform( "Kernel", &(_kernel.front()), _kernel.size() );
	shader.uniform( "Strength", float( strength()));
	
	input.bindTexture(0);

	#warning GaussianBlurFilter doesn't support an alpha channel - disabling blend makes it work for non-alpha rendering
	glDisable( GL_BLEND );

	_blit();
	input.unbindTexture();
	
	shader.unbind();
}

void GaussianBlurFilter::_createKernel( int radius, std::vector< Vec2 > &kernel )
{
	kernel.clear();
	
	for ( int i = -radius; i <= radius; i++ )
	{
		int dist = std::abs( i );
		float mag = 1.0f - ( float(dist) / float(radius) );			
		kernel.push_back( Vec2( i, std::sqrt(mag) ));
	}

	//
	// normalize
	//
	
	float sum = 0;
	for ( int i = 0, N = kernel.size(); i < N; i++ )
	{
		sum += kernel[i].y;
	}

	for ( int i = 0, N = kernel.size(); i < N; i++ )
	{
		kernel[i].y /= sum;
	}							
}	

#pragma mark - NoiseFilter

/*
		ci::gl::Texture _noiseTexture;
		ci::gl::GlslProg _shader;
		bool _monochromeNoise;
*/

NoiseFilter::NoiseFilter():
	_monochromeNoise(true)
{}

NoiseFilter::~NoiseFilter()
{}

void NoiseFilter::_render( ci::gl::Fbo input, const render_state &state )
{
	if ( !_noiseTexture )
	{
		_noiseTexture = _createNoiseTexture( 200, 200, _monochromeNoise );
	}
	
	if ( !_shader )
	{
		_shader = resourceManager()->getShader( "FilterPassthrough.vert", "NoiseFilter.frag" );
	}

	_shader.bind();
	_shader.uniform( "InputTex", int(0) );
	_shader.uniform( "NoiseTex", int(1) );
	_shader.uniform( "NoiseTexSize", ci::Vec2( _noiseTexture.getSize() ));
	_shader.uniform( "NoiseTexOffset", ci::Vec2( Rand::randInt( _noiseTexture.getWidth() ), Rand::randInt( _noiseTexture.getHeight())));
	_shader.uniform( "Strength", float(strength()));
	
	input.bindTexture( 0 );
	_noiseTexture.bind(1);
	
	_blit();

	input.unbindTexture();
	_noiseTexture.unbind(1);
	
	_shader.unbind();
}

ci::gl::Texture NoiseFilter::_createNoiseTexture( int width, int height, bool monochrome ) const
{
	ci::Surface s( width, height, false, SurfaceChannelOrder::RGB );	
	Surface::Iter iter = s.getIter();

	if ( monochrome )
	{
		while( iter.line() )
		{
			while( iter.pixel() )
			{
				iter.r() = iter.g() = iter.b() = Rand::randInt( 255 );
			}
		}
	}
	else
	{
		while( iter.line() )
		{
			while( iter.pixel() )
			{
				iter.r() = Rand::randInt( 255 );
				iter.g() = Rand::randInt( 255 );
				iter.b() = Rand::randInt( 255 );
			}
		}
	}
	
	gl::Texture::Format format;
	format.setTargetRect();
	format.enableMipmapping(false);
	
	return gl::Texture( s, format );
}

#pragma mark - DamagedMonitorFilter

/*
		ci::gl::GlslProg _shader;
		Vec3 _pixelSizesByChannel; 
		Vec4 _colorScale, _colorOffset;
		Vec2 _redChannelOffset, _blueChannelOffset, _greenChannelOffset;
		real _scanlineStrength;
*/

DamagedMonitorFilter::DamagedMonitorFilter():
	_pixelSizesByChannel(1,1,1),
	_colorScale(1,1,1,1),
	_colorOffset(0,0,0,0),
	_redChannelOffset(0,0),
	_greenChannelOffset(0,0),
	_blueChannelOffset(0,0),
	_scanlineStrength(0)
{}

DamagedMonitorFilter::~DamagedMonitorFilter()
{}

void DamagedMonitorFilter::_render( ci::gl::Fbo input, const render_state &state )
{
	if ( !_shader )
	{
		_shader = resourceManager()->getShader( "FilterPassthrough.vert", "DamagedMonitorFilter.frag" );
	}

	_shader.bind();
	_shader.uniform( "InputTex", int(0) );
	_shader.uniform( "Strength", static_cast<float>(strength()));
	_shader.uniform( "SampleSizes", static_cast<Vec3>(_pixelSizesByChannel) );
	_shader.uniform( "RedOffset", static_cast<Vec2>(_redChannelOffset) );
	_shader.uniform( "GreenOffset", static_cast<Vec2>(_greenChannelOffset) );
	_shader.uniform( "BlueOffset", static_cast<Vec2>(_blueChannelOffset) );
	_shader.uniform( "ScanlineStrength", static_cast<float>(_scanlineStrength) );
	_shader.uniform( "ColorScale", lrp( strength(), Vec4(1,1,1,1), _colorScale ));
	_shader.uniform( "ColorOffset", lrp( strength(), Vec4(0,0,0,0), _colorOffset ));
	
	input.bindTexture( 0 );
	
	_blit();

	input.unbindTexture();
	
	_shader.unbind();
}

	
}
