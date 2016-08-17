#pragma once

//
//  Math.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 12/22/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

typedef ci::Vec2<float> Vec2f;
typedef ci::Vec2<real> Vec2r;
typedef ci::Vec2<int> Vec2i;

typedef ci::Vec3<float> Vec3f;
typedef ci::Vec3<real> Vec3r;
typedef ci::Vec3<int> Vec3i;

typedef ci::Vec4<float> Vec4f;
typedef ci::Vec4<real> Vec4r;
typedef ci::Vec4<int> Vec4i;

typedef ci::Matrix44<float> Mat4f;
typedef ci::Matrix44<real> Mat4r;

namespace cinder {
	typedef RectT<int> Recti;


	template< typename T >
	Vec2<T> operator * ( const Vec2<T> &v, const Matrix44<T> &m )
	{
		/*
			Adapted from ci::Matrix44<T>::operator *( const ci::Vec3<T>

			T x = m[ 0]*rhs.x + m[ 4]*rhs.y + m[ 8]*rhs.z + m[12];
			T y = m[ 1]*rhs.x + m[ 5]*rhs.y + m[ 9]*rhs.z + m[13];
			T z = m[ 2]*rhs.x + m[ 6]*rhs.y + m[10]*rhs.z + m[14];
			T w = m[ 3]*rhs.x + m[ 7]*rhs.y + m[11]*rhs.z + m[15];

			return Vec3<T>( x/w, y/w, z/w );
			
		*/

		T x = m.m[ 0]*v.x + m.m[ 4]*v.y + m[12];
		T y = m.m[ 1]*v.x + m.m[ 5]*v.y + m[13];
		T w = m.m[ 3]*v.x + m.m[ 7]*v.y + m[15];

		return Vec2<T>( x/w, y/w );
	}

	template< typename T >
	Vec2<T> operator * ( const Matrix44<T> &m, const Vec2<T> &v )
	{
		T x = m.m[ 0]*v.x + m.m[ 4]*v.y + m[12];
		T y = m.m[ 1]*v.x + m.m[ 5]*v.y + m[13];
		T w = m.m[ 3]*v.x + m.m[ 7]*v.y + m[15];

		return Vec2<T>( x/w, y/w );
	}

}

template < class T >
struct vec2_comparator_
{
	bool operator()( const ci::Vec2<T> &a, const ci::Vec2<T> &b ) const
	{
		if ( a.x != b.x ) return a.x < b.x;
		return a.y < b.y;				
	}
};		

typedef vec2_comparator_<int> Vec2iComparator;
typedef vec2_comparator_<real> Vec2rComparator;

typedef std::set< Vec2i, Vec2rComparator > Vec2iSet;
typedef std::set< Vec2r, Vec2rComparator > Vec2rSet;

template < class T >
struct vec3_comparator_
{
	bool operator()( const ci::Vec3<T> &a, const ci::Vec3<T> &b ) const
	{
		if ( a.x != b.x ) return a.x < b.x;
		if ( a.y != b.y ) return a.y < b.y;
		return a.z < b.z;				
	}
};		

typedef vec3_comparator_<int> Vec3iComparator;
typedef vec3_comparator_<real> Vec3rComparator;

typedef std::set< Vec3i, Vec3iComparator > Vec3iSet;
typedef std::set< Vec3r, Vec3rComparator > Vec3rSet;

typedef std::vector< Vec2f > Vec2fVec;
typedef std::vector< Vec2r > Vec2rVec;

typedef std::vector< Vec3f > Vec3fVec;
typedef std::vector< Vec3r > Vec3rVec;

typedef std::vector< Vec4f > Vec4fVec;
typedef std::vector< Vec4r > Vec4rVec;

template <typename T>
inline T sign( const T s )
{
	T result;
	if ( s > 0 ) result = T(1);
	else if ( s < 0 ) result = T(-1);
	else result = T(0);
	
	return result;
}

template< class T >
inline ci::Vec2<T> min( const ci::Vec2<T> &a, const ci::Vec2<T> &b )
{
	return ci::Vec2<T>( a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y );
}

template< class T >
inline ci::Vec2<T> max( const ci::Vec2<T> &a, const ci::Vec2<T> &b )
{
	return ci::Vec2<T>( a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y );
}

template< class T >
inline ci::Vec3<T> min( const ci::Vec3<T> &a, const ci::Vec3<T> &b )
{
	return ci::Vec3<T>( a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z );
}

template< class T >
inline ci::Vec3<T> max( const ci::Vec3<T> &a, const ci::Vec3<T> &b )
{
	return ci::Vec3<T>( a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z );
}

template< class T >
inline ci::Vec4<T> min( const ci::Vec4<T> &a, const ci::Vec4<T> &b )
{
	return ci::Vec4<T>( a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z, a.w < b.w ? a.w : b.w );
}

template< class T >
inline ci::Vec4<T> max( const ci::Vec4<T> &a, const ci::Vec4<T> &b )
{
	return ci::Vec4<T>( a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z, a.w > b.w ? a.w : b.w );
}



