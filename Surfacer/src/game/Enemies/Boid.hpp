//
//  Boid.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/14/17.
//
//

#ifndef Boid_hpp
#define Boid_hpp

#include "Core.hpp"
#include "GameLevel.hpp"
#include <cinder/Xml.h>

namespace core { namespace game { namespace enemy {

	SMART_PTR(BoidPhysicsComponent);
	SMART_PTR(BoidDrawComponent);
	SMART_PTR(Boid);
	SMART_PTR(BoidFlockController);

#pragma mark - BoidPhysicsComponent

	class BoidPhysicsComponent : public PhysicsComponent {
	public:

		struct config {
			dvec2 position;
			dvec2 dir;
			double radius;
			double speed;
			double density;
			cpShapeFilter filter;
		};

	public:

		BoidPhysicsComponent(config c);
		virtual ~BoidPhysicsComponent();

		dvec2 getPosition() const;
		double getRadius() const;

		void onReady(GameObjectRef parent, LevelRef level) override;
		void onCleanup() override;
		void step(const time_state &time) override;
		cpBB getBB() const override;
		double getGravityModifier() const override { return 0; }

	protected:

		config _config;
		cpBody *_body;
		cpShape *_shape;
		double _mass;

	};

#pragma mark - BoidDrawComponent

	class BoidDrawComponent : public DrawComponent {
	public:
		struct config {

		};

	public:

		BoidDrawComponent(config c);
		virtual ~BoidDrawComponent();

		void onReady(GameObjectRef parent, LevelRef level) override;

		void update(const time_state &time) override;
		void draw(const render_state &renderState) override;
		VisibilityDetermination::style getVisibilityDetermination() const override { return VisibilityDetermination::FRUSTUM_CULLING; }
		int getLayer() const override { return DrawLayers::ENEMY; }
		int getDrawPasses() const override { return 1; }
		BatchDrawDelegateRef getBatchDrawDelegate() const override { return nullptr; }

	protected:

		config _config;
		BoidPhysicsComponentWeakRef _physics;

	};

#pragma mark - Boid

	class Boid : public GameObject {
	public:

		struct config {
			BoidPhysicsComponent::config physics;
			BoidDrawComponent::config draw;
		};

		static BoidRef create(string name, BoidFlockControllerRef flockController, config c, dvec2 initialPosition, dvec2 initialDirection);

	public:

		Boid(string name, BoidFlockControllerRef flockController, config c);
		virtual ~Boid();

		BoidFlockControllerRef getFlockController() const { return _flockController.lock(); }

		void onReady(LevelRef level) override;
		void onCleanup() override;

	private:

		config _config;
		BoidFlockControllerWeakRef _flockController;

	};

#pragma mark - BoidFlockController

	class BoidFlockController : public Component {
	public:

		static Boid::config loadConfig(ci::XmlTree node);

		// signal fired when all boids in the flock are gone
		signals::signal< void(const BoidFlockControllerRef &) > onFlockDidFinish;

		// signal fired when a boid in the flock is killed right before it's removed from the level
		signals::signal< void(const BoidFlockControllerRef &, BoidRef &) > onBoidWillFinish;

	public:

		BoidFlockController(string name);
		virtual ~BoidFlockController();

		/**
		Spawn `count boids from `origin in `initialDirection with a given `config
		*/
		void spawn(size_t count, dvec2 origin, dvec2 initialDirection, Boid::config config);

		/**
		Get the number of living boids in the flock
		*/
		size_t getFlockSize() const;

		string getName() const { return _name; }

		void update(const time_state &time) override;

	protected:

		friend class Boid;

		void updateFlock(const time_state &time);
		void onBoidFinished(const BoidRef &boid);

	protected:

		string _name;
		vector<BoidRef> _flock;

	};

}}}

#endif /* Boid_hpp */
