//
//  Boid.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/14/17.
//
//

#include <cinder/Rand.h>

#include "Boid.hpp"

#include "Xml.hpp"

namespace core { namespace game { namespace enemy {

#pragma mark - BoidPhysicsComponent

	namespace {

		void BoidPhysicsComponent_spaceShapeQuery(cpShape *shape, cpContactPointSet *points, void *data) {
			auto sensorData = static_cast<vector<BoidPhysicsComponent::sensor_data>*>(data);
			for (int i = 0; i < points->count; i++) {
				// TODO: Confirm that we care about pointB since pointA ought to be our sensor shape
				sensorData->emplace_back(BoidPhysicsComponent::sensor_data{v2(points->points[i].pointB), points->points[i].distance});
			}
		}

	}

	/*
		config _config;
		cpBody *_body;
		cpShape *_shape, *_sensorShape;
		vector<sensor_data> _sensorData;
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
		cpBodySetVelocity(_body, cpv(_config.dir * _config.speed));

		// TODO: Determine why gear joint is unstable
		//add(cpGearJointNew(cpSpaceGetStaticBody(getSpace()->getSpace()), _body, 0, 0));

		_shape = add(cpCircleShapeNew(_body, _config.radius, cpvzero));
		cpShapeSetFriction(_shape, 0);

		_sensorShape = add(cpCircleShapeNew(_body, _config.sensorRadius, cpvzero));
		cpShapeSetSensor(_sensorShape, true);

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

		dvec2 targetVelocityDir = _targetVelocity;
		double targetVelocity = length(targetVelocityDir);

		if (targetVelocity > 0) {
			// normalize
			targetVelocityDir /= targetVelocity;

			// get the current velocity
			dvec2 vel = v2(cpBodyGetVelocity(_body));
			double linearVelocity = length(vel);
			if (linearVelocity > 0) {
				vel /= linearVelocity;
			}

			// if we're not at max vel we can apply force
			if (linearVelocity < _config.speed) {

				// TODO: Force calculation is fake here

				// scale down the force applied as it gets close to max speed
				double factor = clamp(linearVelocity/_config.speed, 0.0, 1.0);
				factor = (1.0 - factor * factor);

				if (factor > 0) {
					double force = (targetVelocity * time.deltaT) * _mass * factor;
					cpBodyApplyForceAtWorldPoint(_body, cpv(targetVelocityDir * force), cpBodyGetPosition(_body));
				}
			}
		}

		// apply damping
		cpBodySetVelocity(_body, cpvmult(cpBodyGetVelocity(_body), 0.95));

		// TODO: Remove angular velocity damping when gear joint is fixed
		cpBodySetAngularVelocity(_body, cpBodyGetAngularVelocity(_body) * 0.9);

		//
		//	Run a collision query
		//

		_sensorData.clear();
		cpSpaceShapeQuery(getSpace()->getSpace(), _sensorShape, BoidPhysicsComponent_spaceShapeQuery, &_sensorData);
	}

	cpBB BoidPhysicsComponent::getBB() const {
		return cpShapeGetBB(_shape);
	}

#pragma mark - BoidDrawComponent

	/*
		config _config;
		BoidPhysicsComponentWeakRef _physics;
	 */

	BoidDrawComponent::BoidDrawComponent(config c):
	_config(c)
	{}

	BoidDrawComponent::~BoidDrawComponent()
	{}

	void BoidDrawComponent::onReady(GameObjectRef parent, LevelRef level) {
		DrawComponent::onReady(parent, level);
		_physics = getSibling<BoidPhysicsComponent>();
	}

	void BoidDrawComponent::update(const time_state &time) {
		// boids are always movin'
		notifyMoved();
	}

	void BoidDrawComponent::draw(const render_state &renderState) {
		BoidPhysicsComponentRef physics = _physics.lock();
		CI_ASSERT_MSG(physics, "Expect physics component");

		gl::color(0,1,1);
		gl::drawSolidCircle(physics->getPosition(), physics->getRadius(), 8);

		if (physics->getSensorData().empty()) {
			gl::color(0,1,1, 0.25);
		} else {
			gl::color(1,0.25,0.25, 0.5);
		}
		gl::drawSolidCircle(physics->getPosition(), physics->getSensorRadius(), 8);
	}

#pragma mark - Boid 

	/*
	config _config;
	BoidFlockControllerWeakRef _flockController;
	*/

	BoidRef Boid::create(string name, BoidFlockControllerRef flockController, config c, dvec2 initialPosition, dvec2 initialDirection) {
		auto b = make_shared<Boid>(name, flockController, c);

		c.physics.position = initialPosition;
		c.physics.dir = initialDirection;

		auto physics = make_shared<BoidPhysicsComponent>(c.physics);
		auto draw = make_shared<BoidDrawComponent>(c.draw);

		b->addComponent(physics);
		b->addComponent(draw);

		// keep a typed BoidPhysicsComponent to get rid of some dynamic_pointer_casts
		b->_boidPhysics = physics;

		return b;
	}

	Boid::Boid(string name, BoidFlockControllerRef flockController, config c):
	core::GameObject(name),
	_config(c),
	_flockController(flockController)
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
			flockController->onBoidFinished(dynamic_pointer_cast<Boid>(shared_from_this()));
		}

		GameObject::onCleanup();
	}

#pragma mark - BoidFlockController

