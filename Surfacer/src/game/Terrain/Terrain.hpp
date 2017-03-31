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

using namespace ci;
using namespace std;

namespace terrain {

	static const int LAYER = 0;

	class TerrainObject : core::GameObject {
	public:
		TerrainObject(string name, WorldRef world);
		virtual ~TerrainObject();

		const WorldRef & getWorld() const { return _world; }

	private:
		WorldRef _world;
	};

	class TerrainDrawComponent : core::DrawComponent {
	public:

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

}

#endif /* Terrain_hpp */
