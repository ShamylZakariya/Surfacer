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

	static const int LAYER = 0;

	SMART_PTR(TerrainObject);

	class TerrainObject : public core::GameObject {
	public:

		static TerrainObjectRef create(string name, WorldRef world) {
			return make_shared<TerrainObject>(name, world);
		}

	public:

		TerrainObject(string name, WorldRef world);
		virtual ~TerrainObject();

		void onReady(core::LevelRef level) override;
		void step(const core::time_state &timeState) override;
		void update(const core::time_state &timeState) override;

		const WorldRef & getWorld() const { return _world; }

	private:
		WorldRef _world;
	};

	/**
	 TerrainDrawComponent is a thin adapter to TerrainWorld's built-in draw dispatch system
	 */
	class TerrainDrawComponent : public core::DrawComponent {
	public:

		TerrainDrawComponent(){}
		virtual ~TerrainDrawComponent(){}

		void onReady(core::GameObjectRef parent, core::LevelRef level) override;
		cpBB getBB() const override { return cpBBInfinity; }
		void draw(const core::render_state &renderState) override;
		core::VisibilityDetermination::style getVisibilityDetermination() const override { return core::VisibilityDetermination::ALWAYS_DRAW; }
		int getLayer() const override { return LAYER; };
		int getDrawPasses() const override { return 1; }
		BatchDrawDelegateRef getBatchDrawDelegate() const override { return nullptr; }

	private:

		WorldRef _world;

	};

	/**
	 TerrainPhysicsComponent is a thin adapter to TerrainWorld's physics system
	 */
	class TerrainPhysicsComponent : public core::PhysicsComponent {
	public:
		TerrainPhysicsComponent(){}
		virtual ~TerrainPhysicsComponent(){}

		void onReady(core::GameObjectRef parent, core::LevelRef level) override;
		vector<cpBody*> getBodies() const override;

	private:

		WorldRef _world;

	};

}

#endif /* Terrain_hpp */
