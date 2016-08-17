#pragma once

//
//  ChipmunkHelpers.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 12/22/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#pragma mark -
#pragma mark Chipmunk <-> Cinder conversion

// convert a chipmunk cpVect to cinder Vec2r
inline Vec2r v2r( const cpVect &v )
{
	return Vec2r( v.x, v.y );
}

// convert a chipmunk cpVect to cinder Vec2f
inline Vec2f v2f( const cpVect &v )
{
	return Vec2f( v.x, v.y );
}


// convert a cinder Vec2r or Vec2f to chipmunk cpVect
template< typename T >
cpVect cpv( const ci::Vec2<T> &v )
{
	return ::cpv( v.x, v.y );
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

template < class V >
void cpBBExpand( cpBB &bb, const V &v )
{
	bb.l = cpfmin( bb.l, v.x );
	bb.b = cpfmin( bb.b, v.y );
	bb.r = cpfmax( bb.r, v.x );
	bb.t = cpfmax( bb.t, v.y );
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

inline void cpBBExpand( cpBB &bb, const cpBB &bb2 )
{
	bb.l = cpfmin( bb.l, bb2.l );
	bb.b = cpfmin( bb.b, bb2.b );
	bb.r = cpfmax( bb.r, bb2.r );
	bb.t = cpfmax( bb.t, bb2.t );
}

inline void cpBBExpand( cpBB &bb, cpFloat amt )
{
	bb.l -= amt;
	bb.b -= amt;
	bb.r += amt;
	bb.t += amt;
}

/**
	If two bounding boxes intersect, returns true and writes their overlapping regions into @a intersection.
	Otherwise returns false.
*/
bool cpBBIntersection( const cpBB &bb1, const cpBB &bb2, cpBB &intersection );

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

template< typename T >
void cpBody_ToMatrix( cpBody *body, ci::Matrix44<T> &mat )
{
	mat.m[0]=body->rot.x;
	mat.m[1]=body->rot.y;
	mat.m[2]=0;
	mat.m[3]=0;
	
	mat.m[4]=-body->rot.y;
	mat.m[5]=body->rot.x;
	mat.m[6]=0;
	mat.m[7]=0;
	
	mat.m[8]=0;
	mat.m[9]=0;
	mat.m[10]=1.0;
	mat.m[11]=0;
	
	mat.m[12]=body->p.x;
	mat.m[13]=body->p.y;
	mat.m[14]=0;
	mat.m[15]=1;		
}

#pragma mark - containers

typedef std::set< cpShape* > cpShapeSet;
typedef std::vector< cpShape* > cpShapeVec;

typedef std::set< cpBody* > cpBodySet;
typedef std::vector< cpBody* > cpBodyVec;

typedef std::set< cpConstraint* > cpConstraintSet;
typedef std::vector< cpConstraint* > cpConstraintVec;

#pragma mark - destroy macros

inline void cpShapeCleanupAndFree( cpShape **shape )
{
	if ( *shape )
	{
		if ( (*shape)->space_private )
		{
			cpSpaceRemoveShape( (*shape)->space_private, *shape );
		}
		
		cpShapeFree( *shape );
		*shape = NULL;
	}
}

inline void cpBodyCleanupAndFree( cpBody **body )
{
	if ( *body )
	{
		if ((*body)->space_private)
		{
			cpSpaceRemoveBody( (*body)->space_private, *body );
		}
		
		cpBodyFree( *body );
		*body = NULL;
	}		
}

inline void cpConstraintCleanupAndFree( cpConstraint **constraint )
{
	if ( *constraint )
	{
		if ( (*constraint)->space_private )
		{
			cpSpaceRemoveConstraint( (*constraint)->space_private, *constraint );
		}
		
		cpConstraintFree( *constraint );
		*constraint = NULL;
	}
}

inline void cpCleanupAndFree( cpShape *shape ) { cpShapeCleanupAndFree( &shape ); }
inline void cpCleanupAndFree( cpBody *body ) { cpBodyCleanupAndFree( &body ); }
inline void cpCleanupAndFree( cpConstraint *constraint ) { cpConstraintCleanupAndFree( &constraint ); }

template< typename T >
void cpCleanupAndFree( std::set<T> &s )
{
	typedef typename std::set<T>::iterator iterator;
	for( iterator it(s.begin()),end(s.end()); it != end; ++it )
	{
		T obj = *it;
		cpCleanupAndFree( obj );
	}
	
	s.clear();
}

template< typename T >
void cpCleanupAndFree( std::vector<T> &s )
{
	typedef typename std::vector<T>::iterator iterator;
	for( iterator it(s.begin()),end(s.end()); it != end; ++it )
	{
		T obj = *it;
		cpCleanupAndFree( obj );
	}
	
	s.clear();
}

