//
//  ContourSimplification.cpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/26/17.
//
//

#include "ContourSimplification.hpp"

namespace core {

	void simplify( const vector<vec2> &in, vector<vec2> &out, float threshold ) {
		if ( in.size() <= 2 ) {
			out = in;
			return;
		}

		//
		//	Find the vertex farthest from the line defined by the start and and of the path
		//

		float maxDist = 0;
		std::size_t maxDistIndex = 0;
		line_segment line( in.front(), in.back() );

		for ( vector<vec2>::const_iterator it(in.begin()+1),end(in.end()); it != end; ++it ) {
			float dist = line.distance( *it );
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

		if ( maxDist > threshold ) {

			//
			//	Partition 'in' into left and right subvectors, and optimize them
			//

			vector<vec2> left( maxDistIndex+1 ),
			right( in.size() - maxDistIndex ),
			leftSimplified,
			rightSimplified;

			std::copy( in.begin(), in.begin() + maxDistIndex + 1, left.begin() );
			std::copy( in.begin() + maxDistIndex, in.end(), right.begin() );

			simplify(left, leftSimplified, threshold );
			simplify(right, rightSimplified, threshold );

			//
			//	Stitch optimized left and right into 'out'
			//

			out.resize( leftSimplified.size() + rightSimplified.size() - 1 );
			std::copy( leftSimplified.begin(), leftSimplified.end(), out.begin());
			std::copy( rightSimplified.begin() + 1, rightSimplified.end(), out.begin() + leftSimplified.size() );
		}
		else {
			out.resize(2);
			out.front() = line.a;
			out.back() = line.b;
		}
	}

	PolyLine2f simplify( const PolyLine2f &contour, float threshold ) {
		vector<vec2> result;
		simplify(contour.getPoints(), result, threshold);

		PolyLine2f pl(result);
		pl.setClosed();
		return pl;
	}

	/**
		@brief Remove vertices which are closer than @a threshold to the previous vertex
	 */
	void dedup( const vector<vec2> &in, vector<vec2> &out, float threshold ) {
		float threshold2 = threshold * threshold;
		out.push_back( in.front() );
		for ( vector<vec2>::const_iterator it(in.begin()+1),end(in.end()); it != end; ++it ) {
			if (distanceSquared(*it, out.back()) > threshold2) {
				out.push_back(*it);
			}
		}
	}

	/**
		Ramer-Douglas-Peucker simplification with deduplication - this is probably unuecessary.
		@param in the path to optimize
		@param out the optimized path
		@param threshold the maximum allowed linear deviation from the optimized path to the original path
	 */
	void simplify_dedup( const vector<vec2> &in, vector<vec2> &out, float threshold ) {
		vector<vec2> collector;
		simplify( in, collector, threshold );
		dedup( collector, out, 1 );
	}
	
}
