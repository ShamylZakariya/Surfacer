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

namespace core { namespace game {

	SMART_PTR(HealthComponent);
	SMART_PTR(Entity);

	class HealthComponent : public Component {
	public:

		struct config {
			double health;
			double maxHealth;
			double regenerationRate;
		};

		static config loadConfig(ci::XmlTree node);

		signals::signal<void()> onDeath;
		signals::signal<void(double, double)> onHealthChanged; // passes previous health, new health


	public:

		HealthComponent(config c);
		virtual ~HealthComponent();

		bool isDead() const {
			return _died;
		}

		double getHealth() const { return _config.health; }
		void setHealth(double health);

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

		// GameObject
		void addComponent(ComponentRef component) override;
		void removeComponent(ComponentRef component) override;

	protected:

		void _onHealthChanged(double previousHealth, double newHealth);
		void _onDeath();

	protected:

		HealthComponentRef _healthComponent;


	};

}} // namespace core::game

#endif /* Entity_hpp */
