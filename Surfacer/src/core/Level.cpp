//
//  Level.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/27/17.
//
//

#include "Level.hpp"

#include <cinder/CinderAssert.h>

#include "ChipmunkHelpers.hpp"
#include "Scenario.hpp"

namespace core {

	/*
		cpSpace *_space;
		bool _ready, _paused;
		ScenarioWeakRef _scenario;
		set<GameObjectRef> _objects;
		map<size_t,GameObjectRef> _objectsById;
		time_state _time;
		 string _name;
	 */

	Level::Level(string name):
	_space(cpSpaceNew()),
	_ready(false),
	_paused(false),
	_time(0,0,0),
	_name(name)
	{
		// some defaults
		cpSpaceSetIterations( _space, 20 );
		cpSpaceSetDamping( _space, 0.95 );
		cpSpaceSetSleepTimeThreshold( _space, 1 );
	}

	Level::~Level()
	{
		_objects.clear();
		_objectsById.clear();
		cpSpaceFree(_space);
	}

	void Level::setPaused(bool paused) {
		if (paused != isPaused()) {
			_paused = isPaused();
			if (isPaused()) {
				levelWasPaused(shared_from_this());
			} else {
				levelWasUnpaused(shared_from_this());
			}
		}
	}

	void Level::resize( ivec2 newSize )
	{}

	void Level::step( const time_state &time )
	{
		if (!_ready) {
			onReady();
		}

		if (!_paused) {
			cpSpaceStep(_space, time.deltaT);
		}

		if (!_paused) {
			vector<GameObjectRef> moribund;
			for (auto &obj : _objects) {
				if (!obj->isFinished()) {
					obj->step(time);
				} else {
					moribund.push_back(obj);
				}
			}

			if (!moribund.empty()) {
				for (auto &obj : moribund) {
					removeGameObject(obj);
				}
			}
		}
	}

	void Level::update( const time_state &time )
	{
		if (!_paused) {
			_time = time;
		}

		if (!_ready) {
			onReady();
		}

		if (!_paused) {
			vector<GameObjectRef> moribund;
			for (auto &obj : _objects) {
				if (!obj->isFinished()) {
					obj->update(time);
				} else {
					moribund.push_back(obj);
				}
			}

			if (!moribund.empty()) {
				for (auto &obj : moribund) {
					removeGameObject(obj);
				}
			}
		}
	}

	void Level::draw( const render_state &state )
	{}

	void Level::addGameObject(GameObjectRef obj) {
		CI_ASSERT_MSG(!obj->getLevel(), "Can't add a GameObject that already has been added to this or another Level");
		_objects.insert(obj);
		_objectsById[obj->getId()] = obj;
	}

	void Level::removeGameObject(GameObjectRef obj) {
		CI_ASSERT_MSG(obj->getLevel().get() == this, "Can't remove a GameObject which isn't a child of this Level");
	}

	GameObjectRef Level::getGameObjectById(size_t id) const {
		auto pos = _objectsById.find(id);
		if (pos != _objectsById.end()) {
			return pos->second;
		} else {
			return nullptr;
		}
	}

	vector<GameObjectRef> Level::getGameObjectsByName(string name) const {

		vector<GameObjectRef> result;
		copy_if(_objects.begin(), _objects.end(), back_inserter(result),[&name](const GameObjectRef &gameObject) -> bool {
			return gameObject->getName() == name;
		});

		return result;
	}

	const Viewport &Level::camera() const {
		return getScenario()->getCamera();
	}

	Viewport &Level::camera() {
		return getScenario()->getCamera();
	}

	void Level::addedToScenario(ScenarioRef scenario) {
		_scenario = scenario;
	}

	void Level::removeFromScenario() {
		_scenario.reset();
	}

	void Level::onReady() {
		CI_ASSERT_MSG(!_ready, "Can't call onReady() on Level that is already ready");
		for (auto &obj : _objects) {
			obj->onReady();
		}
		_ready = true;
	}


}
