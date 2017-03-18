//
//  ContourSimplification.hpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/26/17.
//
//

#ifndef ContourSimplification_hpp
#define ContourSimplification_hpp

#include <cinder/PolyLine.h>

#include "MathHelpers.hpp"
#include "LineSegment.hpp"

using namespace ci;
using namespace std;

namespace core {

	/**
		Ramer-Douglas-Peucker simplification
		@param in the path to optimize
		@param out the optimized path
		@param threshold the maximum allowed linear deviation from the optimized path to the original path
	 */

	void simplify( const vector<vec2> &in, vector<vec2> &out, float threshold );

	PolyLine2f simplify( const PolyLine2f &contour, float threshold );

	/**
		@brief Remove vertices which are closer than @a threshold to the previous vertex
	 */

	void dedup( const vector<vec2> &in, vector<vec2> &out, float threshold );

	/**
		Ramer-Douglas-Peucker simplification with deduplication - this is probably unuecessary.
		@param in the path to optimize
		@param out the optimized path
		@param threshold the maximum allowed linear deviation from the optimized path to the original path
	 */
	void simplify_dedup( const vector<vec2> &in, vector<vec2> &out, float threshold );
}

#endif /* ContourSimplification_hpp */
