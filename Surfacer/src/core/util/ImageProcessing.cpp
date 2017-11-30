//
//  ImageProgressing.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 11/30/17.
//

#include "ImageProcessing.hpp"

#include <queue>

using namespace ci;
namespace core { namespace util { namespace ip {
	
	void dilate(const ci::Channel8u &src, ci::Channel8u &dst, int size) {
		// we want an odd kernel size
		if (size % 2 == 0) {
			size++;
		}
		
		if (dst.getSize() != src.getSize()) {
			dst = Channel8u(src.getWidth(), src.getHeight());
		}

		Channel8u::ConstIter srcIt = src.getIter();
		Channel8u::Iter dstIt = dst.getIter();
		const int hSize = size / 2;
		const int endX = src.getWidth() - 1;
		const int endY = src.getHeight() - 1;
		
		while(srcIt.line() && dstIt.line()) {
			while(srcIt.pixel() && dstIt.pixel()) {
				const int px = srcIt.x();
				const int py = srcIt.y();
				uint8_t maxValue = 0;
				for (int ky = -hSize; ky <= hSize; ky++) {
					if (py + ky < 0 || py + ky > endY) continue;
					
					for (int kx = -hSize; kx <= hSize; kx++) {
						if (px + kx < 0 || px + kx > endX) continue;
						
						maxValue = max(maxValue, srcIt.v(kx, ky));
					}
				}
				
				dstIt.v() = maxValue;
			}
		}
	}
	
	void erode(const Channel8u &src, Channel8u &dst, int size) {
		
		// we want an odd kernel size
		if (size % 2 == 0) {
			size++;
		}

		if (dst.getSize() != src.getSize()) {
			dst = Channel8u(src.getWidth(), src.getHeight());
		}

		Channel8u::ConstIter srcIt = src.getIter();
		Channel8u::Iter dstIt = dst.getIter();

		const int hSize = size / 2;
		const int endX = src.getWidth() - 1;
		const int endY = src.getHeight() - 1;
		
		while(srcIt.line() && dstIt.line()) {
			while(srcIt.pixel() && dstIt.pixel()) {
				const int px = srcIt.x();
				const int py = srcIt.y();
				uint8_t minValue = 255;
				for (int ky = -hSize; ky <= hSize; ky++) {
					if (py + ky < 0 || py + ky > endY) continue;
					
					for (int kx = -hSize; kx <= hSize; kx++) {
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
			dst.copyFrom(src, Area(0,0,src.getWidth(),src.getHeight()));
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
		
		auto get = [&](int px, int py)-> uint8_t {
			return srcData[py * rowBytes + px * increment];
		};
		
		if (get(start.x, start.y) != targetValue) {
			return;
		}
		
		std::queue<ivec2> Q;
		Q.push(start);
		
		while(!Q.empty()) {
			ivec2 coord = Q.front();
			Q.pop();
			
			int32_t west = coord.x;
			int32_t east = coord.x;
			int32_t cy = coord.y;
			
			while(west > 0 && get(west - 1, cy) == targetValue) {
				west--;
			}
			
			while(east < lastX && get(east + 1, cy) == targetValue) {
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

	void remap(Channel8u &channel, uint8_t targetValue, uint8_t newTargetValue, uint8_t defaultValue) {
		
		Channel8u::Iter src = channel.getIter();
		while(src.line()) {
			while(src.pixel()) {
				uint8_t &v = src.v();
				v = v == targetValue ? newTargetValue : defaultValue;
			}
		}
	}
	
}}} // end namespace core::util::ip
