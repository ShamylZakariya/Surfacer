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

namespace core { namespace game { namespace terrain {

	SMART_PTR(TerrainObject);

	class TerrainObject : public Object {
	public:

		static TerrainObjectRef create(string name, WorldRef world, int drawLayer) {
			return make_shared<TerrainObject>(name, world, drawLayer);
		}

	public:

		TerrainObject(string name, WorldRef world, int drawLayer);
		virtual ~TerrainObject();

		void onReady(LevelRef level) override;
		void onCleanup() override;
		void step(const time_state &timeState) override;
		void update(const time_state &timeState) override;

		const WorldRef & getWorld() const { return _world; }

	private:
		WorldRef _world;
	};

	/**
	 TerrainDrawComponent is a thin adapter to TerrainWorld's built-in draw dispatch system
	 */
	class TerrainDrawComponent : public DrawComponent {
	public:

		TerrainDrawComponent(int drawLayer):_drawLayer(drawLayer){}
		virtual ~TerrainDrawComponent(){}

		void onReady(ObjectRef parent, LevelRef level) override;
		cpBB getBB() const override { return cpBBInfinity; }
		void draw(const render_state &renderState) override;
		VisibilityDetermination::style getVisibilityDetermination() const override { return VisibilityDetermination::ALWAYS_DRAW; }
		int getLayer() const override { return _drawLayer; }
		int getDrawPasses() const override { return 1; }
		BatchDrawDelegateRef getBatchDrawDelegate() const override { return nullptr; }

	private:

		WorldRef _world;
		int _drawLayer;

	};

	/**
	 TerrainPhysicsComponent is a thin adapter to TerrainWorld's physics system
	 */
	class TerrainPhysicsComponent : public PhysicsComponent {
	public:
		TerrainPhysicsComponent(){}
		virtual ~TerrainPhysicsComponent(){}

		void onReady(ObjectRef parent, LevelRef level) override;
		cpBB getBB() const override;
		vector<cpBody*> getBodies() const override;

	private:

		WorldRef _world;

	};

}}} // namespace core::game::terrain

#endif /* Terrain_hpp */
