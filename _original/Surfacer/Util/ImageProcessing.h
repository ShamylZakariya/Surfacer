#pragma once

//
//  ImageProcessing.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 6/29/11.
//  Copyright 2011 Shamyl Zakariya. All rights reserved.
//

#include "Common.h"
#include <cinder/Color.h>
#include <cinder/Surface.h>
#include <cinder/Channel.h>
#include <cinder/ip/Resize.h>

namespace core { namespace util { namespace ip {

typedef std::pair<int,real> kernel_t;
typedef std::vector< kernel_t > kernel;

inline void create_kernel( int radius, kernel &k )
{
	k.clear();

	for ( int i = -radius; i <= radius; i++ )
	{
		int dist = std::abs( i );
		real mag = 1.0f - ( real(dist) / real(radius) );			
		k.push_back( kernel_t( i, std::sqrt(mag) ));
	}

	//
	// Get sum
	//
	
	real sum = 0;
	for ( int i = 0, N = k.size(); i < N; i++ )
	{
		sum += k[i].second;
	}

	//
	//	Now normalize kernelf into kernel
	//

	for ( int i = 0, N = k.size(); i < N; i++ )
	{
		k[i].second /= sum;
	}
}

class ChannelSampler 
{
	public:
	
		ChannelSampler( const ci::Channel8u &channel ):
			_channel( channel )
		{}
				
		int width() const { return _channel.getWidth(); }
		int height() const { return _channel.getHeight(); }
		uint8_t value( int x, int y ) const { return _channel.getValue( Vec2i( x,y ) ); }

	protected:

		ci::Channel8u _channel;
};

/**
	Perform a 

	@note Sampler must have the following methods
			
		int width() const;
		int height() const;
		uint8_t value( int x, int y ) const;	// clamped
*/

template< class SAMPLER >
ci::Channel8u blurIntoChannel8u( const SAMPLER &s, int radius )
{
	//
	//	Create the kernel and the buffers to hole horizontal & vertical passes
	//

	kernel krnl;
	create_kernel( radius, krnl );

	ci::Channel8u horizontalPass( s.width(), s.height() ),
	              verticalPass( s.width(), s.height() );

	//
	//	Store our iteration bounds
	//

	const int rows = s.height(),
	          cols = s.width();

	const kernel::const_iterator kend = krnl.end();

	//
	//	Run the horizontal pass
	//

	for ( int y = 0; y < rows; y++ )
	{
		for ( int x = 0; x < cols; x++ )
		{
			real accum = 0;
			for ( kernel::const_iterator k(krnl.begin()); k != kend; ++k )
			{
				accum += s.value( x + k->first, y ) * k->second;				
			}
			
			horizontalPass.setValue( Vec2i( x,y ), clamp( int(lrintf(accum)), 0, 255 ) );
		}
	}
	
	//
	//	Run the vertical pass
	//

	for ( int y = 0; y < rows; y++ )
	{
		for ( int x = 0; x < cols; x++ )
		{
			real accum = 0;
			for ( kernel::const_iterator k(krnl.begin()); k != kend; ++k )
			{
				accum += horizontalPass.getValue( Vec2i( x, y + k->first )) * k->second;
			}
			
			verticalPass.setValue( Vec2i( x,y ), clamp( int(lrintf(accum)), 0, 255 ) );
		}
	}
	
	return verticalPass;
}

/**
	Perform a downsampled blur. The source image will be downsampled by @a downsampleFactor,
	e.g., a 1024x1024 image with downsampleFactor of four will be reduced to 256x256. The blur
	will be run with a similarly downsampled radius on the reduced image, and then finally the 
	result will be scaled back up to the original size.

	@note SAMPLER must have a constructor that takes SOURCE
*/
template<class SAMPLER >
ci::Channel8u blurIntoChannel8uDownsampled( const ci::Channel8u &s, int radius, int downsampleFactor )
{
	ci::Channel8u downsampled( s.getWidth() / downsampleFactor, s.getHeight() / downsampleFactor );
	ci::ip::resize( s, &downsampled );
	
	ci::Channel8u downsampledResult = blurIntoChannel8u( SAMPLER( downsampled ), radius / downsampleFactor );
	ci::Channel8u result( s.getWidth(), s.getHeight() );
	ci::ip::resize( downsampledResult, &result );
	return result;
}

template< typename T >
ci::ColorAT<T> averageColor( const ci::SurfaceT<T> &surface )
{
	typedef typename ci::SurfaceT<T>::ConstIter ConstIter;

	if ( surface.hasAlpha())
	{
		double r=0, g=0, b=0, a=0;
		ConstIter iter = surface.getIter();
		while( iter.getLine() )
		{
			while( iter.getPixel() )
			{
				r += iter.r();
				g += iter.g();
				b += iter.b();
				a += iter.a();
			}
		}
		
		unsigned int count = surface.getWidth() * surface.getHeight();
		return ci::ColorA( r/count, g/count, b/count, a/count );	
	}
	
	double r=0, g=0, b=0;
	ConstIter iter = surface.getIter();
	while( iter.getLine() )
	{
		while( iter.getPixel() )
		{
			r += iter.r();
			g += iter.g();
			b += iter.b();
		}
	}
	
	unsigned int count = surface.getWidth() * surface.getHeight();
	return ci::ColorA( r/count, g/count, b/count, 1 );	
}

/**
	Premultiply the color channels of the Surface by it's alpha channel.
	
	note: If the Surface is marked as being premultiplied already, or if this
	Surface has no alpha channel, this will do nothing.
	
	Returns the input surface, modified in-place, to aid in composibility.
*/

inline ci::Surface &premultiply( ci::Surface &surface )
{
	if ( surface.hasAlpha() && !surface.isPremultiplied() )
	{
		ci::Surface::Iter iterator = surface.getIter();

		while( iterator.line() )
		{
			while( iterator.pixel() )
			{
				unsigned int 
					a = iterator.a(),
					r = iterator.r() * a,
					g = iterator.g() * a,
					b = iterator.b() * a;
						 
				iterator.r() = r / 255;
				iterator.g() = g / 255;
				iterator.b() = b / 255;
			}
		}
		
		surface.setPremultiplied(true);
	}
	
	return surface;
}


}}} // end namespace core::util::ip
