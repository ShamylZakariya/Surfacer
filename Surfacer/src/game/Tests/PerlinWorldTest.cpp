//
//  PerlinWorldTest.cpp
//  Tests
//
//  Created by Shamyl Zakariya on 11/25/17.
//

#include "PerlinWorldTest.hpp"

#include <cinder/Perlin.h>

#include "App.hpp"
#include "Strings.hpp"
#include "DevComponents.hpp"
#include "PerlinNoise.hpp"
#include "MarchingCubes.hpp"
#include "ContourSimplification.hpp"
#include "TerrainWorld.hpp"
#include "ImageProcessing.hpp"

using namespace ci;
using namespace core;

namespace {
	
#pragma mark - Constants
	
	const double COLLISION_SHAPE_RADIUS = 0;
	const double MIN_SURFACE_AREA = 2;
	const ci::Color TERRAIN_COLOR(0.2,0.2,0.2);
	const ci::Color ANCHOR_COLOR(1,0,1);
	
	namespace CollisionType {
		
		/*
		 The high 16 bits are a mask, the low are a type_id, the actual type is the logical OR of the two.
		 */
		
		namespace is {
			enum bits {
				SHOOTABLE   = 1 << 16,
				TOWABLE		= 1 << 17
			};
		}
		
		enum type_id {
			TERRAIN				= 1 | is::SHOOTABLE | is::TOWABLE,
			ANCHOR				= 2 | is::SHOOTABLE,
		};
	}
	
	namespace ShapeFilters {
		
		enum Categories {
			_TERRAIN = 1 << 0,
			_GRABBABLE = 1 << 1,
			_ANCHOR = 1 << 2,
		};
		
		cpShapeFilter TERRAIN = cpShapeFilterNew(CP_NO_GROUP, _TERRAIN,			_TERRAIN | _GRABBABLE | _ANCHOR);
		cpShapeFilter ANCHOR= cpShapeFilterNew(CP_NO_GROUP, _ANCHOR,			_TERRAIN);
		cpShapeFilter GRABBABLE= cpShapeFilterNew(CP_NO_GROUP, _GRABBABLE,		_TERRAIN);
	}
	
	namespace DrawLayers {
		enum layer {
			TERRAIN = 1,
		};
	}
	
	const terrain::material TerrainMaterial(1, 0.5, COLLISION_SHAPE_RADIUS, ShapeFilters::TERRAIN, CollisionType::TERRAIN, MIN_SURFACE_AREA, TERRAIN_COLOR);
	terrain::material AnchorMaterial(1, 1, COLLISION_SHAPE_RADIUS, ShapeFilters::ANCHOR, CollisionType::ANCHOR, MIN_SURFACE_AREA, ANCHOR_COLOR);
	
	
#pragma mark - Marching Cubes
	
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
		
		// filter out degenerate polylines
		CI_LOG_D("before removing degenrates, soup.size: " << soup.size());
		soup.erase(std::remove_if(soup.begin(), soup.end(), [](const PolyLine2d &contour)
						  {
							  return contour.size() < 3;
						  }), soup.end());
		CI_LOG_D("after removing degenrates, soup.size: " << soup.size());

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
	
	_buffer = createWorldMap();

	if (/* DISABLES CODE */ (false)) {
		auto terrainWorld = createTerrainWorld(_buffer);
		auto terrain = terrain::TerrainObject::create("Terrain", terrainWorld, DrawLayers::TERRAIN);
		getStage()->addObject(terrain);
	}
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
	
	if (_buffer.getData()) {
		if (!_tex) {
			_tex = ci::gl::Texture2d::create(_buffer);
		}
		gl::draw(_tex);
	}
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

ci::Channel8u PerlinWorldTestScenario::createWorldMap() const {
	
	int size = 1024;
	Channel8u buffer = Channel8u(size, size);

	//
	//	Initilialize our buffer with perlin noise
	//
	
	{
		ci::Perlin pn;
		const float frequency = 16;
		const float width = buffer.getWidth();
		const float height = buffer.getHeight();

		Channel8u::Iter iter = buffer.getIter();
		while(iter.line()) {
			while(iter.pixel()) {
				float noise = pn.fBm(frequency * iter.x() / width, frequency * iter.y() / height);
				iter.v() = noise < -0.05 || noise > 0.05 ? 255 : 0;
			}
		}
	}
	
	//
	// now perform radial samples from inside out, floodfilling
	// white blobs to grey. grey will be our marker for "solid land"
	//
	
	const uint8_t landValue = 128;
	{
		uint8_t *data = buffer.getData();
		int32_t rowBytes = buffer.getRowBytes();
		int32_t increment = buffer.getIncrement();
		
		auto get = [&](const ivec2 &p)-> uint8_t {
			return data[p.y * rowBytes + p.x * increment];
		};
		
		double ringThickness = 16;
		int ringSteps = ((size / 2) * 0.5) / ringThickness;
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
					util::ip::in_place::floodfill(buffer, plot, 255, landValue);
				}
			}
		}
	}
	
	//
	// Remap to make "land" white, and eveything else black. Then a dilate and blur pass
	// to expand land masses to touch.
	//
	
	util::ip::in_place::remap(buffer, landValue, 255, 0);
	buffer = util::ip::dilate(buffer, 4);
	buffer = util::ip::blur(buffer, 15);
	buffer = util::ip::threshold(buffer, 96);

	return buffer;
}

terrain::WorldRef PerlinWorldTestScenario::createTerrainWorld(const ci::Channel8u &worldMap) const {
	// Create shapes
	vector<terrain::ShapeRef> shapes = march(worldMap, 1);

	auto world = make_shared<terrain::World>(getStage()->getSpace(), TerrainMaterial, AnchorMaterial);
	world->build(shapes);
	return world;
}


