//
//  Boid.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/14/17.
//
//


#include "Boid.hpp"

#include "Xml.hpp"

using namespace core;

namespace surfacer { namespace enemy {

#pragma mark - BoidPhysicsComponent

	/*
		config _config;
		cpGroup _group;
		cpBody *_body;
		cpConstraint *_gear;
		cpShape *_shape;
		dvec2 _targetVelocity, _position, _velocity, _rotation, _facingDirection;
		double _mass, _ruleVariance;
	*/

	BoidPhysicsComponent::BoidPhysicsComponent(config c, double ruleVariance, cpGroup group):
	_config(c),
	_group(group),
	_body(nullptr),
	_gear(nullptr),
	_shape(nullptr),
	_targetVelocity(0),
	_position(c.position),
	_velocity(0),
	_rotation(0,0),
	_facingDirection(1,0),
	_mass(0),
	_ruleVariance(ruleVariance)
	{}

	BoidPhysicsComponent::~BoidPhysicsComponent()
	{}

	void BoidPhysicsComponent::setTargetVelocity(dvec2 tv) {
		_targetVelocity = tv;
	}

	void BoidPhysicsComponent::addToTargetVelocity(dvec2 vel) {
		_targetVelocity += vel;
	}

	void BoidPhysicsComponent::setFacingDirection(dvec2 dir) {
		_facingDirection = dir;
	}

	void BoidPhysicsComponent::onReady(ObjectRef parent, StageRef stage) {
		PhysicsComponent::onReady(parent, stage);

		//
		//	A Boid's physics is just a circle
		//

		_mass = _config.radius * _config.radius * M_PI * _config.density;
		_body = add(cpBodyNew(_mass, cpMomentForCircle(_mass, 0, _config.radius, cpvzero)));
		cpBodySetPosition(_body, cpv(_config.position));

		// TODO: Determine why gear joint is unstable
		//_gear = add(cpGearJointNew(cpSpaceGetStaticBody(getSpace()->getSpace()), _body, 0, 0));

		_shape = add(cpCircleShapeNew(_body, _config.radius, cpvzero));
		cpShapeSetFriction(_shape, 0.1);

		cpShapeFilter filter = ShapeFilters::ENEMY;
		filter.group = _group;

		build(filter, CollisionType::ENEMY);
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
		targetVelocity = min(targetVelocity, _config.speed);

		if (targetVelocity > 0) {
			// normalize
			targetDir /= targetVelocity;

			//
			// get the current velocity
			//

			dvec2 currentDir = v2(cpBodyGetVelocity(_body));
			const double currentVelocity = length(currentDir);
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
			} else {
				// dampen motion to slow down - note this is not rigorous math, just tapping the brakes a little
				cpBodySetVelocity(_body, cpvmult(cpBodyGetVelocity(_body), 0.98));
			}

			_position = v2(cpBodyGetPosition(_body));
			_velocity = currentDir * currentVelocity;

			// we face in the direction of motion, ramping down to the assigned facingDirection as we slow down
			_rotation = normalize(_facingDirection + _velocity);
		}

		// prevent rotation
		cpBodySetAngularVelocity(_body, 0);
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

	BoidRef Boid::create(string name, BoidFlockControllerRef flockController, config c, dvec2 initialPosition, dvec2 initialVelocity, double ruleVariance, cpGroup group) {
		auto b = make_shared<Boid>(name, flockController, c);

		c.physics.position = initialPosition;
		auto physics = make_shared<BoidPhysicsComponent>(c.physics, ruleVariance, group);
		physics->addToTargetVelocity(initialVelocity);

		b->addComponent(physics);

		// keep a typed BoidPhysicsComponent to get rid of some dynamic_pointer_casts
		b->_boidPhysics = physics;

		// create health component
		b->addComponent(make_shared<HealthComponent>(c.health));

		return b;
	}

	Boid::Boid(string name, BoidFlockControllerRef flockController, config c):
	Entity(name),
	_config(c),
	_flockController(flockController)
	{}

