//
//  Level.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/27/17.
//
//

#ifndef Level_hpp
#define Level_hpp

#include "Common.hpp"
#include "Object.hpp"
#include "Signals.hpp"
#include "RenderState.hpp"
#include "TimeState.hpp"
#include "ViewportController.hpp"

namespace core {

	SMART_PTR(Scenario);
	SMART_PTR(Level);
	SMART_PTR(DrawDispatcher);
	SMART_PTR(SpaceAccess);
	SMART_PTR(GravitationCalculator);
	SMART_PTR(DirectionalGravitationCalculator);
	SMART_PTR(RadialGravitationCalculator);
	
#pragma mark - GravitationCalculator
	
	class GravitationCalculator {
	public:
		
		struct force {
			dvec2 dir;
			double magnitude;
			
			force(const dvec2 &d, double m):
			dir(d),
			magnitude(m)
			{}
			
			force(const force &g):
			dir(g.dir),
			magnitude(g.magnitude)
			{}
			
			dvec2 getForce() const {
				return dir * magnitude;
			}
		};
		
	public:
		GravitationCalculator();
		virtual ~GravitationCalculator(){}
		
		virtual force calculate(const dvec2 &world) const = 0;
		virtual void update(const time_state &time){}
		
		void setFinished(bool finished) { _finished = finished; }
		virtual bool isFinished() const { return _finished; }

	private:
		
		bool _finished;
		
	};
	
	class DirectionalGravitationCalculator : public GravitationCalculator {
	public:
		
		static DirectionalGravitationCalculatorRef create(dvec2 dir, double magnitude);
		
	public:
		
		DirectionalGravitationCalculator(dvec2 dir, double magnitude);
		~DirectionalGravitationCalculator(){}
		
		// GravitationCalculator
		force calculate(const dvec2 &world) const override;
		
		// DirectionalGravitationCalculator
		void setDir(dvec2 dir);
		dvec2 getDir() const { return _force.dir; }
		
		void setMagnitude(double mag);
		double getMagnitude() const { return _force.magnitude; }
		
	private:
		
		force _force;
		
	};
	
	class RadialGravitationCalculator : public GravitationCalculator {
	public:
		
		static RadialGravitationCalculatorRef create(dvec2 centerOfMass, double magnitude, double falloffPower);
		
	public:
		RadialGravitationCalculator(dvec2 centerOfMass, double magnitude, double falloffPower);
		~RadialGravitationCalculator(){}
		
		// GravitationCalculator
		force calculate(const dvec2 &world) const override;
		
		virtual force calculate(const dvec2 &world, const dvec2 &centerOfMass, double magnitude, double falloffPower) const;

		void setCenterOfMass(dvec2 centerOfMass);
		dvec2 getCenterOfMass() const { return _centerOfMass; }
		
		void setMagnitude(double mag);
		double getMagnitude() const { return _magnitude; }
		
		void setFalloffPower(double fp);
		double getFalloffPower() const { return _falloffPower; }
		
	private:
		
		dvec2 _centerOfMass;
		double _magnitude;
		double _falloffPower;
		
	};
	
#pragma mark - SpaceAccess

	class SpaceAccess {
	public:

		signals::signal< void(cpBody*) > bodyWasAddedToSpace;
		signals::signal< void(cpShape*) > shapeWasAddedToSpace;
		signals::signal< void(cpConstraint*) > constraintWasAddedToSpace;

	public:


		void addBody(cpBody *body);
		void addShape(cpShape *shape);
		void addConstraint(cpConstraint *constraint);

		/**
		 Get gravity vector (direction and magnitude) for a given position in space.
		 */
		GravitationCalculator::force getGravity(const dvec2 &world);

		cpSpace *getSpace() const { return _space; }

	private:
		friend class Level;
		SpaceAccess(cpSpace *space, Level *level);

		cpSpace *_space;
		Level *_level;
	};


#pragma mark - DrawDispatcher

