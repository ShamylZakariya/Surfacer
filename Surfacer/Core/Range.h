#pragma once

#include "Common.h"

//
//  range.h
//  Surfacer
//
//  Created by Zakariya Shamyl on 8/24/11.
//  Copyright 2011 TomorrowPlusX. All rights reserved.
//


/**
	Represents a min and max value pair.
*/

template< class T >
struct range
{
	T min, max;
	
	range()
	{}
	
	range( T _min, T _max ):
		min( _min ),
		max( _max )
	{}
	
	range<T> &operator=( T v )
	{
		min = max = v;
		return *this;
	}
	
};

template< class T >
inline T lrp( real v, const range<T> &r )
{
	return ::lrp( v, r.min, r.max );
}

template< class T >
inline range<T> clamp( const range<T> &r, const T &clampMin, const T &clampMax )
{
	return range<T>( ::clamp( r.min, clampMin, clampMax ), ::clamp( r.max, clampMin, clampMax ));
}

template< class T >
inline range<T> saturate( const range<T> &r )
{
	return range<T>( ::saturate( r.min ), ::saturate( r.max ));
}

template< class T >
inline range<T> min( const range<T> &r, const T &clampMin )
{
	using namespace std;
	return range<T>( min( r.min, clampMin ), min( r.max, clampMin ));
}

template< class T >
inline range<T> max( const range<T> &r, const T &clampMax )
{
	using namespace std;
	return range<T>( max( r.min, clampMax ), max( r.max, clampMax ));
}
