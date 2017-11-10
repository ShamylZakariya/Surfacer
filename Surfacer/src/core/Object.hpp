//
//  Object.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/27/17.
//
//

#ifndef Object_hpp
#define Object_hpp

#include "Common.hpp"
#include "InputDispatcher.hpp"
#include "RenderState.hpp"
#include "Signals.hpp"
#include "TimeState.hpp"

namespace core {

	SMART_PTR(Object);
	SMART_PTR(Stage);
	SMART_PTR(SpaceAccess);

#pragma mark - IChipmunkUserData

	/**
	 IChipmunkUserData
	 Interface for things which will want to set themselves as user data for a chipmunk shape/body/etc
	 */
	class IChipmunkUserData {
	public:

		IChipmunkUserData();
		virtual ~IChipmunkUserData();

		// return the Object owning this thing, whatever this thing is
		virtual ObjectRef getObject() const { return nullptr; };

	};

	// convenience functions for getting a game object from a cpShape/cpBody/cpConstraint where the user data is a IChipmunkUserData
	ObjectRef cpShapeGetObject(const cpShape *shape);
	ObjectRef cpBodyGetObject(const cpBody *body);
	ObjectRef cpConstraintGetObject(const cpConstraint *constraint);


#pragma mark - Component

	SMART_PTR(Component);
	class Component : public enable_shared_from_this<Component>, public signals::receiver, public IChipmunkUserData {
	public:

		Component(){}
		virtual ~Component(){}

		// IChipmunkUserData
		ObjectRef getObject() const override { return _object.lock(); }

		// get typed shared_from_this, e.g., shared_ptr<FooComponent> = shared_from_this_as<FooComponent>();
		template<typename T>
		shared_ptr<T const> shared_from_this_as() const {
			return static_pointer_cast<T const>(shared_from_this());
		}

		// get typed shared_from_this, e.g., shared_ptr<FooComponent> = shared_from_this_as<FooComponent>();
		template<typename T>
		shared_ptr<T> shared_from_this_as() {
			return static_pointer_cast<T>(shared_from_this());
		}

		StageRef getStage() const;

		template<typename T>
		shared_ptr<T> getObjectAs() {
			return static_pointer_cast<T>(_object.lock());
		}

		template<typename T>
		shared_ptr<T> getSibling() const;

		// called when the owning Object has been added to a stage
		virtual void onReady(ObjectRef parent, StageRef stage){}
		virtual void onCleanup(){}
		virtual void step(const time_state &timeState){}
		virtual void update(const time_state &timeState){}

	protected:
		friend class Object;

		// called on Components immediately after being added to a Object
		// a component at this point can access its Object owne, but does not
		// necessarily have access to its neighbors, or to a Stage. Wait for onReady
		// for these types of actions.
		virtual void attachedToObject(ObjectRef object) {
			_object = object;
		}

		virtual void detachedFromObject() {
			_object.reset();
		}

		// call this if some change moved the represented object. it will be dispatched
		// up to object, and down to DrawComponents to notify the draw dispatch graph
		virtual void notifyMoved();

	private:

		ObjectWeakRef _object;

	};

#pragma mark - PhysicsComponent

	SMART_PTR(PhysicsComponent);
	class PhysicsComponent : public Component {
	public:
		PhysicsComponent():_space(nullptr){}
		virtual ~PhysicsComponent();

		void onReady(ObjectRef parent, StageRef stage) override;
		void onCleanup() override;

		const SpaceAccessRef &getSpace() const { return _space; }

		virtual vector<cpBody*> getBodies() const { return _bodies; }
		virtual vector<cpShape*> getShapes() const { return _shapes; }
		virtual vector<cpConstraint*> getConstraints() const { return _constraints; }

		cpShapeFilter getShapeFilter() const { return _shapeFilter; }
		cpCollisionType getCollisionType() const { return _collisionType; }

		virtual double getGravityModifier(cpBody *body) const { return 1; }

		// get bounding box for all shapes in use
		virtual cpBB getBB() const = 0;

	protected:

		virtual void onBodyWillBeDestroyed(cpBody* body);
		virtual void onShapeWillBeDestroyed(cpShape* shape);
		virtual void onConstraintWillBeDestroyed(cpConstraint* constraint);

		void build(cpShapeFilter filter, cpCollisionType collisionType);
		void setShapeFilter(cpShapeFilter sf);
		void setCollisionType(cpCollisionType ct);

		cpBody *add(cpBody *body) { _bodies.push_back(body); return body; }
		cpShape *add(cpShape *shape) { _shapes.push_back(shape); return shape; }
		cpConstraint *add(cpConstraint *c) { _constraints.push_back(c); return c; }

		void remove(cpBody *);
		void remove(cpShape *);
		void remove(cpConstraint *);

