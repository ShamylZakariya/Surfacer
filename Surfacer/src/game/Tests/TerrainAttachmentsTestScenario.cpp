//
//  TerrainAttachmentsTestScenario.cpp
//  Tests
//
//  Created by Shamyl Zakariya on 2/8/18.
//

#include <cinder/Rand.h>

#include "TerrainAttachmentsTestScenario.hpp"
#include "App.hpp"
#include "DevComponents.hpp"
#include "Svg.hpp"

using namespace core;

namespace {
    
    const double COLLISION_SHAPE_RADIUS = 0;
    const double MIN_SURFACE_AREA = 2;
    const ci::Color TERRAIN_COLOR(0.2, 0.2, 0.2);
    const ci::Color ANCHOR_COLOR(1, 0, 1);
    
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
            ATTACHMENTS = 2
        };
    }
    
    PolyLine2d rect(float left, float bottom, float right, float top) {
        PolyLine2d pl;
        pl.push_back(vec2(left, bottom));
        pl.push_back(vec2(right, bottom));
        pl.push_back(vec2(right, top));
        pl.push_back(vec2(left, top));
        pl.setClosed();
        return pl;
    }
    
    class AttachmentAdapter : public core::Component {
    public:
        AttachmentAdapter(terrain::AttachmentRef attachment):
                _attachment(attachment),
                _orphaned(false),
                _positioned(false) {
            attachment->onOrphaned.connect(this, &AttachmentAdapter::_onOrphaned);
        }
        
        terrain::AttachmentRef getAttachment() const { return _attachment; }
        
        void update(const time_state &timeState) {
            if (!_orphaned) {
                const double epsilon = 1e-5;
                dvec2 position = _attachment->getWorldPosition();
                dvec2 rotation = _attachment->getWorldRotation();
                if (!_positioned || lengthSquared(position - _lastPosition) > epsilon || lengthSquared(rotation-_lastRotation) > epsilon) {
                    _lastPosition = position;
                    _lastRotation = rotation;
                    updatePosition(position, rotation, _attachment->getWorldTransform());
                    _positioned = true;
                }
            }
        }
        
        // called when the terrain::Attachment moves, passing world space position/rotation and those two combined into a transform
        virtual void updatePosition(dvec2 position, dvec2 rotation, dmat4 transform) {}
        
        // attachment has been orphaned in the terrain::World.
        // call to getAttachment() valid for duration of this method, subsequently, this
        // object should act as a free agent, self terminating or flying away to heaven, whatever
        virtual void onOrphaned() {}
        
    private:
        
        void _onOrphaned() {
            onOrphaned();
            _orphaned = true;
            _attachment.reset();
        }

    private:
        
        terrain::AttachmentRef _attachment;
        bool _orphaned, _positioned;
        dvec2 _lastPosition, _lastRotation;
        
    };
    
    class SvgAttachmentAdapter : public AttachmentAdapter {
    public:
        SvgAttachmentAdapter(terrain::AttachmentRef attachment, util::svg::GroupRef svgDoc):
                AttachmentAdapter(attachment),
                _svgDoc(svgDoc)
        {}
        
        util::svg::GroupRef getSvgDoc() const { return _svgDoc; }

        void updatePosition(dvec2 position, dvec2 rotation, dmat4 transform) override {
            _svgDoc->setPosition(position);
            _svgDoc->setRotation(rotation);
            notifyMoved();
        }
        
        void onOrphaned() override {
            CI_LOG_D("orphaned from " << getAttachment()->getId() << " - self terminating!!!");
            getObject()->setFinished(true);
        }

    private:
      
        util::svg::GroupRef _svgDoc;
        
    };
}

/*
 terrain::TerrainObjectRef _terrain;
 */

TerrainAttachmentsTestScenario::TerrainAttachmentsTestScenario() {
}

TerrainAttachmentsTestScenario::~TerrainAttachmentsTestScenario() {
}

