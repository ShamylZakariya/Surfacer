//
//  Eggsac.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/8/17.
//
//

#include "Eggsac.hpp"

#include "GameLevel.hpp"
#include "Xml.hpp"
#include "ChipmunkDebugDraw.hpp"


namespace core { namespace game { namespace enemy {

#pragma mark - EggsacDrawComponent


	EggsacDrawComponent::EggsacDrawComponent(config c):
	_config(c)
	{}

	EggsacDrawComponent::~EggsacDrawComponent(){
	}

	// DrawComponent
	void EggsacDrawComponent::onReady(GameObjectRef parent, LevelRef level) {
		DrawComponent::onReady(parent, level);
		_physics = getSibling<EggsacPhysicsComponent>();
	}

	void EggsacDrawComponent::draw(const render_state &renderState) {
		EggsacPhysicsComponentRef physics = _physics.lock();
		CI_ASSERT_MSG(physics, "Expect to find an EggsacPhysicsComponent");

		cpShape *bodyShape = physics->getBodyShape();
		cpBody *body = physics->getBody();

		{
			auto a = v2(cpBodyLocalToWorld(body, cpSegmentShapeGetA(bodyShape)));
			auto b = v2(cpBodyLocalToWorld(body, cpSegmentShapeGetB(bodyShape)));
			auto radius = cpSegmentShapeGetRadius(bodyShape);
			gl::color(1,1,1);
			util::cdd::DrawCapsule(a, b, radius);
		}

		if (cpConstraint *spring = physics->getAttachmentSpring()) {
			auto a = v2(cpBodyLocalToWorld(cpConstraintGetBodyA(spring), cpDampedSpringGetAnchorA(spring)));
			auto b = v2(cpBodyLocalToWorld(cpConstraintGetBodyB(spring), cpDampedSpringGetAnchorB(spring)));

			gl::color(1,0,0);
			gl::drawLine(a, b);

			gl::color(0,0,1);
			gl::drawSolidCircle(a, 2, 12);

			gl::color(0,1,0);
			gl::drawSolidCircle(b, 2, 12);
		}

		// draw up and right vectors
		double len = 20 * renderState.viewport->getReciprocalScale();

		gl::color(1,0,0);
		gl::drawLine(physics->getPosition(), physics->getPosition() + len * physics->getUp());

		gl::color(0,1,0);
		gl::drawLine(physics->getPosition(), physics->getPosition() + len * physics->getRight());
	}

	int EggsacDrawComponent::getLayer() const {
		return DrawLayers::ENEMY;
	}

#pragma mark - EggsacPhysicsComponent

	/*
		config _config;
		cpBody *_sacBody, *_attachedToBody;
		cpShape *_sacShape, *_attachedToShape;
		cpConstraint *_attachmentSpring, *_orientationSpring;
		dvec2 _up, _right;
		double _mass;
	 */

	EggsacPhysicsComponent::EggsacPhysicsComponent(config c):
	_config(c),
	_sacBody(nullptr),
	_attachedToBody(nullptr),
	_sacShape(nullptr),
	_attachedToShape(nullptr),
	_attachmentSpring(nullptr),
	_orientationSpring(nullptr),
	_mass(0)
	{}

	EggsacPhysicsComponent::~EggsacPhysicsComponent() {}

	bool EggsacPhysicsComponent::isAttached() const {
		return _attachedToBody != nullptr;
	}

