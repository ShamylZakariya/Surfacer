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

	/*
	config _config;
	cpBody *_body;
	cpShape *_shape;
	double _mass;
	*/

	BoidPhysicsComponent::BoidPhysicsComponent(config c):
	_config(c),
	_body(nullptr),
	_shape(nullptr),
	_mass(0)
	{}

	BoidPhysicsComponent::~BoidPhysicsComponent()
	{}

	dvec2 BoidPhysicsComponent::getPosition() const {
		return v2(cpBodyGetPosition(_body));
	}

	double BoidPhysicsComponent::getRadius() const {
		return _config.radius;
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

		// add a rotation constraint so our boids aren't spinning like crazy
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

		b->addComponent(make_shared<BoidPhysicsComponent>(c.physics));
		b->addComponent(make_shared<BoidDrawComponent>(c.draw));

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

	Boid::config BoidFlockController::loadConfig(ci::XmlTree node) {
		Boid::config config;

		config.physics.radius = util::xml::readNumericAttribute(node, "radius", 1);
		config.physics.speed = util::xml::readNumericAttribute(node, "speed", 2);
		config.physics.density = util::xml::readNumericAttribute(node, "density", 0.5);

		return config;
	}


	BoidFlockController::BoidFlockController(string name):
	_name(name)
	{}

	BoidFlockController::~BoidFlockController()
	{}

	void BoidFlockController::spawn(size_t count, dvec2 origin, dvec2 initialDirection, Boid::config config) {
		auto self = shared_from_this_as<BoidFlockController>();
		auto level = getLevel();
		Rand rand;

		for (size_t i = 0; i < count; i++) {
			dvec2 offset = dvec2(rand.nextVec2()) * 0.25;
			dvec2 dir = normalize(initialDirection + offset);

			CI_LOG_D(i << " offset: " << offset << " origin: " << (origin + offset) << " dir: " << dir);

			auto b = Boid::create(_name + "_Boid_" + str(i), self, config, origin + offset, dir);
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

	}

	void BoidFlockController::onBoidFinished(const BoidRef &boid) {
		_flock.erase(remove(_flock.begin(), _flock.end(), boid), _flock.end());
	}




}}} // namespace core::game::enemy
