//
//  ImageProgressing.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 11/30/17.
//

#include "ImageProcessing.hpp"

#include <queue>

using namespace ci;
namespace core {
    namespace util {
        namespace ip {

            namespace {
                typedef std::pair<int, double> kernel_t;
                typedef std::vector<kernel_t> kernel;

                inline void create_kernel(int radius, kernel &k) {
                    k.clear();

                    for (int i = -radius; i <= radius; i++) {
                        int dist = std::abs(i);
                        double mag = 1.0f - (double(dist) / double(radius));
                        k.push_back(kernel_t(i, std::sqrt(mag)));
                    }

                    //
                    // Get sum
                    //

                    double sum = 0;
                    for (size_t i = 0, N = k.size(); i < N; i++) {
                        sum += k[i].second;
                    }

                    //
                    //	normalize
                    //

                    for (size_t i = 0, N = k.size(); i < N; i++) {
                        k[i].second /= sum;
                    }
                }

            }


            void dilate(const ci::Channel8u &src, ci::Channel8u &dst, int radius) {

                if (dst.getSize() != src.getSize()) {
                    dst = Channel8u(src.getWidth(), src.getHeight());
                }

                Channel8u::ConstIter srcIt = src.getIter();
                Channel8u::Iter dstIt = dst.getIter();
                const int endX = src.getWidth() - 1;
                const int endY = src.getHeight() - 1;

                while (srcIt.line() && dstIt.line()) {
                    while (srcIt.pixel() && dstIt.pixel()) {
                        const int px = srcIt.x();
                        const int py = srcIt.y();
                        uint8_t maxValue = 0;
                        for (int ky = -radius; ky <= radius; ky++) {
                            if (py + ky < 0 || py + ky > endY) continue;

                            for (int kx = -radius; kx <= radius; kx++) {
                                if (px + kx < 0 || px + kx > endX) continue;

                                maxValue = max(maxValue, srcIt.v(kx, ky));
                            }
                        }

                        dstIt.v() = maxValue;
                    }
                }
            }

            void erode(const Channel8u &src, Channel8u &dst, int radius) {

                if (dst.getSize() != src.getSize()) {
                    dst = Channel8u(src.getWidth(), src.getHeight());
                }

                Channel8u::ConstIter srcIt = src.getIter();
                Channel8u::Iter dstIt = dst.getIter();

                const int endX = src.getWidth() - 1;
                const int endY = src.getHeight() - 1;

                while (srcIt.line() && dstIt.line()) {
                    while (srcIt.pixel() && dstIt.pixel()) {
                        const int px = srcIt.x();
                        const int py = srcIt.y();
                        uint8_t minValue = 255;
                        for (int ky = -radius; ky <= radius; ky++) {
                            if (py + ky < 0 || py + ky > endY) continue;

                            for (int kx = -radius; kx <= radius; kx++) {
                                if (px + kx < 0 || px + kx > endX) continue;

                                minValue = min(minValue, srcIt.v(kx, ky));
                            }
                        }

                        dstIt.v() = minValue;
                    }
                }
            }

            void floodfill(const ci::Channel8u &src, ci::Channel8u &dst, ivec2 start, uint8_t targetValue, uint8_t newValue, bool copy) {
                // https://en.wikipedia.org/wiki/Flood_fill

                if (dst.getSize() != src.getSize()) {
                    dst = Channel8u(src.getWidth(), src.getHeight());
                }

                if (copy) {
                    dst.copyFrom(src, Area(0, 0, src.getWidth(), src.getHeight()));
                }

                const uint8_t *srcData = src.getData();
                uint8_t *dstData = dst.getData();
                int32_t rowBytes = src.getRowBytes();
                int32_t increment = src.getIncrement();
                int32_t lastX = src.getWidth() - 1;
                int32_t lastY = src.getHeight() - 1;

                auto set = [&](int px, int py) {
                    dstData[py * rowBytes + px * increment] = newValue;
                };

                auto get = [&](int px, int py) -> uint8_t {
                    return srcData[py * rowBytes + px * increment];
                };

                if (get(start.x, start.y) != targetValue) {
                    return;
                }

                std::queue<ivec2> Q;
                Q.push(start);

                while (!Q.empty()) {
                    ivec2 coord = Q.front();
                    Q.pop();

                    int32_t west = coord.x;
                    int32_t east = coord.x;
                    int32_t cy = coord.y;

                    while (west > 0 && get(west - 1, cy) == targetValue) {
                        west--;
                    }

                    while (east < lastX && get(east + 1, cy) == targetValue) {
                        east++;
                    }

                    for (int32_t cx = west; cx <= east; cx++) {
                        set(cx, cy);
                        if (cy > 0 && get(cx, cy - 1) == targetValue) {
                            Q.push(ivec2(cx, cy - 1));
                        }
                        if (cy < lastY && get(cx, cy + 1) == targetValue) {
                            Q.push(ivec2(cx, cy + 1));
                        }
                    }
                }
            }