	class DrawDispatcher {
	public:

		struct collector
		{
			set<DrawComponentRef> visible;
			vector<DrawComponentRef> sorted;

			// remove a game object from the collection
			void remove( const DrawComponentRef &dc  )
			{
				visible.erase(dc);
				sorted.erase(std::remove(sorted.begin(),sorted.end(),dc), sorted.end());
			}
		};

		DrawDispatcher();
		virtual ~DrawDispatcher();

		void add( size_t id, const DrawComponentRef & );
		void remove(size_t id);
		void remove( const DrawComponentRef & );
		void moved( const DrawComponentRef & );
		void moved( DrawComponent* );

		void cull( const render_state & );
		void draw( const render_state & );

		const set< DrawComponentRef > &all() const { return _all; }

		/**
			Check if @a obj was visible in the last call to cull()
		 */
		bool visible( const DrawComponentRef &dc ) const {
			return _alwaysVisible.find(dc) != _alwaysVisible.end() || _collector.visible.find(dc) != _collector.visible.end();
		}

	private:

		/**
			render a run of delegates
			returns iterator to last object drawn
		 */
		vector<DrawComponentRef>::iterator _drawDelegateRun( vector<DrawComponentRef>::iterator first, vector<DrawComponentRef>::iterator storageEnd, const render_state &state );

	private:

		cpSpatialIndex *_index;
		set<DrawComponentRef> _all, _alwaysVisible, _deferredSpatialIndexInsertion;
		map<size_t, set<DrawComponentRef>> _drawComponentsById;
		collector _collector;

	};

#pragma mark - Level

	namespace detail {
		extern cpBool Level_collisionBeginHandler(cpArbiter *arb, struct cpSpace *space, cpDataPointer data);
		extern cpBool Level_collisionPreSolveHandler(cpArbiter *arb, struct cpSpace *space, cpDataPointer data);
		extern void Level_collisionPostSolveHandler(cpArbiter *arb, struct cpSpace *space, cpDataPointer data);
		extern void Level_collisionSeparateHandler(cpArbiter *arb, struct cpSpace *space, cpDataPointer data);
	}


	class Level : public enable_shared_from_this<Level>, public core::signals::receiver {
	public:

		/**
			Define signals for elements in the Level to bind to.
			Any object can define its own signals and be bound to directly, but for clarity, I'd like
			important signals to be routed through Level::signals. Viewport gets a pass for declaring 
			signals directly, because it's fairly low in the stack. Otherwise, signals should be defined
			here and objects that want to fire a signal should do so through their Level.
		 */
		struct signals_ {
			signals::signal< void( const Viewport & ) > onViewportMotion;
			signals::signal< void( const Viewport & ) > onViewportBoundsChanged;

			signals::signal< void(const LevelRef &) > onLevelPaused;
			signals::signal< void(const LevelRef &) > onLevelUnpaused;

			signals::signal< void(const PhysicsComponentRef &, cpBody*) > onBodyWillBeDestroyed;
			signals::signal< void(const PhysicsComponentRef &, cpShape*) > onShapeWillBeDestroyed;
			signals::signal< void(const PhysicsComponentRef &, cpConstraint*) > onConstraintWillBeDestroyed;
		};

		signals_ signals;

	public:

		static LevelRef getLevelFromSpace(cpSpace *space) {
			return static_cast<Level*>(cpSpaceGetUserData(space))->shared_from_this_as<Level>();
		}

		inline static Level* getLevelPtrFromSpace(cpSpace *space) {
			return static_cast<Level*>(cpSpaceGetUserData(space));
		}

		struct collision_type_pair {
			cpCollisionType a;
			cpCollisionType b;
			collision_type_pair(cpCollisionType cta, cpCollisionType ctb):
			a(cta),
			b(ctb)
			{}

