//
//  Flicker.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/16/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Flicker.h"
#include "PerlinNoise.h"
#include <cinder/Rand.h>

namespace core { namespace util {

/*
		int _octaves, _seed;
		double _falloff, _scale;
		std::vector< double > _noise;
		double _value, _offset, _offsetRate;
*/

Flicker::Flicker( double rate, int octaves, double falloff, double scale, int seed, unsigned int bufferSize ):
	_octaves(std::max(octaves,1)),
	_seed( seed ),
	_falloff(saturate(falloff)),
	_scale( std::max( scale, 0.01 )),
	_noise( bufferSize ),
	_value(0),
	_offset(0),
	_offsetRate(std::abs(rate))
{
	_generate();
}

void Flicker::setBufferSize( unsigned int bs )
{
	_noise.resize( bs );
	_generate();
}


double Flicker::update( seconds_t elapsedTime )
{
	_offset += _offsetRate * elapsedTime;

	double	
		offsetIdx = std::floor( _offset );

	const unsigned int 
		Size = _noise.size(),
		NoiseIdx = static_cast<unsigned int>(offsetIdx) % Size;

	double 
		a = _noise[NoiseIdx],
		b = _noise[(NoiseIdx + 1) % Size],
		frac = _offset - offsetIdx;
	  
	_value = lrp( frac, a, b);
	return _value;
}

void Flicker::_generate()
{
	int seed = _seed > 0 ? _seed : std::abs(ci::Rand::randInt());
	PerlinNoise generator( _octaves, _falloff, _scale, seed );
	fill( _noise, generator, 0, true );
}

}} // end namespace core::util