            void remap(const Channel8u &src, Channel8u &dst, uint8_t targetValue, uint8_t newTargetValue, uint8_t defaultValue) {

                if (dst.getSize() != src.getSize()) {
                    dst = Channel8u(src.getWidth(), src.getHeight());
                }

                Channel8u::ConstIter srcIt = src.getIter();
                Channel8u::Iter dstIt = dst.getIter();

                while (srcIt.line() && dstIt.line()) {
                    while (srcIt.pixel() && dstIt.pixel()) {
                        dstIt.v() = srcIt.v() == targetValue ? newTargetValue : defaultValue;
                    }
                }
            }
      
            void blur(const ci::Channel8u &src, ci::Channel8u &dst, int radius) {
                //
                //    Create the kernel and the buffers
                //
                
                kernel krnl;
                create_kernel(radius, krnl);
                const kernel::const_iterator kend = krnl.end();
                
                ci::Channel8u horizontalPass(src.getWidth(), src.getHeight());
                if (dst.getSize() != src.getSize()) {
                    dst = Channel8u(src.getWidth(), src.getHeight());
                }
                
                //
                //    Run the horizontal pass
                //
                
                {
                    Channel8u::ConstIter srcIt = src.getIter();
                    Channel8u::Iter dstIt = horizontalPass.getIter();
                    
                    while (srcIt.line() && dstIt.line()) {
                        while (srcIt.pixel() && dstIt.pixel()) {
                            
                            double accum = 0;
                            for (kernel::const_iterator k(krnl.begin()); k != kend; ++k) {
                                accum += srcIt.vClamped(k->first, 0) * k->second;
                            }
                            
                            uint8_t v = clamp<uint8_t>(static_cast<uint8_t>(lrint(accum)), 0, 255);
                            dstIt.v() = v;
                        }
                    }
                }
                
                //
                //    Run the vertical pass
                //
                
                {
                    Channel8u::Iter srcIt = horizontalPass.getIter();
                    Channel8u::Iter dstIt = dst.getIter();

                    while (srcIt.line() && dstIt.line()) {
                        while (srcIt.pixel() && dstIt.pixel()) {
                            double accum = 0;
                            for (kernel::const_iterator k(krnl.begin()); k != kend; ++k) {
                                accum += srcIt.vClamped(0, k->first) * k->second;
                            }
                            
                            uint8_t v = clamp<uint8_t>(static_cast<uint8_t>(lrint(accum)), 0, 255);
                            dstIt.v() = v;
                        }
                    }
                }
            }

            void threshold(const ci::Channel8u &src, ci::Channel8u &dst, uint8_t threshV, uint8_t maxV, uint8_t minV) {

                if (dst.getSize() != src.getSize()) {
                    dst = Channel8u(src.getWidth(), src.getHeight());
                }

                Channel8u::ConstIter srcIt = src.getIter();
                Channel8u::Iter dstIt = dst.getIter();

                while (srcIt.line() && dstIt.line()) {
                    while (srcIt.pixel() && dstIt.pixel()) {
                        uint8_t v = srcIt.v();
                        dstIt.v() = v >= threshV ? maxV : minV;
                    }
                }
            }

        }
    }
} // end namespace core::util::ip
