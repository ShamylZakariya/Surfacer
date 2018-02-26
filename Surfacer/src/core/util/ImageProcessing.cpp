//
//  ImageProgressing.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 11/30/17.
//

#include "ImageProcessing.hpp"

#include <queue>
#include <thread>

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
                
                size_t _threads = 0;
                size_t get_num_threads() {
                    if (_threads == 0) {
                        _threads = std::thread::hardware_concurrency();
                        CI_LOG_D("_threads: " << _threads);
                    }
                    return _threads;
                }

                Area get_thread_working_area(int channelWidth, int channelHeight, size_t threadIdx) {
                    size_t count = get_num_threads();
                    return Area(0, static_cast<int>(threadIdx * channelHeight / count), channelWidth, static_cast<int>((threadIdx+1) * channelHeight / count));
                }
            }
            
            namespace {
                
                void dilate_area(const ci::Channel8u &src, ci::Channel8u &dst, Area area, int radius) {
                    Channel8u::ConstIter srcIt = src.getIter(area);
                    Channel8u::Iter dstIt = dst.getIter(area);
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

            }

            void dilate(const ci::Channel8u &src, ci::Channel8u &dst, int radius) {

                if (dst.getSize() != src.getSize()) {
                    dst = Channel8u(src.getWidth(), src.getHeight());
                }
                
                const size_t threadCount = get_num_threads();
                if (threadCount > 1) {
                    vector<std::thread> threads;
                    for (size_t idx = 0; idx < threadCount; idx++) {
                        Area workingArea = get_thread_working_area(src.getWidth(), src.getHeight(), idx);
                        threads.emplace_back(std::thread(&dilate_area, std::ref(src), std::ref(dst), workingArea, radius));
                    }
                    
                    for(auto &t : threads) { t.join(); }
                    
                } else {
                    dilate_area(src, dst, src.getBounds(), radius);
                }

            }
            
            namespace {
                
                void erode_area(const Channel8u &src, Channel8u &dst, Area area, int radius) {
                    Channel8u::ConstIter srcIt = src.getIter(area);
                    Channel8u::Iter dstIt = dst.getIter(area);
                    
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
                
            }

            void erode(const Channel8u &src, Channel8u &dst, int radius) {

                if (dst.getSize() != src.getSize()) {
                    dst = Channel8u(src.getWidth(), src.getHeight());
                }

                const size_t threadCount = get_num_threads();
                if (threadCount > 1) {
                    vector<std::thread> threads;
                    for (size_t idx = 0; idx < threadCount; idx++) {
                        Area workingArea = get_thread_working_area(src.getWidth(), src.getHeight(), idx);
                        threads.emplace_back(std::thread(&erode_area, std::ref(src), std::ref(dst), workingArea, radius));
                    }
                    
                    for(auto &t : threads) { t.join(); }
                    
                } else {
                    erode_area(src, dst, src.getBounds(), radius);
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
            
            namespace {
                
                void remap_area(const Channel8u &src, Channel8u &dst, Area area, uint8_t targetValue, uint8_t newTargetValue, uint8_t defaultValue) {
                    Channel8u::ConstIter srcIt = src.getIter(area);
                    Channel8u::Iter dstIt = dst.getIter(area);
                    
                    while (srcIt.line() && dstIt.line()) {
                        while (srcIt.pixel() && dstIt.pixel()) {
                            dstIt.v() = (srcIt.v() == targetValue ? newTargetValue : defaultValue);
                        }
                    }
                }
                
            }

            void remap(const Channel8u &src, Channel8u &dst, uint8_t targetValue, uint8_t newTargetValue, uint8_t defaultValue) {

                if (dst.getSize() != src.getSize()) {
                    dst = Channel8u(src.getWidth(), src.getHeight());
                }
                
                const size_t threadCount = get_num_threads();
                if (threadCount > 1) {
                    vector<std::thread> threads;
                    for (size_t idx = 0; idx < threadCount; idx++) {
                        Area workingArea = get_thread_working_area(src.getWidth(), src.getHeight(), idx);
                        threads.emplace_back(std::thread(&remap_area, std::ref(src), std::ref(dst), workingArea, targetValue, newTargetValue, defaultValue));
                    }

                    for(auto &t : threads) { t.join(); }

                } else {
                    remap_area(src, dst, src.getBounds(), targetValue, newTargetValue, defaultValue);
                }
            }
            
            namespace {
                
                void blur_horizontal(const ci::Channel8u &src, ci::Channel8u &horizontalPass, Area area, const kernel &krnl) {
                    Channel8u::ConstIter srcIt = src.getIter(area);
                    Channel8u::Iter dstIt = horizontalPass.getIter(area);
                    const kernel::const_iterator kend = krnl.end();

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
                
                void blur_vertical(const ci::Channel8u &horizontalPass, ci::Channel8u &dst, Area area, const kernel &krnl) {
                    Channel8u::ConstIter srcIt = horizontalPass.getIter(area);
                    Channel8u::Iter dstIt = dst.getIter(area);
                    const kernel::const_iterator kend = krnl.end();
                    const int lastY = dst.getHeight()-1;
                    while (srcIt.line() && dstIt.line()) {
                        while (srcIt.pixel() && dstIt.pixel()) {
                            double accum = 0;
                            for (kernel::const_iterator k(krnl.begin()); k != kend; ++k) {
                                
                                // clamp our offset to stay between [0,height)
                                int y = srcIt.y() + k->first;
                                y = clamp(y, 0, lastY);
                                int offset = y - srcIt.y();

                                accum += srcIt.v(0, offset) * k->second;
                            }
                            
                            uint8_t v = clamp<uint8_t>(static_cast<uint8_t>(lrint(accum)), 0, 255);
                            dstIt.v() = v;
                        }
                    }
                }
                
            }
            
            void blur(const ci::Channel8u &src, ci::Channel8u &dst, int radius) {
                //
                //    Create the kernel and the buffers
                //
                
                kernel krnl;
                create_kernel(radius, krnl);
                
                ci::Channel8u horizontalPass(src.getWidth(), src.getHeight());
                if (dst.getSize() != src.getSize()) {
                    dst = Channel8u(src.getWidth(), src.getHeight());
                }
                
                const size_t threadCount = get_num_threads();
                if (threadCount > 1) {
                    vector<std::thread> threads;
                    for (size_t idx = 0; idx < threadCount; idx++) {
                        Area workingArea = get_thread_working_area(src.getWidth(), src.getHeight(), idx);
                        threads.emplace_back(std::thread(&blur_horizontal, std::ref(src), std::ref(horizontalPass), workingArea, std::ref(krnl)));
                    }
                    
                    for(auto &t : threads) { t.join(); }
                    threads.clear();

                    for (size_t idx = 0; idx < threadCount; idx++) {
                        Area workingArea = get_thread_working_area(src.getWidth(), src.getHeight(), idx);
                        threads.emplace_back(std::thread(&blur_vertical, std::ref(horizontalPass), std::ref(dst), workingArea, std::ref(krnl)));
                    }
                    
                    for(auto &t : threads) { t.join(); }
                    
                } else {
                    const auto bounds(src.getBounds());
                    blur_horizontal(src, horizontalPass, bounds, krnl);
                    blur_vertical(horizontalPass, dst, bounds, krnl);
                }
            }
            
            namespace {

                void threshold_area(const ci::Channel8u &src, ci::Channel8u &dst, Area area, uint8_t threshV, uint8_t maxV, uint8_t minV) {
                    Channel8u::ConstIter srcIt = src.getIter(area);
                    Channel8u::Iter dstIt = dst.getIter(area);
                    
                    while (srcIt.line() && dstIt.line()) {
                        while (srcIt.pixel() && dstIt.pixel()) {
                            uint8_t v = srcIt.v();
                            dstIt.v() = v >= threshV ? maxV : minV;
                        }
                    }
                }

            }

            void threshold(const ci::Channel8u &src, ci::Channel8u &dst, uint8_t threshV, uint8_t maxV, uint8_t minV) {

                if (dst.getSize() != src.getSize()) {
                    dst = Channel8u(src.getWidth(), src.getHeight());
                }
                
                const size_t threadCount = get_num_threads();
                if (threadCount > 1) {
                    vector<std::thread> threads;
                    for (size_t idx = 0; idx < threadCount; idx++) {
                        Area workingArea = get_thread_working_area(src.getWidth(), src.getHeight(), idx);
                        threads.emplace_back(std::thread(&threshold_area, std::ref(src), std::ref(dst), workingArea, threshV, maxV, minV));
                    }
                    
                    for(auto &t : threads) { t.join(); }
                    
                } else {
                    threshold_area(src, dst, src.getBounds(), threshV, maxV, minV);
                }

            }
            
            namespace in_place {
                
                void fill(ci::Channel8u &channel, uint8_t value) {
                    Channel8u::Iter it = channel.getIter();
                    
                    while(it.line()) {
                        while (it.pixel()) {
                            it.v() = value;
                        }
                    }
                }
                
                void fill(ci::Channel8u &channel, ci::Area rect, uint8_t value) {
                    Channel8u::Iter it = channel.getIter(rect);
                    
                    while(it.line()) {
                        while (it.pixel()) {
                            it.v() = value;
                        }
                    }
                }
                
                namespace {
                    void perlin_area(ci::Channel8u &channel, Area area, ci::Perlin &noise, double frequency) {
                        Channel8u::Iter iter = channel.getIter(area);
                        
                        while (iter.line()) {
                            while (iter.pixel()) {
                                float v = noise.fBm(frequency * iter.x(), frequency * iter.y());
                                iter.v() = static_cast<uint8_t>(255.f * (v * 0.5f + 0.5f));
                            }
                        }
                    }
                }
                
                void perlin(ci::Channel8u &channel, ci::Perlin &noise, double frequency) {
                    const size_t threadCount = get_num_threads();
                    if (threadCount > 1) {
                        vector<std::thread> threads;
                        for (size_t idx = 0; idx < threadCount; idx++) {
                            Area workingArea = get_thread_working_area(channel.getWidth(), channel.getHeight(), idx);
                            threads.emplace_back(std::thread(&perlin_area, std::ref(channel), workingArea, std::ref(noise), frequency));
                        }
                        
                        for(auto &t : threads) { t.join(); }
                        
                    } else {
                        perlin_area(channel, channel.getBounds(), noise, frequency);
                    }
                }
                
                namespace {
                    void perlin_abs_thresh_area(ci::Channel8u &channel, Area area, ci::Perlin &noise, double frequency, uint8_t threshold) {
                        Channel8u::Iter iter = channel.getIter(area);
                        
                        while (iter.line()) {
                            while (iter.pixel()) {
                                float v = noise.fBm(frequency * iter.x(), frequency * iter.y());
                                v = abs(v);
                                uint8_t pv = static_cast<uint8_t>(255.f * abs(v));
                                iter.v() = pv >= threshold ? 255 : 0;
                            }
                        }
                    }
                }
                
                void perlin_abs_thresh(ci::Channel8u &channel, ci::Perlin &noise, double frequency, uint8_t threshold) {
                    const size_t threadCount = get_num_threads();
                    if (threadCount > 1) {
                        vector<std::thread> threads;
                        for (size_t idx = 0; idx < threadCount; idx++) {
                            Area workingArea = get_thread_working_area(channel.getWidth(), channel.getHeight(), idx);
                            threads.emplace_back(std::thread(&perlin_abs_thresh_area, std::ref(channel), workingArea, std::ref(noise), frequency, threshold));
                        }
                        
                        for(auto &t : threads) { t.join(); }
                        
                    } else {
                        perlin_abs_thresh_area(channel, channel.getBounds(), noise, frequency, threshold);
                    }
                }



            }

        }
    }
} // end namespace core::util::ip
