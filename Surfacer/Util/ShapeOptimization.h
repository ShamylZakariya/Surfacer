#pragma once

/*
 *  ShapeOptimization.h
 *  ConvexDecomposition2
 *
 *  Created by Shamyl Zakariya on 2/21/11.
 *  Copyright 2011 Shamyl Zakariya. All rights reserved.
 *
 */

#include <cinder/Vector.h>
#include "Common.h"
#include "LineSegment.h"


namespace core { namespace util { namespace shape_optimization
{	
	/**
		My (Shamyl Zakariya) crude-assed line simplifier.

		Remove vertices that aren't necessary because they're collinear with the the line defined by the previous and next vertex
		within the error margin of @a angleThreshold.		
		Also discards vertices that are closer than snapThreshold distance from eachother.
	*/
	inline
	void szSimplify(
		Vec2rVec &in, 
		Vec2rVec &out, 
		real angleThreshold, 
		real snapThreshold )
	{
		using namespace ci;

		real snapThreshold2 = snapThreshold * snapThreshold,
		      cosAngleThreshold = cosf( angleThreshold );

		int idx = 0;
		Vec2r z,a,b,c;
		a = in.back();
		z = in[in.size()-2];
		
		for ( Vec2rVec::iterator it( in.begin() ), end( in.end() ); it != end; ++it )
		{
			b = *it;

			Vec2rVec::iterator nextIt = it;
			nextIt++;
			if ( nextIt == end ) nextIt = in.begin();
			c = *nextIt;
			
			Vec2r ba = b - a,
				  cb = c - b,
				  az = a - z;
				  
			ba.normalize();
			cb.normalize();
			az.normalize();
			
			//
			//	Take dot product, and check if we pass the corner, noise and distance thresholds.
			//
	
			real d = az.dot( cb );
			bool passesAngleThreshold = d < cosAngleThreshold,
				 passesDistanceThreshold = b.distanceSquared( out.empty() ? a : out.back()) > snapThreshold2;

			if ( passesAngleThreshold && passesDistanceThreshold )
			{
				out.push_back( b );
			}

			idx++;	
			z = a;		
			a = b;
		}
	}
	
	/**
		Ramer-Douglas-Peucker simplification
		@param in the path to optimize
		@param out the optimized path
		@param threshold the maximum allowed linear deviation from the optimized path to the original path
	*/
	
	inline
	void rdpSimplify( const Vec2rVec &in, Vec2rVec &out, real threshold )
	{
		if ( in.size() <= 2 ) 
		{
			out = in;
			return;
		}
		
		//
		//	Find the vertex farthest from the line defined by the start and and of the path
		//
		
		real maxDist = 0;
		std::size_t maxDistIndex = 0;		
		line_segment line( in.front(), in.back() );

		for ( Vec2rVec::const_iterator it(in.begin()+1),end(in.end()); it != end; ++it )
		{
			real dist = line.distance( *it );
			if ( dist > maxDist )
			{
				maxDist = dist;
				maxDistIndex = it - in.begin();
			}
		}
		
		//
		//	If the farthest vertex is greater than our threshold, we need to
		//	partition and optimize left and right separately
		//
		
		if ( maxDist > threshold )
		{
			//
			//	Partition 'in' into left and right subvectors, and optimize them
			//


			Vec2rVec left( maxDistIndex+1 ),
					 right( in.size() - maxDistIndex ),
					 leftSimplified,
					 rightSimplified;

			std::copy( in.begin(), in.begin() + maxDistIndex + 1, left.begin() );
			std::copy( in.begin() + maxDistIndex, in.end(), right.begin() );
			
			rdpSimplify(left, leftSimplified, threshold );
			rdpSimplify(right, rightSimplified, threshold );
			
			//
			//	Stitch optimized left and right into 'out'
			//
			
			out.resize( leftSimplified.size() + rightSimplified.size() - 1 );
			std::copy( leftSimplified.begin(), leftSimplified.end(), out.begin());
			std::copy( rightSimplified.begin() + 1, rightSimplified.end(), out.begin() + leftSimplified.size() );
		}
		else
		{
			out.resize(2);
			out.front() = line.a;
			out.back() = line.b;
		}
	}

	/**
		@brief Remove vertices which are closer than @a threshold to the previous vertex
	*/
	inline
	void dedup( const Vec2rVec &in, Vec2rVec &out, real threshold )
	{
		real threshold2 = threshold * threshold;
		out.push_back( in.front() );
		for ( Vec2rVec::const_iterator it(in.begin()+1),end(in.end()); it != end; ++it )
		{
			if ( it->distanceSquared( out.back()) > threshold2 )
			{
				out.push_back( *it );
			}
		}
	}			
	
	/**
		Ramer-Douglas-Peucker simplification with deduplication - this is probably unuecessary.
		@param in the path to optimize
		@param out the optimized path
		@param threshold the maximum allowed linear deviation from the optimized path to the original path
	*/
	inline void rdpSimplifyDedup( const Vec2rVec &in, Vec2rVec &out, real threshold )
	{
		Vec2rVec collector;
		rdpSimplify( in, collector, threshold );
		dedup( collector, out, 1 );
	}
	
}}} // end namespace core::util::shape_optimization
