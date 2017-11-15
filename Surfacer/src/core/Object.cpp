//
//  Object.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/27/17.
//
//

#include "Object.hpp"

#include <cinder/CinderAssert.h>

#include "Stage.hpp"

namespace core {

#pragma mark - IChipmunkUserData

	IChipmunkUserData::IChipmunkUserData()
	{}

	IChipmunkUserData::~IChipmunkUserData()
	{}

	ObjectRef cpShapeGetObject(const cpShape *shape) {
		IChipmunkUserData *e = static_cast<IChipmunkUserData*>(cpShapeGetUserData(shape));
		return e ? e->getObject() : nullptr;
	}

	ObjectRef cpBodyGetObject(const cpBody *body) {
		IChipmunkUserData *e = static_cast<IChipmunkUserData*>(cpBodyGetUserData(body));
		return e ? e->getObject() : nullptr;
	}

	ObjectRef cpConstraintGetObject(const cpConstraint *constraint) {
		IChipmunkUserData *e = static_cast<IChipmunkUserData*>(cpConstraintGetUserData(constraint));
		return e ? e->getObject() : nullptr;
	}

#pragma mark - Component	

	StageRef Component::getStage() const {
		return getObject()->getStage();
	}

	void Component::notifyMoved() {
		getObject()->notifyMoved();
	}

#pragma mark - DrawComponent

	cpBB DrawComponent::getBB() const {
		return getObject()->getBB();
	}
	
	void DrawComponent::onReady(ObjectRef parent, StageRef stage) {
		Component::onReady(parent, stage);
		stage->getDrawDispatcher()->moved(this);
	}

#pragma mark - InputComponent

	/*
	 bool _attached;
	 map< int, bool > _monitoredKeyStates;
	 */

	InputComponent::InputComponent():
	_attached(false)
	{}

	InputComponent::InputComponent(int dispatchReceiptIndex):
	InputListener(dispatchReceiptIndex),
	_attached(false)
	{}

	void InputComponent::onReady(ObjectRef parent, StageRef stage) {
		Component::onReady(parent, stage);
		setListening(true);
		_attached = true;
	}

	bool InputComponent::isListening() const {
		return _attached && InputListener::isListening();
	}

	void InputComponent::monitorKey( int keyCode ) {
		_monitoredKeyStates[keyCode] = false;
	}

	void InputComponent::ignoreKey( int keyCode ) {
		_monitoredKeyStates.erase( keyCode );
	}
	
	void InputComponent::monitorKeys(const initializer_list<int> &keyCodes) {
		for (int kc : keyCodes) {
			monitorKey(kc);
		}
	}

	void InputComponent::ignoreKeys(const initializer_list<int> &keyCodes) {
		for (int kc : keyCodes) {
			ignoreKey(kc);
		}
	}

	bool InputComponent::isMonitoredKeyDown( int keyCode ) const {
		auto pos = _monitoredKeyStates.find( keyCode );
		if ( pos != _monitoredKeyStates.end() )
		{
			return pos->second;
		}

		return false;
	}

	bool InputComponent::keyDown( const ci::app::KeyEvent &event ) {
		// if this is a key code we're monitoring, consume the event
		int keyCode = event.getCode();
		auto pos = _monitoredKeyStates.find( keyCode );
		if ( pos != _monitoredKeyStates.end() ) {
			if (!pos->second){
				pos->second = true;
				monitoredKeyDown(keyCode);
			}
			return true;
		}

		return false;
	}

	bool InputComponent::keyUp( const ci::app::KeyEvent &event ) {
		// if this is a key code we're monitoring, consume the event
		int keyCode = event.getCode();
		auto pos = _monitoredKeyStates.find( keyCode );
		if ( pos != _monitoredKeyStates.end() ) {
			if (pos->second) {
				pos->second = false;
				monitoredKeyUp(keyCode);
			}
			return true;
		}

		return false;
	}

#pragma mark - PhysicsComponent

	PhysicsComponent::~PhysicsComponent() {
	}

