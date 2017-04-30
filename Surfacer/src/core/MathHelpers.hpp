//
//  MathHelpers.h
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/11/17.
//
//

#ifndef MathHelpers_h
#define MathHelpers_h

#include <cinder/CinderMath.h>
#include <cinder/Matrix.h>
#include <cinder/Vector.h>
#include <cinder/Quaternion.h>

namespace cinder {
	typedef RectT<int> Recti;

	template< class T, glm::precision P >
	inline glm::tvec2<T,P> operator * ( const glm::tmat4x4<T,P> &m, const glm::tvec2<T,P> &v )
	{
		return glm::tvec2<T,P>(m * glm::tvec4<T,P>(v.x, v.y, T(0), T(1)));
	}

	template< class T, glm::precision P >
	inline glm::tvec2<T,P> operator * ( const glm::tmat3x3<T,P> &m, const glm::tvec2<T,P> &v )
	{
		return glm::tvec2<T,P>(m * glm::tvec4<T,P>(v.x, v.y, T(0), T(1)));
	}

}

template < typename T, glm::precision P >
struct vec2_comparator_
{
	bool operator()( const glm::tvec2<T,P> &a, const glm::tvec2<T,P> &b ) const
	{
		if ( a.x != b.x ) return a.x < b.x;
		return a.y < b.y;
	}
};

typedef vec2_comparator_<int, glm::highp> ivec2Comparator;
typedef vec2_comparator_<float, glm::highp> vec2Comparator;
typedef vec2_comparator_<double, glm::highp> dvec2Comparator;

typedef std::set< ci::ivec2, ivec2Comparator > ivec2Set;
typedef std::set< ci::vec2, vec2Comparator > vec2Set;
typedef std::set< ci::dvec2, dvec2Comparator > dvec2Set;

template < typename T, glm::precision P >
struct vec3_comparator_
{
	bool operator()( const glm::tvec3<T,P> &a, const glm::tvec3<T,P> &b ) const
	{
		if ( a.x != b.x ) return a.x < b.x;
		if ( a.y != b.y ) return a.y < b.y;
		return a.z < b.z;
	}
};

typedef glm::tvec3<int,glm::highp> vec3i;

typedef vec3_comparator_<int, glm::highp> vec3iComparator;
typedef vec3_comparator_<float, glm::highp> vec3Comparator;
typedef vec3_comparator_<double, glm::highp> dvec3Comparator;

typedef std::set< vec3i, vec3iComparator > vec3iSet;
typedef std::set< ci::vec3, vec3Comparator > vec3Set;
typedef std::set< ci::dvec3, dvec3Comparator > dvec3Set;

template <typename T>
inline T sign( const T s )
{
	T result;
	if ( s > 0 ) result = T(1);
	else if ( s < 0 ) result = T(-1);
	else result = T(0);

	return result;
}

template< class T, glm::precision P >
inline T lengthSquared( const glm::tvec2<T,P> &a)
{
	return ( a.x * a.x + a.y * a.y );
}

template< class T, glm::precision P >
inline T lengthSquared( const glm::tvec3<T,P> &a)
{
	return ( a.x * a.x + a.y * a.y + a.z * a.z );
}


template< class T, glm::precision P >
inline glm::tvec2<T,P> min( const glm::tvec2<T,P> &a, const glm::tvec2<T,P> &b )
{
	return glm::tvec2<T,P>( a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y );
}

template< class T, glm::precision P >
inline glm::tvec2<T,P> max( const glm::tvec2<T,P> &a, const glm::tvec2<T,P> &b )
{
	return glm::tvec2<T,P>( a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y );
}

template< class T, glm::precision P >
inline glm::tvec3<T,P> min( const glm::tvec3<T,P> &a, const glm::tvec3<T,P> &b )
{
	return glm::tvec3<T,P>( a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z );
}

template< class T, glm::precision P >
inline glm::tvec3<T,P> max( const glm::tvec3<T,P> &a, const glm::tvec3<T,P> &b )
{
	return glm::tvec3<T,P>( a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z );
}

template< class T, glm::precision P >
inline glm::tvec4<T,P> min( const glm::tvec4<T,P> &a, const glm::tvec4<T,P> &b )
{
	return glm::tvec4<T,P>( a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z, a.w < b.w ? a.w : b.w );
}

template< class T, glm::precision P >
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

template< class T, glm::precision P >
inline glm::tvec4<T,P> clamp( const glm::tvec4<T,P> &c, const glm::tvec4<T,P> &min, const glm::tvec4<T,P> &max )
{
	return glm::tvec4<T,P>(
						   ::clamp( c.x, min.x, max.x ),
						   ::clamp( c.y, min.y, max.y ),
						   ::clamp( c.z, min.z, max.z ),
						   ::clamp( c.w, min.w, max.w ));
}

template< class T, glm::precision P >
inline glm::tvec2<T,P> saturate( const glm::tvec2<T,P> &c )
{
	return glm::tvec2<T,P>(
						   ::saturate( c.x ),
						   ::saturate( c.y ));
}

