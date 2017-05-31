//
//  Entity.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/26/17.
//
//

#include "Entity.hpp"
#include "Xml.hpp"

namespace core { namespace game {

	/*
	 config _config;
	 */

	HealthComponent::config HealthComponent::loadConfig(ci::XmlTree node) {
		config c;
		c.maxHealth = util::xml::readNumericAttribute(node, "max_health", 100);
		c.health = util::xml::readNumericAttribute(node, "health", c.maxHealth);
		c.regenerationRate = util::xml::readNumericAttribute(node, "regeneration_rate", 1);
		return c;
	}

	HealthComponent::HealthComponent(config c):
	_config(c)
	{}

	HealthComponent::~HealthComponent() {
	}

	void HealthComponent::setHealth(double health) {

		const double previousHealth = _config.health;
		_config.health = min(health, _config.maxHealth);

		onHealthChanged(previousHealth, _config.health);

		if (_config.health <= 0) {
			die();
		}
	}

	// Component
	void HealthComponent::update(const time_state &time) {
		if (!_died) {

			if (_config.regenerationRate > 0 && _config.health < _config.maxHealth) {
				const auto previousHealth = _config.health;
				_config.health = min(_config.health + _config.regenerationRate * time.deltaT, _config.maxHealth);
				onHealthChanged(previousHealth, _config.health);
			}

		}

	}

	void HealthComponent::die() {
		_died = true;
		onDeath();
	}

#pragma mark - Entity

	Entity::Entity(string name):
	GameObject(name)
	{}

	Entity::~Entity()
	{}

	void Entity::onHealthChanged(double oldHealth, double newHealth) {
		CI_LOG_D(getName() << " -- oldHealth: " << oldHealth << " newHealth: " << newHealth);
	}

	void Entity::onDeath() {
		CI_LOG_D(getName() << " -- DEATH!");
	}

	void Entity::addComponent(ComponentRef component) {
		GameObject::addComponent(component);
		if (auto hc = dynamic_pointer_cast<HealthComponent>(component)) {
			CI_ASSERT_MSG(!_healthComponent, "Can't assign more than one HealthComponent to an Entity");
			_healthComponent = hc;
			_healthComponent->onDeath.connect(this, &Entity::_onDeath);
			_healthComponent->onHealthChanged.connect(this, &Entity::_onHealthChanged);
		}
	}

	void Entity::removeComponent(ComponentRef component) {
		GameObject::removeComponent(component);
		if (component == _healthComponent) {
			_healthComponent->onDeath.disconnect(this);
			_healthComponent->onHealthChanged.disconnect(this);
			_healthComponent.reset();
		}
	}

	void Entity::_onHealthChanged(double previousHealth, double newHealth) {
		onHealthChanged(previousHealth, newHealth);
	}

	void Entity::_onDeath() {
		onDeath();
	}




}} // namespace core::game