	void PhysicsComponent::onReady(ObjectRef parent, StageRef stage) {
		Component::onReady(parent, stage);
		_space = stage->getSpace();
	}

	void PhysicsComponent::onCleanup() {

		if (StageRef stage = getStage()) {
			auto self = shared_from_this_as<PhysicsComponent>();
			for( cpConstraint *c : getConstraints() )
			{
				stage->signals.onConstraintWillBeDestroyed(self, c);
			}

			for( cpShape *s : getShapes() )
			{
				stage->signals.onShapeWillBeDestroyed(self, s);
			}

			for( cpBody *b : getBodies() )
			{
				stage->signals.onBodyWillBeDestroyed(self, b);
			}
		}

		cpCleanupAndFree(_shapes);
		cpCleanupAndFree(_constraints);
		cpCleanupAndFree(_bodies);
		_space.reset();
	}
	
	size_t PhysicsComponent::getGravitationLayerMask(cpBody *body) const {
		return ALL_GRAVITATION_LAYERS;		
	}

	void PhysicsComponent::build(cpShapeFilter filter, cpCollisionType collisionType) {
		CI_ASSERT_MSG(_space, "Can't call ::build before SpaceAccess has been assigned.");

		auto parent = getObject();
		CI_ASSERT_MSG(parent, "Can't call ::build without a valid Object parent instance");

		_shapeFilter = filter;
		_collisionType = collisionType;

		for( cpShape *s : getShapes() )
		{
			cpShapeSetUserData( s, parent.get() );
			cpShapeSetFilter(s, filter);
			cpShapeSetCollisionType( s, collisionType );
			getSpace()->addShape(s);
		}

		for( cpBody *b : getBodies() )
		{
			cpBodySetUserData( b, parent.get() );
			getSpace()->addBody(b);
		}

		for( cpConstraint *c : getConstraints() )
		{
			cpConstraintSetUserData( c, parent.get() );
			getSpace()->addConstraint(c);
		}
	}

	void PhysicsComponent::setShapeFilter(cpShapeFilter sf) {
		_shapeFilter = sf;
	}

	void PhysicsComponent::setCollisionType(cpCollisionType ct) {
		_collisionType = ct;
	}

	void PhysicsComponent::onBodyWillBeDestroyed(cpBody* body) {
		if (StageRef l = getStage()) {
			l->signals.onBodyWillBeDestroyed(shared_from_this_as<PhysicsComponent>(), body);
		}
	}

	void PhysicsComponent::onShapeWillBeDestroyed(cpShape* shape) {
		if (StageRef l = getStage()) {
			l->signals.onShapeWillBeDestroyed(shared_from_this_as<PhysicsComponent>(), shape);
		}
	}

	void PhysicsComponent::onConstraintWillBeDestroyed(cpConstraint* constraint) {
		if (StageRef l = getStage()) {
			l->signals.onConstraintWillBeDestroyed(shared_from_this_as<PhysicsComponent>(), constraint);
		}
	}

	void PhysicsComponent::remove(cpBody *b) {
		_bodies.erase(std::remove(_bodies.begin(), _bodies.end(), b), _bodies.end());
	}

	void PhysicsComponent::remove(cpShape *s) {
		_shapes.erase(std::remove(_shapes.begin(), _shapes.end(), s), _shapes.end());
	}

	void PhysicsComponent::remove(cpConstraint *c) {
		_constraints.erase(std::remove(_constraints.begin(), _constraints.end(), c), _constraints.end());
	}



#pragma mark - Object

	/*
	 static size_t _idCounter;
	 size_t _id;
	 string _name;
	 bool _finished, _finishingAfterDelay;
	 seconds_t _finishingDelay, _finishedAfterTime;
	 bool _ready;
	 set<ComponentRef> _components;
	 set<DrawComponentRef> _drawComponents;
	 PhysicsComponentRef _physicsComponent;
	 StageWeakRef _stage;
	 */

	size_t Object::_idCounter = 0;

