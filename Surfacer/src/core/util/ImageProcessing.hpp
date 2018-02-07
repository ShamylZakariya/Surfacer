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


namespace core {
    namespace util {
        namespace ip {
            
            struct Channel8uSampler {
            private:
                const ci::Channel8u &channel;
                const uint8_t *bytes;
                int32_t rowBytes;
                int32_t increment;
                int32_t lastCol;
                int32_t lastRow;
                
            public:
                
                Channel8uSampler(const ci::Channel8u &src) :
                channel(src),
                bytes(src.getData()),
                rowBytes(src.getRowBytes()),
                increment(src.getIncrement()),
                lastCol(src.getWidth() - 1),
                lastRow(src.getHeight() - 1) {
                }
                
                inline uint8_t sampleClamped(int x, int y) const {
                    x = min(max(x, 0), lastCol);
                    y = min(max(y, 0), lastRow);
                    return bytes[y * rowBytes + x * increment];
                }
                
                inline uint8_t sampleUnsafe(int x, int y) const {
                    return bytes[y * rowBytes + x * increment];
                }
            };
            
            struct Channel8uWriter {
            private:
                ci::Channel8u &channel;
                uint8_t *bytes;
                int32_t rowBytes;
                int32_t increment;
                int32_t cols;
                int32_t rows;
                
            public:
                
                Channel8uWriter(ci::Channel8u &src) :
                channel(src),
                bytes(src.getData()),
                rowBytes(src.getRowBytes()),
                increment(src.getIncrement()),
                cols(src.getWidth()),
                rows(src.getHeight()) {
                }
                
                inline void writeClamped(int x, int y, uint8_t v) {
                    if (x >= 0 && x <= cols && y >= 0 && y < rows) {
                        bytes[y * rowBytes + x * increment] = v;
                    }
                }
                
                inline void writeUnsafe(int x, int y, uint8_t v) {
                    bytes[y * rowBytes + x * increment] = v;
                }
            };

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

            namespace in_place {

                // operations which can act on a single channel, in-place, safely

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

            }

        }
    }
} // end namespace core::util::ip

#endif /* ImageProcessing_hpp */
