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

		const time_state &time() const { return _time; }
		const Viewport &camera() const;
		Viewport &camera();

	protected:

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

	};

}

#endif /* Level_hpp */