	Object::Object(string name):
	_id(_idCounter++),
	_name(name),
	_finished(false),
	_finishingAfterDelay(false),
	_finishingDelay(0),
	_finishedAfterTime(0),
	_ready(false)
	{}

	Object::~Object(){}

	string Object::getDescription() const {
		stringstream buf;
		buf << "[id: " << getId() << " name: " << getName() << " isReady: " << isReady() << " isFinished: " << isFinished() << "]";
		return buf.str();
	}

	void Object::addComponent(ComponentRef component) {
		CI_ASSERT_MSG(component->getObject() == nullptr, "Cannot add a component that already has been added to another Object");

		_components.insert(component);

		if (DrawComponentRef dc = dynamic_pointer_cast<DrawComponent>(component)) {
			_drawComponents.insert(dc);
		}

		if (PhysicsComponentRef pc = dynamic_pointer_cast<PhysicsComponent>(component)) {
			CI_ASSERT_MSG(!_physicsComponent, "Can't assign more than one PhysicsComponent");
			_physicsComponent = pc;
		}

		if (_ready) {
			const auto self = shared_from_this();
			component->attachedToObject(self);
			component->onReady(self,getStage());
		}
	}

	void Object::removeComponent(ComponentRef component) {
		if (component->getObject() && component->getObject().get() == this) {
			_components.erase(component);
			component->detachedFromObject();

			if (DrawComponentRef dc = dynamic_pointer_cast<DrawComponent>(component)) {
				_drawComponents.erase(dc);
			}
		}
	}

	void Object::setFinished(bool finished, seconds_t secondsFromNow) {
		if (finished) {
			if (secondsFromNow > 0) {
				_finished = false;
				_finishingAfterDelay = true;
				_finishingDelay = secondsFromNow;
				_finishedAfterTime = time_state::now() + secondsFromNow;
			} else {
				_finished = true; // immediate
				_finishingAfterDelay = false;
				_finishingDelay = 0;
				_finishedAfterTime = 0;
			}
		} else {
			_finished = false;
			_finishingAfterDelay = false;
			_finishingDelay = 0;
			_finishedAfterTime = 0;
		}
	}


	void Object::onReady(StageRef stage){
		if (!_ready) {
			const auto self = shared_from_this();
			for (auto &component : _components) {
				component->attachedToObject(self);
			}
			for (auto &component : _components) {
				component->onReady(self, stage);
			}
			_ready = true;
		}
	}

	void Object::onCleanup() {
		for (auto &component : _components){
			component->onCleanup();
		}

		_stage.reset();
		_finished = false;
		_ready = false;
	}

	void Object::step(const time_state &timeState) {
		if (!_finished) {
			for (auto &component : _components) {
				component->step(timeState);
			}
		}
	}

	void Object::update(const time_state &timeState) {
		if (_finishingAfterDelay > 0) {
			seconds_t remaining = _finishedAfterTime - timeState.time;
			bool finished = remaining <= 0;
			remaining = max<seconds_t>(remaining, 0);
			double amountComplete = 1.0 - clamp(remaining / _finishingDelay, 0.0, 1.0);

			onFinishing(remaining, amountComplete);
			
			if (finished) {
				_finished = true;
			}
		}

		if (!_finished) {
			for (auto &component : _components) {
				component->update(timeState);
			}
		}
	}

	// if this Object has a DrawComponent or a PhysicsComponent get the reported BB, else return cpBBInvalid
	cpBB Object::getBB() const {
		if (_physicsComponent) {
			return _physicsComponent->getBB();
		}
		return cpBBInvalid;
	}
		
	size_t Object::getGravitationLayerMask(cpBody *body) const {
		if (_physicsComponent) {
			return _physicsComponent->getGravitationLayerMask(body);
		}
		return ALL_GRAVITATION_LAYERS;
	}

	void Object::notifyMoved() {
		const auto &dispatcher = getStage()->getDrawDispatcher();
		for (const auto &dc : _drawComponents) {
			dispatcher->moved(dc.get());
		}
	}


}
