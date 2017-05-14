//
//  GameObject.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/27/17.
//
//

#include "GameObject.hpp"

#include <cinder/CinderAssert.h>

#include "Level.hpp"

namespace core {

#pragma mark - IChipmunkUserData

	IChipmunkUserData::IChipmunkUserData()
	{}

	IChipmunkUserData::~IChipmunkUserData()
	{}

	GameObjectRef cpShapeGetGameObject(cpShape *shape) {
		IChipmunkUserData *e = static_cast<IChipmunkUserData*>(cpShapeGetUserData(shape));
		return e ? e->getGameObject() : nullptr;
	}

	GameObjectRef cpBodyGetGameObject(cpBody *body) {
		IChipmunkUserData *e = static_cast<IChipmunkUserData*>(cpBodyGetUserData(body));
		return e ? e->getGameObject() : nullptr;
	}

	GameObjectRef cpConstraintGetGameObject(cpConstraint *constraint) {
		IChipmunkUserData *e = static_cast<IChipmunkUserData*>(cpConstraintGetUserData(constraint));
		return e ? e->getGameObject() : nullptr;
	}

#pragma mark - Component	

	LevelRef Component::getLevel() const {
		return getGameObject()->getLevel();
	}

#pragma mark - DrawComponent

	cpBB DrawComponent::getBB() const {
		return getGameObject()->getPhysicsComponent()->getBB();
	}

	void DrawComponent::notifyMoved() {
		getGameObject()->getLevel()->getDrawDispatcher()->moved(this);
	}

	void DrawComponent::onReady(GameObjectRef parent, LevelRef level) {
		Component::onReady(parent, level);
		level->getDrawDispatcher()->moved(this);
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

	void InputComponent::onReady(GameObjectRef parent, LevelRef level) {
		Component::onReady(parent, level);
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

	void PhysicsComponent::onReady(GameObjectRef parent, LevelRef level) {
		Component::onReady(parent, level);
		_space = level->getSpace();
	}

	void PhysicsComponent::onCleanup() {

		if (LevelRef level = getLevel()) {
			auto self = shared_from_this<PhysicsComponent>();
			for( cpConstraint *c : getConstraints() )
			{
				level->signals.onConstraintWillBeDestroyed(self, c);
			}

			for( cpShape *s : getShapes() )
			{
				level->signals.onShapeWillBeDestroyed(self, s);
			}

			for( cpBody *b : getBodies() )
			{
				level->signals.onBodyWillBeDestroyed(self, b);
			}
		}

		cpCleanupAndFree(_shapes);
		cpCleanupAndFree(_constraints);
		cpCleanupAndFree(_bodies);
		_space.reset();
	}

	void PhysicsComponent::build(cpShapeFilter filter, cpCollisionType collisionType) {
		CI_ASSERT_MSG(_space, "Can't call ::build before SpaceAccess has been assigned.");

		auto parent = getGameObject();
		CI_ASSERT_MSG(parent, "Can't call ::build without a valid GameObject parent instance");

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
		if (LevelRef l = getLevel()) {
			l->signals.onBodyWillBeDestroyed(shared_from_this<PhysicsComponent>(), body);
		}
	}

	void PhysicsComponent::onShapeWillBeDestroyed(cpShape* shape) {
		if (LevelRef l = getLevel()) {
			l->signals.onShapeWillBeDestroyed(shared_from_this<PhysicsComponent>(), shape);
		}
	}

	void PhysicsComponent::onConstraintWillBeDestroyed(cpConstraint* constraint) {
		if (LevelRef l = getLevel()) {
			l->signals.onConstraintWillBeDestroyed(shared_from_this<PhysicsComponent>(), constraint);
		}
	}


#pragma mark - GameObject

	/*
		static size_t _idCounter;
		size_t _id;
		string _name;
		bool _finished, _ready;
		set<ComponentRef> _components;
		DrawComponentRef _drawComponent;
	 */

	size_t GameObject::_idCounter = 0;

	GameObject::GameObject(string name):
	_id(_idCounter++),
	_name(name),
	_finished(false),
	_ready(false)
	{}

	GameObject::~GameObject(){}

	void GameObject::addComponent(ComponentRef component) {
		CI_ASSERT_MSG(component->getGameObject() == nullptr, "Cannot add a component that already has been added to another GameObject");

		_components.insert(component);

		if (DrawComponentRef dc = dynamic_pointer_cast<DrawComponent>(component)) {
			CI_ASSERT_MSG(!_drawComponent, "Can't assign more than one DrawComponent");
			_drawComponent = dc;
		}

		if (PhysicsComponentRef pc = dynamic_pointer_cast<PhysicsComponent>(component)) {
			CI_ASSERT_MSG(!_physicsComponent, "Can't assign more than one PhysicsComponent");
			_physicsComponent = pc;
		}

		if (_ready) {
			const auto self = shared_from_this();
			component->attachedToGameObject(self);
			component->onReady(self,getLevel());
		}
	}

	void GameObject::removeComponent(ComponentRef component) {
		if (component->getGameObject() && component->getGameObject().get() == this) {
			_components.erase(component);
			component->detachedFromGameObject();

			if (_drawComponent == component) {
				_drawComponent.reset();
			}
		}
	}

	void GameObject::onReady(LevelRef level){
		if (!_ready) {
			const auto self = shared_from_this();
			for (auto &component : _components) {
				component->attachedToGameObject(self);
			}
			for (auto &component : _components) {
				component->onReady(self, level);
			}
			_ready = true;
		}
	}

	void GameObject::onCleanup() {
		for (auto &component : _components){
			component->onCleanup();
		}

		_level.reset();
		_finished = false;
		_ready = false;
	}

	void GameObject::step(const time_state &timeState) {
		for (auto &component : _components) {
			component->step(timeState);
		}
	}

	void GameObject::update(const time_state &timeState) {
		for (auto &component : _components) {
			component->update(timeState);
		}
	}


}
