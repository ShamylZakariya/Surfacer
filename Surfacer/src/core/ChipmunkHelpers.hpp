//
//  ChipmunkHelpers.hpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/11/17.
//
//

#ifndef ChipmunkHelpers_hpp
#define ChipmunkHelpers_hpp

#include <cinder/app/App.h>
#include <chipmunk/chipmunk.h>

#include "Common.hpp"
#include "MathHelpers.hpp"

inline cpVect cpv(const dvec2 &v) {
	return ::cpv(v.x, v.y);
}

inline dvec2 v2(const cpVect &v) {
	return dvec2(v.x, v.y);
}

#pragma mark -
#pragma mark Chipmunk Helper Functions

/**
	create an invalid aabb, ready to be extended via cpBBExpand
 */
inline void cpBBInvalidate( cpBB &bb )
{
	bb.l = +std::numeric_limits<cpFloat>::max();
	bb.b = +std::numeric_limits<cpFloat>::max();
	bb.r = -std::numeric_limits<cpFloat>::max();
	bb.t = -std::numeric_limits<cpFloat>::max();
}

static const cpBB cpBBInvalid = {
	+std::numeric_limits<cpFloat>::max(),
	+std::numeric_limits<cpFloat>::max(),
	-std::numeric_limits<cpFloat>::max(),
	-std::numeric_limits<cpFloat>::max()
};

/**
	create an aabb which fits a circle given position and radius.
 */

template< class V >
inline void cpBBNewCircle( cpBB &bb, const V &position, cpFloat radius )
{
	bb.l = position.x - radius;
	bb.b = position.y - radius;
	bb.r = position.x + radius;
	bb.t = position.y + radius;
}

/**
	create an aabb which contains a line segment
 */

template< class V >
inline void cpBBNewLineSegment( cpBB &bb, const V &a, const V &b )
{
	bb.l = cpfmin( a.x, b.x );
	bb.b = cpfmin( a.y, b.y );
	bb.r = cpfmax( a.x, b.x );
	bb.t = cpfmax( a.y, b.y );
}

/**
	create an aabb which contains a line segment with an outset
 */

template< class V >
inline void cpBBNewLineSegment( cpBB &bb, const V &a, const V &b, cpFloat outset )
{
	bb.l = cpfmin( a.x, b.x ) - outset;
	bb.b = cpfmin( a.y, b.y ) - outset;
	bb.r = cpfmax( a.x, b.x ) + outset;
	bb.t = cpfmax( a.y, b.y ) + outset;
}

template< class V >
inline void cpBBNewTriangle(cpBB &bb, const V &a, const V &b, const V &c, cpFloat outset) {
	bb.l = cpfmin( a.x, cpfmin(b.x, c.x)) - outset;
	bb.b = cpfmin( a.y, cpfmin(b.y, c.y)) - outset;
	bb.r = cpfmax( a.x, cpfmax(b.x, c.x)) + outset;
	bb.t = cpfmax( a.y, cpfmax(b.y, c.y)) + outset;
}

template< class V >
inline cpBB cpBBNewTriangle(const V &a, const V &b, const V &c, cpFloat outset) {
	cpBB bb = {
		cpfmin( a.x, cpfmin(b.x, c.x)) - outset,
		cpfmin( a.y, cpfmin(b.y, c.y)) - outset,
		cpfmax( a.x, cpfmax(b.x, c.x)) + outset,
		cpfmax( a.y, cpfmax(b.y, c.y)) + outset
	};
	return bb;
}


/**
	create an 'infinitely' large aabb which will pass all viewport intersection tests
 */
inline void cpBBInfinite( cpBB &bb )
{
	bb.l = -std::numeric_limits<cpFloat>::max();
	bb.b = -std::numeric_limits<cpFloat>::max();
	bb.r = +std::numeric_limits<cpFloat>::max();
	bb.t = +std::numeric_limits<cpFloat>::max();
}

