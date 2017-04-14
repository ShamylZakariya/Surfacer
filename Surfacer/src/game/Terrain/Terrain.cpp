//
//  Terrain.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/30/17.
//
//

#include "Terrain.hpp"

#include "GameLevel.hpp"

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

#pragma mark - TerrainPhysicsComponent

	void TerrainPhysicsComponent::onReady(core::GameObjectRef parent, core::LevelRef level) {
		_world = dynamic_pointer_cast<TerrainObject>(parent)->getWorld();
	}

	vector<cpBody*> TerrainPhysicsComponent::getBodies() const {
		vector<cpBody*> bodies;
		for (terrain::GroupBaseRef g : _world->getDynamicGroups()) {
			bodies.push_back(g->getBody());
		}
		bodies.push_back(_world->getStaticGroup()->getBody());
		return bodies;
	}

}
