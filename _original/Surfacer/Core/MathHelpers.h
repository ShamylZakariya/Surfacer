#pragma once

//
//  Math.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 12/22/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

typedef ci::vec2 Vec2;
typedef ci::vec3 Vec3;
typedef ci::vec4 Vec4;

typedef ci::ivec2 Vec2i;
typedef ci::ivec3 Vec3i;
typedef ci::ivec4 Vec4i;

typedef ci::mat4 Mat4;

namespace cinder {

	typedef RectT<int> Recti;


	template< typename T, glm::precision P = glm::defaultp >
	glm::tvec2<T,P> operator * ( const glm::tvec2<T,P> &v, const glm::tmat4x4<T,P> &m )
	{
		/*
			Adapted from glm::tmat4x4<T,P>::operator *( const glm::tvec3<T,P>

			T x = m[ 0]*rhs.x + m[ 4]*rhs.y + m[ 8]*rhs.z + m[12];
			T y = m[ 1]*rhs.x + m[ 5]*rhs.y + m[ 9]*rhs.z + m[13];
			T z = m[ 2]*rhs.x + m[ 6]*rhs.y + m[10]*rhs.z + m[14];
			T w = m[ 3]*rhs.x + m[ 7]*rhs.y + m[11]*rhs.z + m[15];

			return Vec3<T>( x/w, y/w, z/w );
			
		*/

		T x = m.m[ 0]*v.x + m.m[ 4]*v.y + m[12];
		T y = m.m[ 1]*v.x + m.m[ 5]*v.y + m[13];
		T w = m.m[ 3]*v.x + m.m[ 7]*v.y + m[15];

		return glm::tvec2<T,P>( x/w, y/w );
	}

	template< typename T, glm::precision P = glm::defaultp >
	glm::tvec2<T,P> operator * ( const glm::tmat4x4<T,P> &m, const glm::tvec2<T,P> &v )
	{
		T x = m.m[ 0]*v.x + m.m[ 4]*v.y + m[12];
		T y = m.m[ 1]*v.x + m.m[ 5]*v.y + m[13];
		T w = m.m[ 3]*v.x + m.m[ 7]*v.y + m[15];

		return glm::tvec2<T>( x/w, y/w );
	}

}

template< typename T, glm::precision P = glm::defaultp >
struct vec2_comparator_
{
	bool operator()( const glm::tvec2<T,P> &a, const glm::tvec2<T,P> &b ) const
	{
		if ( a.x != b.x ) return a.x < b.x;
		return a.y < b.y;				
	}
};		

typedef vec2_comparator_<int> Vec2iComparator;
typedef vec2_comparator_<real> Vec2rComparator;

typedef std::set< Vec2i, Vec2rComparator > Vec2iSet;
typedef std::set< Vec2, Vec2rComparator > Vec2Set;

template< typename T, glm::precision P = glm::defaultp >
struct vec3_comparator_
{
	bool operator()( const glm::tvec3<T,P> &a, const glm::tvec3<T,P> &b ) const
	{
		if ( a.x != b.x ) return a.x < b.x;
		if ( a.y != b.y ) return a.y < b.y;
		return a.z < b.z;				
	}
};		

typedef vec3_comparator_<int> Vec3iComparator;
typedef vec3_comparator_<real> Vec3rComparator;

typedef std::set< Vec3i, Vec3iComparator > Vec3iSet;

typedef std::vector< Vec2 > Vec2Vec;
typedef std::vector< Vec3 > Vec3Vec;
typedef std::vector< Vec4 > Vec4Vec;

template <typename T>
inline T sign( const T s )
{
	T result;
	if ( s > 0 ) result = T(1);
	else if ( s < 0 ) result = T(-1);
	else result = T(0);
	
	return result;
}

template< typename T, glm::precision P = glm::defaultp >
glm::tvec2<T,P> operator*(glm::tmat4x4<T,P> const & m, glm::tvec2<T,P> const & v) {
	glm::tvec4<T,P> v4 = m * glm::tvec4<T,P>(v.x, v.y, 0, 0);
	return glm::tvec2<T,P>(v4.x, v4.y);
}

template< typename T, glm::precision P = glm::defaultp >
glm::tvec3<T,P> operator*(glm::tmat4x4<T,P> const & m, glm::tvec3<T,P> const & v) {
	glm::tvec4<T,P> v4 = m * glm::tvec4<T,P>(v.x, v.y, 0, 0);
	return glm::tvec3<T,P>(v4.x, v4.y, v4.z);
}


template< typename T, glm::precision P = glm::defaultp >
inline glm::tvec2<T,P> min( const glm::tvec2<T,P> &a, const glm::tvec2<T,P> &b )
{
	return glm::tvec2<T,P>( a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y );
}

template< typename T, glm::precision P = glm::defaultp >
inline glm::tvec2<T,P> max( const glm::tvec2<T,P> &a, const glm::tvec2<T,P> &b )
{
	return glm::tvec2<T,P>( a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y );
}

template< typename T, glm::precision P = glm::defaultp >
inline glm::tvec3<T,P> min( const glm::tvec3<T,P> &a, const glm::tvec3<T,P> &b )
{
	return glm::tvec3<T,P>( a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z );
}

template< typename T, glm::precision P = glm::defaultp >
inline glm::tvec3<T,P> max( const glm::tvec3<T,P> &a, const glm::tvec3<T,P> &b )
{
	return glm::tvec3<T,P>( a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z );
}

template< typename T, glm::precision P = glm::defaultp >
inline glm::tvec4<T,P> min( const glm::tvec4<T,P> &a, const glm::tvec4<T,P> &b )
{
	return glm::tvec4<T,P>( a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z, a.w < b.w ? a.w : b.w );
}