	BoidFlockControllerRef BoidFlockController::create(string name, ci::XmlTree flockNode) {
		config c = loadConfig(flockNode);
		return make_shared<BoidFlockController>(name, c);

	}

	BoidFlockController::config BoidFlockController::loadConfig(ci::XmlTree flockNode) {
		config c;

		ci::XmlTree rulesNode = flockNode.getChild("rule_contributions");
		c.ruleContributions.collisionAvoidance = util::xml::readNumericAttribute(rulesNode, "collision_avoidance", 0.1);
		c.ruleContributions.flockVelocity = util::xml::readNumericAttribute(rulesNode, "flock_velocity", 0.1);
		c.ruleContributions.flockCentroid = util::xml::readNumericAttribute(rulesNode, "flock_centroid", 0.1);
		c.ruleContributions.targetSeeking = util::xml::readNumericAttribute(rulesNode, "target_seeking", 0.1);

		ci::XmlTree boidNode = flockNode.getChild("boid");
		c.boid.physics.radius = util::xml::readNumericAttribute(boidNode, "radius", 1);
		c.boid.physics.speed = util::xml::readNumericAttribute(boidNode, "speed", 2);
		c.boid.physics.density = util::xml::readNumericAttribute(boidNode, "density", 0.5);
		c.boid.physics.sensorRadius = util::xml::readNumericAttribute(boidNode, "sensorRadius", c.boid.physics.radius * 2);

		return c;
	}


	BoidFlockController::BoidFlockController(string name, config c):
	_name(name),
	_config(c)
	{}

	BoidFlockController::~BoidFlockController()
	{}

	void BoidFlockController::spawn(size_t count, dvec2 origin, dvec2 initialDirection) {
		auto self = shared_from_this_as<BoidFlockController>();
		auto level = getLevel();
		Rand rand;

		for (size_t i = 0; i < count; i++) {
			dvec2 position = origin + dvec2(rand.nextVec2()) * _config.boid.physics.radius;
			dvec2 dir = normalize(initialDirection + dvec2(rand.nextVec2()) * 0.25);

			CI_LOG_D(i << " position: " << position << " dir: " << dir);

			auto b = Boid::create(_name + "_Boid_" + str(i), self, _config.boid, position, dir);
			_flock.push_back(b);

			level->addGameObject(b);
		}
	}

	size_t BoidFlockController::getFlockSize() const {
		return _flock.size();
	}

	void BoidFlockController::update(const time_state &time) {
		updateFlock(time);
	}

	void BoidFlockController::updateFlock(const time_state &time) {

		// early exit
		if (_flock.empty()) {
			return;
		}

		// this is a ROUGH implementation of http://www.vergenet.net/~conrad/boids/pseudocode.html

		const double flockCentroid = _config.ruleContributions.flockCentroid;
		const double flockVelocity = _config.ruleContributions.flockVelocity;
		const double collisionAvoidance = _config.ruleContributions.collisionAvoidance;

		// compute flock centroid
		const double count = _flock.size();
		dvec2 centroid(0,0);
		dvec2 averageVelocity(0,0);

		for (const auto &b : _flock) {
			BoidPhysicsComponentRef boid = b->getBoidPhysicsComponent();
			centroid += boid->getPosition();
			averageVelocity += boid->getVelocity();

			// zero the boid's current force
			boid->setTargetVelocity(dvec2(0));
		}

		centroid /= count;

		// RULES 1 & 3 require dealing with average centroid/velocity which is meaningless if there's only one boid
		if (count >= 2) {
			for (const auto &b : _flock) {
				BoidPhysicsComponentRef boid = b->getBoidPhysicsComponent();
				const dvec2 pos = boid->getPosition();
				const dvec2 vel = boid->getVelocity();

				// RULE1: compute centroid of flock without this boid
				dvec2 nonInclusiveCentroid = ((count * centroid) - pos) / (count-1);
				dvec2 toCentroidVelocity = nonInclusiveCentroid - pos;
				boid->addToTargetVelocity(toCentroidVelocity * flockCentroid);

				// RULE3: compute averageVelocity of flock without this boid
				dvec2 nonInclusiveAverageVel = ((count * averageVelocity) - vel) / (count - 1);
				dvec2 toAverageVelocity = nonInclusiveAverageVel - vel;
				boid->addToTargetVelocity(toAverageVelocity * flockVelocity);
			}
		}

		// RULE 2: Collision Avoidance
		for (const auto &b : _flock) {
			BoidPhysicsComponentRef boid = b->getBoidPhysicsComponent();
			dvec2 boidPosition = boid->getPosition();

			// examine boids' potential collisions and compute corrective velocity
			dvec2 collisionAvoidanceVel(0);
			for (const auto &sensorData : boid->getSensorData()) {
				if (sensorData.distance < 0) { // we have a collision!
					collisionAvoidanceVel += normalize(boidPosition - sensorData.position) * -sensorData.distance;
				}
			}

			boid->addToTargetVelocity(collisionAvoidanceVel * collisionAvoidance);
		}

	}

	void BoidFlockController::onBoidFinished(const BoidRef &boid) {
		_flock.erase(remove(_flock.begin(), _flock.end(), boid), _flock.end());
		if (_flock.empty()) {
			getGameObject()->setFinished(true);
		}
	}




}}} // namespace core::game::enemy
