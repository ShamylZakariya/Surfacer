//
//  Terrain.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/30/17.
//
//

#include "Terrain.hpp"

#include "GameLevel.hpp"

using namespace core;
namespace terrain {

#pragma mark - TerrainObject

	TerrainObject::TerrainObject(string name, WorldRef world):
	GameObject(name),
	_world(world)
	{
		addComponent(make_shared<TerrainDrawComponent>());
		addComponent(make_shared<TerrainPhysicsComponent>());
	}

	TerrainObject::~TerrainObject()
	{}

	void TerrainObject::onReady(LevelRef level) {
		GameObject::onReady(level);
	}

	void TerrainObject::step(const time_state &timeState) {
		_world->step(timeState);
	}

	void TerrainObject::update(const time_state &timeState) {
		_world->update(timeState);
	}

#pragma mark - TerrainDrawComponent

	void TerrainDrawComponent::onReady(GameObjectRef parent, LevelRef level) {
		_world = dynamic_pointer_cast<TerrainObject>(parent)->getWorld();
	}

	int TerrainDrawComponent::getLayer() const {
		return DrawLayers::TERRAIN;
	}

	void TerrainDrawComponent::draw(const render_state &renderState) {
		_world->draw(renderState);
	}

#pragma mark - TerrainPhysicsComponent

	void TerrainPhysicsComponent::onReady(GameObjectRef parent, LevelRef level) {
		_world = dynamic_pointer_cast<TerrainObject>(parent)->getWorld();

		// assign level ref to bodies
		

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