template< class T, glm::precision P >
inline glm::tvec3<T,P> saturate( const glm::tvec3<T,P> &c )
{
	return glm::tvec3<T,P>(
						   ::saturate( c.x ),
						   ::saturate( c.y ),
						   ::saturate( c.z ));
}

template< class T, glm::precision P >
inline glm::tvec4<T,P> saturate( const glm::tvec4<T,P> &c )
{
	return glm::tvec4<T,P>(
						   ::saturate( c.x ),
						   ::saturate( c.y ),
						   ::saturate( c.z ),
						   ::saturate( c.w ));
}

template< class T, glm::precision P >
inline float distanceSquared( const glm::tvec2<T,P> &a, const glm::tvec2<T,P> &b )
{
	return length2(b-a);
}

template< class T, glm::precision P >
inline float distanceSquared( const glm::tvec3<T,P> &a, const glm::tvec3<T,P> &b )
{
	return length2(b-a);
}

template<typename T>
T distanceSquared( T a, T b )
{
	return (a-b)*(a-b);
}

template< class T, glm::precision P >
glm::tvec2<T,P> normalize( const glm::tvec2<T,P> &v )
{
	// trick from http://altdevblogaday.com/2011/08/21/practical-flt-point-tricks/
	// prevents INF or NaN if vector length is zero
	const T e = 1.0e-037f;
	T l = std::sqrt(v.x * v.x + v.y * v.y) + e;
	T inv = T(1.0) / l;
	return glm::tvec2<T,P>( v.x * inv, v.y * inv );
}

template< class T, glm::precision P >
glm::tvec3<T,P> normalize( const glm::tvec3<T,P> &v )
{
	// trick from http://altdevblogaday.com/2011/08/21/practical-flt-point-tricks/
	// prevents INF or NaN if vector length is zero
	const T e = 1.0e-037f;
	T l = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z) + e;
	T inv = T(1.0) / l;
	return glm::tvec3<T,P>( v.x * inv, v.y * inv, v.z * inv );
}

template< class T, glm::precision P >
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

template< class T, glm::precision P >
bool normalize( const glm::tvec2<T,P> &v, glm::tvec2<T,P> &nv )
{
	T l = std::sqrt(v.x * v.x + v.y * v.y);
	if ( l > 1e-4 )
	{
		T inv = float(1.0) / l;
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
	return I - (float(2) * dot( N, I ) * N);
}

/**
	rotate vector 90 degrees counter-clockwise, assuming a bottom-left coordinate system
 */
template< class T, glm::precision P >
inline glm::tvec2<T,P> rotateCW( const glm::tvec2<T,P> &v )
{
	return glm::tvec2<T,P>( -v.y, v.x );
}

/**
	rotate vector 90 degrees clockwise, assuming a bottom-left coordinate system
 */
template< class T, glm::precision P >
inline glm::tvec2<T,P> rotateCCW( const glm::tvec2<T,P> &v )
{
	return glm::tvec2<T,P>( v.y, -v.x );
}


/**
	@brief linear interpolation
	@ingroup util
	Linear interpolate between @a min and @a max
	For range = 0, return min, for range = 1, return max.
 */
template <class T, class V> T lrp( V range, T min, T max )
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
template< class T, glm::precision P >
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

template< class T, glm::precision P >
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
template< class T, glm::precision P >
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

template <typename T>
T to_degrees(T rads) {
	return 180 * rads / M_PI;
}

template <typename T>
T to_radians(T degrees) {
	return degrees * M_PI / 180;
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

template< class T, glm::precision P >
void mat4WithPositionAndRotation( glm::tmat4x4<T,P> &R, const glm::tvec2<T,P> &position, const glm::tvec2<T,P> &rotation )
{
	R[0] = glm::tvec4<T,P>(rotation.x, rotation.y, 0, 0);
	R[1] = glm::tvec4<T,P>(-rotation.y, rotation.x, 0, 0);
	R[2] = glm::tvec4<T,P>(0, 0, 1, 0);
	R[3] = glm::tvec4<T,P>(position.x, position.y, 0, 1);
}

template< typename T, glm::precision P >
void mat4WithPositionAndRotation( glm::tmat4x4<T,P> &R, const glm::tvec2<T,P> &position, T rotation )
{
	mat4WithPositionAndRotation( R, position, glm::tvec2<T,P>( std::cos( rotation ), std::sin( rotation )));
}

template< typename T, glm::precision P >
T getRotation(const glm::tmat4x4<T,P> &R) {
	return std::atan2(R[0].y, R[0].x);
}

template< typename T, glm::precision P >
glm::tvec2<T,P> getTranslation(const glm::tmat4x4<T,P> &R) {
	return glm::tvec2<T,P>(R[3].x, R[3].y);
}

using ci::ivec2;
using ci::vec2;
using ci::dvec2;

using ci::ivec3;
using ci::vec3;
using ci::dvec3;

using ci::ivec4;
using ci::vec4;
using ci::dvec4;

using ci::mat4;
using ci::dmat4;

using glm::length;
using glm::distance;
using glm::dot;
using glm::cross;
using glm::normalize;
using glm::translate;
using glm::rotate;
using glm::scale;

#endif /* MathHelpers_h */
