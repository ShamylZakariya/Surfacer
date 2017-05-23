//
//  Boid.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/14/17.
//
//


#include "Boid.hpp"

#include "Xml.hpp"

namespace core { namespace game { namespace enemy {

#pragma mark - BoidPhysicsComponent

	/*
		config _config;
		cpBody *_body;
		cpShape *_shape;
		dvec2 _targetVelocity;
		double _mass, _thrustForce;
	*/

	BoidPhysicsComponent::BoidPhysicsComponent(config c):
	_config(c),
	_body(nullptr),
	_shape(nullptr),
	_targetVelocity(0),
	_mass(0)
	{
		_config.filter.group = reinterpret_cast<cpGroup>(this);
	}

	BoidPhysicsComponent::~BoidPhysicsComponent()
	{}

	dvec2 BoidPhysicsComponent::getPosition() const {
		return v2(cpBodyGetPosition(_body));
	}

	dvec2 BoidPhysicsComponent::getVelocity() const {
		return v2(cpBodyGetVelocity(_body));
	}

	double BoidPhysicsComponent::getRadius() const {
		return _config.radius;
	}

	double BoidPhysicsComponent::getSensorRadius() const {
		return _config.sensorRadius;
	}

	void BoidPhysicsComponent::setTargetVelocity(dvec2 tv) {
		_targetVelocity = tv;
	}

	void BoidPhysicsComponent::addToTargetVelocity(dvec2 vel) {
		_targetVelocity += vel;
	}

	void BoidPhysicsComponent::onReady(GameObjectRef parent, LevelRef level) {
		PhysicsComponent::onReady(parent, level);

		//
		//	A Boid's physics is just a circle
		//

		_mass = _config.radius * _config.radius * M_PI * _config.density;
		_body = add(cpBodyNew(_mass, cpMomentForCircle(_mass, 0, _config.radius, cpvzero)));
		cpBodySetPosition(_body, cpv(_config.position));

		// TODO: Determine why gear joint is unstable
		//add(cpGearJointNew(cpSpaceGetStaticBody(getSpace()->getSpace()), _body, 0, 0));

		_shape = add(cpCircleShapeNew(_body, _config.radius, cpvzero));
		cpShapeSetFriction(_shape, 0);

		build(_config.filter, CollisionType::ENEMY);
	}


	void BoidPhysicsComponent::onCleanup() {
		PhysicsComponent::onCleanup();
	}

	void BoidPhysicsComponent::step(const time_state &time) {
		PhysicsComponent::step(time);

		//
		//	Update velocity
		//

		dvec2 targetDir = _targetVelocity;
		double targetVelocity = length(targetDir);

		if (targetVelocity > 0) {
			// normalize
			targetDir /= targetVelocity;

			//CI_LOG_D("tvd: " << targetVelocityDir << " vel: " << targetVelocity);

			//
			// get the current velocity
			//

			dvec2 currentDir = v2(cpBodyGetVelocity(_body));
			double currentVelocity = length(currentDir);
			if (currentVelocity > 0) {
				currentDir /= currentVelocity;
			}

			//
			// if we're not at max vel we can apply force
			//

			if (currentVelocity < _config.speed) {

				// TODO: This is a crude way to limit velocity in a force application scenario, but who wants to write a PID

				//
				// scale down the force to zero applied as it gets close to max speed.
				//

				double easeOffFactor = clamp(currentVelocity/_config.speed, 0.0, 1.0);
				easeOffFactor = (1.0 - easeOffFactor * easeOffFactor);

				//
				//	Scale back up as target velocity dir diverges from current vel - this is to allow
				//	application of force in opposite direction of current velocity. This matters because
				//	if a boid is traveling at maxvel and needs to turn or reverse direction, no force can
				//	be applied since we scale down right above!
				//	As targetDir diverges from currentDir correctiveFactor goes from 0 to 1
				//

				double correctiveFactor = 0.5 - 0.5 * dot(currentDir, targetDir);

				double factor = easeOffFactor + correctiveFactor;
				if (factor > 0) {
					double force = targetVelocity * _mass * factor;
					cpBodyApplyForceAtWorldPoint(_body, cpv(targetDir * force), cpBodyGetPosition(_body));
				}
			}
		}

		// apply damping
		cpBodySetVelocity(_body, cpvmult(cpBodyGetVelocity(_body), 0.99));

		// TODO: Remove angular velocity damping when gear joint is fixed
		cpBodySetAngularVelocity(_body, cpBodyGetAngularVelocity(_body) * 0.9);
	}

