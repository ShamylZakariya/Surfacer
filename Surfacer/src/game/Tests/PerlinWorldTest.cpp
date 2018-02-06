//
//  PerlinWorldTest.cpp
//  Tests
//
//  Created by Shamyl Zakariya on 11/25/17.
//

#include "PerlinWorldTest.hpp"

#include <cinder/Perlin.h>

#include "DevComponents.hpp"
#include "PerlinNoise.hpp"
#include "MarchingSquares.hpp"
#include "ContourSimplification.hpp"
#include "TerrainDetail.hpp"
#include "TerrainDetail_MarchingSquares.hpp"
#include "ImageProcessing.hpp"

#include "PlanetGenerator.hpp"

using namespace ci;
using namespace core;

namespace {

#pragma mark - Constants

    const double COLLISION_SHAPE_RADIUS = 0;
    const double MIN_SURFACE_AREA = 2;
    const ci::Color TERRAIN_COLOR(1, 0, 1);
    const ci::Color ANCHOR_COLOR(0, 1, 1);

    namespace CollisionType {

        /*
         The high 16 bits are a mask, the low are a type_id, the actual type is the logical OR of the two.
         */

        namespace is {
            enum bits {
                SHOOTABLE = 1 << 16,
                TOWABLE = 1 << 17
            };
        }

        enum type_id {
            TERRAIN = 1 | is::SHOOTABLE | is::TOWABLE,
            ANCHOR = 2 | is::SHOOTABLE,
        };
    }

    namespace ShapeFilters {

        enum Categories {
            _TERRAIN = 1 << 0,
            _GRABBABLE = 1 << 1,
            _ANCHOR = 1 << 2,
        };

        cpShapeFilter TERRAIN = cpShapeFilterNew(CP_NO_GROUP, _TERRAIN, _TERRAIN | _GRABBABLE | _ANCHOR);
        cpShapeFilter ANCHOR = cpShapeFilterNew(CP_NO_GROUP, _ANCHOR, _TERRAIN);
        cpShapeFilter GRABBABLE = cpShapeFilterNew(CP_NO_GROUP, _GRABBABLE, _TERRAIN);
    }

    namespace DrawLayers {
        enum layer {
            TERRAIN = 1,
        };
    }

    const terrain::material TerrainMaterial(1, 0.5, COLLISION_SHAPE_RADIUS, ShapeFilters::TERRAIN, CollisionType::TERRAIN, MIN_SURFACE_AREA, TERRAIN_COLOR);
    terrain::material AnchorMaterial(1, 1, COLLISION_SHAPE_RADIUS, ShapeFilters::ANCHOR, CollisionType::ANCHOR, MIN_SURFACE_AREA, ANCHOR_COLOR);

#pragma mark -
    
    void fill_rect(Channel8u &c, ci::Area region, uint8_t value) {
        Area clippedRegion(max(region.getX1(), 0), max(region.getY1(), 0), min(region.getX2(), c.getWidth() - 1), min(region.getY2(), c.getHeight() - 1));
        uint8_t *data = c.getData();
        int32_t rowBytes = c.getRowBytes();
        int32_t increment = c.getIncrement();
        
        for (int y = clippedRegion.getY1(), yEnd = clippedRegion.getY2(); y <= yEnd; y++) {
            uint8_t *row = &data[y * rowBytes];
            for (int x = clippedRegion.getX1(), xEnd = clippedRegion.getX2(); x <= xEnd; x++) {
                row[x * increment] = value;
            }
        }
    }
    
    ci::Channel8u create_simple_test_map() {
        int size = 32;
        Channel8u buffer = Channel8u(size, size);
        fill_rect(buffer, Area(0, 0, size, size), 0);
        fill_rect(buffer, Area(6, 6, size - 6, size - 6), 255);
        fill_rect(buffer, Area(12, 12, size - 12, size - 12), 0);
        fill_rect(buffer, Area(14, 14, size - 14, size - 14), 255);
        fill_rect(buffer, Area(0, 0, 5, 5), 255);
        
        return util::ip::blur(buffer, 2);
    }
}

/*
 float _surfaceSolidity;
 int32_t _seed;
 
 vector<ci::Channel8u> _isoSurfaces;
 vector<ci::gl::Texture2dRef> _isoTexes;
 vector <segment> _marchSegments;
 vector <polyline> _marchedPolylines;
*/

PerlinWorldTestScenario::PerlinWorldTestScenario() :
    _seed(1234),
    _surfaceSolidity(0.5)
{
}

PerlinWorldTestScenario::~PerlinWorldTestScenario() {
}