	Boid::~Boid()
	{}

	void Boid::onHealthChanged(double oldHealth, double newHealth) {
		Entity::onHealthChanged(oldHealth, newHealth);
	}

	void Boid::onDeath() {
		Entity::onDeath();
	}

	void Boid::onReady(StageRef stage) {
		Object::onReady(stage);
	}

	void Boid::onCleanup() {

		// notify that we're done. Our owner might have been killed! So check the ptr.
		BoidFlockControllerRef flockController = _flockController.lock();
		if (flockController) {
			flockController->_onBoidFinished(shared_from_this_as<Boid>());
		}

		Object::onCleanup();
	}

#pragma mark - BoidFlockDrawComponent
	/*
	config _config;
	BoidFlockControllerWeakRef _flockController;
	gl::VboMeshRef _unitCircleMesh;
	*/

	BoidFlockDrawComponent::BoidFlockDrawComponent(config c):
	_config(c)
	{}

	BoidFlockDrawComponent::~BoidFlockDrawComponent()
	{}

	void BoidFlockDrawComponent::onReady(ObjectRef parent, StageRef stage) {
		DrawComponent::onReady(parent, stage);
		_flockController = getSibling<BoidFlockController>();

		auto circle = geom::Circle().center(vec2(0)).radius(1).subdivisions(3);
		_unitCircleMesh = gl::VboMesh::create(circle);
	}

	void BoidFlockDrawComponent::update(const time_state &time) {
		notifyMoved(); // flocks are always on the move
	}

