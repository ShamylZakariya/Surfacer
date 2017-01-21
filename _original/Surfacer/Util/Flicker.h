#pragma once

//
//  Flicker.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/16/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Common.h"

namespace core { namespace util {

class Flicker
{
	public:
	
		Flicker( double rate = 16, int octaves = 8, double falloff = 0.5, double scale = 80, int seed = 0, unsigned int bufferSize = 128 );
		
		void setOctaves( int oct ) { _octaves = std::max(oct,1); _generate(); }
		int octaves() const { return _octaves; }

		void setFalloff( double falloff ) { _falloff = saturate(falloff); _generate(); }
		double falloff() const { return _falloff; }

		void setScale( double scale ) { _scale = std::max(scale, 0.01); _generate(); }
		double scale() const { return _scale; }

		// a seed of zero causes Flicker to use a random number
		void setSeed( int seed ) { _seed = seed; _generate(); }
		int seed() const { return _seed; }
		
		void setRate( double r ) { _offsetRate = std::abs( r ); }
		double rate() const { return _offsetRate; }
		
		void setBufferSize( unsigned int bs );
		int bufferSize() const { return _noise.size(); }

		// compute new flicker for elapsed time
		double update( seconds_t elapsedTime );
		
		// get current flicker value, as computed in last call to update(), returns values from -1 to +1
		real value() const { return _value; }

	private:
	
		void _generate();

	private:
	
		int _octaves, _seed;
		double _falloff, _scale;
		std::vector< double > _noise;
		double _value, _offset, _offsetRate;

};




}} // end namespace core::util