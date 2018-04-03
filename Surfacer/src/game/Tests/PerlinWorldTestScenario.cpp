//
//  PerlinWorldTest.cpp
//  Tests
//
//  Created by Shamyl Zakariya on 11/25/17.
//

#include "PerlinWorldTestScenario.hpp"

#include <cinder/Perlin.h>

#include "App.hpp"
#include "ParticleSystem.hpp"
#include "DevComponents.hpp"
#include "PerlinNoise.hpp"
#include "MarchingSquares.hpp"
#include "ContourSimplification.hpp"
#include "Terrain.hpp"
#include "TerrainDetail.hpp"
#include "TerrainDetail_MarchingSquares.hpp"
#include "ImageProcessing.hpp"

#include "PlanetGenerator.hpp"
#include "PlanetGreebling.hpp"

using namespace ci;
using namespace core;

namespace {

#pragma mark - Constants

    const double COLLISION_SHAPE_RADIUS = 0;
    const double MIN_SURFACE_AREA = 2;
    const ci::Color TERRAIN_COLOR(0.3, 0.3, 0.3);
    const ci::Color ANCHOR_COLOR(0, 0, 0);

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
            ATTACHMENTS = 2,
            GREEBLING = 3
        };
    }

    const terrain::material TerrainMaterial(1, 0.5, COLLISION_SHAPE_RADIUS, ShapeFilters::TERRAIN, CollisionType::TERRAIN, MIN_SURFACE_AREA, TERRAIN_COLOR);
    terrain::material AnchorMaterial(1, 1, COLLISION_SHAPE_RADIUS, ShapeFilters::ANCHOR, CollisionType::ANCHOR, MIN_SURFACE_AREA, ANCHOR_COLOR);

}

/*
 float _surfaceSolidity;
 int32_t _seed;
 
 vector<ci::Channel8u> _isoSurfaces;
 vector<ci::gl::Texture2dRef> _isoTexes;
 vector <segment> _marchSegments;
 vector <polyline> _marchedPolylines;
 
 TerrainCutRecorder _terrainCutRecorder;
 bool _recordCuts;
 bool _playingBackRecordedCuts;
*/

PerlinWorldTestScenario::PerlinWorldTestScenario() :
    _seed(1234),
    _surfaceSolidity(0.5),
    _recordCuts(true),
    _playingBackRecordedCuts(false)
{
}

PerlinWorldTestScenario::~PerlinWorldTestScenario() {
}

