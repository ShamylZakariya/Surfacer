#pragma once

//
//  Filters.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 11/14/11.
//  Copyright (c) 2011 TomorrowPlusX. All rights reserved.
//

#include "FilterStack.h"

#include <cinder/gl/GlslProg.h>

namespace core {

/**
	perform color adjustments
	saturation, contrast and brightness all are clamped from 0 to 1, such that 0.5 is the identity value.
*/
class ColorAdjustmentFilter : public Filter
{
	public:
	
		static ColorAdjustmentFilter *createGreyscale()
		{
			ColorAdjustmentFilter *filter = new ColorAdjustmentFilter();
			filter->setSaturation(0);
			filter->setStrength(1);
			return filter;
		}

		static ColorAdjustmentFilter *createBlackAndWhite()
		{
			ColorAdjustmentFilter *filter = new ColorAdjustmentFilter();
			filter->setSaturation(0);
			filter->setContrast(0.875);
			filter->setStrength(1);
			
			return filter;
		}

	public:
	
		ColorAdjustmentFilter();
		virtual ~ColorAdjustmentFilter();
		
		void setSaturation( real sat ) { _saturation = saturate(sat); }
		real saturation() const { return _saturation; }

		void setContrast( real c ) { _contrast = saturate(c); }
		real contrast() const { return _contrast; }

		void setBrightness( real b ) { _brightness = saturate(b); }
		real brightness() const { return _brightness; }
	
	protected:

		virtual void _render( ci::gl::Fbo input, const render_state &state );

	private:
	
		ci::gl::GlslProg _shader;
		real _saturation, _contrast, _brightness;
				
};

class GradientMapFilter : public Filter
{
	public:
	
		static GradientMapFilter *create( const ci::ColorA &start, const ci::ColorA &end )
		{
			GradientMapFilter *gmf = new GradientMapFilter();
			gmf->set( start, end );
			return gmf;
		}

	public:
	
		GradientMapFilter();
		virtual ~GradientMapFilter();
		
		void set( const ci::ColorA &start, const ci::ColorA &end )
		{
			setStart(start);
			setEnd( end );
		}
		
		void setStart( const ci::ColorA &c );
		ci::ColorA start() const;

		void setEnd( const ci::ColorA &c );
		ci::ColorA end() const;
		
	protected:

		virtual void _render( ci::gl::Fbo input, const render_state &state );
								
	private:
	
		ci::gl::GlslProg _shader;
		Vec4f _gradient[2];
		
};



class PalettizingFilter : public Filter
{
	public:
	
		enum color_model {
			RGB,
			YUV,
			HSL
		};
		
		static PalettizingFilter *create( color_model m, int a, int b, int c, int alpha = 255 )
		{
			PalettizingFilter *pf = new PalettizingFilter(m);
			pf->setPaletteSize(a,b,c,alpha);
			return pf;
		}

	public:
	
		PalettizingFilter( color_model m = RGB );
		virtual ~PalettizingFilter();
		
		void setColorModel( color_model m );
		color_model colorModel() const {  return _model; }

		/**
			Set the size of each channel.
			If the mode is RGB, a,b & c correspond to red, green and blue channel sizes.
			If the mode if YUB, a,b & c correspond to luma, and two chrominance channels
			
			Setting each channel size to 8 would essentially result in a no-op 24-bit color space.
			Setting to zero would produce a full-black output. I find that ranges from 20 to 40 produce nice
			palettizing effects.
			
		*/
		void setPaletteSize( int a, int b, int c, int alpha = 255 )
		{
			_paletteSizes.x = clamp( a, 0, 255 );
			_paletteSizes.y = clamp( b, 0, 255 );
			_paletteSizes.z = clamp( c, 0, 255 );
			_paletteSizes.w = clamp( alpha, 0, 255 );
			_rPaletteSizes.x = 1 / _paletteSizes.x;
			_rPaletteSizes.y = 1 / _paletteSizes.y;
			_rPaletteSizes.z = 1 / _paletteSizes.z;
			_rPaletteSizes.w = 1 / _paletteSizes.w;
		}

		void paletteSizes( int &a, int &b, int &c, int &alpha ) const
		{
			a = int(_paletteSizes.x);
			b = int(_paletteSizes.y);
			c = int(_paletteSizes.z);
			alpha = int(_paletteSizes.w);
		}
		
		void setPaletteSize( const Vec4i ps ) { setPaletteSize( ps.x, ps.y, ps.z, ps.w ); }
		Vec4i paletteSize() const { return Vec4i( _paletteSizes.x, _paletteSizes.y, _paletteSizes.z, _paletteSizes.w ); }
	
	protected:
	
		virtual void _render( ci::gl::Fbo input, const render_state &state );
				
	private:
	
		ci::gl::GlslProg _shader;
		color_model _model;
		Vec4f _paletteSizes, _rPaletteSizes;
				
};

class GaussianBlurFilter : public Filter
{
	public:
	
