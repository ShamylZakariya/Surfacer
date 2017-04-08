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
	namespace util {

		/**
		 Ramer-Douglas-Peucker simplification
		 @param in the path to optimize
		 @param out the optimized path
		 @param threshold the maximum allowed linear deviation from the optimized path to the original path
		 */

		template< class T, glm::precision P >
		void simplify( const vector<glm::tvec2<T,P>> &in, vector<glm::tvec2<T,P>> &out, T threshold ) {
			if ( in.size() <= 2 ) {
				out = in;
				return;
			}

			//
			//	Find the vertex farthest from the line defined by the start and and of the path
			//

			T maxDist = 0;
			std::size_t maxDistIndex = 0;
			line_segment<T,P> line( in.front(), in.back() );

			for ( auto it(in.begin()+1),end(in.end()); it != end; ++it ) {
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

				vector<glm::tvec2<T,P>> left( maxDistIndex+1 ),
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

		template< class T, class P>
		PolyLineT<T> simplify( const PolyLineT<T> &contour, P threshold ) {
			vector<T> result;
			simplify(contour.getPoints(), result, threshold);

			PolyLineT<T> pl(result);
			pl.setClosed();
			return pl;
		}

		/**
		 @brief Remove vertices which are closer than @a threshold to the previous vertex
		 */

		template< class T, glm::precision P >
		void dedup( const vector<glm::tvec2<T,P>> &in, vector<glm::tvec2<T,P>> &out, T threshold ) {
			T threshold2 = threshold * threshold;
			out.push_back( in.front() );
			for ( auto it(in.begin()+1),end(in.end()); it != end; ++it ) {
				if (distanceSquared(*it, out.back()) > threshold2) {
					out.push_back(*it);
				}
			}
		}

		/**
		 Ramer-Douglas-Peucker simplification with deduplication - this is probably unnecessary.
		 @param in the path to optimize
		 @param out the optimized path
		 @param threshold the maximum allowed linear deviation from the optimized path to the original path
		 */
		template< class T, glm::precision P >
		void simplify_dedup( const vector<glm::tvec2<T,P>> &in, vector<glm::tvec2<T,P>> &out, T threshold ) {
			vector<glm::tvec2<T,P>> collector;
			simplify( in, collector, threshold );
			dedup( collector, out, 1 );
		}
	}
}

#endif /* ContourSimplification_hpp */