template< typename T, glm::precision P = glm::defaultp >
inline glm::tvec4<T,P> max( const glm::tvec4<T,P> &a, const glm::tvec4<T,P> &b )
{
	return glm::tvec4<T,P>( a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z, a.w > b.w ? a.w : b.w );
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

template< typename T, glm::precision P = glm::defaultp >
inline glm::tvec4<T,P> clamp( const glm::tvec4<T,P> &c, const glm::tvec4<T,P> &min, const glm::tvec4<T,P> &max )
{
	return glm::tvec4<T,P>( 
		::clamp( c.x, min.x, max.x ),
		::clamp( c.y, min.y, max.y ),
		::clamp( c.z, min.z, max.z ),
		::clamp( c.w, min.w, max.w ));
}

template< typename T, glm::precision P = glm::defaultp >
inline glm::tvec2<T,P> saturate( const glm::tvec2<T,P> &c )
{
	return glm::tvec2<T,P>( 
		::saturate( c.x ),
		::saturate( c.y ));
}

template< typename T, glm::precision P = glm::defaultp >
inline glm::tvec3<T,P> saturate( const glm::tvec3<T,P> &c )
{
	return glm::tvec3<T,P>( 
		::saturate( c.x ),
		::saturate( c.y ),
		::saturate( c.z ));
}

template< typename T, glm::precision P = glm::defaultp >
inline glm::tvec4<T,P> saturate( const glm::tvec4<T,P> &c )
{
	return glm::tvec4<T,P>( 
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

template< typename T, glm::precision P = glm::defaultp >
glm::tvec2<T,P> normalize( const glm::tvec2<T,P> &v )
{
	// trick from http://altdevblogaday.com/2011/08/21/practical-flt-point-tricks/
	// prevents INF or NaN if vector length is zero
	const T e = 1.0e-037f;
	T l = std::sqrt(v.x * v.x + v.y * v.y) + e;
	T inv = T(1.0) / l;
	return glm::tvec2<T,P>( v.x * inv, v.y * inv );
}

template< typename T, glm::precision P = glm::defaultp >
glm::tvec3<T,P> normalize( const glm::tvec3<T,P> &v )
{
	// trick from http://altdevblogaday.com/2011/08/21/practical-flt-point-tricks/
	// prevents INF or NaN if vector length is zero
	const T e = 1.0e-037f;
	T l = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z) + e;
	T inv = T(1.0) / l;
	return glm::tvec3<T,P>( v.x * inv, v.y * inv, v.z * inv );
}

template< typename T, glm::precision P = glm::defaultp >
glm::tvec2<T,P> mid( const glm::tvec2<T,P> &a, const glm::tvec2<T,P> &b ) 
{ 
	return glm::tvec2<T,P>( (a.x + b.x) * T(0.5), (a.y + b.y) * T( 0.5 ));
}


/**
	safe normalize
	@param v the vector to attempt to normalize
	@param nv the normalized vector will be written here, if possible
	return true if the vector was normalized, false otherwise
*/

template< typename T, glm::precision P = glm::defaultp >
bool normalize( const glm::tvec2<T,P> &v, glm::tvec2<T,P> &nv )
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
template< typename T, glm::precision P = glm::defaultp >
inline glm::tvec2<T,P> rotateCCW( const glm::tvec2<T,P> &v )
{
	return glm::tvec2<T,P>( -v.y, v.x );
}

/**
	rotate vector 90 degrees clockwise
*/
template< typename T, glm::precision P = glm::defaultp >
inline glm::tvec2<T,P> rotateCW( const glm::tvec2<T,P> &v )
{
	return glm::tvec2<T,P>( v.y, -v.x );
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
template< typename T, glm::precision P = glm::defaultp >
glm::tvec2<T,P> lrp( const glm::tvec2<T,P> &range, const glm::tvec2<T,P> &a, const glm::tvec2<T,P> &b )
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

template< typename T, glm::precision P = glm::defaultp >
glm::tvec3<T,P> lrp( const glm::tvec3<T,P> &range, const glm::tvec3<T,P> &a, const glm::tvec3<T,P> &b )
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
template< typename T, glm::precision P = glm::defaultp >
glm::tvec4<T,P> lrp( const glm::tvec4<T,P> &range, const glm::tvec4<T,P> &a, const glm::tvec4<T,P> &b )
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
	@brief raises an integer to an integerial power, quickly.
	@ingroup maths
	@return @a n raised to the @a p'th power.
	@note if p < 0, since we're dealing with ints, return 0.
 */
inline std::size_t powul( std::size_t n, std::size_t p )
{
	if ( p == 0 ) return 1;
	if ( p == 1 ) return n;

	std::size_t r( n );
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

template< typename T, glm::precision P = glm::defaultp >
void Mat4WithPositionAndRotation( glm::tmat4x4<T,P> &R, const glm::tvec2<T,P> &position, const glm::tvec2<T,P> &rotation )
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
//	const Mat4 R(
//		  dir.x,   dir.y,     0,       0,
//		 -dir.y,   dir.x,     0,       0,
//		      0,       0,     1,       0,
//		start.x, start.y,     0,       1 
//	);
//
}

template< typename T, glm::precision P = glm::defaultp >
void Mat4WithPositionAndRotation( glm::tmat4x4<T,P> &R, const glm::tvec2<T,P> &position, T rotation )
{
	Mat4WithPositionAndRotation( R, position, glm::tvec2<T,P>( std::cos( rotation ), std::sin( rotation )));
}

inline ci::Color RandomColor()
{
	float h = ci::Rand::randFloat(),
		  s = ci::Rand::randFloat( 0.5, 1 ),
		  v = ci::Rand::randFloat( 0.85, 1 );

	return ci::Color( ci::CM_HSV, h,s,v );
}