	private:
		SpaceAccessRef _space;
		vector<cpBody*> _bodies;
		vector<cpShape*> _shapes;
		vector<cpConstraint*> _constraints;
		cpShapeFilter _shapeFilter;
		cpCollisionType _collisionType;
	};


#pragma mark - DrawComponent

	namespace VisibilityDetermination
	{
		enum style {
			ALWAYS_DRAW,
			FRUSTUM_CULLING,
			NEVER_DRAW
		};

		inline string toString(style s) {
			switch(s) {
				case ALWAYS_DRAW: return "ALWAYS_DRAW";
				case FRUSTUM_CULLING: return "FRUSTUM_CULLING";
				case NEVER_DRAW: return "NEVER_DRAW";
			}
		}
	}


	SMART_PTR(DrawComponent);
	class DrawComponent : public Component {
	public:

		/**
		 @class BatchDrawDelegate

		 Some DrawComponents are part of a larger whole ( terrain::Shape, or a swarm of identical enemies, 
		 for example ) and it is a great help to efficiency to mark them as being part of a single batch 
		 to be rendered contiguously. In this case, the render system will group the batch together and 
		 then when drawing them, will for the first call prepareForBatchDraw on the first's batchDrawDelegate, 
		 draw each, and then call cleanupAfterBatchDraw on the last of the contiguous set.
		 Note: if these items are on different layers, the draw system will prioritize layer ordering over batching.
		 */
		class BatchDrawDelegate
		{
		public:

			BatchDrawDelegate(){}
			virtual ~BatchDrawDelegate(){}

			/**
			 Called before a batch of like objects are rendered.
			 Passes the render_state, and the first object in the series. This is to simplify
			 situations where a single object is BatchDrawDelegate to multiple target batches.
			 */
			virtual void prepareForBatchDraw( const render_state &, const DrawComponentRef &firstInBatch ){}
			virtual void cleanupAfterBatchDraw( const render_state &, const DrawComponentRef &firstInBatch, const DrawComponentRef &lastInBatch ){}
			
		};

		typedef shared_ptr<BatchDrawDelegate> BatchDrawDelegateRef;

	public:
		DrawComponent(){}
		virtual ~DrawComponent(){}

		// default implementation asks Object for the physicsComponent's BB
		virtual cpBB getBB() const;
		virtual void draw(const render_state &renderState) = 0;
		virtual void drawScreen( const render_state &state ) {};
		virtual VisibilityDetermination::style getVisibilityDetermination() const = 0;
		virtual int getLayer() const = 0;
		virtual int getDrawPasses() const { return 1; }
		virtual BatchDrawDelegateRef getBatchDrawDelegate() const { return nullptr; }

		// Component
		void onReady(ObjectRef parent, StageRef stage) override;

	};

#pragma mark - InputComponent

	SMART_PTR(InputComponent);
	class InputComponent : public Component, public InputListener {
	public:
		InputComponent();
		InputComponent(int dispatchReceiptIndex);

		virtual ~InputComponent(){}

		void onReady(ObjectRef parent, StageRef stage) override;
		bool isListening() const override;

		void monitorKey( int keyCode );
		void ignoreKey( int keyCode );
		
		void monitorKeys(const initializer_list<int> &keyCodes);
		void ignoreKeys(const initializer_list<int> &keyCodes);

		/**
			Called when a monitored key is pressed
		 */
		virtual void monitoredKeyDown( int keyCode ){}

		/**
			Called when a monitored key is released
		 */
		virtual void monitoredKeyUp( int keyCode ){}

		/**
			return true if any monitored keys are pressed.
			this ignores any keys that haven't been registered for monitoring by monitorKey()
		 */
		bool isMonitoredKeyDown( int keyCode ) const;

		bool keyDown( const ci::app::KeyEvent &event ) override;
		bool keyUp( const ci::app::KeyEvent &event ) override;

	private:

		bool _attached;
		map< int, bool > _monitoredKeyStates;

	};

#pragma mark - Object

	/**
	 Object
	 An Object is a thing added to a Stage. An object generally is a composite of components, and can be anything -
	 it could be the terrain for a game, it could be a power up, it could be an enemy, or it could be a controller
	 dispatching high stage changes to the game state based on player progress.
	 
	 A common use is to create an object and add custom DrawComponent and PhysicsComponent implementations.
	 
	 It is perfectly fine to subclass Object to make custom "prefab" like elements which build their own component sets.
	 
	 Note, things which are "alive", e.g., enemies, the player, NPCs - anything with health and a lifecycle and death
	 should derive from Entity, which is a slim object subclass which provides access to HealthComponent and EntityDrawComponent.
	 */
	class Object : public IChipmunkUserData, public enable_shared_from_this<Object>, public signals::receiver {
	public:

