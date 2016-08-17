#pragma once

/*
 *  Transform.h
 *  ConvexDecomposition
 *
 *  Created by Shamyl Zakariya on 12/29/10.
 *  Copyright 2010 Shamyl Zakariya. All rights reserved.
 *
 */

#include <cinder/Vector.h>
#include <cinder/Matrix.h>

namespace core { namespace util {

template < class T >
class Transform_
{
	public:
	
		ci::Matrix22<T> rotation;
		ci::Vec2<T> position;


	public:
	
		Transform_():
			position(T(0),T(0))
		{}
	
		Transform_( const ci::Vec2<T> &p, const ci::Matrix22<T> &r ):
			rotation(r),
			position(p)
		{}
		
		Transform_( const ci::Vec2<T> &p, T angle ):
			position(p)
		{
			setAngle( angle );
		}
		
		Transform_( const ci::Matrix33<T> &m3 ):
			rotation( m3.getColumn(0).xy(), m3.getColumn(1).xy() ),
			position( m3.getColumn(2).xy() )
		{}
		
		/**
			set the rotation of the transform, in radians
		*/
		void setAngle(T angle)
		{
			/*
				From Box2D

				identity:
				col1.x = 1.0; col2.x = 0.0;
				col1.y = 0.0; col2.y = 1.0;


				rotation:
				real c = cos(angle), s = sin(angle);
				col1.x = c; col2.x = -s;
				col1.y = s; col2.y = c;

				-----
				
				cinder:
				m[idx]
				| m[0] m[2] |
				| m[1] m[3] |

			*/
		
		
			real c = std::cos(angle), s = std::sin(angle);
			rotation.m[0] = c; rotation.m[2] = -s;
			rotation.m[1] = s; rotation.m[3] = c;
		}

		/**
			get the rotation of this transform, in radians
		*/
		T angle() const 
		{
			return std::atan2( rotation.m[1], rotation.m[0] );
		}
		
		void invert()
		{
			rotation.invert();
			position.x = -position.x;
			position.y = -position.y;
		}
		
		Transform_<T> inverted() const
		{
			Transform_<T> i;
			i.rotation = rotation.inverted();
			i.position.x = -position.x;
			i.position.y = -position.y;
			return i;
		}
		
		ci::Matrix33<T> mat3() const
		{
			ci::Vec3<T> 
				c0( rotation.getColumn(0), 0),
				c1( rotation.getColumn(1), 0),
				c2( position, 1 );

			return ci::Matrix33<T>( c0, c1, c2 );			
		}

};

typedef Transform_<real> Transform;

// support Transform_<T> * Vec2<T>
template< typename T >
inline ci::Vec2<T> operator * ( const Transform_<T> &t, const ci::Vec2<T> &v )
{
	return ci::Vec2<T>(
		t.position.x + t.rotation.m[0] * v.x + t.rotation.m[2] * v.y,
		t.position.y + t.rotation.m[1] * v.x + t.rotation.m[3] * v.y
	);
}

template< typename T >
inline Transform_<T> operator * ( const Transform_<T> &t0, const Transform_<T> &t1 )
{
	return Transform_<T>( t0.mat3() * t1.mat3() );
}

}} // end namespace core::util

template < class T >
inline std::ostream &operator << ( std::ostream &os, const core::util::Transform_<T> &t )
{
	T px = std::abs( t.position.x ) > 1e-5 ? t.position.x : 0,
	  py = std::abs( t.position.y ) > 1e-5 ? t.position.y : 0,
	  a  = t.angle() * T(180) / T(M_PI);
	  
	a = std::abs( a ) > 1e-5 ? a : 0;
	return os << "[position (" << px << ", " << py << ") rotation: " << a << "]";

}