void PerlinWorldTestScenario::setup() {
    setStage(make_shared<Stage>("Perlin World"));

    getStage()->addObject(Object::with("ViewportControlComponent", {
            make_shared<ManualViewportControlComponent>(getViewportController())
    }));

    auto grid = WorldCartesianGridDrawComponent::create(1);
    grid->setFillColor(ColorA(0.2, 0.22, 0.25, 1.0));
    grid->setGridColor(ColorA(1, 1, 1, 0.1));
    getStage()->addObject(Object::with("Grid", {grid}));


    auto params = precariously::planet_generation::params(512, _seed).defaultCenteringTransform(4);
    params.noiseOctaves = 2;
    params.noiseFrequencyScale = 2;
    params.pruneFloaters = true;
    params.surfaceSolidity = _surfaceSolidity;
    
    auto result = precariously::planet_generation::generate(params, getStage()->getSpace(), TerrainMaterial, AnchorMaterial);
    
    auto terrain = terrain::TerrainObject::create("Terrain", result.first, DrawLayers::TERRAIN);
    getStage()->addObject(terrain);
    
    _isoSurfaces = vector<Channel8u> { result.second.first, result.second.second };
    
    const auto fmt = ci::gl::Texture2d::Format().mipmap(false).minFilter(GL_NEAREST).magFilter(GL_NEAREST);
    for(Channel8u isoSurface : _isoSurfaces) {
        _isoTexes.push_back(ci::gl::Texture2d::create(isoSurface, fmt));
    }
    
}

void PerlinWorldTestScenario::cleanup() {
    setStage(nullptr);
    _isoSurfaces.clear();
    _isoTexes.clear();
}

void PerlinWorldTestScenario::resize(ivec2 size) {
}

void PerlinWorldTestScenario::step(const time_state &time) {
}

void PerlinWorldTestScenario::update(const time_state &time) {
}

void PerlinWorldTestScenario::clear(const render_state &state) {
    gl::clear(Color(0.2, 0.2, 0.2));
}

void PerlinWorldTestScenario::draw(const render_state &state) {
    Scenario::draw(state);

    if ((true)) {
        gl::ScopedBlend sb(true);
        gl::ScopedModelMatrix smm;
        
        if (!_marchSegments.empty()) {
            double triangleSize = 0.25;
            for (const auto &seg : _marchSegments) {
                gl::color(seg.color);
                gl::drawLine(seg.a, seg.b);
                dvec2 mid = (seg.a + seg.b) * 0.5;
                dvec2 dir = normalize(seg.b - seg.a);
                dvec2 left = rotateCCW(dir);
                dvec2 right = -left;
                gl::drawLine(mid + left * triangleSize, seg.b);
                gl::drawLine(mid + right * triangleSize, seg.b);
            }
        }

        if (!_marchedPolylines.empty()) {
            for (const auto &pl : _marchedPolylines) {
                gl::color(pl.color);
                gl::draw(pl.pl);
            }
        }
    }
}

void PerlinWorldTestScenario::drawScreen(const render_state &state) {
    {
        gl::ScopedModelMatrix smm;
        gl::scale(0.25, 0.25, 1);
        int x = 0;
        for (auto tex : _isoTexes) {
            gl::color(ColorA(1, 1, 1, 1));
            gl::translate(vec3(x, 0, 0));
            gl::draw(tex);
            x += tex->getWidth();
        }
    }
}

bool PerlinWorldTestScenario::keyDown(const ci::app::KeyEvent &event) {
    
    switch(event.getChar()) {
        case 'r':
            reset();
            return true;
        case 's':
            _seed++;
            CI_LOG_D("_seed: " << _seed);
            reset();
            return true;
        case '-':
            _surfaceSolidity = max<float>(_surfaceSolidity - 0.1, 0);
            CI_LOG_D("_surfaceSolidity: " << _surfaceSolidity);
            reset();
            return true;
        case '=':
            _surfaceSolidity = min<float>(_surfaceSolidity + 0.1, 1);
            CI_LOG_D("_surfaceSolidity: " << _surfaceSolidity);
            reset();
            return true;
    }

    return false;
}

void PerlinWorldTestScenario::reset() {
    cleanup();
    setup();
}

vector <PerlinWorldTestScenario::polyline> PerlinWorldTestScenario::marchToPerimeters(ci::Channel8u &iso, size_t expectedContours) const {
    
    std::vector<PolyLine2d> polylines = terrain::detail::march(iso, 0.5, dmat4(), 0.01);

    if (expectedContours > 0) {
        CI_ASSERT_MSG(polylines.size() == expectedContours, "Expected correct number of generated contours");
    }

    CI_LOG_D("Generated " << polylines.size() << " contours");

    ci::Rand rng;
    vector <polyline> ret;
    for (const auto &pl : polylines) {
        CI_LOG_D("polyline: " << ret.size() << " num vertices: " << pl.size());
        ret.push_back({terrain::detail::polyline2d_to_2f(pl), ci::Color(CM_HSV, rng.nextFloat(), 0.7, 0.7)});
    }

    return ret;
}

vector <PerlinWorldTestScenario::segment> PerlinWorldTestScenario::testMarch(ci::Channel8u &iso) const {

    struct segment_consumer {
        ci::Rand rng;
        vector <segment> segments;

        void operator()(int x, int y, const marching_squares::segment &seg) {
            segments.push_back({seg.a, seg.b, ci::Color(CM_HSV, rng.nextFloat(), 0.7, 0.7)});
        }
    } sc;

    const double isoLevel = 0.5;
    marching_squares::march(terrain::detail::Channel8uVoxelStoreAdapter(iso), sc, isoLevel);

    CI_LOG_D("testMarch - generated: " << sc.segments.size() << " unordered segments");

    return sc.segments;
}