void TerrainAttachmentsTestScenario::setup() {
    // go debug for rendering
    setRenderMode(RenderMode::DEVELOPMENT);
    
    setStage(make_shared<Stage>("Terrain Test Stage"));
    
//    auto world = testBasicTerrain();
    auto world = testBasicAttachmentAdapter();
//    auto world = testComplexSvgLoad();
    
    _terrain = terrain::TerrainObject::create("Terrain", world, DrawLayers::TERRAIN);
    getStage()->addObject(_terrain);
    
    auto dragger = Object::with("Dragger", {
        make_shared<MousePickComponent>(ShapeFilters::GRABBABLE),
        make_shared<MousePickDrawComponent>()
    });
    
    auto cutter = Object::with("Cutter", {
        make_shared<terrain::MouseCutterComponent>(_terrain, 4),
        make_shared<terrain::MouseCutterDrawComponent>()
    });
    
    auto cameraController = Object::with("ViewportControlComponent", {
        make_shared<ManualViewportControlComponent>(getViewportController())
    });
    
    auto gridDc = WorldCartesianGridDrawComponent::create();
    gridDc->setFillColor(ColorA(0.30,0.30,0.30,1));
    gridDc->setGridColor(ColorA(0.50,0.50,0.50,1));
    gridDc->setAxisColor(ColorA(0.50,0.90,0.90,1));
    gridDc->setAxisIntensity(0);
    auto grid = Object::with("Grid", { gridDc });
    
    getStage()->addObject(dragger);
    getStage()->addObject(cutter);
    getStage()->addObject(grid);
    getStage()->addObject(cameraController);
}

void TerrainAttachmentsTestScenario::cleanup() {
    _terrain.reset();
    setStage(nullptr);
}

void TerrainAttachmentsTestScenario::resize(ivec2 size) {
}

void TerrainAttachmentsTestScenario::step(const time_state &time) {
}

void TerrainAttachmentsTestScenario::update(const time_state &time) {
}

void TerrainAttachmentsTestScenario::clear(const render_state &state) {
    gl::clear(Color(0.2, 0.225, 0.25));
}

void TerrainAttachmentsTestScenario::drawScreen(const render_state &state) {
    // draw fpf/sps
    float fps = App::get()->getAverageFps();
    float sps = App::get()->getAverageSps();
    string info = strings::format("%.1f %.1f", fps, sps);
    gl::drawString(info, vec2(10, 10), Color(1, 1, 1));
}

bool TerrainAttachmentsTestScenario::keyDown(const ci::app::KeyEvent &event) {
    if (event.getChar() == 'r') {
        reset();
        return true;
    } else if (event.getCode() == ci::app::KeyEvent::KEY_SPACE) {
        //testExplicitCut();
        return true;
    } else if (event.getCode() == ci::app::KeyEvent::KEY_BACKQUOTE) {
        setRenderMode(RenderMode::mode((int(getRenderMode()) + 1) % RenderMode::COUNT));
    }
    return false;
}

void TerrainAttachmentsTestScenario::reset() {
    cleanup();
    setup();
}

terrain::WorldRef TerrainAttachmentsTestScenario::testBasicTerrain() {
    using namespace terrain;
    cpSpaceSetDamping(getStage()->getSpace()->getSpace(), 0.5);
    
    getViewportController()->setLook(vec2(0, 0));
    
    vector<ShapeRef> shapes = {
        Shape::fromContour(rect(0, 0, 100, 50)),          // 0
        Shape::fromContour(rect(100, 0, 150, 50)),        // 1 to right of 0 - binds to 0
        Shape::fromContour(rect(-100, 0, 0, 50)),         // 2 to left of 0 - binds to 0
        Shape::fromContour(rect(-10, 50, 110, 100)),      // 3 above 0, but wider
        Shape::fromContour(rect(-10, 100, 110, 200)),     // 4 above 3, binds to 3
    };
    
    vector<AnchorRef> anchors = {
        Anchor::fromContour(rect(20, 120, 30, 130))       // 5 inside 4, anchors 4 and 3
    };
    
    const material terrainMaterial(1, 0.5, COLLISION_SHAPE_RADIUS, ShapeFilters::TERRAIN, CollisionType::TERRAIN, MIN_SURFACE_AREA, TERRAIN_COLOR);
    material anchorMaterial(1, 1, COLLISION_SHAPE_RADIUS, ShapeFilters::ANCHOR, CollisionType::ANCHOR, MIN_SURFACE_AREA, ANCHOR_COLOR);
    
    auto world = make_shared<World>(getStage()->getSpace(), terrainMaterial, anchorMaterial);
    world->build(shapes, anchors);
    
    // make some attachments
    auto makeAttachment = [&world](dvec2 position, bool shouldBeDynamic) -> bool {
        AttachmentRef attachment = make_shared<Attachment>();
        if (world->addAttachment(attachment, position)) {
            GroupBaseRef group = attachment->getGroup();
            CI_ASSERT_MSG(group->isDynamic() == shouldBeDynamic, (shouldBeDynamic ? "Group added to should be dynamic" : "Group added to should have been static"));
            CI_LOG_D("Added attachment @ " << position << " to group: " << group << " position: " << group->getPosition());
            CI_LOG_D("\tAttachment world: " << attachment->getWorldPosition() << " local: " << attachment->getLocalPosition());
            return true;
        }
        return false;
    };
    
    CI_ASSERT_MSG(makeAttachment(dvec2(5,5), true), "Should have been successfully added");
    CI_ASSERT_MSG(makeAttachment(dvec2(5,55), false), "Should have been successfully added");
    CI_ASSERT_MSG(makeAttachment(dvec2(-100,-100), false) == false, "Should have NOT been added");
    
    return world;
}

