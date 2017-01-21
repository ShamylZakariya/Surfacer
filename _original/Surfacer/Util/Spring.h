#pragma once

/*
 *  Spring.h
 *  LevelLoading
 *
 *  Created by Shamyl Zakariya on 2/6/11.
 *  Copyright 2011 Shamyl Zakariya. All rights reserved.
 *
 */
 
#include <algorithm>
#include "Common.h"

namespace core { namespace util {

/**
	@class Spring
	@ingroup util
	
	@brief Models a simple spring system, where one end is fixed, and the other
	has a weight attached to it. Springs have control over corrective force,
	the mass of the loose end, and dampening.
		
	A Spring can be useful for modeling camera control -- e.g., one might
	attach one end of the spring to the player, and the other to the camera.
	As the player moves, the camera will smoothly follow.	
	
	Also, Springs can be used to dampen vibration, but are better for implementing
	loose tracking. A Spring can be thought of as a heavier, more precise
	alternative to Accumulator.
*/

template < class T >
class Spring
{
	public:
	
		/**
			Create a spring. Default mass is 1, default spring force is 1, and default dampening is 0.
		*/

		Spring( T zero = T() ):
			_target(zero), _value(zero), _velocity(zero), _zero( zero )
		{
			setMass( 1.0 );
			setForce( 1.0 );
			setDampening( 0.0 );
		}
		
		Spring( const Spring &c ):
			_target( c._target ),
			_value( c._value ),
			_velocity( c._velocity ),
			_zero( c._zero ),
			_mass( c._mass ),
			_force( c._force ),
			_dampening( c._dampening )
		{}
				
		Spring &operator = ( const Spring &rhs )
		{
			_target = rhs._target;
			_value = rhs._value;
			_velocity = rhs._velocity;
			_zero = rhs._zero;
			
			_mass = rhs._mass;
			_force = rhs._force;
			_dampening = rhs._dampening;
			
			return *this;
		}
	
		/**
			Create a spring and assign mass, spring force and dampening.
			If dampening is zero, the spring will never converge on its target.
		*/

		Spring( real mass, real force, real dampening, T zero = T() ):
			_target( zero ), _value( zero ), _velocity( zero ), _zero( zero )
		{
			setMass( mass );
			setForce( force );
			setDampening( dampening );
		}
		
		/**
			@return the minimum mass allowed for this spring system. 
			Default returns 0.001f;
		*/
		static real minimumMass( void ) { return 0.001; }
		
		/**
			Assign mass, spring force and dampening.
			If dampening is zero, the spring will never converge on its target.
		*/
		void set( real mass, real force, real dampening )
		{
			setMass( mass );
			setForce( force );
			setDampening( dampening );
		}

		/**
			Set the spring's attachment point. E.g., if set to the origin,
			the value of the spring will converge over time to meet the origin.
		*/
		void setTarget( T target ) { _target = target; }
		
		/**
			@return the target value to spring is aiming for
		*/
		T target( void ) const { return _target; }

		/**
			Set the current position of the weight. This will set automatically
			zero the velocity.
		*/
		void setValue( T v ) { _value = v; _velocity = _zero; }
		
		/**
			@return the spring's current value
		*/
		T value( void ) const { return _value; }

		/**
			Set the mass of the weight. It will be clamped to be at least Spring::minimumMass()
		*/
		void setMass( real m ) { _mass = std::max( m, minimumMass() ); }
		
		/**
			@return the spring's mass
		*/
		real mass( void ) const { return _mass; }
		
		/**
			Set the force the spring applies.
		*/
		void setForce( real f ) { _force = std::max( f, minimumMass() ); }
		
		/**
			@return the spring's force
		*/
		real force( void ) const { return _force; }
		
		/**
			Set the dampening force. The value must be between ( inclusively ) zero and one.
		*/
		void setDampening( real d ) { _dampening = std::max( std::min( d, real(1) ), real(0) ); }
		
		/**
			@return the spring's dampening
		*/
		real dampening( void ) const { return _dampening; }
		
		/**
			Set the current velocity - this is useful if you want to 'pluck' the spring to emulate a bounce or whatever
		*/
		void setVelocity( T vel ) { _velocity = vel; }
		
		/**
			@return the spring's current velocity
		*/
		T velocity( void ) const { return _velocity; }
		
		/**
			@return True if the spring has converged on its target -- convergence
			is being defined here as the distance between the spring's
			position and target being less than @a epsilon
			and the velocity being less than or equal to @a epsilon
		*/
		bool converged( real epsilon = 0.001f ) const 
		{
			return distance( _velocity, 0.0 ) <= epsilon &&
			       distance( _value, _target ) <= epsilon;
				   
			//return _velocity <= epsilon && ( distance( _value, _zero ) < epsilon );
		}
		
		/**
			Update the spring; must be called continuously to 
			accurately calculate position.
			
			@return the updated value of the spring.
		*/
		T update( seconds_t deltaT )
		{
			if ( deltaT < Epsilon ) return _value;
		
			T error = _target - _value;
			T correctiveForce = error * _force;
			T correctiveAcceleration = correctiveForce / _mass;

			_velocity += correctiveAcceleration * deltaT;
			
			if ( _dampening > Epsilon )
			{
				_velocity = _velocity * ( 1.0 - _dampening );
			}
						
			_value = _value + _velocity / deltaT;
			return _value;
		}
		
	protected:
	
		real _mass, _force, _dampening;
		T _target, _value, _velocity, _zero;
		
};

}} // end namespace core::util