			friend bool operator == (const collision_type_pair &ctp0, const collision_type_pair &ctp1) {
				return ctp0.a == ctp1.a && ctp0.b == ctp1.b;
			}

			friend bool operator < (const collision_type_pair &ctp0, const collision_type_pair &ctp1) {
				if (ctp0.a != ctp1.a) {
					return ctp0.a < ctp1.a;
				}
				return ctp0.b < ctp1.b;
			}
		};

		/**
		 Callback functor for early phases in collision dispatch. Returning false here will prevent the collision from happening.
		 */
		typedef std::function<bool(const collision_type_pair &ctp, const ObjectRef &a, const ObjectRef &b, cpArbiter *arbiter)> EarlyCollisionCallback;

		/**
		 Callback functor for late phases in collision dispatch.
		 */
		typedef std::function<void(const collision_type_pair &ctp, const ObjectRef &a, const ObjectRef &b, cpArbiter *arbiter)> LateCollisionCallback;

		/**
		 Callback for simplified contact handling
		 */
		typedef std::function<void(const collision_type_pair &ctp, const ObjectRef &a, const ObjectRef &b)> ContactCallback;

	public:

		Level(string name);
		virtual ~Level();

		// get typed shared_from_this, e.g., shared_ptr<Foo> = shared_from_this_as<Foo>();
		template<typename T>
		shared_ptr<T const> shared_from_this_as() const {
			return static_pointer_cast<T const>(shared_from_this());
		}

		// get typed shared_from_this, e.g., shared_ptr<Foo> = shared_from_this_as<Foo>();
		template<typename T>
		shared_ptr<T> shared_from_this_as() {
			return static_pointer_cast<T>(shared_from_this());
		}

		const string &getName() const { return _name; }
		ScenarioRef getScenario() const { return _scenario.lock(); }
		SpaceAccessRef getSpace() const { return _spaceAccess; }

		bool isReady() const { return _ready; }
		bool isPaused() const { return _paused; }
		void setPaused(bool paused = true);

		virtual void resize( ivec2 newSize );
		virtual void step( const time_state &time );
		virtual void update( const time_state &time );
		virtual void draw( const render_state &state );
		virtual void drawScreen( const render_state &state );

		virtual void addObject(ObjectRef obj);
		virtual void removeObject(ObjectRef obj);
		virtual ObjectRef getObjectById(size_t id) const;
		virtual vector<ObjectRef> getObjectsByName(string name) const;

		const DrawDispatcherRef &getDrawDispatcher() const { return _drawDispatcher; }
		const time_state &getTimeState() const { return _time; }
		ViewportRef getViewport() const;
		ViewportControllerRef getViewportController() const;

		void addGravity(const GravitationCalculatorRef &gravityCalculator);
		void removeGravity(const GravitationCalculatorRef &gravityCalculator);
		const vector<GravitationCalculatorRef> &getGravities() const { return _gravities; }
		
		/**
		 get the direction and strength of gravity at a point in world space
		 */
		GravitationCalculator::force getGravitation(dvec2 world) const;

		/**
		 Listen for collisions between the two collision types. Override onCollision* methods to handle the collisions.
		 */
		virtual collision_type_pair addCollisionMonitor(cpCollisionType a, cpCollisionType b);

		virtual void addCollisionBeginHandler(cpCollisionType a, cpCollisionType b, EarlyCollisionCallback);
		virtual void addCollisionPreSolveHandler(cpCollisionType a, cpCollisionType b, EarlyCollisionCallback);
		virtual void addCollisionPostSolveHandler(cpCollisionType a, cpCollisionType b, LateCollisionCallback);
		virtual void addCollisionSeparateHandler(cpCollisionType a, cpCollisionType b, LateCollisionCallback);

		/**
		 Add a more generic contact handler. This maps to PostSolveHandler, but also receives synthetic contact events like WEAPON->ENEMY
		 */
		virtual void addContactHandler(cpCollisionType a, cpCollisionType b, ContactCallback);

