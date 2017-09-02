//
//  Entity.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/26/17.
//
//

#ifndef Entity_hpp
#define Entity_hpp

#include <cinder/Xml.h>

#include "Core.hpp"
#include "Xml.hpp"

namespace core { namespace game {

	SMART_PTR(HealthComponent);
	SMART_PTR(EntityDrawComponent);
	SMART_PTR(Entity);

	class HealthComponent : public Component {
	public:

		struct config {
			double health;
			double maxHealth;
			double regenerationRate;
		};

		static config loadConfig(util::xml::XmlMultiTree node);

		signals::signal<void()> onDeath;
		signals::signal<void(double, double)> onHealthChanged; // passes previous health, new health


	public:

		HealthComponent(config c);
		virtual ~HealthComponent();

		bool isAlive() const {
			return !_died && _config.health > 0;
		}

		bool isDead() const {
			return _died;
		}

		double getHealth() const { return _config.health; }
		double getHealthiness() const { return max(_config.health / _config.maxHealth, 0.0); }
		void setHealth(double health);
		void takeInjury(double healthPointsToLose);

		double getMaxHealth() const { return _config.maxHealth; }
		void setMaxHealth(double mh) { _config.maxHealth = mh; }

		double getRegenerationRate() const { return _config.regenerationRate; }
		void setRegenerationRate(double rr) { _config.regenerationRate = rr; }

		// Component
		void update(const time_state &time);

	protected:

		virtual void die();

	protected:

		config _config;
		bool _died;

	};

	class EntityDrawComponent : public DrawComponent {
	public:

		EntityDrawComponent();
		virtual ~EntityDrawComponent();

		// returns value from 1 (perfect health) to 0 (dead)
		virtual double getHealthiness() const { return _healthiness; }

		// return true if the parent Entity is alive per its health component
		virtual bool isAlive() const { return _alive; }

		// returns true if the parent Entity is dead, per its health component
		virtual bool isDead() const { return !isAlive(); }

		// returns progress of death cycle, from 0 (starting) to 1 (complete)
		virtual double getDeathCycleProgress() const { return _deathCycleProgress; }

	private:

		friend class Entity;
		double _healthiness, _deathCycleProgress;
		bool _alive;

	};

	/**
	 @class Entity
	 An Entity is a game object which represents a "living" thing in the game world, e.g., something with health and the ability to die.
	 It could be the player, an enemy, or a car which can only take so much damage before being destroyed.
	 */
	class Entity : public GameObject {
	public:

		Entity(string name);
		virtual ~Entity();

		virtual void onHealthChanged(double oldHealth, double newHealth);
		virtual void onDeath();

		HealthComponentRef getHealthComponent() const { return _healthComponent; }
		EntityDrawComponentRef getEntityDrawComponent() const { return _entityDrawComponent; }

		// GameObject
		void update(const time_state &time) override;
		void onFinishing(seconds_t secondsLeft, double amountFinished) override;
		void addComponent(ComponentRef component) override;
		void removeComponent(ComponentRef component) override;

	protected:

		void _onHealthChanged(double previousHealth, double newHealth);
		void _onDeath();

	protected:

		HealthComponentRef _healthComponent;
		EntityDrawComponentRef _entityDrawComponent;

	};

}} // namespace core::game

#endif /* Entity_hpp */
