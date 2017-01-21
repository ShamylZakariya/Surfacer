#pragma once

//
//  LineSegment.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 6/2/11.
//  Copyright 2011 Shamyl Zakariya. All rights reserved.
//

#include "Common.h"

namespace core { namespace util {

struct line_segment 
{
	Vec2 a,b,dir;
	real length;
	
	line_segment( const Vec2 &A, const Vec2 &B ):
		a(A),
		b(B),
		dir(B-A)
	{			
		length = dir.length();
		dir.x /= length;
		dir.y /= length;
	}
	
	Vec2 lrp( real distance ) const
	{
		return a + (distance * dir);
	}
	
	/**
		get distance of a point from this line segment
	*/
	real distance( const Vec2 &point ) const
	{
		Vec2 toPoint = point - a;
		real projLength = toPoint.dot( dir );
		
		// early exit condition, we'll have to get distance from endpoints
		if ( projLength < 0 )
		{
			return point.distance( a );
		}
		else if ( projLength > length ) 
		{
			return point.distance( b );
		}
				
		// compute distance from point to the closest projection point on the line segment
		Vec2 projOnSegment = a + (projLength * dir);
		return point.distance( projOnSegment );
	}
	
	Vec2 snapToLine( const Vec2 &point ) const
	{
		Vec2 toPoint = point - a;
		real projLength = toPoint.dot( dir );
		
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
	
	bool intersects( const Vec2 &c, const Vec2 &d ) const
	{
		/*
			adapted from processing wiki
			http://wiki.processing.org/w/Line-Line_intersection
		*/
	
		real bx = b.x - a.x; 
		real by = b.y - a.y; 
		real dx = d.x - c.x; 
		real dy = d.y - c.y;
		real b_dot_d_perp = bx * dy - by * dx;

		if(std::abs(b_dot_d_perp) < real(1e-3)) 
		{
			return false;
		}

		real cx = c.x - a.x;
		real cy = c.y - a.y;
		real t = (cx * dy - cy * dx) / b_dot_d_perp;
		if(t < real(1e-3) || t > real(1 + 1e-3)) 
		{
			return false;
		}

		real u = (cx * by - cy * bx) / b_dot_d_perp;
		if(u < real(1e-3) || u > real(1 + 1e-3)) 
		{ 
			return false;
		}
							
		return true;
	}

};


}} // end namespace core::util
