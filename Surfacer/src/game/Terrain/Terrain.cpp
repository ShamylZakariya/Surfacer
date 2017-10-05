//
//  Terrain.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/30/17.
//
//

#include "Terrain.hpp"

namespace core { namespace game { namespace terrain {

#pragma mark - TerrainObject

	TerrainObject::TerrainObject(string name, WorldRef world, int drawLayer):
	Object(name),
	_world(world)
	{
		addComponent(make_shared<TerrainDrawComponent>(drawLayer));
		addComponent(make_shared<TerrainPhysicsComponent>());
	}

	TerrainObject::~TerrainObject()
	{}

	void TerrainObject::onReady(LevelRef level) {
		Object::onReady(level);
		_world->setObject(shared_from_this());
	}

	void TerrainObject::onCleanup() {
		_world.reset();
	}

	void TerrainObject::step(const time_state &timeState) {
		_world->step(timeState);
	}

	void TerrainObject::update(const time_state &timeState) {
		_world->update(timeState);
	}

#pragma mark - TerrainDrawComponent

	/*
	 WorldRef _world;
	 int _drawLayer;
	 */
	
	void TerrainDrawComponent::onReady(ObjectRef parent, LevelRef level) {
		_world = dynamic_pointer_cast<TerrainObject>(parent)->getWorld();
	}

	void TerrainDrawComponent::draw(const render_state &renderState) {
		_world->draw(renderState);
	}

#pragma mark - TerrainPhysicsComponent

	void TerrainPhysicsComponent::onReady(ObjectRef parent, LevelRef level) {
		_world = dynamic_pointer_cast<TerrainObject>(parent)->getWorld();
	}

	cpBB TerrainPhysicsComponent::getBB() const {
		return cpBBInfinity;
	}

	vector<cpBody*> TerrainPhysicsComponent::getBodies() const {
		vector<cpBody*> bodies;
		for (terrain::GroupBaseRef g : _world->getDynamicGroups()) {
			bodies.push_back(g->getBody());
		}
		bodies.push_back(_world->getStaticGroup()->getBody());
		return bodies;
	}

}}} // namespace core::game::terrain