static const cpBB cpBBInfinity = {
	-std::numeric_limits<cpFloat>::max(),
	-std::numeric_limits<cpFloat>::max(),
	+std::numeric_limits<cpFloat>::max(),
	+std::numeric_limits<cpFloat>::max()
};

inline bool cpBBIsValid( cpBB bb ) {
	return bb.r > bb.l && bb.t > bb.b;
}

template< class T, glm::precision P >
void cpBBExpand( cpBB &bb, const glm::tvec2<T,P> &v )
{
	bb.l = cpfmin( bb.l, v.x );
	bb.b = cpfmin( bb.b, v.y );
	bb.r = cpfmax( bb.r, v.x );
	bb.t = cpfmax( bb.t, v.y );
}

template< class T, glm::precision P >
cpBB cpBBExpand( const cpBB &bb, const glm::tvec2<T,P> &v ) {
	cpBB bb2 = bb;
	cpBBExpand(bb2 ,v);
	return bb2;
}


/**
	Expand @a bb to contain a circle of @a radius at @a p
 */
template < class V >
void cpBBExpand( cpBB &bb, const V &p, cpFloat radius )
{
	bb.l = cpfmin( bb.l, p.x - radius );
	bb.b = cpfmin( bb.b, p.y - radius );
	bb.r = cpfmax( bb.r, p.x + radius );
	bb.t = cpfmax( bb.t, p.y + radius );
}

template < class V >
cpBB cpBBExpand( const cpBB &bb, const V &p, cpFloat radius ) {
	cpBB bb2 = bb;
	cpBBExpand(bb2, p, radius);
	return bb2;
}

inline void cpBBExpand( cpBB &bb, const cpBB &bb2 )
{
	bb.l = cpfmin( bb.l, bb2.l );
	bb.b = cpfmin( bb.b, bb2.b );
	bb.r = cpfmax( bb.r, bb2.r );
	bb.t = cpfmax( bb.t, bb2.t );
}

inline cpBB cpBBExpand( const cpBB &bb, const cpBB &bb2 ) {
	cpBB bbr = bb;
	cpBBExpand(bbr, bb2);
	return bbr;
}


inline void cpBBExpand( cpBB &bb, cpFloat amt ) {
	bb.l -= amt;
	bb.b -= amt;
	bb.r += amt;
	bb.t += amt;
}

inline cpBB cpBBExpand( const cpBB &bb, cpFloat amt ) {
	cpBB bb2 = bb;
	bb2.l -= amt;
	bb2.b -= amt;
	bb2.r += amt;
	bb2.t += amt;
	return bb2;
}

/**
 Return true iff the two BBs, if expanded by `f` intersect. 
 */
static inline cpBool cpBBIntersects(cpBB a, cpBB b, cpFloat f) {
	return ((a.l - f) <= (b.r + f) && (b.l - f) <= (a.r + f) && (a.b - f) <= (b.t + f) && (b.b - f) <= (a.t + f));
}


/**
	If two bounding boxes intersect, returns true and writes their overlapping regions into @a intersection.
	Otherwise returns false.
 */
bool cpBBIntersection( const cpBB &bb1, const cpBB &bb2, cpBB &intersection );

inline cpTransform cpTransformNew( const dmat4 &M) {
	dvec4 c0 = M[0];
	dvec4 c1 = M[1];
	dvec4 c3 = M[3];
	cpTransform t = {c0.x, c0.y, c1.x, c1.y, c3.x, c3.y};
	return t;
}
/**
	create a new cpBB containing the provided cpBB after it has had the transform applied to it
 */
inline cpBB cpBBTransform(const cpBB &bb, const dmat4 transform){
	return cpTransformbBB(cpTransformNew(transform), bb);
}

inline cpFloat cpBBRadius( const cpBB &bb )
{
	return (std::max( bb.r - bb.l, bb.t - bb.b ) * 0.5) * 1.414213562;
}

inline cpVect cpBBCenter( const cpBB &bb )
{
	return cpv( (bb.l + bb.r) * 0.5, (bb.b + bb.t) * 0.5 );
}

