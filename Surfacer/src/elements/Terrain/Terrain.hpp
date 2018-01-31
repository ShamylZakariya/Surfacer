//
//  Terrain.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/30/17.
//
//

#ifndef Terrain_hpp
#define Terrain_hpp

#include "Core.hpp"
#include "TerrainWorld.hpp"

namespace terrain {

    SMART_PTR(TerrainObject);

    class TerrainObject : public core::Object {
    public:

        static TerrainObjectRef create(string name, WorldRef world, int drawLayer) {
            return make_shared<TerrainObject>(name, world, drawLayer);
        }

    public:

        TerrainObject(string name, WorldRef world, int drawLayer);

        virtual ~TerrainObject();

        void onReady(core::StageRef stage) override;

        void onCleanup() override;

        void step(const core::time_state &timeState) override;

        void update(const core::time_state &timeState) override;

        const WorldRef &getWorld() const {
            return _world;
        }

    private:
        WorldRef _world;
    };

    /**
     TerrainDrawComponent is a thin adapter to TerrainWorld's built-in draw dispatch system
     */
    class TerrainDrawComponent : public core::DrawComponent {
    public:

        TerrainDrawComponent(int drawLayer) : _drawLayer(drawLayer) {
        }

        virtual ~TerrainDrawComponent() {
        }

        void onReady(core::ObjectRef parent, core::StageRef stage) override;

        cpBB getBB() const override {
            return cpBBInfinity;
        }

        void draw(const core::render_state &renderState) override;

        core::VisibilityDetermination::style getVisibilityDetermination() const override {
            return core::VisibilityDetermination::ALWAYS_DRAW;
        }

        int getLayer() const override {
            return _drawLayer;
        }

        int getDrawPasses() const override {
            return 1;
        }

        BatchDrawDelegateRef getBatchDrawDelegate() const override {
            return nullptr;
        }

    private:

        WorldRef _world;
        int _drawLayer;

    };

    /**
     TerrainPhysicsComponent is a thin adapter to TerrainWorld's physics system
     */
    class TerrainPhysicsComponent : public core::PhysicsComponent {
    public:
        TerrainPhysicsComponent() {
        }

        virtual ~TerrainPhysicsComponent() {
        }

        void onReady(core::ObjectRef parent, core::StageRef stage) override;

        cpBB getBB() const override;

        vector<cpBody *> getBodies() const override;

    private:

        WorldRef _world;

    };

    class MouseCutterComponent : public core::InputComponent {
    public:

        MouseCutterComponent(terrain::TerrainObjectRef terrain, float radius, int dispatchReceiptIndex = 0);

        bool mouseDown(const ci::app::MouseEvent &event) override;

        bool mouseUp(const ci::app::MouseEvent &event) override;

        bool mouseMove(const ci::app::MouseEvent &event, const ivec2 &delta) override;

        bool mouseDrag(const ci::app::MouseEvent &event, const ivec2 &delta) override;

        bool isCutting() const {
            return _cutting;
        }

        float getRadius() const {
            return _radius;
        }

        dvec2 getCutStart() const {
            return _cutStart;
        }

        dvec2 getCutEnd() const {
            return _cutEnd;
        }

    private:

        bool _cutting;
        float _radius;
        vec2 _mouseScreen, _mouseWorld, _cutStart, _cutEnd;
        terrain::TerrainObjectRef _terrain;

    };

    class MouseCutterDrawComponent : public core::DrawComponent {
    public:

        MouseCutterDrawComponent(ColorA color = ColorA(1, 1, 1, 0.5));

        void onReady(core::ObjectRef parent, core::StageRef stage) override;

        cpBB getBB() const override {
            return cpBBInfinity;
        }

        void draw(const core::render_state &renderState) override;

        core::VisibilityDetermination::style getVisibilityDetermination() const override {
            return core::VisibilityDetermination::ALWAYS_DRAW;
        }

        int getLayer() const override {
            return numeric_limits<int>::max();
        };

    private:

        ColorA _color;
        weak_ptr<MouseCutterComponent> _cutterComponent;

    };

} // namespace terrain

#endif /* Terrain_hpp */