	void BoidFlockDrawComponent::draw(const render_state &renderState) {
		if (BoidFlockControllerRef flock = _flockController.lock()) {
			gl::color(0,1,1);
			dmat4 M(1);
			for (BoidRef b : flock->getFlock()) {

				const BoidPhysicsComponentRef boid = b->getBoidPhysicsComponent();
				const dvec2 position = boid->getPosition();
				const double scale = boid->getRadius();
				const dvec2 rotation = boid->getRotation();

				M[0] = dvec4(scale * rotation.x, scale * rotation.y, 0, 0);
				M[1] = dvec4(scale * -rotation.y, scale * rotation.x, 0, 0);
				M[3] = dvec4(position.x, position.y, 0, 1);

				gl::ScopedModelMatrix smm;
				gl::multModelMatrix(M);
				gl::draw(_unitCircleMesh);
			}

			if (/* DISABLES CODE */ (true) || (renderState.mode == RenderMode::DEVELOPMENT)) {
				auto trackingState = flock->getTrackingState();
				if (trackingState.targetVisible) {
					gl::color(0,1,0,1);
					gl::drawLine(trackingState.eyeBoidPosition, trackingState.targetPosition);
				} else {
					gl::color(1,0,0,1);
				}
				gl::drawStrokedCircle(trackingState.targetPosition, 20 * renderState.viewport->getReciprocalScale(), 20);
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
		cpSpace *_space;
		cpGroup _group;
		string _name;
		vector<BoidRef> _flock;
		vector<ObjectWeakRef> _targets;
		dvec2 _lastSpawnOrigin;
		config _config;
		ci::Rand _rng;
		cpBB _flockBB;
		tracking_state _trackingState;
		dvec2 _swarmTargetOffset;
		seconds_t _nextSwarmTargetOffsetUpdate;

		// raw ptr for performance - profiling shows 75% of update() loops are wasted on shared_ptr<> refcounting
		vector<BoidPhysicsComponent*> _flockPhysicsComponents;
	*/

	BoidFlockController::BoidFlockController(string name, config c):
	_space(nullptr),
	_name(name),
	_config(c),
	_flockBB(cpBBInvalid),
	_swarmTargetOffset(0),
	_nextSwarmTargetOffsetUpdate(0)
	{
		_group = reinterpret_cast<cpGroup>(this);
	}

	BoidFlockController::~BoidFlockController()
	{}

	void BoidFlockController::spawn(size_t count, dvec2 origin, dvec2 initialDirection) {

		_lastSpawnOrigin = origin;
		auto self = shared_from_this_as<BoidFlockController>();
		auto stage = getStage();

		for (size_t i = 0; i < count; i++) {
			const auto position = origin + dvec2(_rng.nextVec2()) * _config.boid.physics.radius;
			const auto velocity = _config.boid.physics.speed * normalize(initialDirection + (dvec2(_rng.nextVec2()) * 0.25));
			const auto rv = 1.0 + _rng.nextFloat(-_config.ruleContributions.ruleVariance, _config.ruleContributions.ruleVariance);

			auto b = Boid::create(_name + "_Boid_" + str(i), self, _config.boid, position, velocity, rv, _group);
			_flock.push_back(b);
			_flockPhysicsComponents.push_back(b->getBoidPhysicsComponent().get());

			stage->addObject(b);
		}
	}

	size_t BoidFlockController::getFlockSize() const {
		return _flock.size();
	}

	void BoidFlockController::setTargets(vector<ObjectRef> targets) {
		_targets.clear();
		for (auto t : targets) {
			_targets.push_back(t);
		}
	}

	void BoidFlockController::addTarget(ObjectRef target) {
		_targets.push_back(target);
	}

	void BoidFlockController::clearTargets() {
		_targets.clear();
	}

	void BoidFlockController::onReady(ObjectRef parent, StageRef stage) {
		_space = stage->getSpace()->getSpace();
		Component::onReady(parent, stage);

		for (auto targetId : _config.target_ids) {
			auto objs = stage->getObjectsByName(targetId);
			for (auto obj : objs) {
				_targets.push_back(obj);
			}
		}
	}

	void BoidFlockController::update(const time_state &time) {
		_updateTrackingState(time);
		_updateFlock_canonical(time);
	}

	void BoidFlockController::_updateTrackingState(const time_state &time) {

		//
		//	Update the swarm offset vector
		//

		if (time.time > _nextSwarmTargetOffsetUpdate) {
			_swarmTargetOffset = _rng.nextVec2();
			_nextSwarmTargetOffsetUpdate = time.time + _rng.randFloat(2, 3);
		}

		//
		//	If we had recent line-of-sight visibility of the player just keep tracking
		//

		if (_trackingState.targetVisible && (time.time - _trackingState.lastTargetVisibleTime < _config.trackingMemorySeconds)) {

			//
			// we're not performing line of sight, etc, but we still need to update eye and target positions
			// NOTE: since we were able to lock on previously, we know we can dereference the optional<dvec2>
			//

			_trackingState.eyeBoidPosition = _trackingState.eyeBoid->getBoidPhysicsComponent()->getPosition();

			// scale the swarm offset vector and apply
			const auto position = _getObjectPosition(_trackingState.target);
			const double size = (cpBBWidth(position->first) + cpBBHeight(position->first)) * 0.5;
			const dvec2 swarmOffset = _swarmTargetOffset * size;

			_trackingState.targetPosition = position->second + swarmOffset;
		}

		else {

			//
			// we need to update the current seeing-eye Boid and look for a target
			//

			if (!_trackingState.targetVisible) {
				// if we couldn't find a target, try next boid
				_trackingState.eyeBoid = _flock[time.step % _flock.size()];
			}

			_trackingState.eyeBoidPosition = _trackingState.eyeBoid->getBoidPhysicsComponent()->getPosition();

			_trackingState.targetVisible = false;
			for (auto possibleTarget : _targets) {
				if (auto target = possibleTarget.lock()) {
					if (auto position = _getObjectPosition(target)) {
						if (_checkLineOfSight(_trackingState.eyeBoidPosition, position->second, target)) {
							_trackingState.target = target;
							_trackingState.lastTargetVisibleTime = time.time;
							_trackingState.targetVisible = true;

							// scale the swarm offset vector and apply
							const double size = (cpBBWidth(position->first) + cpBBHeight(position->first)) * 0.5;
							const dvec2 swarmOffset = _swarmTargetOffset * size;

							_trackingState.targetPosition = position->second + swarmOffset;
							break;
						}
					}
				}
			}

			if (!_trackingState.targetVisible) {
				_trackingState.eyeBoid.reset();
				_trackingState.target.reset();
				_trackingState.lastTargetVisibleTime = 0;
				_trackingState.targetPosition = _flock[0]->getBoidPhysicsComponent()->getPosition();
			}

		}
	}

	boost::optional<pair<cpBB,dvec2>> BoidFlockController::_getObjectPosition(const ObjectRef &obj) const {
		if (PhysicsComponentRef pc = obj->getPhysicsComponent()) {
			cpBB bounds = pc->getBB();
			return make_pair(bounds, v2(cpBBCenter(bounds)));
		}

		return boost::none;
	}

	bool BoidFlockController::_checkLineOfSight(dvec2 start, dvec2 end, ObjectRef target) const {
		// set up a filter that catches everything BUT our Boids
		cpShapeFilter filter = CP_SHAPE_FILTER_ALL;
		filter.group = _group;

		// perform segment query
		cpSegmentQueryInfo info;
		cpSpaceSegmentQueryFirst(_space, cpv(start), cpv(end), 1, filter, &info);

		// test passes if we hit the target and nothing in between
		return info.shape && cpShapeGetObject(info.shape) == target;
	}

	void BoidFlockController::_updateFlock_canonical(const time_state &time) {

		// early exit
		if (_flock.empty()) {
			return;
		}

		cpBB flockBB = cpBBInvalid;
		const dvec2 targetPosition = _trackingState.targetPosition;

		// this is a ROUGH implementation of http://www.vergenet.net/~conrad/boids/pseudocode.html
		// RULE 1: Centroid
		// RULE 2: Average Velocity
		// RULE 3: Collision Avoidance
		// RULE 4: Target Seeking
		// NOTE: The average centroid of the flock is weighed by proximity to the perceiving Boid - e.g., a Boid cares more
		// about the centroid of its close neighbors than the whole flock. This is to encourage partitioning behaviors.

		const double flockCentroidWeight = _config.ruleContributions.flockCentroid;
		const double flockVelocityWeight = _config.ruleContributions.flockVelocity;
		const double collisionAvoidanceWeight = _config.ruleContributions.collisionAvoidance;
		const double targetSeekingWeight = _config.ruleContributions.targetSeeking;

		const size_t flockSize = _flock.size();
		bool runRules123 = flockSize > 1;

		for (auto boid : _flockPhysicsComponents) {
			dvec2 boidPosition = boid->getPosition();
			double rv = boid->getRuleVariance();
			const double sensorRadius = boid->getConfig().sensorRadius;

			// zero current velocity - we'll add to it in the loop below
			boid->setTargetVelocity(dvec2(0,0));

			// rules 1 & 2 & 3
			if (runRules123) {

				// compute centroid and avg velocity weighted by proximity to self
				dvec2 flockAvgCentroid(0,0);
				dvec2 flockAvgVelocity(0,0);
				dvec2 collisionAvoidance(0,0);
				double weight = 0;

				for (auto otherBoid : _flockPhysicsComponents) {
					if (otherBoid != boid) {
						dvec2 otherBoidPosition = otherBoid->getPosition();

						const auto toOtherBoid = otherBoidPosition - boidPosition;
						const auto distanceToOtherBoid = length(toOtherBoid);

						// I tried doing clever stuff here - making falloffScale 1/distanceToOtherBoid allowed boid flocks to
						// "split up" such that boids would preferentially flock with neighbors, not the whole group, but this tended
						// to cause strange effects where stragglers would be left alone and lost. I've disabled all that for a
						// more traditional boids algo. Setting scale=1 gives us a non-weighted generic flock centroid/velocity average.

						const auto falloffScale = 1.0;
						const auto stragglerScaleFix = 0.0;
						const auto scale = falloffScale + stragglerScaleFix;

						weight += scale;
						flockAvgCentroid += scale * otherBoidPosition;
						flockAvgVelocity += scale * otherBoid->getVelocity();

						if (distanceToOtherBoid < sensorRadius) {
							auto penetration = sensorRadius - distanceToOtherBoid;
							collisionAvoidance += penetration * -toOtherBoid;
						}
					}
				}

				flockAvgCentroid /= weight;
				flockAvgVelocity /= weight;

				boid->addToTargetVelocity((flockAvgCentroid - boidPosition) * flockCentroidWeight * rv);
				boid->addToTargetVelocity((flockAvgVelocity - boid->getVelocity()) * flockVelocityWeight * rv);
				boid->addToTargetVelocity(collisionAvoidance * collisionAvoidanceWeight * rv);
			}

			// target seeking
			dvec2 toTarget = targetPosition - boidPosition;
			double distanceToTarget = length(toTarget) + 1e-3;
			toTarget /= distanceToTarget;

			boid->addToTargetVelocity(toTarget * distanceToTarget * targetSeekingWeight * rv);

			// tell boids that in general they ought to face the target (Boids will override this by facing in direction of motion)
			boid->setFacingDirection(toTarget);

			// update BB
			flockBB = cpBBExpand(flockBB, boid->getBB());
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
			getObject()->setFinished(true);
		}
	}

#pragma mark - BoidFlock

	/*
		BoidFlockControllerRef _flockController;
		BoidFlockDrawComponentRef _drawer;
	*/

	BoidFlock::config BoidFlock::loadConfig(util::xml::XmlMultiTree flockNode) {
		config c;

		if (flockNode.hasChild("target")) {
			auto targetNode = flockNode.getChild("target");
			string targets = targetNode.getAttribute("names", "");
			for (auto target : strings::split(targets, ",")) {
				target = strings::strip(target);
				c.controller.target_ids.push_back(target);
			}

			c.controller.trackingMemorySeconds = util::xml::readNumericAttribute<double>(targetNode, "memory_seconds", 5);
		} else {
			c.controller.trackingMemorySeconds = 5;
		}

		auto rulesNode = flockNode.getChild("rule_contributions");
		c.controller.ruleContributions.collisionAvoidance = util::xml::readNumericAttribute<double>(rulesNode, "collision_avoidance", 0.1);
		c.controller.ruleContributions.flockVelocity = util::xml::readNumericAttribute<double>(rulesNode, "flock_velocity", 0.1);
		c.controller.ruleContributions.flockCentroid = util::xml::readNumericAttribute<double>(rulesNode, "flock_centroid", 0.1);
		c.controller.ruleContributions.targetSeeking = util::xml::readNumericAttribute<double>(rulesNode, "target_seeking", 0.1);
		c.controller.ruleContributions.ruleVariance = saturate(util::xml::readNumericAttribute<double>(rulesNode, "rule_variance", 0.25));

		auto boidNode = flockNode.getChild("boid");
		c.controller.boid.physics.radius = util::xml::readNumericAttribute<double>(boidNode, "radius", 1);
		c.controller.boid.physics.speed = util::xml::readNumericAttribute<double>(boidNode, "speed", 2);
		c.controller.boid.physics.density = util::xml::readNumericAttribute<double>(boidNode, "density", 0.5);
		c.controller.boid.physics.sensorRadius = util::xml::readNumericAttribute<double>(boidNode, "sensorRadius", c.controller.boid.physics.radius * 8);
		c.controller.boid.health = HealthComponent::loadConfig(boidNode.getChild("health"));

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
	core::Object(name),
	_flockController(controller),
	_drawer(drawer)
	{}
	
	BoidFlock::~BoidFlock()
	{}


}} // namespace surfacer::enemy
