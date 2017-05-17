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
			double sensorRadius;
			double speed;
			double density;
			cpShapeFilter filter;
		};

		struct sensor_data {
			dvec2 position;
			double distance;
		};

	public:

		BoidPhysicsComponent(config c);
		virtual ~BoidPhysicsComponent();

		dvec2 getPosition() const;
		dvec2 getVelocity() const;
		double getRadius() const;
		double getSensorRadius() const;

		cpShape *getShape() const { return _shape; }
		cpShape *getSensorShape() const { return _shape; }
		const vector<sensor_data> &getSensorData() const { return _sensorData; }

		void setTargetVelocity(dvec2 vel);
		void addToTargetVelocity(dvec2 vel);
		dvec2 getTargetVelocity() const { return _targetVelocity; }

		void onReady(GameObjectRef parent, LevelRef level) override;
		void onCleanup() override;
		void step(const time_state &time) override;
		cpBB getBB() const override;
		double getGravityModifier() const override { return 0; }

	protected:

		config _config;
		cpBody *_body;
		cpShape *_shape, *_sensorShape;
		vector<sensor_data> _sensorData;
		dvec2 _targetVelocity;
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

		BoidPhysicsComponentRef getBoidPhysicsComponent() const { return _boidPhysics; }
		BoidFlockControllerRef getFlockController() const { return _flockController.lock(); }

		void onReady(LevelRef level) override;
		void onCleanup() override;

	private:

		config _config;
		BoidFlockControllerWeakRef _flockController;
		BoidPhysicsComponentRef _boidPhysics;

	};

#pragma mark - BoidFlockController

	class BoidFlockController : public Component {
	public:

		struct rule_contributions {
			double flockCentroid;
			double flockVelocity;
			double collisionAvoidance;
			double targetSeeking;
			double variance;

			rule_contributions():
			flockCentroid(0.1),
			flockVelocity(0.1),
			collisionAvoidance(0.1),
			targetSeeking(0.1),
			variance(0.05)
			{}
		};

		struct config {
			Boid::config boid;
			rule_contributions ruleContributions;
		};

	public:

		static BoidFlockControllerRef create(string name, ci::XmlTree flockNode);

		static config loadConfig(ci::XmlTree flockNode);

		// signal fired when all boids in the flock are gone
		signals::signal< void(const BoidFlockControllerRef &) > onFlockDidFinish;

		// signal fired when a boid in the flock is killed right before it's removed from the level
		signals::signal< void(const BoidFlockControllerRef &, BoidRef &) > onBoidWillFinish;


	public:

		/**
		 Create a BoidFlockController which will prefix Boids with a given name, and which will use a given configuration
		 to apply rule_contributions while computing flock updates, and template params for each boid.
		 */
		BoidFlockController(string name, config c);
		virtual ~BoidFlockController();

		/**
		Spawn `count boids from `origin in `initialDirection with a given `config
		*/
		void spawn(size_t count, dvec2 origin, dvec2 initialDirection);

		/**
		Get the number of living boids in the flock
		*/
		size_t getFlockSize() const;

		/**
		 Get the name that will be prefixed to Boids
		 */
		string getName() const { return _name; }

		/**
		 Assign a new set of rule contributions overriding those assigned in the constructor
		 */
		void setRuleContributions(rule_contributions rc) { _config.ruleContributions = rc; }

		/**
		 Get the rule contributions which will be used to drive the flock
		 */
		rule_contributions getRuleContributions() const { return _config.ruleContributions; }

		// Component
		void update(const time_state &time) override;

	protected:

		friend class Boid;

		void updateFlock(const time_state &time);
		void onBoidFinished(const BoidRef &boid);

	protected:

		string _name;
		vector<BoidRef> _flock;
		config _config;


	};

}}}

#endif /* Boid_hpp */