	void EggsacPhysicsComponent::onReady(GameObjectRef parent, LevelRef level) {
		PhysicsComponent::onReady(parent,level);

		// register to be notified when bodies/shapes are destroyed. We need to know so we can detach our spring/gear
		level->signals.onBodyWillBeDestroyed.connect(this, &EggsacPhysicsComponent::onBodyWillBeDestroyed);
		level->signals.onShapeWillBeDestroyed.connect(this, &EggsacPhysicsComponent::onShapeWillBeDestroyed);

		dvec2 position = _config.suggestedAttachmentPosition;


		// build our body
		_mass = _config.width * _config.height * _config.density;
		_sacBody = add(cpBodyNew(_mass, cpMomentForBox(_mass, _config.width, _config.height)));
		cpBodySetPosition(_sacBody, cpv(position));

		// add a collision shape
		double segLength = _config.height;
		double segRadius = _config.width/2;
		_sacShape = add(cpSegmentShapeNew(_sacBody, cpv(0, -segLength/2), cpv(0, segLength/2), segRadius));
		cpShapeSetFriction(_sacShape, 4);

		//
		//	Finalize
		//

		// group just has to be a unique integer so the player parts don't collide with eachother
		cpShapeFilter filter = CollisionFilters::ENEMY;
		filter.group = reinterpret_cast<cpGroup>(parent.get());
		build(filter, CollisionType::ENEMY);
	}

	void EggsacPhysicsComponent::onCleanup() {
		PhysicsComponent::onCleanup();
		detach();
	}

	void EggsacPhysicsComponent::step(const time_state &time) {
		PhysicsComponent::step(time);

		if (isAttached()) {

			double len = cpDampedSpringGetRestLength(_attachmentSpring);
			if (len > 1e-3) {
				len = lrp<double>(0.2, len, 0);
				cpDampedSpringSetRestLength(_attachmentSpring, len);
			}

		} else {
			// try to attach to something
			attach();
		}

		double av = cpBodyGetAngularVelocity(_sacBody);
		cpBodySetAngularVelocity(_sacBody, av * 0.9);

		double angle = cpBodyGetAngle(_sacBody) + M_PI_2;
		_up = dvec2(cos(angle), sin(angle));
		_right = rotateCW(_up);

		getGameObject()->getDrawComponent()->notifyMoved();
	}

	cpBB EggsacPhysicsComponent::getBB() const {
		return cpShapeGetBB(_sacShape);
	}

	void EggsacPhysicsComponent::onBodyWillBeDestroyed(PhysicsComponentRef physics, cpBody *body) {
		if (body == _attachedToBody) {
			detach();
		}
	}

	void EggsacPhysicsComponent::onShapeWillBeDestroyed(PhysicsComponentRef physics, cpShape *shape) {
		if (shape == _attachedToShape) {
			detach();
		}
	}

	void EggsacPhysicsComponent::attach() {

		CI_ASSERT_MSG(!isAttached(), "Can't attach when already attached!");

		cpPointQueryInfo pointQueryInfo;
		cpSpace *space = getSpace()->getSpace();
		cpVect currentPosition = cpBodyGetPosition(_sacBody);
		cpShape *terrainShape = cpSpacePointQueryNearest(space, currentPosition, INFINITY, CollisionFilters::TERRAIN_PROBE, &pointQueryInfo);
		if (terrainShape) {

			_attachedToShape = terrainShape;
			_attachedToBody = cpShapeGetBody(terrainShape);

			double stiffness = _mass * getSpace()->getGravity(v2(currentPosition)).force;
			double damping = 1;
			double restLength = cpvdist(currentPosition, pointQueryInfo.point);
			double segLength = _config.height;
			double segRadius = _config.width/2;


			// create spring with length set to current distance to attachment point - we'll reel in in step()
			_attachmentSpring = add(cpDampedSpringNew(_attachedToBody, _sacBody,
													  cpBodyWorldToLocal(_attachedToBody, pointQueryInfo.point), cpv(0,-segLength/2 - segRadius/2),
													  restLength, stiffness, damping));

			GameObjectRef parent = getGameObject();
			cpConstraintSetUserData( _attachmentSpring, parent.get() );
			getSpace()->addConstraint(_attachmentSpring);

			_orientationSpring = add(cpDampedRotarySpringNew(_attachedToBody, _sacBody, 0, stiffness, damping * 4));
			cpConstraintSetUserData(_orientationSpring, parent.get());
			getSpace()->addConstraint(_orientationSpring);
		}
	}

	void EggsacPhysicsComponent::detach() {
		if (isAttached()) {
			cpCleanupAndFree(_attachmentSpring);
			cpCleanupAndFree(_orientationSpring);
			_attachedToBody = nullptr;
			_attachedToShape = nullptr;
		}
	}

#pragma mark - EggsacSpawnComponent