template< typename T >
inline T clamp( T v, T min, T max )
{
	return v < min ? min : (( v > max ) ? max : v);
}

template< typename T >
inline T saturate( T v )
{
	return v < T(0) ? T(0) : (( v > T(1) ) ? T(1) : v);
}

template< typename T >
inline ci::Vec4<T> clamp( const ci::Vec4<T> &c, const ci::Vec4<T> &min, const ci::Vec4<T> &max )
{
	return ci::Vec4<T>( 
		::clamp( c.x, min.x, max.x ),
		::clamp( c.y, min.y, max.y ),
		::clamp( c.z, min.z, max.z ),
		::clamp( c.w, min.w, max.w ));
}

template< typename T >
inline ci::Vec2<T> saturate( const ci::Vec2<T> &c )
{
	return ci::Vec2<T>( 
		::saturate( c.x ),
		::saturate( c.y ));
}

template< typename T >
inline ci::Vec3<T> saturate( const ci::Vec3<T> &c )
{
	return ci::Vec3<T>( 
		::saturate( c.x ),
		::saturate( c.y ),
		::saturate( c.z ));
}

template< typename T >
inline ci::Vec4<T> saturate( const ci::Vec4<T> &c )
{
	return ci::Vec4<T>( 
		::saturate( c.x ),
		::saturate( c.y ),
		::saturate( c.z ),
		::saturate( c.w ));
}

template <class T>
inline real distance( const T &a, const T &b )
{
	return a.distance(b);
}

inline real distance( real a, real b )
{
	return std::abs( a-b );
}

inline int distance( int a, int b )
{
	return std::abs( a-b );
}

template <class T>
inline real distanceSquared( const T &a, const T &b )
{
	return a.distanceSquared(b);
}

inline real distanceSquared( real a, real b )
{
	return (a-b)*(a-b);
}

inline int distanceSquared( int a, int b )
{
	return (a-b)*(a-b);
}

template< typename T >
ci::Vec2<T> normalize( const ci::Vec2<T> &v )
{
	// trick from http://altdevblogaday.com/2011/08/21/practical-flt-point-tricks/
	// prevents INF or NaN if vector length is zero
	const T e = 1.0e-037f;
	T l = std::sqrt(v.x * v.x + v.y * v.y) + e;
	T inv = T(1.0) / l;
	return ci::Vec2<T>( v.x * inv, v.y * inv );
}

template< typename T >
ci::Vec3<T> normalize( const ci::Vec3<T> &v )
{
	// trick from http://altdevblogaday.com/2011/08/21/practical-flt-point-tricks/
	// prevents INF or NaN if vector length is zero
	const T e = 1.0e-037f;
	T l = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z) + e;
	T inv = T(1.0) / l;
	return ci::Vec3<T>( v.x * inv, v.y * inv, v.z * inv );
}

template < typename T >
ci::Vec2<T> mid( const ci::Vec2<T> &a, const ci::Vec2<T> &b ) 
{ 
	return ci::Vec2<T>( (a.x + b.x) * T(0.5), (a.y + b.y) * T( 0.5 ));
}


/**
	safe normalize
	@param v the vector to attempt to normalize
	@param nv the normalized vector will be written here, if possible
	return true if the vector was normalized, false otherwise
*/

template< typename T >
bool normalize( const ci::Vec2<T> &v, ci::Vec2<T> &nv )
{
	T l = std::sqrt(v.x * v.x + v.y * v.y);
	if ( l > Epsilon )
	{
		T inv = real(1.0) / l;
		nv.x = v.x * inv;
		nv.y = v.y * inv;
		return true;
	}
	
	return false;
}

template < typename T >
T normalize( T v )
{
	return v >= 0 ? T(1) : T(-1);
}

inline int normalize( int v ) 
{
	return v >= 0 ? 1 : -1;
}

/**
	@brief Calculate the reflection vector
	@param I The incident vector
	@param N The surface normal vector
	@return I reflected about N
	
	@note It is required that N is normalized.
*/
template <class T>
T reflect( const T &I, const T &N )
{
	return I - (real(2) * dot( N, I ) * N);
}

/**
	rotate vector 90 degrees counter-clockwise
*/
template <class T>
inline ci::Vec2<T> rotateCCW( const ci::Vec2<T> &v )
{
	return ci::Vec2<T>( -v.y, v.x );
}

/**
	rotate vector 90 degrees clockwise
*/
template <class T>
inline ci::Vec2<T> rotateCW( const ci::Vec2<T> &v )
{
	return ci::Vec2<T>( v.y, -v.x );
}


/**
	@brief linear interpolation
	@ingroup util
	Linear interpolate between @a min and @a max
	For range = 0, return min, for range = 1, return max.
*/
template <class T> T lrp( real range, T min, T max )
{
	return min + (( max - min ) * range);
}

/**
	@brief linear per-component interpolation of vec2
	@ingroup util
	Linear interpolation from @a a to @a b, with each
	component being lrpd via the value in the corresponding
	component in @a range
*/
template< class T>
ci::Vec2<T> lrp( const ci::Vec2<T> &range, const ci::Vec2<T> &a, const ci::Vec2<T> &b )
{
	return vec2( lrp( range.x, a.x, b.x ),
				 lrp( range.y, a.y, b.y ) );
}

