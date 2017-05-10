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

		gl::ScopedModelMatrix smm;
		gl::multModelMatrix(translate(dvec3(physics->getAttachmentPosition(), 0)) * rotate(physics->getAngle(), dvec3(0,0,1)));

		const auto width = physics->getConfig().width;
		const auto height = physics->getConfig().height;

		gl::color(0.2,0.2,0.8);
		gl::drawSolidRect(Rectf(-width/2, height, width/2, 0));
		gl::color(0.5, 0.5, 0.9);
		gl::drawSolidTriangle(dvec2(-width/2, height), dvec2(0,0), dvec2(width/2, height));
	}

	int EggsacDrawComponent::getLayer() const {
		return DrawLayers::ENEMY;
	}

#pragma mark - EggsacPhysicsComponent

	/*
		config _config;
		dvec2 _attachmentPosition, _up, _right;
		double _angle;
		vector<cpBody*> _bodies;
		vector<cpShape*> _shapes;
		vector<cpConstraint*> _constraints;
	*/

	EggsacPhysicsComponent::EggsacPhysicsComponent(config c):
	_config(c)
	{}

	EggsacPhysicsComponent::~EggsacPhysicsComponent() {
		cpCleanupAndFree(_constraints);
		cpCleanupAndFree(_shapes);
		cpCleanupAndFree(_bodies);
	}

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

		_attachmentPosition = v2(segQueryInfo.point);
		_up = v2(segQueryInfo.normal);
		_right = rotateCW(_up);
		_angle = atan2(_up.y, _up.x);

		// now build our body, shape, etc
		double mass = _config.width * _config.height * _config.density;
		cpBody *body = cpBodyNew(mass, cpMomentForBox(mass, _config.width, _config.height));
		cpBodySetPosition(body, cpv(_attachmentPosition + _config.height/2 * _up));
		cpBodySetAngle(body, _angle);
		_bodies.push_back(body);

		// add a rotational constraint
		cpConstraint *gear = cpGearJointNew(cpSpaceGetStaticBody(space), body, _angle, 1);
		_constraints.push_back(gear);

		// add a position constraint
		cpConstraint *pivot = cpPivotJointNew(cpSpaceGetStaticBody(space), body, cpBodyLocalToWorld(body, cpvzero));
		_constraints.push_back(pivot);

		// add a collision shape
		cpShape *collisionShape = cpSegmentShapeNew(body, cpv(0, -_config.height/2), cpv(0, _config.height/2), _config.width/2);
		_shapes.push_back(collisionShape);


		//
		//	Finalize
		//

		// group just has to be a unique integer so the player parts don't collide with eachother
		cpShapeFilter filter = CollisionFilters::ENEMY;
		filter.group = reinterpret_cast<cpGroup>(parent.get());

		for( cpShape *s : _shapes )
		{
			cpShapeSetUserData( s, parent.get() );
			cpShapeSetFilter(s, filter);
			cpShapeSetCollisionType( s, CollisionType::ENEMY );
			cpShapeSetElasticity( s, 1 );
			getSpace()->addShape(s);
		}

		for( cpBody *b : _bodies )
		{
			cpBodySetUserData( b, parent.get() );
			getSpace()->addBody(b);
		}

		for( cpConstraint *c : _constraints )
		{
			cpConstraintSetUserData( c, parent.get() );
			getSpace()->addConstraint(c);
		}

	}

	void EggsacPhysicsComponent::step(const time_state &time) {
		PhysicsComponent::step(time);
		getGameObject()->getDrawComponent()->notifyMoved();
	}

	cpBB EggsacPhysicsComponent::getBB() const {
		return cpShapeGetBB(_shapes.front());
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
