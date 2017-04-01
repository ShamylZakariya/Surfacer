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
	{
		addComponent(make_shared<TerrainDrawComponent>());
	}

	TerrainObject::~TerrainObject()
	{}

	void TerrainObject::step(const core::time_state &timeState) {
		_world->step(timeState);
	}

	void TerrainObject::update(const core::time_state &timeState) {
		_world->update(timeState);
	}

#pragma mark - TerrainDrawComponent

	void TerrainDrawComponent::onReady(core::GameObjectRef parent, core::LevelRef level) {
		_world = dynamic_pointer_cast<TerrainObject>(parent)->getWorld();
	}

	void TerrainDrawComponent::draw(const core::render_state &renderState) {
		_world->draw(renderState);
	}

}
