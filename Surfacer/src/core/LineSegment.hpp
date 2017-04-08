//
//  LineSegment.hpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/26/17.
//
//

#ifndef LineSegment_h
#define LineSegment_h

#include <cinder/CinderMath.h>
#include <cinder/Area.h>
#include <chipmunk/chipmunk.h>
#include <cinder/gl/scoped.h>

#include "MathHelpers.hpp"
#include "Signals.hpp"

namespace core {

	template< class T, glm::precision P >
	struct line_segment
	{
		glm::tvec2<T,P> a,b,dir;
		T length;

		line_segment( const glm::tvec2<T,P> &A, const glm::tvec2<T,P> &B ):
		a(A),
		b(B),
		dir(B-A)
		{
			length = glm::length(dir);
			dir.x /= length;
			dir.y /= length;
		}

		glm::tvec2<T,P> lrp( T distance ) const
		{
			return a + (distance * dir);
		}

		/**
		 get distance of a point from this line segment
		 */
		T distance( const glm::tvec2<T,P> &point ) const
		{
			const glm::tvec2<T,P> toPoint = point - a;
			const T projLength = dot(toPoint, dir );

			// early exit condition, we'll have to get distance from endpoints
			if ( projLength < 0 )
			{
				return glm::length(a - point );
			}
			else if ( projLength > length )
			{
				return glm::length( b - point );
			}

			// compute distance from point to the closest projection point on the line segment
			const glm::tvec2<T,P> projOnSegment = a + (projLength * dir);
			return glm::distance( point, projOnSegment );
		}

		glm::tvec2<T,P> snapToLine( const glm::tvec2<T,P> &point ) const
		{
			const glm::tvec2<T,P> toPoint = point - a;
			const T projLength = dot( toPoint, dir );

			// early exit condition, we'll have to get distance from endpoints
			if ( projLength < 0 )
			{
				return a;
			}
			else if ( projLength > length )
			{
				return b;
			}

			return a + (projLength * dir);
		}

		pair<bool,glm::tvec2<T,P>> intersects( const line_segment &other_line, bool bounded = true ) const
		{
			// early exit test for parallel lines
			const T Epsilon = 1e-5;
			if (abs(dot(dir, other_line.dir)) > 1 - Epsilon) {
				return pair<bool,glm::tvec2<T,P>>(false, glm::tvec2<T,P>());
			}

			// cribbed from http://jeffreythompson.org/collision-detection/line-line.php

			const T x1 = a.x;
			const T y1 = a.y;
			const T x2 = b.x;
			const T y2 = b.y;

			const T x3 = other_line.a.x;
			const T y3 = other_line.a.y;
			const T x4 = other_line.b.x;
			const T y4 = other_line.b.y;

			const T u_a = ((x4 - x3) * (y1 - y3) - (y4 - y3) * (x1 - x3)) / ((y4 - y3) * (x2 - x1) - (x4 - x3) * (y2 - y1));
			const T u_b = ((x2 - x1) * (y1 - y3) - (y2 - y1) * (x1 - x3)) / ((y4 - y3) * (x2 - x1) - (x4 - x3) * (y2 - y1));

			if (!bounded || (u_a >= 0 && u_a <=1 && u_b >= 0 && u_b <= 1)) {
				return pair<bool,glm::tvec2<T,P>>(true, glm::tvec2<T,P>(x1 + u_a * (x2 - x1), y1 + u_a * (y2 - y1)));
			}

			return pair<bool,glm::tvec2<T,P>>(false, glm::tvec2<T,P>());
		}
		
	};

}

#endif /* LineSegment_h */
