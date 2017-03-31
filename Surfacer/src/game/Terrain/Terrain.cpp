//
//  Terrain.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/30/17.
//
//

#include "Terrain.hpp"

namespace terrain {

#pragma mark - TerrainObject

	TerrainObject::TerrainObject(string name, WorldRef world):
	GameObject(name),
	_world(world)
	{}

	TerrainObject::~TerrainObject()
	{}

#pragma mark - TerrainDrawComponent

	void TerrainDrawComponent::onReady(core::GameObjectRef parent, core::LevelRef level) {
		_world = dynamic_pointer_cast<TerrainObject>(parent)->getWorld();
	}

	void TerrainDrawComponent::draw(const core::render_state &renderState) {
		_world->draw(renderState);
	}

}
