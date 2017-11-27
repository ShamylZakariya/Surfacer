//
//  PerlinWorldTest.cpp
//  Tests
//
//  Created by Shamyl Zakariya on 11/25/17.
//

#include "PerlinWorldTest.hpp"

#include <queue>
#include <cinder/Perlin.h>

#include "App.hpp"
#include "Strings.hpp"
#include "DevComponents.hpp"
#include "PerlinNoise.hpp"


using namespace ci;
using namespace core;

namespace {
	
	Channel8u dilate(const Channel8u &channel, int size) {
		// we want an odd kernel size
		if (size % 2 == 0) {
			size++;
		}
		
		Channel8u result(channel.getWidth(), channel.getHeight());
		Channel8u::ConstIter src = channel.getIter();
		Channel8u::Iter dst = result.getIter();
		const int hSize = size / 2;
		const int endX = channel.getWidth() - 1;
		const int endY = channel.getHeight() - 1;
		
		while(src.line() && dst.line()) {
			while(src.pixel() && dst.pixel()) {
				const int px = src.x();
				const int py = src.y();
				uint8_t maxValue = 0;
				for (int ky = -hSize; ky <= hSize; ky++) {
					if (py + ky < 0 || py + ky > endY) continue;
					
					for (int kx = -hSize; kx <= hSize; kx++) {
						if (px + kx < 0 || px + kx > endX) continue;
						
						maxValue = max(maxValue, src.v(kx, ky));
					}
				}
				
				dst.v() = maxValue;
			}
		}
		
		return result;
	}
	
	Channel8u erode(const Channel8u &channel, int size) {
		// we want an odd kernel size
		if (size % 2 == 0) {
			size++;
		}
		
		Channel8u result(channel.getWidth(), channel.getHeight());
		Channel8u::ConstIter src = channel.getIter();
		Channel8u::Iter dst = result.getIter();
		const int hSize = size / 2;
		const int endX = channel.getWidth() - 1;
		const int endY = channel.getHeight() - 1;
		
		while(src.line() && dst.line()) {
			while(src.pixel() && dst.pixel()) {
				const int px = src.x();
				const int py = src.y();
				uint8_t minValue = 255;
				for (int ky = -hSize; ky <= hSize; ky++) {
					if (py + ky < 0 || py + ky > endY) continue;

					for (int kx = -hSize; kx <= hSize; kx++) {
						if (px + kx < 0 || px + kx > endX) continue;
						
						minValue = min(minValue, src.v(kx, ky));
					}
				}
				
				dst.v() = minValue;
			}
		}
		
		return result;
	}
	
	void floodfill(Channel8u &channel, ivec2 start, uint8_t targetValue, uint8_t newValue) {
		// https://en.wikipedia.org/wiki/Flood_fill
		
		uint8_t *data = channel.getData();
		int32_t rowBytes = channel.getRowBytes();
		int32_t increment = channel.getIncrement();
		int32_t lastX = channel.getWidth() - 1;
		int32_t lastY = channel.getHeight() - 1;
		
		auto set = [&](int px, int py) {
			data[py * rowBytes + px * increment] = newValue;
		};
		
		auto get = [&](int px, int py)-> uint8_t {
			return data[py * rowBytes + px * increment];
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
	
}

PerlinWorldTestScenario::PerlinWorldTestScenario()
{}

PerlinWorldTestScenario::~PerlinWorldTestScenario() {
}

void PerlinWorldTestScenario::setup() {
	setStage(make_shared<Stage>("Perlin World"));
	
	getStage()->addObject(Object::with("ViewportControlComponent", {
		make_shared<ManualViewportControlComponent>(getViewportController())
	}));
	
	auto grid = WorldCartesianGridDrawComponent::create(1);
	grid->setFillColor(ColorA(0.2,0.22,0.25, 1.0));
	grid->setGridColor(ColorA(1,1,1,0.1));
	getStage()->addObject(Object::with("Grid", { grid }));
	
	int size = 1024;
	_buffer = Channel8u(size, size);
	
	
	Channel8u::Iter iter = _buffer.getIter();
	const float width = _buffer.getWidth(), height = _buffer.getHeight();
	
	const float frequency = 32;
	uint8_t pinionMin = 122;
	uint8_t pinionMax = 134;
	
	ci::Perlin pn;
	while(iter.line()) {
		while(iter.pixel()) {
			float noise = pn.fBm(frequency * iter.x() / width, frequency * iter.y() / height);
			noise = noise * 0.5 + 0.5;
			uint8_t b = static_cast<uint8_t>(noise * 255);
			if (b >= pinionMin && b <= pinionMax) {
				b = 0;
			} else {
				b = 255;
			}
			iter.v() = b;
		}
	}
	
	// erode - make the boundary walls stronger
	_buffer = erode(_buffer, 3);

	{
		uint8_t *data = _buffer.getData();
		int32_t rowBytes = _buffer.getRowBytes();
		int32_t increment = _buffer.getIncrement();
		
		auto get = [&](const ivec2 &p)-> uint8_t {
			return data[p.y * rowBytes + p.x * increment];
		};
		
		// now we'll flood fill white regions intersecting rings from center going out
		double ringThickness = 32;
		int ringSteps = ((size / 2) * 0.75) / ringThickness;
		int radsSteps = 90;
		double radsIncrement = 2 * M_PI / radsSteps;
		const ivec2 center(size/2, size/2);
		const uint8_t landValue = 128;
		
		for (int ringStep = 0; ringStep < ringSteps; ringStep++) {
			double radius = ringStep * ringThickness;
			double rads = 0;
			for (int radsStep = 0; radsStep < radsSteps; radsStep++, rads += radsIncrement) {
				double px = center.x + radius * cos(rads);
				double py = center.y + radius * sin(rads);
				ivec2 plot(static_cast<int>(round(px)), static_cast<int>(round(py)));
				if (get(plot) == 255) {
					floodfill(_buffer, plot, 255, landValue);
				}
			}
		}
	}
		
	_tex = ci::gl::Texture2d::create(_buffer);
}

void PerlinWorldTestScenario::cleanup() {
	setStage(nullptr);
}

void PerlinWorldTestScenario::resize( ivec2 size ) {
}

void PerlinWorldTestScenario::step( const time_state &time ) {
}

void PerlinWorldTestScenario::update( const time_state &time ) {
}

void PerlinWorldTestScenario::clear( const render_state &state ) {
	gl::clear( Color( 0.2, 0.2, 0.2 ) );
}

void PerlinWorldTestScenario::draw( const render_state &state ) {
	Scenario::draw(state);
	gl::draw(_tex);
}

void PerlinWorldTestScenario::drawScreen( const render_state &state ) {
}

bool PerlinWorldTestScenario::keyDown( const ci::app::KeyEvent &event ) {
	if (event.getChar() == 'r') {
		reset();
		return true;
	}
	return false;
}

void PerlinWorldTestScenario::reset() {
	cleanup();
	setup();
}
