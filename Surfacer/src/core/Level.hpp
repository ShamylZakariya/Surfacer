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
#include "GameObject.hpp"
#include "Signals.hpp"
#include "RenderState.hpp"
#include "TimeState.hpp"
#include "ViewportController.hpp"

namespace core {

	SMART_PTR(Scenario);
	SMART_PTR(Level);
	SMART_PTR(DrawDispatcher);
	SMART_PTR(SpaceAccess);

#pragma mark - SpaceAccess

	class SpaceAccess {
	public:

		signals::signal< void(cpBody*) > bodyWasAddedToSpace;
		signals::signal< void(cpShape*) > shapeWasAddedToSpace;
		signals::signal< void(cpConstraint*) > constraintWasAddedToSpace;

		struct gravitation {
			dvec2 dir;
			double force;

			gravitation(const dvec2 &d, double f):
			dir(d),
			force(f)
			{}

			gravitation(const gravitation &g):
			dir(g.dir),
			force(g.force)
			{}
		};


	public:


		void addBody(cpBody *body);
		void addShape(cpShape *shape);
		void addConstraint(cpConstraint *constraint);

		/**
		 Get gravity vector (direction and magnitude) for a given position in space.
		 */
		 gravitation getGravity(const dvec2 &world);

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

		void add( const DrawComponentRef & );
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
		collector _collector;

	};


#pragma mark - Level

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
			return static_cast<Level*>(cpSpaceGetUserData(space))->shared_from_this<Level>();
		}

		inline static Level* getLevelPtrFromSpace(cpSpace *space) {
			return static_cast<Level*>(cpSpaceGetUserData(space));
		}


		enum GravityType {
			DIRECTIONAL,
			RADIAL
		};

		struct radial_gravity_info {
			dvec2 centerOfMass;
			double strength;
			double falloffPower;
		};

	public:

		Level(string name);
		virtual ~Level();

		// get typed shared_from_this, e.g., shared_ptr<Foo> = shared_from_this<Foo>();
		template<typename T>
		shared_ptr<T> shared_from_this() const {
			return dynamic_pointer_cast<T>(enable_shared_from_this<Level>::shared_from_this());
		}

		// get typed shared_from_this, e.g., shared_ptr<Foo> = shared_from_this<Foo>();
		template<typename T>
		shared_ptr<T> shared_from_this() {
			return dynamic_pointer_cast<T>(enable_shared_from_this<Level>::shared_from_this());
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

		virtual void addGameObject(GameObjectRef obj);
		virtual void removeGameObject(GameObjectRef obj);
		virtual GameObjectRef getGameObjectById(size_t id) const;
		virtual vector<GameObjectRef> getGameObjectsByName(string name) const;

		const DrawDispatcherRef &getDrawDispatcher() const { return _drawDispatcher; }
		const time_state &getTime() const { return _time; }
		ViewportRef getViewport() const;
		ViewportControllerRef getViewportController() const;

		void setGravityType(GravityType type);
		GravityType getGravityType() const { return _gravityType; }

		void setDirectionalGravityDirection(dvec2 dir);
		dvec2 getDirectionalGravityDirection(void) const { return _directionalGravityDir; }

		void setRadialGravity(radial_gravity_info rgi);
		radial_gravity_info getRadialGravity() const { return _radialGravityInfo; }

		/**
		 get the direction and strength of gravity at a point in world space
		 */
		SpaceAccess::gravitation getGravitation(dvec2 world) const;

	protected:

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


	private:

		cpSpace *_space;
		SpaceAccessRef _spaceAccess;
		bool _ready, _paused;
		ScenarioWeakRef _scenario;
		set<GameObjectRef> _objects;
		map<size_t,GameObjectRef> _objectsById;
		time_state _time;
		string _name;
		DrawDispatcherRef _drawDispatcher;
		cpBodyVelocityFunc _bodyVelocityFunc;

		GravityType _gravityType;
		dvec2 _directionalGravityDir;
		radial_gravity_info _radialGravityInfo;

	};

}

#endif /* Level_hpp */
