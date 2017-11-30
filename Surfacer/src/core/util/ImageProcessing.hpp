//
//  ImageProgressing.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 11/30/17.
//

#ifndef ImageProcessing_hpp
#define ImageProcessing_hpp

#include "Common.hpp"
#include "MathHelpers.hpp"

#include <cinder/Channel.h>


namespace core { namespace util { namespace ip {
	
	// perform a dilation pass such that each pixel in dst image is the max of the pixels in src kernel for that pixel
	void dilate(const ci::Channel8u &src, ci::Channel8u &dst, int size);

	// perform a dilation pass such that each pixel in result image is the max of the pixels in the kernel
	inline ci::Channel8u dilate(const ci::Channel8u &src, int size) {
		ci::Channel8u dst;
		dilate(src, dst, size);
		return dst;
	}
	
	// perform a dilation pass such that each pixel in dst image is the min of the pixels in src kernel for that pixel
	void erode(const ci::Channel8u &src, ci::Channel8u &dst, int size);

	// perform a dilation pass such that each pixel in result image is the min of the pixels in the kernel
	inline ci::Channel8u erode(const ci::Channel8u &src, int size) {
		ci::Channel8u dst;
		erode(src, dst, size);
		return dst;
	}

	// acting on copy of `src, floodfill into `dst
	void floodfill(const ci::Channel8u &src, ci::Channel8u &dst, ivec2 start, uint8_t targetValue, uint8_t newValue, bool copy);
	
	// flood fill into channel starting at `start, where pixels of `targetValue are changed to `newValue - modifies `channel
	inline void floodfill(ci::Channel8u &channel, ivec2 start, uint8_t targetValue, uint8_t newValue) {
		floodfill(channel, channel, start, targetValue, newValue, false);
	}

	// remap values in `channel which are of value `targetValue to `newTargetValue; all other values are converted to `defaultValue
	void remap(ci::Channel8u &channel, uint8_t targetValue, uint8_t newTargetValue, uint8_t defaultValue);
	
}}} // end namespace core::util::ip

#endif /* ImageProcessing_hpp */
