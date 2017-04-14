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

namespace core {

	SMART_PTR(Scenario);
	SMART_PTR(Level);
	SMART_PTR(DrawDispatcher);

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
		std::set< DrawComponentRef > _all;
		std::set< DrawComponentRef > _alwaysVisible;
		collector _collector;

	};


#pragma mark - Level

	class Level : public enable_shared_from_this<Level> {
	public:

		signals::signal< void(const LevelRef &) > levelWasPaused;
		signals::signal< void(const LevelRef &) > levelWasUnpaused;

	public:

		Level(string name);
		virtual ~Level();

		const string &getName() const { return _name; }
		ScenarioRef getScenario() const { return _scenario.lock(); }
		cpSpace* getSpace() const { return _space; }

		bool isReady() const { return _ready; }
		bool isPaused() const { return _paused; }
		void setPaused(bool paused = true);

		virtual void resize( ivec2 newSize );
		virtual void step( const time_state &time );
		virtual void update( const time_state &time );
		virtual void draw( const render_state &state );

		virtual void addGameObject(GameObjectRef obj);
		virtual void removeGameObject(GameObjectRef obj);
		virtual GameObjectRef getGameObjectById(size_t id) const;
		virtual vector<GameObjectRef> getGameObjectsByName(string name) const;

		const DrawDispatcherRef &getDrawDispatcher() const { return _drawDispatcher; }
		const time_state &time() const { return _time; }
		ViewportRef camera();

	protected:

		void setName(string name) { _name = name; }

		friend class Scenario;
		virtual void addedToScenario(ScenarioRef scenario);
		virtual void removeFromScenario();
		virtual void onReady();

	private:

		cpSpace *_space;
		bool _ready, _paused;
		ScenarioWeakRef _scenario;
		set<GameObjectRef> _objects;
		map<size_t,GameObjectRef> _objectsById;
		time_state _time;
		string _name;
		DrawDispatcherRef _drawDispatcher;

	};

}

#endif /* Level_hpp */
