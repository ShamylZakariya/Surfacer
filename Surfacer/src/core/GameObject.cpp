//
//  GameObject.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/27/17.
//
//

#include "GameObject.hpp"

#include <cinder/CinderAssert.h>

namespace core {

#pragma mark - Component	

	LevelRef Component::getLevel() const {
		return getGameObject()->getLevel();
	}

#pragma mark - DrawComponent

	void DrawComponent::notifyMoved() {

	}

#pragma mark - GameObject

	/*
		static size_t _idCounter;
		size_t _id;
		string _name;
		bool _finished, _ready;
		set<ComponentRef> _components;
		DrawComponentRef _drawComponent;
	 */

	size_t GameObject::_idCounter = 0;

	GameObject::GameObject(string name):
	_id(_idCounter++),
	_name(name),
	_finished(false),
	_ready(false)
	{}

	GameObject::~GameObject(){}

	void GameObject::addComponent(ComponentRef component) {
		CI_ASSERT_MSG(component->getGameObject() == nullptr, "Cannot add a component that already has been added to another GameObject");

		_components.insert(component);
		component->attachedToGameObject(shared_from_this());

		if (DrawComponentRef dc = dynamic_pointer_cast<DrawComponent>(component)) {
			_drawComponent = dc;
		}

		if (_ready) {
			component->onReady(shared_from_this(),getLevel());
		}
	}

	void GameObject::removeComponent(ComponentRef component) {
		if (component->getGameObject() && component->getGameObject().get() == this) {
			_components.erase(component);
			component->detachedFromGameObject();

			if (_drawComponent == component) {
				_drawComponent.reset();
			}
		}
	}

	void GameObject::onReady(LevelRef level){
		const auto self = shared_from_this();
		for (auto &component : _components) {
			component->onReady(self, level);
		}
		_ready = true;
	}

	void GameObject::step(const time_state &timeState) {
		for (auto &component : _components) {
			component->step(timeState);
		}
	}

	void GameObject::update(const time_state &timeState) {
		for (auto &component : _components) {
			component->update(timeState);
		}
	}


}