		/**
		 Some contacts can't be dispatched via chipmunk's collision system (e.g., synthetic contacts like WEAPON -> ENEMY.
		 It becomes the responsibility of such mechanisms to dispatch their contacts to Level directly, here.
		 */
		virtual void registerContactBetweenObjects(cpCollisionType a, const ObjectRef &ga, cpCollisionType b, const ObjectRef &gb);
		
		struct query_nearest_result {
			dvec2 point;
			double distance;
			cpShape *shape;
			cpBody *body;
			ObjectRef object;
			
			query_nearest_result():
			distance(0),
			shape(nullptr),
			body(nullptr)
			{}
			
			operator bool() const {
				return shape != nullptr;
			}
		};
		
		query_nearest_result queryNearest(dvec2 point, cpShapeFilter filter, double maxDistance = INFINITY);
		
		struct query_segment_result {
			dvec2 point;
			dvec2 normal;
			cpShape *shape;
			cpBody *body;
			ObjectRef object;
			
			query_segment_result():
			shape(nullptr),
			body(nullptr)
			{}
			
			operator bool() const {
				return shape != nullptr;
			}
		};
		
		query_segment_result querySegment(dvec2 a, dvec2 b, double radius, cpShapeFilter filter);

	protected:

		// friend functions for chipmunk collision dispatch - these will call onCollision* methods below
		friend cpBool detail::Level_collisionBeginHandler(cpArbiter *arb, struct cpSpace *space, cpDataPointer data);
		friend cpBool detail::Level_collisionPreSolveHandler(cpArbiter *arb, struct cpSpace *space, cpDataPointer data);
		friend void detail::Level_collisionPostSolveHandler(cpArbiter *arb, struct cpSpace *space, cpDataPointer data);
		friend void detail::Level_collisionSeparateHandler(cpArbiter *arb, struct cpSpace *space, cpDataPointer data);

		// collision dispatchers, called after CollisionCallbacks
		virtual bool onCollisionBegin(cpArbiter *arb) { return true; }
		virtual bool onCollisionPreSolve(cpArbiter *arb) { return true; }
		virtual void onCollisionPostSolve(cpArbiter *arb) {}
		virtual void onCollisionSeparate(cpArbiter *arb) {}

		void setCpBodyVelocityUpdateFunc(cpBodyVelocityFunc f);
		cpBodyVelocityFunc getCpBodyVelocityUpdateFunc() const { return _bodyVelocityFunc; }
		void setName(string name) { _name = name; }

		friend class Scenario;
		virtual void addedToScenario(ScenarioRef scenario);
		virtual void removeFromScenario();
		virtual void onReady();

		virtual void onBodyAddedToSpace(cpBody *body);
		virtual void onShapeAddedToSpace(cpShape *shape);
		virtual void onConstraintAddedToSpace(cpConstraint *constraint);

		virtual void dispatchSyntheticContacts();

	private:

		cpSpace *_space;
		SpaceAccessRef _spaceAccess;
		bool _ready, _paused;
		ScenarioWeakRef _scenario;
		set<ObjectRef> _objects;
		map<size_t,ObjectRef> _objectsById;
		time_state _time;
		string _name;
		DrawDispatcherRef _drawDispatcher;
		cpBodyVelocityFunc _bodyVelocityFunc;
		vector<GravitationCalculatorRef> _gravities;
		
		set<collision_type_pair> _monitoredCollisions;
		map<collision_type_pair, vector<EarlyCollisionCallback>> _collisionBeginHandlers, _collisionPreSolveHandlers;
		map<collision_type_pair, vector<LateCollisionCallback>> _collisionPostSolveHandlers, _collisionSeparateHandlers;
		map<collision_type_pair, vector<ContactCallback>> _contactHandlers;
		map<collision_type_pair, vector<pair<ObjectRef, ObjectRef>>> _syntheticContacts;

	};

}

#endif /* Level_hpp */