	cpBB BoidPhysicsComponent::getBB() const {
		return cpShapeGetBB(_shape);
	}

#pragma mark - Boid 

	/*
		config _config;
		BoidFlockControllerWeakRef _flockController;
		BoidPhysicsComponentRef _boidPhysics;
		double _ruleVariance;
	*/

	BoidRef Boid::create(string name, BoidFlockControllerRef flockController, config c, dvec2 initialPosition, dvec2 initialVelocity, double ruleVariance) {
		auto b = make_shared<Boid>(name, flockController, c, ruleVariance);

		c.physics.position = initialPosition;
		auto physics = make_shared<BoidPhysicsComponent>(c.physics);
		physics->addToTargetVelocity(initialVelocity);

		b->addComponent(physics);

		// keep a typed BoidPhysicsComponent to get rid of some dynamic_pointer_casts
		b->_boidPhysics = physics;

		return b;
	}

	Boid::Boid(string name, BoidFlockControllerRef flockController, config c, double ruleVariance):
	core::GameObject(name),
	_config(c),
	_flockController(flockController),
	_ruleVariance(ruleVariance)
	{}

	Boid::~Boid()
	{}

	void Boid::onReady(LevelRef level) {
		GameObject::onReady(level);
	}

	void Boid::onCleanup() {

		// notify that we're done. Our owner might have been killed! So check the ptr.
		BoidFlockControllerRef flockController = _flockController.lock();
		if (flockController) {
			flockController->_onBoidFinished(dynamic_pointer_cast<Boid>(shared_from_this()));
		}

		GameObject::onCleanup();
	}

#pragma mark - BoidFlockDrawComponent
	/*
	config _config;
	BoidFlockControllerWeakRef _flockController;
	*/

	BoidFlockDrawComponent::BoidFlockDrawComponent(config c):
	_config(c)
	{}

	BoidFlockDrawComponent::~BoidFlockDrawComponent()
	{}

	void BoidFlockDrawComponent::onReady(GameObjectRef parent, LevelRef level) {
		DrawComponent::onReady(parent, level);
		_flockController = getSibling<BoidFlockController>();
	}

	void BoidFlockDrawComponent::update(const time_state &time) {
		notifyMoved(); // flocks are always on the move
	}

	void BoidFlockDrawComponent::draw(const render_state &renderState) {
		if (BoidFlockControllerRef flock = _flockController.lock()) {
			gl::color(0,1,1);
			for (BoidRef b : flock->getFlock()) {
				BoidPhysicsComponentRef boid = b->getBoidPhysicsComponent();
				gl::drawSolidCircle(boid->getPosition(), boid->getRadius(), 8);
			}
		}
	}

	cpBB BoidFlockDrawComponent::getBB() const {
		if (BoidFlockControllerRef c = _flockController.lock()) {
			return c->getFlockBB();
		}
		return cpBBInvalid;
	}



#pragma mark - BoidFlockController

	/*
		string _name;
		vector<BoidRef> _flock;
		vector<GameObjectWeakRef> _targets;
		dvec2 _lastSpawnOrigin;
		config _config;
		ci::Rand _rng;
		cpBB _flockBB;

		vector<BoidPhysicsComponent*> _flockPhysicsComponents;
	*/

	BoidFlockController::BoidFlockController(string name, config c):
	_name(name),
	_config(c)
	{}

	BoidFlockController::~BoidFlockController()
	{}