		/**
		 Create a non-specialized vanilla Object with some components. This is handy for if you
		 have a simple set of component which needs to be added to a Stage but not be on a complex thing.
		 */
		static ObjectRef with(string name, const initializer_list<ComponentRef> &components) {
			auto obj = make_shared<Object>(name);
			for (auto &component : components) {
				obj->addComponent(component);
			}

			return obj;
		}
		
		/**
		 Create a non-specialized vanilla Object with some components. This is handy for if you
		 have a simple set of component which needs to be added to a Stage but not be on a complex thing.
		 */
		template <typename T>
		static shared_ptr<T> create(string name, const initializer_list<ComponentRef> &components) {
			auto obj = make_shared<T>(name);
			for (auto &component : components) {
				obj->addComponent(component);
			}
			
			return obj;
		}
		
		/**
		 Create a non-specialized vanilla Object with a single component. This is handy for if you
		 have a simple component which needs to be added to a Stage.
		 */
		static ObjectRef with(string name, ComponentRef component) {
			auto obj = make_shared<Object>(name);
			obj->addComponent(component);
			return obj;
		}

		template <typename T>
		static shared_ptr<T> create(string name, ComponentRef component) {
			auto obj = make_shared<T>(name);
			obj->addComponent(component);
			return obj;
		}

	public:

		Object(string name);
		virtual ~Object();

		// get typed shared_from_this, e.g., shared_ptr<FooObj> = shared_from_this_as<FooObj>();
		template<typename T>
		shared_ptr<T const> shared_from_this_as() const {
			return static_pointer_cast<T const>(shared_from_this());
		}

		// get typed shared_from_this, e.g., shared_ptr<FooObj> = shared_from_this_as<FooObj>();
		template<typename T>
		shared_ptr<T> shared_from_this_as() {
			return static_pointer_cast<T>(shared_from_this());
		}

		// IChipmunkUserData
		ObjectRef getObject() const override { return const_cast<Object*>(this)->shared_from_this(); }

		// Object

		// the unique id for this Object - each object is guaranteed a unique id at runtime
		size_t getId() const { return _id; }
		
		// get the name assigned to this object
		string getName() const { return _name; }
		
		// get a debug-friendly description of this object
		string getDescription() const;

		// add a component to this object
		virtual void addComponent(ComponentRef component);
		
		// remove a component from this object
		virtual void removeComponent(ComponentRef component);

		// flag this object as "finished" - it will be removed from the Stage at the next timestep
		// and if nobody is holding any strong references, it will then be deallocated.
		// if secondsFromNow is > 0, the removal will be queued to happen at least that many seconds
		// in the future.
		virtual void setFinished(bool finished=true, seconds_t secondsFromNow=0);

		// returns true if setFinished(true) has been called.
		virtual bool isFinished() const { return _finished; }

		// returns true after this object has been added to the Stage
		// and onReady() has been called, during which all components are attached and have onReady called on them.
		bool isReady() const { return _ready; }

		// gets first component by type, e.g., fooObj->getComponent<PhysicsComponent>()
		template<typename T>
		shared_ptr<T> getComponent() const {
			for (const auto &c : _components) {
				shared_ptr<T> typedC = dynamic_pointer_cast<T>(c);
				if (typedC) {
					return typedC;
				}
			}
			return nullptr;
		}

		// get all draw components attached to this Object
		const set<DrawComponentRef> &getDrawComponents() const { return _drawComponents; }
		
		// get the PhysicsComponent attached to this Object
		PhysicsComponentRef getPhysicsComponent() const { return _physicsComponent; }

		// get the stage this Object is in, or null if it hasn't been added
		StageRef getStage() const { return _stage.lock(); }

		// called after an Object is added to a Stage. Subclasses must call inherited
		virtual void onReady(StageRef stage);

		// called when setFinished is passed a time delay. secondsLeft will count to zero, and amountFinished will ramp from 0->1
		virtual void onFinishing(seconds_t secondsLeft, double amountFinished){}

		// called after a Object is removed from a Stage (directly, or by calling setFinished(true)
		virtual void onCleanup();

		virtual void step(const time_state &timeState);
		virtual void update(const time_state &timeState);

		// if this Object has a PhysicsComponent get the reported BB, else return cpBBInfinity
		virtual cpBB getBB() const;

	protected:

		friend class Stage;
		friend class Component;

		virtual void onAddedToStage(StageRef stage) { _stage = stage; }
		virtual void onRemovedFromStage() {
			onCleanup();
		}

		void notifyMoved();

	private:

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

	};

#pragma mark - Impls

	template<typename T>
	shared_ptr<T> Component::getSibling() const {
		return getObject()->getComponent<T>();
	}

} // namespace core

inline ostream &operator << ( ostream &os, core::VisibilityDetermination::style style )
{
	return os << core::VisibilityDetermination::toString(style);
}


#endif /* Object_hpp */
