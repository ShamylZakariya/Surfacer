#pragma once

/*
 *  PathInterpolator.h
 *  Interpolatrix
 *
 *  Created by Shamyl Zakariya on 4/16/05.
 *  Copyright 2005 Shamyl Zakariya. All rights reserved.
 *
 */

#include <algorithm>
#include <vector>
#include "Common.h"

namespace core { namespace util {

/**
	@class PathInterpolator
	@ingroup util
	@brief Performs a simple linear interpolation across a set of points. Does
	not attempt to be clever and normalize segment lengths.
*/
template <class T, class REAL >
class PathInterpolator
{
	public:
	
		typedef std::vector< T > Path;
		typedef REAL ScalarType;
	
	public:
	
		PathInterpolator( void ):
			_closed(false), 
			_totalPathLength(0)
		{}
		
		PathInterpolator( const PathInterpolator &c ):
			_path( c._path ),
			_closed( c._closed ),
			_totalPathLength( c._totalPathLength ),
			_segmentLengths( c._segmentLengths ),
			_pathTotalLengths( c._pathTotalLengths )
		{}
		
		PathInterpolator &operator = ( const PathInterpolator &c )
		{
			_path = c._path;
			_closed = c._closed;
			_totalPathLength = c._totalPathLength;
			_segmentLengths = c._segmentLengths;
			_pathTotalLengths = c._pathTotalLengths;

			return *this;
		}
			
			
		~PathInterpolator( void ){}
		
		void clear()
		{
			_path.clear();
			_pathTotalLengths.clear();
			_segmentLengths.clear();
			_totalPathLength = REAL(0);
		}
		
		/**
			If @a shouldClose is true, the path will be treated as a closed loop, where
			the final element is equal to the first. If the final element is not equal to
			the first, a copy of the first will be appended.			
		*/
		void setPath( const Path &path, bool shouldClose )
		{
			clear();
			
			_path = path;
			_closed = shouldClose;

			if ( _closed )
			{
				if ( _path.front() != _path.back() )
				{
					_path.push_back( _path.front() );
				}
			}
						
			//
			// Sum up
			//
			
			int N = _path.size() - 1;
			
			_pathTotalLengths.push_back(0);
			
			for ( int i = 0; i < N; i++ )
			{
				REAL l = distance( _path[i], _path[i+1] );

				_segmentLengths.push_back( l );
				_totalPathLength += l;
				_pathTotalLengths.push_back( _totalPathLength );
			}
			
			if ( _closed )
			{
				_pathTotalLengths.push_back( _totalPathLength );
			}
		}
		
		const Path &path( void ) const
		{
			return _path;
		}
		
		bool closed( void ) const
		{
			return _closed;
		}
		
		REAL length( void ) const
		{
			return _totalPathLength;
		}
		