void PerlinWorldTestScenario::setup() {
    setStage(make_shared<Stage>("Perlin World"));

    auto params = precariously::planet_generation::params(512).defaultCenteringTransform(4);
    
    params.terrain.seed = _seed;
    params.terrain.noiseOctaves = 2;
    params.terrain.noiseFrequencyScale = 2;
    params.terrain.pruneFloaters = false;
    params.terrain.partitionSize = 100;
    params.terrain.surfaceSolidity = _surfaceSolidity;
    params.terrain.material = TerrainMaterial;
    
    params.anchors.seed = _seed + 1;
    params.anchors.noiseOctaves = 2;
    params.anchors.noiseFrequencyScale = 2;
    params.anchors.surfaceSolidity = _surfaceSolidity;
    params.anchors.vignetteStart *= 0.5;
    params.anchors.vignetteEnd *= 0.5;
    params.anchors.material = AnchorMaterial;

    if ((true)) {
        auto ap = precariously::planet_generation::params::perimeter_attachment_params(0);
        ap.batchId = 0;
        ap.normalToUpDotTarget = 1;
        ap.normalToUpDotRange = 1;
        ap.probability = 0.875;
        ap.density = 1;
        ap.includeHoleContours = true;
        params.attachments.push_back(ap);
    }
    
    auto terrainGen = precariously::planet_generation::generate(params, getStage()->getSpace());
    _terrain = terrain::TerrainObject::create("Terrain", terrainGen.world, DrawLayers::TERRAIN);
    getStage()->addObject(_terrain);
    
    if ((false)) {
        
        // we're making a single attachment, for testing
        terrain::AttachmentRef attachment = make_shared<terrain::Attachment>();
        dvec2 pos(550.0825,  781.675);
        // now attempt insertion

        if (terrainGen.world->addAttachment(attachment, pos, dvec2(0,1))) {
            attachment->setTag(0);
            terrainGen.attachmentsByBatchId[0].push_back(attachment);
        } else {
            CI_LOG_E("Unable to plant test attachment at: " << pos);
        }
        
        _recordCuts = false;
    }
    
    //
    // greebling
    //

    {
        auto image = loadImage(app::loadAsset("precariously/textures/Explosion.png"));
        gl::Texture2d::Format fmt = gl::Texture2d::Format().mipmap(false);
        
        precariously::GreeblingParticleSystem::config config;
        config.drawConfig.textureAtlas = gl::Texture2d::create(image, fmt);
        config.drawConfig.atlasType = particles::Atlas::TwoByTwo;
        config.drawConfig.drawLayer = DrawLayers::ATTACHMENTS;
        
        for(auto v : terrainGen.attachmentsByBatchId) {
            CI_LOG_D("Creating GreeblingParticleSystem to render " << v.second.size() << " greebles");
            auto greebles = precariously::GreeblingParticleSystem::create("greeble_" + str(v.first), config, v.second);
            getStage()->addObject(greebles);
        }
    }
    
    //
    // debug views of the iso surfaces
    //
    if ((false)) {
        _isoSurfaces = vector<Channel8u> { terrainGen.terrainMap, terrainGen.anchorMap };
        const auto fmt = ci::gl::Texture2d::Format().mipmap(false).minFilter(GL_NEAREST).magFilter(GL_NEAREST);
        for(Channel8u isoSurface : _isoSurfaces) {
            _isoTexes.push_back(ci::gl::Texture2d::create(isoSurface, fmt));
        }
    }

    //
    // basic scaffolding - grid, mousclick, etc
    //

    auto grid = WorldCartesianGridDrawComponent::create(1);
    grid->setFillColor(ColorA(0.2, 0.22, 0.25, 1.0));
    grid->setGridColor(ColorA(1, 1, 1, 0.1));
    getStage()->addObject(Object::with("Grid", {grid}));

    getStage()->addObject(Object::with("Dragger", {
        make_shared<MousePickComponent>(ShapeFilters::GRABBABLE),
        make_shared<MousePickDrawComponent>()
    }));
    
    auto cutter = make_shared<terrain::MouseCutterComponent>(_terrain, 4);
    getStage()->addObject(Object::with("Cutter", {
        cutter, make_shared<terrain::MouseCutterDrawComponent>()
    }));
    
    cutter->onCut.connect(this, &PerlinWorldTestScenario::onCutPerformed);

    getStage()->addObject(Object::with("ViewportControlComponent", {
        make_shared<ManualViewportControlComponent>(getViewportController())
    }));
    
    //
    //  Build terrain cut recorder
    //
    
    auto path = core::App::get()->getAppPath() / "cuts.txt";
    _terrainCutRecorder = make_shared<TerrainCutRecorder>(path);
    _recordCuts = true;
    _playingBackRecordedCuts = false;
}

void PerlinWorldTestScenario::cleanup() {
    _terrainCutRecorder.reset();
    _terrain.reset();
    _isoSurfaces.clear();
    _isoTexes.clear();

    setStage(nullptr);
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
            
        case 'c':
            if (!_terrainCutRecorder->getCuts().empty()) {
                _playingBackRecordedCuts = true;

                CI_LOG_D("Will play back " << _terrainCutRecorder->getCuts().size() << " recorded cuts");
                for (const auto &c : _terrainCutRecorder->getCuts()) {
                    _terrain->getWorld()->cut(c.a, c.b, c.width);
                }
                
                _playingBackRecordedCuts = false;
            }
            return true;
            
        case '.':
            _recordCuts = !_recordCuts;
            CI_LOG_D("_recordCuts: " << boolalpha << _recordCuts);
            return true;
    }
    
    switch(event.getCode()) {
        case ci::app::KeyEvent::KEY_BACKQUOTE: {
            setRenderMode(RenderMode::mode((int(getRenderMode()) + 1) % RenderMode::COUNT));
            return true;
        }
        case ci::app::KeyEvent::KEY_BACKSPACE: {
            CI_LOG_D("Clearing cut recorder");
            _terrainCutRecorder->clearCuts();
            _terrainCutRecorder->save();
            return true;
        }
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

void PerlinWorldTestScenario::onCutPerformed(dvec2 a, dvec2 b, double radius) {
    if (_recordCuts && !_playingBackRecordedCuts) {
        CI_LOG_D("onCutPerformed a: " << a << " b: " << b << " radius: " << radius);
        _terrainCutRecorder->addCut(a,b,radius);
        _terrainCutRecorder->save();
    }
 }
