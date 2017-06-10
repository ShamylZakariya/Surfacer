//
//  Boid.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/14/17.
//
//

#ifndef Boid_hpp
#define Boid_hpp

#include <cinder/Rand.h>
#include <cinder/Xml.h>

#include "Core.hpp"
#include "Entity.hpp"
#include "GameLevel.hpp"
#include "Xml.hpp"

namespace core { namespace game { namespace enemy {

	SMART_PTR(BoidPhysicsComponent);
	SMART_PTR(BoidDrawComponent);
	SMART_PTR(Boid);
	SMART_PTR(BoidFlockDrawComponent);
	SMART_PTR(BoidFlockController);
	SMART_PTR(BoidFlock);

#pragma mark - BoidPhysicsComponent

	class BoidPhysicsComponent : public PhysicsComponent {
	public:

		struct config {
			dvec2 position;
			double radius;
			double sensorRadius;
			double speed;
			double density;
			cpShapeFilter filter;
		};

	public:

		BoidPhysicsComponent(config c);
		virtual ~BoidPhysicsComponent();

		const config &getConfig() const { return _config; }

		dvec2 getPosition() const { return _position; }
		dvec2 getVelocity() const { return _velocity; }
		double getRadius() const { return _config.radius; }
		double getSensorRadius() const { return _config.sensorRadius; }

		cpShape *getShape() const { return _shape; }

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
		cpConstraint *_gear;
		cpShape *_shape;
		dvec2 _targetVelocity, _position, _velocity;
		double _mass;

	};

#pragma mark - Boid

	class Boid : public Entity {
	public:

		struct config {
			BoidPhysicsComponent::config physics;
			HealthComponent::config health;
		};

		static BoidRef create(string name, BoidFlockControllerRef flockController, config c, dvec2 initialPosition, dvec2 initialVelocity, double ruleVariance);

	public:

		Boid(string name, BoidFlockControllerRef flockController, config c, double ruleVariance);
		virtual ~Boid();

		const config &getConfig() const { return _config; }
		double getRuleVariance() const { return _ruleVariance; }

		BoidPhysicsComponentRef getBoidPhysicsComponent() const { return _boidPhysics; }
		BoidFlockControllerRef getFlockController() const { return _flockController.lock(); }

		// Entity
		void onHealthChanged(double oldHealth, double newHealth) override;
		void onDeath() override;

		// GameObject
		void onReady(LevelRef level) override;
		void onCleanup() override;

	private:

		config _config;
		BoidFlockControllerWeakRef _flockController;
		BoidPhysicsComponentRef _boidPhysics;
		double _ruleVariance;

	};

#pragma mark - BoidDrawComponent

	class BoidFlockDrawComponent : public DrawComponent {
	public:
		struct config {
		};

	public:

		BoidFlockDrawComponent(config c);
		virtual ~BoidFlockDrawComponent();

		void onReady(GameObjectRef parent, LevelRef level) override;

		void update(const time_state &time) override;
		void draw(const render_state &renderState) override;
		VisibilityDetermination::style getVisibilityDetermination() const override { return VisibilityDetermination::FRUSTUM_CULLING; }
		int getLayer() const override { return DrawLayers::ENEMY; }
		int getDrawPasses() const override { return 1; }
		BatchDrawDelegateRef getBatchDrawDelegate() const override { return nullptr; }
		cpBB getBB() const override;

	protected:

		config _config;
		BoidFlockControllerWeakRef _flockController;
		gl::VboMeshRef _unitCircleMesh;
		
	};


#pragma mark - BoidFlockController

	class BoidFlockController : public Component {
	public:

		struct rule_contributions {
			// scale of rule causing flock to converge about centroid
			double flockCentroid;

			// scale of rule causing flock to attempt to maintain a shared velocity
			double flockVelocity;

			// scale of rule causing flock to avoid collisions with eachother, environment
			double collisionAvoidance;

			// scale of rule causing flock to seek target
			double targetSeeking;

			// each boid gets a random variance of its rule scaling. ruleVairance of zero means each boid
			// follows the rules precisely the same. a variance of 0.5 means each boid gets a scaling of  (1 + rand(-0.5,0.5))
			// meaning some boids will follow the rules 50% less, and some by 50% more.
			double ruleVariance;

			rule_contributions():
			flockCentroid(0.1),
			flockVelocity(0.1),
			collisionAvoidance(0.1),
			targetSeeking(0.1),
			ruleVariance(0.25)
			{}
		};

		struct config {
			Boid::config boid;
			rule_contributions ruleContributions;
			vector<string> target_ids;
		};

	public:

		// signal fired when all boids in the flock are gone
		signals::signal< void(BoidFlockControllerRef) > onFlockDidFinish;

		// signal fired when a boid in the flock is killed right before it's removed from the level
		signals::signal< void(BoidFlockControllerRef, BoidRef) > onBoidFinished;

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

		const vector<BoidRef> &getFlock() const { return _flock; }

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

		/**
		 Set the targets this Boid flock will pursue.
		 Internally, the targets are held as weak_ptr<> and the flock will pursue the first which is
		 live, is in the level, and has a PhysicsRepresentation to query for position.
		 */
		void setTargets(vector<GameObjectRef> targets);

		/**
		 Add a target for the flock to pursue
		 */
		void addTarget(GameObjectRef target);

		/**
		 Clear the targets this flock will pursue. Flock will continue to follow flcoking rules, but without target seeking
		 */
		void clearTargets();

		const vector<GameObjectWeakRef> getTargets() const { return _targets; }

		/**
		 Get the target the flock is current pursuing. Return pair of GameObjectRef, and its position
		 */
		const pair<GameObjectRef, dvec2> getCurrentTarget() const;

		cpBB getFlockBB() const { return _flockBB; }

		// Component
		void onReady(GameObjectRef parent, LevelRef level) override;
		void update(const time_state &time) override;

	protected:

		friend class Boid;

		void _updateFlock_canonical(const time_state &time);
		void _onBoidFinished(const BoidRef &boid);

	protected:

		string _name;
		vector<BoidRef> _flock;
		vector<GameObjectWeakRef> _targets;
		dvec2 _lastSpawnOrigin;
		config _config;
		ci::Rand _rng;
		cpBB _flockBB;

		// raw ptr for performance - profiling shows 75% of update() loops are wasted on shared_ptr<> refcounting
		vector<BoidPhysicsComponent*> _flockPhysicsComponents;

	};

#pragma mark - BoidFlock

	/**
	 BoidFlock is a GameObject which acts as a controller for a flock of boids.
	 Generally speaking, since BoidFlock is just a GameObject and not an Entity, it's not
	 a thing which can be drawn or have health or be shot. Rather, an "owner" Entity, say
	 the Eggsac, creates a BoidFlock and adds it to the Level.
	 */
	class BoidFlock : public GameObject {
	public:

		struct config {
			BoidFlockController::config controller;
			BoidFlockDrawComponent::config draw;
		};

		static config loadConfig(util::xml::XmlMultiTree flockNode);

		static BoidFlockRef create(string name, config c);

	public:

		BoidFlock(string name, BoidFlockControllerRef controller, BoidFlockDrawComponentRef drawer);
		virtual ~BoidFlock();

		BoidFlockControllerRef getBoidFlockController() const { return _flockController; }
		BoidFlockDrawComponentRef getBoidDrawComponent() const { return _drawer; }

	private:

		BoidFlockControllerRef _flockController;
		BoidFlockDrawComponentRef _drawer;

	};


}}}

#endif /* Boid_hpp */
