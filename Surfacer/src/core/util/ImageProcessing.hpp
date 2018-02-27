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
#include <cinder/Perlin.h>


namespace core {
    namespace util {
        namespace ip {
            
            // perform a dilation pass such that each pixel in dst image is the max of the pixels in src kernel for that pixel
            void dilate(const ci::Channel8u &src, ci::Channel8u &dst, int radius);

            // perform a dilation pass such that each pixel in dst image is the min of the pixels in src kernel for that pixel
            void erode(const ci::Channel8u &src, ci::Channel8u &dst, int radius);

            // acting on copy of `src, floodfill into `dst
            void floodfill(const ci::Channel8u &src, ci::Channel8u &dst, ivec2 start, uint8_t targetValue, uint8_t newValue, bool copy);

            // remap values in `src which are of value `targetValue to `newTargetValue; all other values are converted to `defaultValue
            void remap(const ci::Channel8u &src, ci::Channel8u &dst, uint8_t targetValue, uint8_t newTargetValue, uint8_t defaultValue);

            // perform blur of size `radius of `src into `dst
            void blur(const ci::Channel8u &src, ci::Channel8u &dst, int radius);

            // for every value in src, maxV if pixelV >= threshV else minV
            void threshold(const ci::Channel8u &src, ci::Channel8u &dst, uint8_t threshV = 128, uint8_t maxV = 255, uint8_t minV = 0);

            // apply vignette effect to src, into dst, where pixels have vignetteColor applied as pixel radius from center approaches outerRadius
            void vignette(const ci::Channel8u &src, ci::Channel8u &dst, double innerRadius, double outerRadius, uint8_t vignetteColor = 0);
            
            
            // perform a dilation pass such that each pixel in result image is the max of the pixels in the kernel
            inline ci::Channel8u dilate(const ci::Channel8u &src, int size) {
                ci::Channel8u dst;
                ::core::util::ip::dilate(src, dst, size);
                return dst;
            }

            // perform a dilation pass such that each pixel in result image is the min of the pixels in the kernel
            inline ci::Channel8u erode(const ci::Channel8u &src, int size) {
                ci::Channel8u dst;
                ::core::util::ip::erode(src, dst, size);
                return dst;
            }

            // remap values in `src which are of value `targetValue to `newTargetValue; all other values are converted to `defaultValue
            inline ci::Channel8u remap(const ci::Channel8u &src, uint8_t targetValue, uint8_t newTargetValue, uint8_t defaultValue) {
                ci::Channel8u dst;
                ::core::util::ip::remap(src, dst, targetValue, newTargetValue, defaultValue);
                return dst;
            }

            // perform blur of size `radius
            inline ci::Channel8u blur(const ci::Channel8u &src, int radius) {
                ci::Channel8u dst;
                ::core::util::ip::blur(src, dst, radius);
                return dst;
            }

            // for every value in src, maxV if pixelV >= threshV else minV
            inline ci::Channel8u threshold(const ci::Channel8u &src, uint8_t threshV = 128, uint8_t maxV = 255, uint8_t minV = 0) {
                ci::Channel8u dst;
                ::core::util::ip::threshold(src, dst, threshV, maxV, minV);
                return dst;
            }
            
            // apply vignette effect to src, where pixels have vignetteColor applied as pixel radius from center approaches outerRadius
            inline ci::Channel8u vignette(const ci::Channel8u &src, double innerRadius, double outerRadius, uint8_t vignetteColor = 0) {
                ci::Channel8u dst;
                ::core::util::ip::vignette(src, dst, innerRadius, outerRadius, vignetteColor);
                return dst;
            }


            namespace in_place {

                // operations which can act on a single channel, in-place, safely
                
                // fill all pixels in channel with a given value
                void fill(ci::Channel8u &channel, uint8_t value);
                
                // fill all pixels of rect in channel with a given value
                void fill(ci::Channel8u &channel, ci::Area rect, uint8_t value);

                // fill all pixels of channel with noise at a given frequency
                void perlin(ci::Channel8u &channel, ci::Perlin &noise, double frequency);
                
                // fill all pixels of channel with noise at a given frequency, where the noise is absoluted and thresholded
                void perlin_abs_thresh(ci::Channel8u &channel, ci::Perlin &noise, double frequency, uint8_t threshold);
                
                // flood fill into channel starting at `start, where pixels of `targetValue are changed to `newValue - modifies `channel
                inline void floodfill(ci::Channel8u &channel, ivec2 start, uint8_t targetValue, uint8_t newValue) {
                    ::core::util::ip::floodfill(channel, channel, start, targetValue, newValue, false);
                }

                // remap values in `src which are of value `targetValue to `newTargetValue; all other values are converted to `defaultValue
                inline void remap(ci::Channel8u &channel, uint8_t targetValue, uint8_t newTargetValue, uint8_t defaultValue) {
                    ::core::util::ip::remap(channel, channel, targetValue, newTargetValue, defaultValue);
                }

                // for every value in src, maxV if pixelV >= threshV else minV
                inline void threshold(ci::Channel8u &channel, uint8_t threshV = 128, uint8_t maxV = 255, uint8_t minV = 0) {
                    ::core::util::ip::threshold(channel, channel, threshV, maxV, minV);
                }
                
                // apply vignette effect to src, where pixels have vignetteColor applied as pixel radius from center approaches outerRadius
                inline void vignette(ci::Channel8u &src, double innerRadius, double outerRadius, uint8_t vignetteColor = 0) {
                    ::core::util::ip::vignette(src, src, innerRadius, outerRadius, vignetteColor);
                }

            }

        }
    }
} // end namespace core::util::ip

#endif /* ImageProcessing_hpp */
