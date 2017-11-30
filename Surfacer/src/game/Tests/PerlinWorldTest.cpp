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
#include "MarchingCubes.hpp"
#include "ContourSimplification.hpp"
#include "TerrainWorld.hpp"

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
	
	void remap(Channel8u &channel, uint8_t targetValue, uint8_t newTargetValue, uint8_t defaultValue) {
		Channel8u::Iter src = channel.getIter();
		while(src.line()) {
			while(src.pixel()) {
				uint8_t &v = src.v();
				v = v == targetValue ? newTargetValue : defaultValue;
			}
		}
	}
	
	// implements marching_cubes::march voxel store object
	class Channel8uVoxelStoreAdapter {
	public:
		
		Channel8uVoxelStoreAdapter(const Channel8u &s):
		_store(s),
		_min(0,0),
		_max(s.getWidth()-1, s.getHeight()-1)
		{
			_data = _store.getData();
			_rowBytes = _store.getRowBytes();
			_pixelIncrement = _store.getIncrement();
		}
		
		ivec2 min() const { return _min; }
		
		ivec2 max() const { return _max; }

		double valueAt( int x, int y ) const {
			if (x >= _min.x && x <= _max.x && y >= _min.y && y <= _max.y) {
				uint8_t pv = _data[y*_rowBytes + x*_pixelIncrement];
				return static_cast<double>(pv) / 255.0;
			}
			return 0.0;
		}

	private:
		
		Channel8u _store;
		ivec2 _min, _max;
		uint8_t *_data;
		int32_t _rowBytes, _pixelIncrement;
	};
	
	// implements marching_cubes::march segment callback - stitching a soup of short segments into line loops
	struct PerimeterGenerator {
	public:
		
		enum winding {
			CLOCKWISE,
			COUNTER_CLOCKWISE
		};
		
	private:
		
		typedef std::pair< ivec2, ivec2 > Edge;
		
		std::map< ivec2, Edge, ivec2Comparator > _edgesByFirstVertex;
		winding _winding;
		
	public:
		
		PerimeterGenerator( winding w ):
		_winding( w )
		{}
		
		inline void operator()( int x, int y, const marching_squares::segment &seg ) {
			switch( _winding ) {
				case CLOCKWISE: {
					Edge e(scaleUp( seg.a ), scaleUp( seg.b ));
					
					if ( e.first != e.second ) { _edgesByFirstVertex[e.first] = e; }
					break;
				}
					
				case COUNTER_CLOCKWISE: {
					Edge e(scaleUp( seg.b ),
						   scaleUp( seg.a ));
					
					if ( e.first != e.second ) { _edgesByFirstVertex[e.first] = e; }
					break;
				}
			}
		}
		
		/*
		 Populate a vector of PolyLine2d with every perimeter computed for the isosurface
		 Exterior perimeters will be in the current winding direction, interior perimeters
		 will be in the opposite winding.
		 */
		
		size_t generate( std::vector< ci::PolyLine2d > &perimeters, double scale ) {
			perimeters.clear();
			
			while( !_edgesByFirstVertex.empty() ) {
				std::map< ivec2, Edge, ivec2Comparator >::iterator
					begin = _edgesByFirstVertex.begin(),
					end = _edgesByFirstVertex.end(),
					it = begin;
				
				size_t count = _edgesByFirstVertex.size();
				perimeters.push_back( PolyLine2d() );
				
				do {
					perimeters.back().push_back( scaleDown(it->first) * scale );
					_edgesByFirstVertex.erase(it);
					
					it = _edgesByFirstVertex.find(it->second.second);
					count--;
					
				} while( it != begin && it != end && count > 0 );
			}
			
			return perimeters.size();
		}
		
	private:
		
		const double V_SCALE = 256.0;
		
		inline ivec2 scaleUp( const dvec2 &v ) const {
			return ivec2( lrint( V_SCALE * v.x ), lrint( V_SCALE * v.y ) );
		}
		
		inline dvec2 scaleDown( const ivec2 &v ) const {
			return dvec2( static_cast<double>(v.x) / V_SCALE, static_cast<double>(v.y) / V_SCALE );
		}
	};
	
	bool march(const Channel8u &store, double scale, std::vector<PolyLine2d> &optimizedPerimeters) {
		Channel8uVoxelStoreAdapter adapter(store);
		PerimeterGenerator pgen(PerimeterGenerator::CLOCKWISE);
		marching_squares::march(adapter, pgen, 0.5);
		
		// make optimization threshold track the scale of the terrain
		const double PerimeterOptimizationLinearDistanceThreshold = 0.25;
		const double LinearDistanceOptimizationThreshold = PerimeterOptimizationLinearDistanceThreshold * scale;
		
		//
		//	Now, generate our perimeter polylines -- note that since our shapes are
		//	guaranteed closed, and since a map using Vec2iComparator will consider the
		//	first entry to be lowest ordinally, it will represent the outer perimeter
		//
		
		std::vector< PolyLine2d > perimeters;
		if ( pgen.generate( perimeters, scale ) )
		{
			for(auto &perimeter : perimeters) {
				if ( perimeter.size() > 0) {
					if ( LinearDistanceOptimizationThreshold > 0 ) {
						optimizedPerimeters.push_back(util::simplify(perimeter, LinearDistanceOptimizationThreshold));
					}
					else
					{
						optimizedPerimeters.push_back( perimeter );
					}
				}
			}
		}
		
		//
		//	return whether we got any usable perimeters
		//
		
		return optimizedPerimeters.empty() && optimizedPerimeters.front().size() > 0;
	}
	
	vector<terrain::ShapeRef> march(const Channel8u &store, double scale) {
		std::vector<PolyLine2d> soup;
		march(store, scale, soup);
		
		return terrain::Shape::fromContours(soup);
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
	
	ci::Perlin pn;
	while(iter.line()) {
		while(iter.pixel()) {
			float noise = pn.fBm(frequency * iter.x() / width, frequency * iter.y() / height);
			iter.v() = noise < -0.05 || noise > 0.05 ? 255 : 0;
		}
	}
	
	// erode - make the boundary walls stronger
	//_buffer = erode(_buffer, 3);

	//
	// now perform radial samples from inside out, floodfilling
	// white blobs to grey. grey will be our marker for "solid land"
	//
	const uint8_t landValue = 128;
	{
		uint8_t *data = _buffer.getData();
		int32_t rowBytes = _buffer.getRowBytes();
		int32_t increment = _buffer.getIncrement();

		auto get = [&](const ivec2 &p)-> uint8_t {
			return data[p.y * rowBytes + p.x * increment];
		};

		double ringThickness = 16;
		int ringSteps = ((size / 2) * 0.75) / ringThickness;
		const ivec2 center(size/2, size/2);

		for (int ringStep = 0; ringStep < ringSteps; ringStep++) {
			double radius = ringStep * ringThickness;

			double rads = 0;
			int radsSteps = 180 - 5 * ringStep;
			double radsIncrement = 2 * M_PI / radsSteps;

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

	//
	// Now remap grey to white, and everything else to black, and run a dilate pass.
	// Dilate will erase thin walls between white blobs.
	//

	remap(_buffer, landValue, 255, 0);
	_buffer = dilate(_buffer, 9);

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