terrain::WorldRef TerrainAttachmentsTestScenario::testBasicAttachmentAdapter() {
    using namespace terrain;
    cpSpaceSetDamping(getStage()->getSpace()->getSpace(), 0.5);
    
    getViewportController()->setLook(vec2(0, 0));
    
    vector<ShapeRef> shapes = {
        Shape::fromContour(rect(0, 0, 100, 50)),          // 0
        Shape::fromContour(rect(100, 0, 150, 50)),        // 1 to right of 0 - binds to 0
        Shape::fromContour(rect(-100, 0, 0, 50)),         // 2 to left of 0 - binds to 0
        Shape::fromContour(rect(-10, 50, 110, 100)),      // 3 above 0, but wider
        Shape::fromContour(rect(-10, 100, 110, 200)),     // 4 above 3, binds to 3
    };
    
    vector<AnchorRef> anchors = {
        Anchor::fromContour(rect(20, 120, 30, 130))       // 5 inside 4, anchors 4 and 3
    };
    
    const material terrainMaterial(1, 0.5, COLLISION_SHAPE_RADIUS, ShapeFilters::TERRAIN, CollisionType::TERRAIN, MIN_SURFACE_AREA, TERRAIN_COLOR);
    material anchorMaterial(1, 1, COLLISION_SHAPE_RADIUS, ShapeFilters::ANCHOR, CollisionType::ANCHOR, MIN_SURFACE_AREA, ANCHOR_COLOR);
    
    auto world = make_shared<World>(getStage()->getSpace(), terrainMaterial, anchorMaterial);
    world->build(shapes, anchors);
    
    // make some attachments
    
    auto makeDingus = [&](dvec2 position) {
        AttachmentRef attachment = make_shared<Attachment>();
        if (world->addAttachment(attachment, position)) {
            auto svg = util::svg::Group::loadSvgDocument(app::loadAsset("svg_tests/dingus.svg"), 1);
            getStage()->addObject(Object::with("Dingus", {
                make_shared<util::svg::SvgDrawComponent>(svg, DrawLayers::ATTACHMENTS),
                make_shared<SvgAttachmentAdapter>(attachment, svg)
            }));
        }
    };

    makeDingus(dvec2(5,5));
    makeDingus(dvec2(5,55));

    return world;
}


terrain::WorldRef TerrainAttachmentsTestScenario::testComplexSvgLoad() {
    
    // load shapes and anchors from SVG
    vector<terrain::ShapeRef> shapes;
    vector<terrain::AnchorRef> anchors;
    vector<terrain::ElementRef> elements;
    terrain::World::loadSvg(app::loadAsset("svg_tests/complex_world_test.svg"), dmat4(), shapes, anchors, elements, true);
    
    // partition
    auto partitionedShapes = terrain::World::partition(shapes, dvec2(0, 0), 500);
    
    // construct
    const terrain::material terrainMaterial(1, 0.5, COLLISION_SHAPE_RADIUS, ShapeFilters::TERRAIN, CollisionType::TERRAIN, MIN_SURFACE_AREA, TERRAIN_COLOR);
    const terrain::material anchorMaterial(1, 1, COLLISION_SHAPE_RADIUS, ShapeFilters::ANCHOR, CollisionType::ANCHOR, MIN_SURFACE_AREA, ANCHOR_COLOR);
    auto world = make_shared<terrain::World>(getStage()->getSpace(), terrainMaterial, anchorMaterial);
    world->build(partitionedShapes, anchors, elements);
    
    return world;
}