		/**
			@brief Export this path interpolated as a bezier spline
			@param samples The number of points in the target path
			@param tension The spline tension. 0 results in straight lines, 1 results in smooth interpolated curves
			@param finalPath The generated path
		*/
		void spline( int samples, REAL tension, PathInterpolator<T,REAL> &finalPath )
		{
			if ( tension <= Epsilon )
			{
				finalPath = *this;
				return;
			}

			tension = std::min( std::max( tension, REAL(0)), REAL(1));
			
			REAL inc = REAL(1) / REAL(samples), 
			     position = 0;
				 
			Path path;

			for ( int i = 0; i < samples; i++ )
			{
				path.push_back( pointOnPath( position, tension ) );	
				position += inc;			
			}
			
			finalPath.setPath( path, _closed );			
		}
		
		
		/**
			@brief Get a point on the path
			@param position The position on the path
			
			If the path is 'closed' you can pass values greater than one ( which will loop ) and values
			less than zero, which will result in reverse iteration.
			
			

			@return a point interpolated along the path corresponding to the fractional value.
			E.g., if distane == 0, the first element of the path will be returned, if distance
			== 1, the final point will be returned. If the path is closed, values > 1 will result
			in looping, starting from the beginning. If the path is open, values greater than one
			will be capped to the final value.
		*/
		T pointOnPath( REAL position, REAL tension = 0, T *dir = NULL ) const
		{
			//
			//	If the path is not closed, cap to endpoints
			//
			
			if ( !_closed )
			{
				if ( position >= REAL(1) - Epsilon )
				{
					return _path.back();
				}		
				else if ( position <= Epsilon )
				{
					return _path.front();
				}
			}
			
			//
			// if closed loop values outside of range
			//
			
			if ( _closed )
			{
				if ( position > REAL(1) ) position = position - floorf( position );
				else if ( position < REAL(0) ) position = REAL(1) + position - ceilf( position );
			}
			else
			{
				position = std::min( std::max( position, REAL(0) ), REAL(1) );
			}

			//
			// Now get the segment we're interested in
			//
			
			REAL segmentDistance = REAL(0);
			int segmentIndex = segmentForDistance( position * _totalPathLength, segmentDistance );
						
			if ( segmentIndex >= 0 )
			{
				int nPoints( _path.size());
				
				if ( tension <= Epsilon )
				{
					T a( _path[ segmentIndex ] ),
					  b( _path[ (segmentIndex + 1) % nPoints ]);
					  
					if ( dir )
					{
						*dir = normalize(b-a);
					}
					  					  
					return ( a * ( REAL(1) - segmentDistance )) + ( b * segmentDistance );
				}
				else
				{
					REAL u = segmentDistance,
					      uSquared = u*u,
						  uCubed = uSquared*u;
				
					REAL c0 = tension * (-uCubed + REAL(2) * uSquared - u),
					      c1 = (REAL(2) - tension) * uCubed + (tension - REAL(3)) * uSquared + REAL(1),
					      c2 = (tension - REAL(2)) * uCubed + (REAL(3) - REAL(2) * tension) * uSquared + tension * u,
					      c3 = tension * (uCubed - uSquared);
				
					T vBegin, 
					  vEnd,
					  vOne( _path[ segmentIndex ] ),
					  vTwo( _path[ (segmentIndex+1) % nPoints ] );
					
					if ( segmentIndex == 0 )
					{
						if ( _closed )
						{
							vBegin = _path[ nPoints - 1 ];
						}
						else
						{
							vBegin = vOne * REAL(2) - vTwo;
						}
					}
					else
					{
						vBegin = _path[ segmentIndex - 1 ];
					}
					
					if ( segmentIndex == nPoints - 2 )
					{
						if ( _closed )
						{
							vEnd = _path[0];
						}
						else
						{
							vEnd = vTwo * REAL(2) - vOne;
						}
					}
					else
					{
						vEnd = _path[ segmentIndex + 2 ];
					}
					
					T point = (vBegin*c0) + (vOne* c1) + (vTwo * c2) + (vEnd * c3);				
					
					if ( dir )
					{
						T p2 = pointOnPath( position + REAL(0.1), tension );
						*dir = normalize( p2 - point );
					}
					
					return point;
				}
			}

			//
			// Disaster!
			//
			
			return T();
		}

	private:
	
		/**
			@brief Get the segment index for a distance along the path
		*/
		int segmentForDistance( REAL totalDistance, REAL &distanceFraction ) const
		{
			int segmentIndex = -1, 
			    nSegments = _segmentLengths.size(),
			    first = 0, last = nSegments - 1;

			//
			// Binary search to find segment
			// If distance is 0.77 and we have a segment from 0.7 to 0.8, we want that segment,
			// and return distance fraction of 0.7 ( 0.77 - 0.7 ) / ( 0.8 - 0.7 )
			//

			while( first <= last )
			{
				int mid = ( first + last ) / 2;
				REAL min = _pathTotalLengths[mid],
				     max = _pathTotalLengths[mid + 1];

				if ( max < totalDistance ) // repeat search in top half
				{
					first = mid + 1;
				}
				else if ( min > totalDistance )
				{
					last = mid - 1;
				}
				else
				{
					segmentIndex = mid;
					break;
				}
			}

			if ( segmentIndex >= 0 )
			{
			
				//
				// Calculate distance fraction
				//
				
				REAL min = _pathTotalLengths[segmentIndex],
				     max = _pathTotalLengths[segmentIndex + 1];
				
				distanceFraction = ( totalDistance - min ) / ( max - min );
				return segmentIndex;
			}
			
			return -1;
		}
		
		Path _path;
		bool _closed;
		REAL _totalPathLength;
		std::vector< REAL > _segmentLengths, _pathTotalLengths;

};

typedef PathInterpolator< real,real > ScalarInterpolator;
typedef PathInterpolator< Vec2,real > Vec2rInterpolator;
typedef PathInterpolator< Vec3,real > Vec3rInterpolator;

}} // end namespace core::util