/**
	@brief linear per-component interpolation of vec3
	@ingroup util
	Linear interpolation from @a a to @a b, with each
	component being lrpd via the value in the corresponding
	component in @a range
*/

template< class T>
ci::Vec3<T> lrp( const ci::Vec3<T> &range, const ci::Vec3<T> &a, const ci::Vec3<T> &b )
{
	return vec3( lrp( range.x, a.x, b.x ),
				 lrp( range.y, a.y, b.y ),
				 lrp( range.z, a.z, b.z ) );
}

/**
	@brief linear per-component interpolation of vec4
	@ingroup util
	Linear interpolation from @a a to @a b, with each
	component being lrpd via the value in the corresponding
	component in @a range
*/
template< class T>
ci::Vec4<T> lrp( const ci::Vec4<T> &range, const ci::Vec4<T> &a, const ci::Vec4<T> &b )
{
	return vec4( lrp( range.x, a.x, b.x ),
				 lrp( range.y, a.y, b.y ),
				 lrp( range.z, a.z, b.z ),
				 lrp( range.w, a.w, b.w ) );
}


/**
	@brief linear interpolate between two angles, taking the shortest distance
*/
template < class T >
T lrpRadians( T v, T r0, T r1 )
{
	const T PI2 = 2 * M_PI;
	T cwR1 = r1 < r0 ? r1 : (-PI2 + r1),
	  ccwR1 = r1 > r0 ? r1 : (PI2 + r1),
	  cwDistance = std::abs( cwR1 - r0 ),
	  ccwDistance = std::abs( ccwR1 - r0 );

	if ( cwDistance < ccwDistance )
	{
		return r0 + (( cwR1 - r0 ) * v );
		//return lrp( v, r0, cwR1 );
	}
	
	return r0 + (( ccwR1 - r0 ) * v );
	//return lrp( v, r0, ccwR1 );		
}

/**
	@brief raises an integer to an integerial power, quickly.
	@ingroup maths
	@return @a n raised to the @a p'th power.
	@note if p < 0, since we're dealing with ints, return 0.
*/
inline int powi( int n, int p )
{
	if ( p < 0 ) return 0;
	if ( p == 0 ) return 1;
	if ( p == 1 ) return n;

	int r( n );
	while( p-- > 1 )
	{
		r *= n;
	}
	
	return r;
}

/**
	@brief raises 2 to an integerial power, quickly.
	@ingroup maths
	@return 2 raised to the power @a p
*/
inline int pow2i( int p )
{
	return ( p <= 0 ) ? 1 : ( 2 << ( p - 1));
}

template <typename T>
T fract( T v )
{
	return v - std::floor(v);
}

/**
	@brief returns the highest power of two less than a given value
	@ingroup maths
	@return the highest power of two less than @a x
*/
inline unsigned previousPowerOf2(unsigned x)
{
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x - (x >> 1);
}

/**
	@brief returns the next highest power of two following a provided integer
	@ingroup maths
	@return the next highest power of two for @a x
*/
inline unsigned nextPowerOf2(unsigned x)
{
	x -= 1;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}

/**
	@brief determines if an integer is a power of two
	@ingroup maths
	@return true if @a n is a power of two
*/

inline bool isPow2( int n )
{  
	return (!(n & ((n) - 1 )));
}

template< typename T >
void Mat4WithPositionAndRotation( ci::Matrix44<T> &R, const ci::Vec2<T> &position, const ci::Vec2<T> &rotation )
{
	R.m[ 0] = rotation.x;
	R.m[ 1] = rotation.y;
	R.m[ 2] = 0;
	R.m[ 3] = 0;

	R.m[ 4] = -rotation.y;
	R.m[ 5] = rotation.x;
	R.m[ 6] = 0;
	R.m[ 7] = 0;

	R.m[ 8] = 0;
	R.m[ 9] = 0;
	R.m[10] = 1;
	R.m[11] = 0;

	R.m[12] = position.x;
	R.m[13] = position.y;
	R.m[14] = 0;
	R.m[15] = 1;
//
//	const Mat4f R(
//		  dir.x,   dir.y,     0,       0,
//		 -dir.y,   dir.x,     0,       0,
//		      0,       0,     1,       0,
//		start.x, start.y,     0,       1 
//	);
//
}

template< typename T >
void Mat4WithPositionAndRotation( ci::Matrix44<T> &R, const ci::Vec2<T> &position, T rotation )
{
	Mat4WithPositionAndRotation( R, position, ci::Vec2<T>( std::cos( rotation ), std::sin( rotation )));
}

inline ci::Color RandomColor()
{
	float h = ci::Rand::randFloat(),
		  s = ci::Rand::randFloat( 0.5, 1 ),
		  v = ci::Rand::randFloat( 0.85, 1 );

	return ci::Color( ci::CM_HSV, h,s,v );
}


