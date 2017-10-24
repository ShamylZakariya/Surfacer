//
//  UniversalParticleSystem.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 10/23/17.
//

#ifndef UniversalParticleSystem_hpp
#define UniversalParticleSystem_hpp

#include "ParticleSystem.hpp"

namespace particles {
	
	using core::seconds_t;

	/**
	 Interpolator
	 A keyframe-type interpolation but where the values are evenly spaced.
	 If you provide the values, [0,1,0], you'd get a mapping like so:
	 0.00 -> 0
	 0.25 -> 0.5
	 0.50 -> 1
	 0.75 -> 0.5
	 1.00 -> 0
	 */
	template<class T>
	struct interpolator {
	public:
		
		interpolator(const initializer_list<T> values):
		_values(values)
		{}
		
		interpolator(const interpolator<T> &copy):
		_values(copy._values)
		{}
		
		interpolator &operator = (const interpolator<T> &rhs) {
			_values = rhs._values;
			return *this;
		}
		
		// get the interpolated value for a given time `v` from [0 to 1]
		T value(double v) {
			if (v <= 0) {
				return _values.front();
			} else if (v >= 1) {
				return _values.back();
			}
			
			size_t s = _values.size() - 1;
			double dist = v * s;
			double distFloor = floor(dist);
			double distCeil = distFloor + 1;
			size_t aIdx = static_cast<size_t>(distFloor);
			size_t bIdx = aIdx + 1;
			
			T a = _values[aIdx];
			T b = _values[bIdx];
			double factor = (dist - distFloor) / (distCeil - distFloor);
			
			return a + (factor * (b - a));
		}
		
	private:
		
		vector<T> _values;
		
	};
	
//	class UniversalParticleSimulation : public ParticleSimulation {
//	public:
//		
//		struct particle_template {
//			int atlasIdx;
//			seconds_t lifespan;
//			interpolator<double> radius;
//			interpolator<double> damping;
//			interpolator<double> additivity;
//		};
//		
//	};
	
	
}

#endif /* UniversalParticleSystem_hpp */
