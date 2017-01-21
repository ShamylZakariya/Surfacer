#pragma once

//
//  FastTrig.h
//  Surfacer
//
//  Created by Zakariya Shamyl on 9/14/11.
//  Copyright 2011 TomorrowPlusX. All rights reserved.
//

namespace core { namespace util { namespace fasttrig {

	namespace _private {

		template <typename T>
		inline T sign( const T s )
		{
			T result;
			if ( s > 0 ) result = T(1);
			else if ( s < 0 ) result = T(-1);
			else result = T(0);
			
			return result;
		}
		
		template <typename T>
		inline T abs( const T s )
		{
			return s < 0 ? -s : s;
		}

		template <typename T>
		inline T normalize( T r )
		{
			// wrap r from -2pi to +2pi
			r = std::fmod( r + T(M_PI), T(2*M_PI)) - T(M_PI);

			// now wrap it from -pi to +pi
			if ( r > T(M_PI) ) return T(-2*M_PI) + r;
			else if ( r < T(-M_PI) ) return T(2*M_PI) + r;

			return r;
		}

	}



	/*
		Implementation adapted from http://glm.g-truc.net/ under the MIT license.
	*/

	template <typename T> 
	inline T sin(T x)
	{
		x = _private::normalize(x);
		return x - ((x * x * x) / T(6)) + ((x * x * x * x * x) / T(120)) - ((x * x * x * x * x * x * x) / T(5040));
	}
	
	template <typename T> 
	inline T cos(T x)
	{
		x = _private::normalize(x);
		return T(1) - (x * x * T(0.5)) + (x * x * x * x * T(0.041666666666)) - (x * x * x * x * x * x * T(0.00138888888888));
	}
	
	template <typename T> 
	inline T tan(T x)
	{
		x = _private::normalize(x);
		return x + (x * x * x * T(0.3333333333)) + (x * x * x * x * x * T(0.1333333333333)) + (x * x * x * x * x * x * x * T(0.0539682539));
	}
	
	template <typename T> 
	inline T asin(const T x)
	{
		return x + (x * x * x * T(0.166666667)) + (x * x * x * x * x * T(0.075)) + (x * x * x * x * x * x * x * T(0.0446428571)) + (x * x * x * x * x * x * x * x * x * T(0.0303819444));// + (x * x * x * x * x * x * x * x * x * x * x * T(0.022372159));
	}
	template <typename T> 
	inline T acos(const T x)
	{
		return T(1.5707963267948966192313216916398) - asin(x); //(PI / 2)
	}
	
	template <typename T> 
	inline T atan(const T x)
	{
		return x - (x * x * x * T(0.333333333333)) + (x * x * x * x * x * T(0.2)) - (x * x * x * x * x * x * x * T(0.1428571429)) + (x * x * x * x * x * x * x * x * x * T(0.111111111111)) - (x * x * x * x * x * x * x * x * x * x * x * T(0.0909090909));
	}
	
	template <typename T> 
	inline T atan(const T y, const T x)
	{
		T sgn = _private::sign(y) * _private::sign(x);
		return _private::abs(atan(y / x)) * sgn;
	}
	
	/*
		unsafe versions which require input radians to be in range [-pi,+pi]
	*/
	
	namespace unsafe {

		template <typename T> 
		inline T sin(const T x)
		{
			return x - ((x * x * x) / T(6)) + ((x * x * x * x * x) / T(120)) - ((x * x * x * x * x * x * x) / T(5040));
		}
		
		template <typename T> 
		inline T cos(const T x)
		{
			return T(1) - (x * x * T(0.5)) + (x * x * x * x * T(0.041666666666)) - (x * x * x * x * x * x * T(0.00138888888888));
		}
		
		template <typename T> 
		inline T tan(const T x)
		{
			return x + (x * x * x * T(0.3333333333)) + (x * x * x * x * x * T(0.1333333333333)) + (x * x * x * x * x * x * x * T(0.0539682539));
		}
		
		
	}
	
	
}}} // end namespace core::util::fasttrig