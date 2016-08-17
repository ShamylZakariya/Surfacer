#pragma once

//
//  LineChunking.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/3/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Common.h"

namespace core { namespace util {

	struct chunked_line_sort
	{
		Vec2r _origin;
		chunked_line_sort( const Vec2r &origin ):_origin(origin){}

		inline bool operator()( const Vec2r &a, const Vec2r &b ) const
		{
			return ( _origin.distanceSquared(a) < _origin.distanceSquared(b) );
		}

	};
	
	
	inline void ChunkLine( const Vec2r &start, const Vec2r &end, const Vec2i chunkSize, Vec2rVec &points )
	{
		Vec2r dir = normalize( end - start );
		bool chunkedX = false, chunkedY = false;

		points.push_back(start);

		if ( chunkSize.x > 0 )
		{
			if ( dir.x > Epsilon )
			{
				real slopeX = dir.y / dir.x,
					 slopeXB = start.y - slopeX * start.x;

				int firstXChunkIndex = int( std::ceil( start.x / chunkSize.x )),
					lastXChunkIndex = int( std::floor( end.x / chunkSize.x ));
			
				for ( int chunkXIndex = firstXChunkIndex; chunkXIndex <= lastXChunkIndex; chunkXIndex++ )
				{
					real xChunkX = chunkXIndex * chunkSize.x,
						 xChunkY = slopeX * xChunkX;
					
					points.push_back( Vec2r( xChunkX, slopeXB + xChunkY ) );
					chunkedX = true;
				}				
			}
			else if ( dir.x < -Epsilon )
			{
				real slopeX = dir.y / dir.x,
					 slopeXB = start.y - slopeX * start.x;

				int firstXChunkIndex = int( std::floor( start.x / chunkSize.x )),
					lastXChunkIndex = int( std::ceil( end.x / chunkSize.x ));
			
				for ( int chunkXIndex = firstXChunkIndex; chunkXIndex >= lastXChunkIndex; chunkXIndex--)
				{
					real xChunkX = chunkXIndex * chunkSize.x,
						 xChunkY = slopeX * xChunkX;
					
					points.push_back( Vec2r( xChunkX, slopeXB + xChunkY ) );
					chunkedX = true;
				}				
			}
		}


		if ( chunkSize.y > 0 )
		{
			if ( dir.y > Epsilon )
			{
				real slopeY = dir.x / dir.y,
					 slopeYB = start.x - slopeY * start.y;

				int firstYChunkIndex = int( std::ceil( start.y / chunkSize.y )),
					lastYChunkIndex = int( std::floor( end.y / chunkSize.y ));
				
				for ( int chunkYIndex = firstYChunkIndex; chunkYIndex <= lastYChunkIndex; chunkYIndex++ )
				{
					real yChunkY = chunkYIndex * chunkSize.y,
						 yChunkX = slopeY * yChunkY;
						 
					points.push_back( Vec2r( slopeYB + yChunkX, yChunkY ));
					chunkedY = true;
				}
			}
			else if ( dir.y < -Epsilon )
			{
				real slopeY = dir.x / dir.y,
					 slopeYB = start.x - slopeY * start.y;

				int firstYChunkIndex = int( std::floor( start.y / chunkSize.y )),
					lastYChunkIndex = int( std::ceil( end.y / chunkSize.y ));
				
				for ( int chunkYIndex = firstYChunkIndex; chunkYIndex >= lastYChunkIndex; chunkYIndex-- )
				{
					real yChunkY = chunkYIndex * chunkSize.y,
						 yChunkX = slopeY * yChunkY;
						 
					points.push_back( Vec2r( slopeYB + yChunkX, yChunkY ));
					chunkedY = true;
				}
			}
		}


		points.push_back(end);
		
		// if both x & y were chunked, we need to sort
		if ( chunkedX && chunkedY )
		{
			std::sort( points.begin(), points.end(), chunked_line_sort(start));
		}
	}

}} // end namespace core::util