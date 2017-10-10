#pragma once

/*
 *  PerlinNoise.h
 *  Terrain
 *
 *  Created by Shamyl Zakariya on 6/28/09.
 *  Copyright 2009 Shamyl Zakariya. All rights reserved.
 *
 */

#include <algorithm>

#include "Common.hpp"
#include "MathHelpers.hpp"

#include <cinder/Channel.h>

namespace core { namespace util {
	
	/**
	 @class PerlinNoise
	 Implements a simple PerlinNoise algorithm with octaves, persistence and freqency scaling
	 */
	class PerlinNoise {
	protected:
		
		struct rng {
			size_t _seed, _lo, _hi;
			
			rng( void ):
			_seed( 4357U ),
			_lo( 4357U ),
			_hi( 4357U )
			{}
			
			rng( const rng &c ):
			_seed( c._seed ),
			_lo( c._lo ),
			_hi( c._hi )
			{}
			
			rng & operator = ( const rng &rhs ) {
				_seed = rhs._seed;
				_lo = rhs._lo;
				_hi = rhs._hi;
				return *this;
			}
			
			void seed( size_t s ) {
				_seed = _lo = s;
				_hi = ~s;
			}
			
			int next( void ) {
				_hi = ( _hi << 16 ) + ( _hi >> 16 );
				_hi += _lo;
				_lo += _hi;
				
				return static_cast<int>(_hi);
			}
		};
		
		size_t _octaves;
		int _seed;
		rng _rng;
		double _persistence, _persistenceMax, _scale;
		double *_octFrequency, *_octPersistence;
		const int *_p;
		int _offsetX, _offsetY, _offsetZ;
		bool _initialized;
		
	public:
		
		PerlinNoise( size_t octaves = 4, double falloff = 0.5, double scale = 1, int seed = 123 );
		PerlinNoise( const PerlinNoise & );
		
		~PerlinNoise( void ) {
			delete [] _octFrequency;
			delete [] _octPersistence;
		}
		
		PerlinNoise &operator = ( const PerlinNoise & );
		
		double noiseUnit( double x, double y = 1, double z = 1) {
			return (noise(x,y,z)+1.0) * 0.5;
		}
		
		// return noise i range -1 -> +1
		double noise( double x, double y = 1, double z = 1) {
			if (!_initialized) init();
			
			double s = 0;
			x*= _scale;
			y*= _scale;
			z*= _scale;
			
			for ( size_t i = 0; i < _octaves; i++ )
			{
				double freq = _octFrequency[i],
				pers = _octPersistence[i];
				
				s += n( (x+_offsetX)*freq,
					   (y+_offsetY)*freq,
					   (z+_offsetZ)*freq ) * pers;
			}
			
			return s*_persistenceMax;
		}
		
		void setOctaves( size_t oct ) {
			oct = std::max( oct, size_t(1) );
			if ( oct != _octaves ) {
				_octaves = oct;
				octFreqPers();
			}
		}
		
		size_t octaves( void ) const { return _octaves; }
		
		void setScale( double scale ) { _scale = scale; }
		double scale( void ) const { return _scale; }
		
		void setFalloff( double pers ) {
			pers = saturate(pers);
			if ( std::abs( pers - _persistence ) > 0 ) {
				_persistence = pers;
				octFreqPers();
			}
		}
		
		double falloff( void ) const { return _persistence; }
		
		void setSeed( size_t s ) {
			if ( !_initialized ) init();
			_rng.seed( s );
			_seed = static_cast<int>(s);
			seedOffset();
		}
		
		size_t seed( void ) {
			if ( !_initialized ) init();
			return _seed;
		}
		
	protected:
		
		void octFreqPers( void ) {
			delete [] _octFrequency;
			delete [] _octPersistence;
			
			_octFrequency = new double[_octaves];
			_octPersistence = new double[_octaves];
			
			_persistenceMax = 0;
			for ( size_t i = 0; i < _octaves; i++ ) {
				double freq = std::pow( 2.0, int(i) );
				double pers = std::pow( _persistence, int(i) );
				
				_persistenceMax += pers;
				_octFrequency[i] = freq;
				_octPersistence[i] = pers;
			}
			
			_persistenceMax = 1.0f / _persistenceMax;
		}
		
		void seedOffset( void ) {
			_offsetX = _rng.next();
			_offsetY = _rng.next();
			_offsetZ = _rng.next();
		}
		
		void init( void ) {
			_rng.seed( _seed );
			seedOffset();
			octFreqPers();
			_initialized = true;
		}
		
		double n( double x, double y, double z ) const {
			int X = int( std::floor( x )) & 255,
			Y = int( std::floor( y )) & 255,
			Z = int( std::floor( z )) & 255;
			
			x -= std::floor( x );
			y -= std::floor( y );
			z -= std::floor( z );
			
			double u = fade( x ),
			v = fade( y ),
			w = fade( z );
			
			// local aliasing
			const int *p = this->_p;
			
			int A  = p[X  ]+Y,
			AA = p[A]+Z,
			AB = p[A+1]+Z,
			B  = p[X+1]+Y,
			BA = p[B]+Z,
			BB = p[B+1]+Z;
			
			return lerp(w, lerp(v, lerp(u,	grad(p[AA  ], x  , y  , z   ),
			                                grad(p[BA  ], x-1, y  , z   )),
			                       lerp(u,  grad(p[AB  ], x  , y-1, z   ),
			                                grad(p[BB  ], x-1, y-1, z   ))),
			               lerp(v, lerp(u,  grad(p[AA+1], x  , y  , z-1 ),
			                                grad(p[BA+1], x-1, y  , z-1 )),
			                       lerp(u,  grad(p[AB+1], x  , y-1, z-1 ),
			                                grad(p[BB+1], x-1, y-1, z-1 ))));
		}
		
		double fade( double t ) const {
			return t * t * t * (t * (t * 6 - 15) + 10);
		}
		
		double lerp( double t, double a, double b ) const {
			return a + t * (b - a);
		}
		
		double grad( int hash, double x, double y, double z ) const {
			int h = hash & 15;
			double u = h < 8 ? x : y;
			double v = h < 4 ? y : h==12||h==14 ? x : z;
			return ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
		}
		
	};
	
	
	/**
	 create a ci::Channel with the contents of a PerlinNoise instance
	 @a pn The PerlinNoise instance
	 @a size The size of the Channel to create and fill
	 @a offset An offset to add to each lookup
	 @a normalized If true values will be in [-1,+1] range, otherwise they'll be in [0,1] range
	 */
	ci::Channel32f fill( PerlinNoise &pn, const ivec2 &size, const ivec2 &offset = ivec2(0,0), bool normalized = false );
	
	void fill( ci::Channel32f &channel, PerlinNoise &pn, const ivec2 &offset = ivec2( 0,0 ), bool normalized = false );
	
	void fill( std::vector< double > &data, PerlinNoise &pn, const int offset = 0, bool normalized = false );
	
	
}} // end namespace core::util
