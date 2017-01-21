#pragma once

//
//  Spline.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 12/13/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#include <vector>
#include "Common.h"

namespace core { namespace util { namespace spline {

	/**
		draw a spline from b to c
		a is the vertex before b, d is the vertex after c. They're used to create control points. They can be NULL, signifying begin or end of path.
	*/
	template< typename T >
	void spline_segment( 
		const ci::Vec2<T> *a, const ci::Vec2<T> &b, const ci::Vec2<T> &c, const ci::Vec2<T> *d, 
		T tension, 
		int count, 
		std::vector< ci::Vec2<T> > &result )
	{
		T u = 0,
		  inc = T(1) / T(count);

		for ( int i = 0; i < count; i++, u += inc )
		{
			const T 
				uSquared = u*u,
				uCubed = uSquared*u,
				c0 = tension * (-uCubed + T(2) * uSquared - u),
				c1 = (T(2) - tension) * uCubed + (tension - T(3)) * uSquared + T(1),
				c2 = (tension - T(2)) * uCubed + (T(3) - T(2) * tension) * uSquared + tension * u,
				c3 = tension * (uCubed - uSquared);
		
			ci::Vec2<T> 
				vOne( b ),
				vTwo( c ),
				vBegin( a ? *a : (vOne * 2 - vTwo)), 
				vEnd( d ? *d : (vTwo * 2 - vOne));
						
			result.push_back((vBegin*c0) + (vOne* c1) + (vTwo * c2) + (vEnd * c3));				
		}
	}
	
	/**
		generate the spline of the path described by @a vertices with @a tension and a target count of @a count into @a result.
		if @a closed the path is assumed to be closed.
	*/
	template< typename T >
	void spline( const std::vector< ci::Vec2<T> > &vertices, T tension, int count, bool closed, std::vector< ci::Vec2<T> > &result )
	{
		typedef std::vector< ci::Vec2<T> > Vec2Vec;
		typedef typename std::vector< ci::Vec2<T> >::const_iterator Vec2VecConstIterator;
		typedef typename std::vector< ci::Vec2<T> >::iterator Vec2VecIterator;
	
		result.clear();

		if ( vertices.size() < 2 ) return;
		int segCount = count / ( vertices.size() - 1 );
		
		if ( closed )
		{
			ci::Vec2<T> 
				* aPtr = closed ? const_cast< ci::Vec2<T>* >(&vertices.back()) : NULL,
				* dPtr = NULL;

			for( Vec2VecConstIterator 
				b(vertices.begin()),
				c(vertices.begin()+1),
				d(vertices.begin()+2),
				end(vertices.end()); 
				b != end; 
				++b,++c,++d )
			{
				if ( c == end ) 
				{
					c = vertices.begin();
					d = vertices.begin()+1; 
				}

				dPtr = const_cast< ci::Vec2<T>* >(&(*d));
				
				spline_segment( aPtr, *b, *c, dPtr, tension, segCount, result);
				aPtr = const_cast< ci::Vec2<T>* >(&(*b));
			}

			result.push_back(vertices.front());
		}
		else
		{
			ci::Vec2<T> 
				* aPtr = NULL,
				* dPtr = NULL;

			for( Vec2VecConstIterator 
				b(vertices.begin()),
				c(vertices.begin()+1),
				d(vertices.begin()+2),
				end(vertices.end()); 
				c != end; 
				++b,++c,++d )
			{
				dPtr = d != end ? const_cast< ci::Vec2<T>* >(&(*d)) : NULL;
				
				spline_segment( aPtr, *b, *c, dPtr, tension, segCount, result);
				aPtr = const_cast< ci::Vec2<T>* >(&(*b));
			}

			result.push_back(vertices.back());
		}		
	}

}}} // end namespace core::util::spline