		static GaussianBlurFilter *create( int kernelRadius )
		{
			return new GaussianBlurFilter( kernelRadius );
		}

	public:
	
		GaussianBlurFilter( int kernelRadius );
		virtual ~GaussianBlurFilter();
		
		virtual void setStrength( real s );

		void setKernelRadius( int kr );
		int kernelRadius() const { return _kernelRadius; }
		
		virtual bool execute( ci::gl::Fbo input, ci::gl::Fbo output, const render_state &state, const ci::ColorA &clearColor );
	
	protected:
	
		void _resetShaders();
		void _createKernel( int radius, std::vector< ci::Vec2f > &kernel );
		void _render( ci::gl::Fbo input, ci::gl::Fbo output, ci::gl::GlslProg shader );
				
	private:
	
		ci::gl::GlslProg _shaders[2];
		std::vector< ci::Vec2f > _kernel;
		int _kernelRadius;
		ci::ColorA _clearColor;
		
};


class NoiseFilter : public Filter
{
	public:
	
		static NoiseFilter *create( bool monochromatic )
		{
			NoiseFilter *nf = new NoiseFilter();
			nf->setMonochromeNoise(monochromatic);
			return nf;
		}
	
	public:
	
		NoiseFilter();
		virtual ~NoiseFilter();
		
		void setMonochromeNoise( bool mcn ) { _monochromeNoise = mcn; }
		bool monochromeNoise() const { return _monochromeNoise; }

	protected:
	
		virtual void _render( ci::gl::Fbo input, const render_state &state );
		virtual ci::gl::Texture _createNoiseTexture( int width, int height, bool monochrome ) const;

	private:
	
		ci::gl::Texture _noiseTexture;
		ci::gl::GlslProg _shader;
		bool _monochromeNoise;

};

class DamagedMonitorFilter : public Filter
{
	public:
	
		DamagedMonitorFilter();
		virtual ~DamagedMonitorFilter();
		
		void setPixelSizes( real redChannel, real blueChannel, real greenChannel )
		{
			setPixelSizes( Vec3r( redChannel, blueChannel, greenChannel ));
		}
		
		void setPixelSizes( const Vec3r sizesByChannel ) { _pixelSizesByChannel = static_cast<Vec3f>(max( sizesByChannel, Vec3r(1,1,1))); }
		Vec3r pixelSizes() const { return static_cast<Vec3r>(_pixelSizesByChannel); }
		
		void setChannelOffsets( const Vec2r &redChannel, const Vec2r &greenChannel, const Vec2r &blueChannel )
		{
			setRedChannelOffset(redChannel);
			setGreenChannelOffset(greenChannel);
			setBlueChannelOffset(blueChannel);
		}

		void setRedChannelOffset( const Vec2r &offset ) { _redChannelOffset = static_cast<Vec2f>(offset); }
		Vec2r redChannelOffset() const { return static_cast<Vec2r>(_redChannelOffset); }

		void setGreenChannelOffset( const Vec2r &offset ) { _greenChannelOffset = static_cast<Vec2f>(offset); }
		Vec2r greenChannelOffset() const { return static_cast<Vec2r>(_greenChannelOffset); }

		void setBlueChannelOffset( const Vec2r &offset ) { _blueChannelOffset = static_cast<Vec2f>(offset); }
		Vec2r blueChannelOffset() const { return static_cast<Vec2r>(_blueChannelOffset); }
		
		void setScanlineStrength( real s ) { _scanlineStrength = saturate( s ); }
		real scanlineStrength() const { return _scanlineStrength; }
		

		void setColorScale( const Vec3r scale ) { _colorScale = Vec4f( scale, 1.0 ); }
		Vec3r colorScale() const { return _colorScale.xyz(); }
		
		void setRedScale( real s ) { _colorScale.x = s; }
		void redScale() const { return _colorScale.x; }

		void setGreenScale( real s ) { _colorScale.y = s; }
		void greenScale() const { return _colorScale.y; }

		void setBlueScale( real s ) { _colorScale.z = s; }
		void blueScale() const { return _colorScale.z; }


		void setColorOffset( const Vec3r offset ) { _colorOffset = Vec4f( offset, 1.0 ); }
		Vec3r colorOffset() const { return _colorOffset.xyz(); }
		
		void setRedOffset( real offset ) { _colorOffset.x = offset; }
		void redOffset() const { return _colorOffset.x; }

		void setGreenOffset( real offset ) { _colorOffset.y = offset; }
		void greenOffset() const { return _colorOffset.y; }

		void setBlueOffset( real offset ) { _colorOffset.z = offset; }
		void blueOffset() const { return _colorOffset.z; }
		
		
	protected:
	
		virtual void _render( ci::gl::Fbo input, const render_state &state );

	private:
	
		ci::gl::GlslProg _shader;
		Vec3f _pixelSizesByChannel; 
		Vec4f _colorScale, _colorOffset;
		Vec2f _redChannelOffset, _blueChannelOffset, _greenChannelOffset;
		real _scanlineStrength;

};

}