	/*
		config _config;
		int _spawnCount;
		BoidFlockControllerRef _flock;
	*/

	EggsacSpawnComponent::EggsacSpawnComponent(config c):
	_config(c),
	_spawnCount(0)
	{}

	EggsacSpawnComponent::~EggsacSpawnComponent()
	{}

	int EggsacSpawnComponent::getSpawnCount() const {
		return _spawnCount;
	}

	bool EggsacSpawnComponent::canSpawn() const {
		// we can spawn iff we haven't exhausted self, and if there's no current flock
		BoidFlockControllerRef flock = _flock.lock();
		return (_spawnCount < _config.spawnCount) && (!flock || flock->getFlockSize() == 0);
	}

	void EggsacSpawnComponent::spawn() {
		CI_ASSERT_MSG(canSpawn(), "Can't spawn when canSpawn() == false");
		CI_LOG_D("Spawning a flock!");

		_spawnCount++;

		EggsacPhysicsComponentRef physics = getSibling<EggsacPhysicsComponent>();
		BoidFlockControllerRef flock = _flock.lock();

		if (!flock) {

			// build a config for our boids with our shape filter (so boids don't collide with eachother or the eggsac)
			auto flockConfig = _config.flock;
			flockConfig.boid.physics.filter = CP_SHAPE_FILTER_ALL;

			_flock = flock = make_shared<BoidFlockController>(getGameObject()->getName(), flockConfig);
			flock->onFlockDidFinish.connect(this, &EggsacSpawnComponent::onFlockDidFinish);

			GameObjectRef parent = getGameObject();
			string flockName = parent->getName() + "_BoidFlockController";
			auto flockObject = GameObject::with(flockName, {flock});
			getLevel()->addGameObject(flockObject);
		}

		dvec2 origin = physics->getPosition() + physics->getHeight() * 1.25 * physics->getUp();
		dvec2 dir = physics->getUp();

		flock->spawn(_config.spawnCount, origin, dir);
	}

	void EggsacSpawnComponent::update(const time_state &time) {
		if (canSpawn()) {
			spawn();
		}
	}

	void EggsacSpawnComponent::onFlockDidFinish(const BoidFlockControllerRef &fc) {
		_flock.reset();
	}


#pragma mark - Eggsac

	EggsacRef Eggsac::create(string name, dvec2 position, ci::XmlTree node) {
		Eggsac::config config;

		config.physics.width = util::xml::readNumericAttribute(node, "width", 8);
		config.physics.height = util::xml::readNumericAttribute(node, "height", 8);
		config.physics.density = util::xml::readNumericAttribute(node, "density", 1);
		config.physics.suggestedAttachmentPosition = position;

		if (node.hasChild("spawn")) {
			auto spawnNode = node.getChild("spawn");
			config.spawn.spawnCount = util::xml::readNumericAttribute(spawnNode, "count", 1);
			config.spawn.spawnSize = util::xml::readNumericAttribute(spawnNode, "size", 1);

			CI_ASSERT_MSG(spawnNode.hasChild("flock"), "If <eggsac> node specifies a child <spawn> node, that child must have a <boid> subnode to configure its flock");
			auto flockNode = spawnNode.getChild("flock");
			config.spawn.flock = BoidFlockController::loadConfig(flockNode);
		}


		EggsacRef eggsac = make_shared<Eggsac>(name);
		eggsac->addComponent(make_shared<EggsacPhysicsComponent>(config.physics));
		eggsac->addComponent(make_shared<EggsacDrawComponent>(config.draw));
		eggsac->addComponent(make_shared<EggsacSpawnComponent>(config.spawn));

		return eggsac;
	}

	Eggsac::Eggsac(string name):
	core::GameObject(name){}

	Eggsac::~Eggsac(){}

	// GameObject
	void Eggsac::update(const time_state &time) {
		GameObject::update(time);
	}




}}} // namespace core::game::enemy
