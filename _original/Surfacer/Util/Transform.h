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

template < typename T, glm::precision P = glm::defaultp >
class Transform_
{
	public:
	
	glm::tmat2x2<T, P> rotation;
	glm::tvec2<T, P> position;


	public:
	
		Transform_():
			position(T(0),T(0))
		{}
	
		Transform_( const glm::tvec2<T,P> &p, const glm::tmat2x2<T,P> &r ):
			rotation(r),
			position(p)
		{}
		
		Transform_( const glm::tvec2<T,P> &p, T angle ):
			position(p)
		{
			setAngle( angle );
		}
		
		Transform_( const glm::tmat3x3<T,P> &m3 ):
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
		
		glm::tmat3x3<T,P> mat3() const
		{
			glm::tvec3<T,P>
				c0( rotation.getColumn(0), 0),
				c1( rotation.getColumn(1), 0),
				c2( position, 1 );

			return glm::tmat3x3<T>( c0, c1, c2 );
		}

};

typedef Transform_<real> Transform;

// support Transform_<T> * Vec2<T>
template< typename T, glm::precision P = glm::defaultp  >
inline glm::tvec2<T,P> operator * ( const Transform_<T,P> &t, const glm::tvec2<T,P> &v )
{
	return glm::tvec2<T>(
		t.position.x + t.rotation.m[0] * v.x + t.rotation.m[2] * v.y,
		t.position.y + t.rotation.m[1] * v.x + t.rotation.m[3] * v.y
	);
}

template< typename T, glm::precision P = glm::defaultp  >
inline Transform_<T,P> operator * ( const Transform_<T,P> &t0, const Transform_<T,P> &t1 )
{
	return Transform_<T>( t0.mat3() * t1.mat3() );
}

}} // end namespace core::util

template < typename T, glm::precision P = glm::defaultp  >
inline std::ostream &operator << ( std::ostream &os, const core::util::Transform_<T,P> &t )
{
	T px = std::abs( t.position.x ) > 1e-5 ? t.position.x : 0,
	  py = std::abs( t.position.y ) > 1e-5 ? t.position.y : 0,
	  a  = t.angle() * T(180) / T(M_PI);
	  
	a = std::abs( a ) > 1e-5 ? a : 0;
	return os << "[position (" << px << ", " << py << ") rotation: " << a << "]";

}
