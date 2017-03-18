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

using namespace ci;
using namespace std;


namespace core {

	struct line_segment
	{
		vec2 a,b,dir;
		float length;

		line_segment( const vec2 &A, const vec2 &B ):
		a(A),
		b(B),
		dir(B-A)
		{
			length = ::length(dir);
			dir.x /= length;
			dir.y /= length;
		}

		vec2 lrp( float distance ) const
		{
			return a + (distance * dir);
		}

		/**
		 get distance of a point from this line segment
		 */
		float distance( const vec2 &point ) const
		{
			vec2 toPoint = point - a;
			float projLength = dot(toPoint, dir );

			// early exit condition, we'll have to get distance from endpoints
			if ( projLength < 0 )
			{
				return ::length(a - point );
			}
			else if ( projLength > length )
			{
				return ::length( b - point );
			}

			// compute distance from point to the closest projection point on the line segment
			vec2 projOnSegment = a + (projLength * dir);
			return ::distance( point, projOnSegment );
		}

		vec2 snapToLine( const vec2 &point ) const
		{
			vec2 toPoint = point - a;
			float projLength = dot( toPoint, dir );

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

		pair<bool,vec2> intersects( const line_segment &other_line, bool bounded = true ) const
		{
			// early exit test for parallel lines
			const float Epsilon = 1e-5;
			if (abs(dot(dir, other_line.dir)) > 1 - Epsilon) {
				return pair<bool,vec2>(false, vec2());
			}

			// cribbed from http://jeffreythompson.org/collision-detection/line-line.php

			float x1 = a.x;
			float y1 = a.y;
			float x2 = b.x;
			float y2 = b.y;

			float x3 = other_line.a.x;
			float y3 = other_line.a.y;
			float x4 = other_line.b.x;
			float y4 = other_line.b.y;

			float u_a = ((x4 - x3) * (y1 - y3) - (y4 - y3) * (x1 - x3)) / ((y4 - y3) * (x2 - x1) - (x4 - x3) * (y2 - y1));
			float u_b = ((x2 - x1) * (y1 - y3) - (y2 - y1) * (x1 - x3)) / ((y4 - y3) * (x2 - x1) - (x4 - x3) * (y2 - y1));

			if (!bounded || (u_a >= 0 && u_a <=1 && u_b >= 0 && u_b <= 1)) {
				return pair<bool,vec2>(true, vec2(x1 + u_a * (x2 - x1), y1 + u_a * (y2 - y1)));
			}

			return pair<bool,vec2>(false, vec2());
		}
		
	};

}

#endif /* LineSegment_h */
