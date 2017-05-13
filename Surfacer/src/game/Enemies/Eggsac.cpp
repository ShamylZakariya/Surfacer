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

		// draw attachment spring
		{
			cpConstraint *spring = physics->getAttachmentSpring();
			auto a = v2(cpBodyLocalToWorld(cpConstraintGetBodyA(spring), cpDampedSpringGetAnchorA(spring)));
			auto b = v2(cpBodyLocalToWorld(cpConstraintGetBodyB(spring), cpDampedSpringGetAnchorB(spring)));

			gl::color(1,0,0);
			gl::drawLine(a, b);

			gl::color(0,0,1);
			gl::drawSolidCircle(a, 2, 12);

			gl::color(0,1,0);
			gl::drawSolidCircle(b, 2, 12);
		}

	}

	int EggsacDrawComponent::getLayer() const {
		return DrawLayers::ENEMY;
	}

#pragma mark - EggsacPhysicsComponent

	/*
		config _config;
		cpBody *_sacBody;
		cpShape *_sacShape;
		cpConstraint *_attachmentSpring;
		dvec2 _up, _right, _springAttachmentWorld, _currentSpringAttachmentWorld;
		double _angle, _maxSpringStiffness, _currentSpringStiffness;
	 */

	EggsacPhysicsComponent::EggsacPhysicsComponent(config c):
	_config(c),
	_sacBody(nullptr),
	_sacShape(nullptr),
	_attachmentSpring(nullptr),
	_maxSpringStiffness(0),
	_currentSpringStiffness(0)
	{}

	EggsacPhysicsComponent::~EggsacPhysicsComponent() {}

	void EggsacPhysicsComponent::onReady(GameObjectRef parent, LevelRef level) {
		PhysicsComponent::onReady(parent,level);

		cpSpace *space = getSpace()->getSpace();
		dvec2 suggestedPosition = _config.suggestedAttachmentPosition;

		// use a point query to find closest terrain
		cpPointQueryInfo pointQueryInfo;
		cpShape *terrainShape = cpSpacePointQueryNearest(space, cpv(suggestedPosition), INFINITY, CollisionFilters::TERRAIN_PROBE, &pointQueryInfo);
		CI_ASSERT_MSG(terrainShape, "Unable to find a nearby chunk of terrain for the Eggsac to attach to!");

		// now raycast to that point to find attachment angle
		dvec2 dir = v2(pointQueryInfo.point) - suggestedPosition;
		double len = length(dir);
		dir /= len;
		const cpVect segA = cpv(suggestedPosition);
		const cpVect segB = cpv(suggestedPosition + 2 * len * dir ); // extend segment to guaranteee intersect
		cpSegmentQueryInfo segQueryInfo;
		bool hit = cpShapeSegmentQuery(terrainShape, segA, segB, min(_config.width, _config.height) * 0.1, &segQueryInfo);
		CI_ASSERT_MSG(hit, "shape->segment query should have hit!");

		auto up = v2(segQueryInfo.normal);
		auto angle = atan2(up.y, up.x) - M_PI_2;

		// now build our body, shape, etc
		double mass = _config.width * _config.height * _config.density;
		dvec2 attachmentPosition = v2(segQueryInfo.point);
		_sacBody = add(cpBodyNew(mass, cpMomentForBox(mass, _config.width, _config.height)));
		cpBodySetPosition(_sacBody, cpv(attachmentPosition + _config.height/2 * up));
		cpBodySetAngle(_sacBody, angle);

		// add a collision shape
		double segLength = _config.height;
		double segRadius = _config.width/2;
		_sacShape = add(cpSegmentShapeNew(_sacBody, cpv(0, -segLength/2), cpv(0, segLength/2), segRadius));
		cpShapeSetFriction(_sacShape, 4);

		// attach a spring to bottom of body

		_currentSpringStiffness = _maxSpringStiffness = 1 * mass * getSpace()->getGravity(suggestedPosition).force;
		_currentSpringAttachmentWorld = _springAttachmentWorld = v2(segQueryInfo.point);
		double springDamping = 1000;
		_attachmentSpring = add(cpDampedSpringNew(cpSpaceGetStaticBody(space), _sacBody,
												  segQueryInfo.point, cpv(0,-segLength/2 - segRadius/2),
												  0, _currentSpringStiffness, springDamping));


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
		_sacBody = nullptr;
		_sacShape = nullptr;
		_attachmentSpring = nullptr;
	}

	void EggsacPhysicsComponent::step(const time_state &time) {
		PhysicsComponent::step(time);

		const double SpringReattachDist2 = 1 * 1;

		//
		//	As in onReady, run a point query to find closest terrain, and then a segment query to get the normal.
		//	unlike above, this is failable. if we can't get a hit, we go floppy until we do by releasing the constraints.
		//
		cpPointQueryInfo pointQueryInfo;
		cpSpace *space = getSpace()->getSpace();
		cpVect currentPosition = cpBodyGetPosition(_sacBody);
		cpShape *terrainShape = cpSpacePointQueryNearest(space, currentPosition, INFINITY, CollisionFilters::TERRAIN_PROBE, &pointQueryInfo);
		if (terrainShape) {
			// now raycast to that point to find attachment angle
			cpVect dir = cpvsub(pointQueryInfo.point, currentPosition);
			double len = cpvlength(dir);
			dir = cpvmult(dir, 1/len);
			const cpVect segA = currentPosition;
			const cpVect segB = cpvadd(currentPosition, cpvmult(dir, 2*len));
			cpSegmentQueryInfo segQueryInfo;
			bool hit = cpShapeSegmentQuery(terrainShape, segA, segB, min(_config.width, _config.height) * 0.1, &segQueryInfo);
			if (hit) {
				_currentSpringStiffness = lrp<double>(0.2, _currentSpringStiffness, _maxSpringStiffness);

				dvec2 newAttachmentPoint = v2(segQueryInfo.point);
				if (distanceSquared(newAttachmentPoint, _springAttachmentWorld) > SpringReattachDist2) {
					_springAttachmentWorld = newAttachmentPoint;
					CI_LOG_D("Would move attachment from " << _springAttachmentWorld << " -> " << newAttachmentPoint);
				}

			} else {
				_currentSpringStiffness = lrp<double>(0.2, _currentSpringStiffness, 0);
			}
		}

		_currentSpringAttachmentWorld = lrp<dvec2>(0.2, _currentSpringAttachmentWorld, _springAttachmentWorld);
		cpDampedSpringSetAnchorA(_attachmentSpring, cpv(_currentSpringAttachmentWorld));
		cpDampedSpringSetStiffness(_attachmentSpring, _currentSpringStiffness);

		_angle = cpBodyGetAngle(_sacBody);
		_up = dvec2(cos(_angle), sin(_angle));
		_right = rotateCW(_up);
		getGameObject()->getDrawComponent()->notifyMoved();
	}

	cpBB EggsacPhysicsComponent::getBB() const {
		return cpShapeGetBB(_sacShape);
	}

#pragma mark - Eggsac

	EggsacRef Eggsac::create(string name, dvec2 position, ci::XmlTree node) {
		Eggsac::config config;

		config.physics.width = util::xml::readNumericAttribute(node, "width", 8);
		config.physics.height = util::xml::readNumericAttribute(node, "height", 8);
		config.physics.density = util::xml::readNumericAttribute(node, "density", 1);
		config.physics.suggestedAttachmentPosition = position;


		EggsacRef eggsac = make_shared<Eggsac>(name);
		eggsac->addComponent(make_shared<EggsacPhysicsComponent>(config.physics));
		eggsac->addComponent(make_shared<EggsacDrawComponent>(config.draw));

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