inline cpFloat cpBBWidth( const cpBB &bb )
{
	return bb.r - bb.l;
}

inline cpFloat cpBBHeight( const cpBB &bb )
{
	return bb.t - bb.b;
}

inline cpVect cpBBSize( const cpBB &bb )
{
	return cpv( bb.r - bb.l, bb.t - bb.b );
}

template < class V >
bool cpBBContains( const cpBB &bb, const V &v )
{
	return (bb.l < v.x && bb.r > v.x && bb.b < v.y && bb.t > v.y);
}

template < class V, class T >
bool cpBBContainsCircle( const cpBB &bb, const V &v, T radius )
{
	return ((bb.l < (v.x - radius)) && (bb.r > (v.x + radius)) && (bb.b < (v.y - radius)) && (bb.t > (v.y + radius)));
}


inline bool operator == ( const cpBB &a, const cpBB &b )
{
	return a.l == b.l && a.b == b.b && a.r == b.r && a.t == b.t;
}

inline std::ostream &operator << ( std::ostream &os, const cpBB &bb )
{
	return os << "[left: " << bb.l << " top: " << bb.t << " right: " << bb.r << " bottom: " << bb.b << "]";
}

#pragma mark - containers

typedef std::set< cpShape* > cpShapeSet;
typedef std::vector< cpShape* > cpShapeVec;

typedef std::set< cpBody* > cpBodySet;
typedef std::vector< cpBody* > cpBodyVec;

typedef std::set< cpConstraint* > cpConstraintSet;
typedef std::vector< cpConstraint* > cpConstraintVec;

#pragma mark - destroy macros

inline void cpShapeCleanupAndFree( cpShape **shape ) {
	if ( *shape ) {
		if (cpShapeGetSpace(*shape)) {
			cpSpaceRemoveShape( cpShapeGetSpace(*shape), *shape );
		}

		cpShapeSetUserData(*shape, nullptr);
		cpShapeFree( *shape );
		*shape = NULL;
	}
}

inline void cpBodyCleanupAndFree( cpBody **body ) {
	if ( *body ) {
		if (cpBodyGetSpace(*body)) {
			cpSpaceRemoveBody(cpBodyGetSpace(*body), *body );
		}

		cpBodySetUserData(*body, nullptr);
		cpBodyFree( *body );
		*body = NULL;
	}
}

inline void cpConstraintCleanupAndFree( cpConstraint **constraint ) {
	if ( *constraint ) {
		if (cpConstraintGetSpace(*constraint)) {
			cpSpaceRemoveConstraint(cpConstraintGetSpace(*constraint), *constraint );
		}

		cpConstraintSetUserData(*constraint, nullptr);
		cpConstraintFree( *constraint );
		*constraint = NULL;
	}
}

inline void cpCleanupAndFree( cpShape* &shape ) { cpShapeCleanupAndFree( &shape ); }
inline void cpCleanupAndFree( cpBody* &body ) { cpBodyCleanupAndFree( &body ); }
inline void cpCleanupAndFree( cpConstraint* &constraint ) { cpConstraintCleanupAndFree( &constraint ); }

template< typename T >
void cpCleanupAndFree( set<T> &s ) {
	for( auto it = ::begin(s), end = ::end(s); it != end; ++it ) {
		T obj = *it;
		cpCleanupAndFree( obj );
	}

	s.clear();
}

template< typename T >
void cpCleanupAndFree( vector<T> &s ) {
	for( auto it = ::begin(s), end = ::end(s); it != end; ++it ) {
		T obj = *it;
		cpCleanupAndFree( obj );
	}

	s.clear();
}

inline bool
cpShapeFilterReject(cpShapeFilter a, cpShapeFilter b)
{
	// Reject the collision if:
	return (
			// They are in the same non-zero group.
			(a.group != 0 && a.group == b.group) ||
			// One of the category/mask combinations fails.
			(a.categories & b.mask) == 0 ||
			(b.categories & a.mask) == 0
			);
}

#endif /* ChipmunkHelpers_hpp */