	void BoidFlockController::spawn(size_t count, dvec2 origin, dvec2 initialDirection) {

		_lastSpawnOrigin = origin;
		auto self = shared_from_this_as<BoidFlockController>();
		auto level = getLevel();

		for (size_t i = 0; i < count; i++) {
			dvec2 position = origin + dvec2(_rng.nextVec2()) * _config.boid.physics.radius;
			dvec2 velocity = _config.boid.physics.speed * normalize(initialDirection + (dvec2(_rng.nextVec2()) * 0.25));

			double rv = _rng.nextFloat(1-_config.ruleContributions.ruleVariance, 1+_config.ruleContributions.ruleVariance);
			auto b = Boid::create(_name + "_Boid_" + str(i), self, _config.boid, position, velocity, rv);
			_flock.push_back(b);
			_flockPhysicsComponents.push_back(b->getBoidPhysicsComponent().get());

			level->addGameObject(b);
		}
	}

	size_t BoidFlockController::getFlockSize() const {
		return _flock.size();
	}

	void BoidFlockController::setTargets(vector<GameObjectRef> targets) {
		_targets.clear();
		for (auto t : targets) {
			_targets.push_back(t);
		}
	}

	void BoidFlockController::addTarget(GameObjectRef target) {
		_targets.push_back(target);
	}

	void BoidFlockController::clearTargets() {
		_targets.clear();
	}

	const pair<GameObjectRef, dvec2> BoidFlockController::getCurrentTarget() const {
		for (auto possibleTarget : _targets) {
			if (auto target = possibleTarget.lock()) {
				if (PhysicsComponentRef pc = target->getPhysicsComponent()) {
					return make_pair(target, v2(cpBBCenter(pc->getBB())));
				} else if (DrawComponentRef dc = target->getDrawComponent()) {
					return make_pair(target, v2(cpBBCenter(dc->getBB())));
				}
			}
		}
		return make_pair(GameObjectRef(), dvec2(0));
	}


	void BoidFlockController::onReady(GameObjectRef parent, LevelRef level) {
		Component::onReady(parent, level);

		for (auto targetId : _config.target_ids) {
			auto objs = level->getGameObjectsByName(targetId);
			for (auto obj : objs) {
				_targets.push_back(obj);
			}
		}
	}

	void BoidFlockController::update(const time_state &time) {
		_updateFlock_canonical(time);
	}

	void BoidFlockController::_updateFlock_canonical(const time_state &time) {

		// early exit
		if (_flock.empty()) {
			return;
		}

		cpBB flockBB = cpBBInvalid;

		// find the first available target
		auto tp = getCurrentTarget();
		dvec2 targetPosition = tp.first ? tp.second : _lastSpawnOrigin;

		// this is a ROUGH implementation of http://www.vergenet.net/~conrad/boids/pseudocode.html
		// RULE 1: Centroid
		// RULE 2: Average Velocity
		// RULE 3: Collision Avoidance
		// RULE 4: Target Seeking

		const double flockCentroidWeight = _config.ruleContributions.flockCentroid;
		const double flockVelocityWeight = _config.ruleContributions.flockVelocity;
		const double collisionAvoidanceWeight = _config.ruleContributions.collisionAvoidance;
		const double targetSeekingWeight = _config.ruleContributions.targetSeeking;

		bool runRules123 = _flock.size() > 1;

		for (auto boid : _flockPhysicsComponents) {
			dvec2 boidPosition = boid->getPosition();
			double rv = 1;//boidGameObject->getRuleVariance();
			const double sensorRadius = boid->getConfig().sensorRadius;

			// zero current velocity - we'll add to it in the loop below
			boid->setTargetVelocity(dvec2(0,0));

			// rules 1 & 2 & 3
			if (runRules123) {

				// compute centroid and avg velocity
				dvec2 flockCentroid(0,0);
				dvec2 flockVelocity(0,0);
				dvec2 collisionAvoidance(0,0);
				double weight = 0;
				for (auto otherBoid : _flockPhysicsComponents) {
					if (otherBoid != boid) {
						dvec2 otherBoidPosition = otherBoid->getPosition();

						auto toBoid = otherBoidPosition - boidPosition;
						auto dist = length(toBoid);
						auto scale = 1 / dist;
						weight += scale;

						flockCentroid += scale * otherBoidPosition;
						flockVelocity += scale * otherBoid->getVelocity();

						if (dist < sensorRadius) {
							auto penetration = sensorRadius - dist;
							collisionAvoidance += penetration * penetration * (toBoid * scale);
						}
					}
				}

				flockCentroid /= weight;
				flockVelocity /= weight;

				boid->addToTargetVelocity((flockCentroid - boidPosition) * flockCentroidWeight * rv);
				boid->addToTargetVelocity((flockVelocity - boid->getVelocity()) * flockVelocityWeight * rv);
				boid->addToTargetVelocity(collisionAvoidance * collisionAvoidanceWeight * rv);
			}

			// target seeking
			boid->addToTargetVelocity((targetPosition - boidPosition) * targetSeekingWeight * rv);

			// update BB
			cpBBExpand(flockBB, boid->getBB());
		}

		_flockBB = flockBB;

	}

