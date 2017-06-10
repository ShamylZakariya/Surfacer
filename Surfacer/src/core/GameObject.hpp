//
//  GameObject.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/27/17.
//
//

#ifndef GameObject_hpp
#define GameObject_hpp

#include "Common.hpp"
#include "InputDispatcher.hpp"
#include "RenderState.hpp"
#include "Signals.hpp"
#include "TimeState.hpp"

namespace core {

	SMART_PTR(GameObject);
	SMART_PTR(Level);
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

		// return the GameObject owning this thing, whatever this thing is
		virtual GameObjectRef getGameObject() const { return nullptr; };

	};

	// convenience functions for getting a game object from a cpShape/cpBody/cpConstraint where the user data is a IChipmunkUserData
	GameObjectRef cpShapeGetGameObject(cpShape *shape);
	GameObjectRef cpBodyGetGameObject(cpBody *body);
	GameObjectRef cpConstraintGetGameObject(cpConstraint *constraint);


#pragma mark - Component

	SMART_PTR(Component);
	class Component : public enable_shared_from_this<Component>, public signals::receiver, public IChipmunkUserData {
	public:

		Component(){}
		virtual ~Component(){}

		// IChipmunkUserData
		GameObjectRef getGameObject() const override { return _gameObject.lock(); }

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

		LevelRef getLevel() const;

		template<typename T>
		shared_ptr<T> getGameObjectAs() {
			return static_pointer_cast<T>(_gameObject.lock());
		}

		template<typename T>
		shared_ptr<T> getSibling() const;

		// called when the owning GameObject has been added to a level
		virtual void onReady(GameObjectRef parent, LevelRef level){}
		virtual void onCleanup(){}
		virtual void step(const time_state &timeState){}
		virtual void update(const time_state &timeState){}

	protected:
		friend class GameObject;

		// called on Components immediately after being added to a GameObject
		// a component at this point can access its GameObject owne, but does not
		// necessarily have access to its neighbors, or to a Level. Wait for onReady
		// for these types of actions.
		virtual void attachedToGameObject(GameObjectRef gameObject) {
			_gameObject = gameObject;
		}

		virtual void detachedFromGameObject() {
			_gameObject.reset();
		}

	private:

		GameObjectWeakRef _gameObject;

	};

#pragma mark - PhysicsComponent

	SMART_PTR(PhysicsComponent);
	class PhysicsComponent : public Component {
	public:
		PhysicsComponent():_space(nullptr){}
		virtual ~PhysicsComponent();

		void onReady(GameObjectRef parent, LevelRef level) override;
		void onCleanup() override;

		const SpaceAccessRef &getSpace() const { return _space; }

		virtual vector<cpBody*> getBodies() const { return _bodies; }
		virtual vector<cpShape*> getShapes() const { return _shapes; }
		virtual vector<cpConstraint*> getConstraints() const { return _constraints; }

		cpShapeFilter getShapeFilter() const { return _shapeFilter; }
		cpCollisionType getCollisionType() const { return _collisionType; }

		virtual double getGravityModifier() const { return 1; }

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

		// default implementation asks GameObject for the physicsComponent's BB
		virtual cpBB getBB() const;
		virtual void draw(const render_state &renderState) = 0;
		virtual void drawScreen( const render_state &state ) {};
		virtual VisibilityDetermination::style getVisibilityDetermination() const = 0;
		virtual int getLayer() const = 0;
		virtual int getDrawPasses() const { return 1; }
		virtual BatchDrawDelegateRef getBatchDrawDelegate() const { return nullptr; }

		// call this to notify the draw dispatch system that this DrawComponent has an updated BB
		virtual void notifyMoved();

		// Component
		void onReady(GameObjectRef parent, LevelRef level) override;

	};

#pragma mark - InputComponent

	SMART_PTR(InputComponent);
	class InputComponent : public Component, public InputListener {
	public:
		InputComponent();
		InputComponent(int dispatchReceiptIndex);

		virtual ~InputComponent(){}

		void onReady(GameObjectRef parent, LevelRef level) override;
		bool isListening() const override;

		void monitorKey( int keyCode );
		void ignoreKey( int keyCode );

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

		virtual bool keyDown( const ci::app::KeyEvent &event ) override;
		virtual bool keyUp( const ci::app::KeyEvent &event ) override;

	private:

		bool _attached;
		map< int, bool > _monitoredKeyStates;

	};

#pragma mark - GameObject

	class GameObject : public IChipmunkUserData, public enable_shared_from_this<GameObject>, public signals::receiver {
	public:

		/**
		 Create a non-specialized vanilla GameObject with some components. This is handy for if you
		 have a simple component which needs to be added to a Level.
		 */
		static GameObjectRef with(string name, const initializer_list<ComponentRef> &components) {
			auto obj = make_shared<GameObject>(name);
			for (auto &component : components) {
				obj->addComponent(component);
			}

			return obj;
		}

	public:

		GameObject(string name);
		virtual ~GameObject();

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
		GameObjectRef getGameObject() const override { return const_cast<GameObject*>(this)->shared_from_this(); }

		// GameObject

		size_t getId() const { return _id; }
		string getName() const { return _name; }
		string getDescription() const;

		virtual void addComponent(ComponentRef component);
		virtual void removeComponent(ComponentRef component);

		virtual void setFinished(bool finished=true) {
			_finished = finished;
		}

		virtual bool isFinished() const { return _finished; }

		bool isReady() const { return _ready; }

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

		DrawComponentRef getDrawComponent() const { return _drawComponent; }
		PhysicsComponentRef getPhysicsComponent() const { return _physicsComponent; }

		LevelRef getLevel() const { return _level.lock(); }

		virtual void onReady(LevelRef level);

		// called after a GameObject is removed from a Level (directly, or by calling setFinished(true)
		virtual void onCleanup();

		virtual void step(const time_state &timeState);
		virtual void update(const time_state &timeState);

	protected:

		friend class Level;
		virtual void onAddedToLevel(LevelRef level) { _level = level; }
		virtual void onRemovedFromLevel() {
			onCleanup();
		}

	private:

		static size_t _idCounter;
		size_t _id;
		string _name;
		bool _finished;
		bool _ready;
		set<ComponentRef> _components;
		DrawComponentRef _drawComponent;
		PhysicsComponentRef _physicsComponent;
		LevelWeakRef _level;


	};

#pragma mark - Impls

	template<typename T>
	shared_ptr<T> Component::getSibling() const {
		return getGameObject()->getComponent<T>();
	}

} // namespace core

inline ostream &operator << ( ostream &os, core::VisibilityDetermination::style style )
{
	return os << core::VisibilityDetermination::toString(style);
}


#endif /* GameObject_hpp */