	void BoidFlockController::_onBoidFinished(const BoidRef &boid) {

		// remove boid from _flock and its raw ptr BoidPhysicsComponent from _flockPhysicsComponents
		BoidPhysicsComponent* physicsPtr = boid->getBoidPhysicsComponent().get();
		_flockPhysicsComponents.erase(remove(_flockPhysicsComponents.begin(), _flockPhysicsComponents.end(), physicsPtr), _flockPhysicsComponents.end());
		_flock.erase(remove(_flock.begin(), _flock.end(), boid), _flock.end());

		// notify
		onBoidFinished(shared_from_this_as<BoidFlockController>(), boid);

		if (_flock.empty()) {
			onFlockDidFinish(shared_from_this_as<BoidFlockController>());
			getGameObject()->setFinished(true);
		}
	}

#pragma mark - BoidFlock

	/*
		BoidFlockControllerRef _flockController;
		BoidFlockDrawComponentRef _drawer;
	*/

	BoidFlock::config BoidFlock::loadConfig(ci::XmlTree flockNode) {
		config c;

		if (flockNode.hasChild("target")) {
			ci::XmlTree targetNode = flockNode.getChild("target");
			string targets = targetNode.getAttributeValue<string>("names", "");
			for (auto target : strings::split(targets, ",")) {
				target = strings::strip(target);
				c.controller.target_ids.push_back(target);
			}
		}

		ci::XmlTree rulesNode = flockNode.getChild("rule_contributions");
		c.controller.ruleContributions.collisionAvoidance = util::xml::readNumericAttribute(rulesNode, "collision_avoidance", 0.1);
		c.controller.ruleContributions.flockVelocity = util::xml::readNumericAttribute(rulesNode, "flock_velocity", 0.1);
		c.controller.ruleContributions.flockCentroid = util::xml::readNumericAttribute(rulesNode, "flock_centroid", 0.1);
		c.controller.ruleContributions.targetSeeking = util::xml::readNumericAttribute(rulesNode, "target_seeking", 0.1);
		c.controller.ruleContributions.ruleVariance = saturate(util::xml::readNumericAttribute(rulesNode, "rule_variance", 0.25));

		ci::XmlTree boidNode = flockNode.getChild("boid");
		c.controller.boid.physics.radius = util::xml::readNumericAttribute(boidNode, "radius", 1);
		c.controller.boid.physics.speed = util::xml::readNumericAttribute(boidNode, "speed", 2);
		c.controller.boid.physics.density = util::xml::readNumericAttribute(boidNode, "density", 0.5);
		c.controller.boid.physics.sensorRadius = util::xml::readNumericAttribute(boidNode, "sensorRadius", c.controller.boid.physics.radius * 8);

		return c;

	}

	BoidFlockRef BoidFlock::create(string name, config c) {
		auto controller = make_shared<BoidFlockController>(name, c.controller);
		auto drawer = make_shared<BoidFlockDrawComponent>(c.draw);
		auto flock = make_shared<BoidFlock>(name, controller, drawer);
		flock->addComponent(controller);
		flock->addComponent(drawer);
		return flock;
	}


	BoidFlock::BoidFlock(string name, BoidFlockControllerRef controller, BoidFlockDrawComponentRef drawer):
	core::GameObject(name),
	_flockController(controller),
	_drawer(drawer)
	{}
	
	BoidFlock::~BoidFlock()
	{}


}}} // namespace core::game::enemy
