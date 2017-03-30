//
//  Scenario.hpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/14/17.
//
//

#ifndef Scenario_hpp
#define Scenario_hpp

#include "InputDispatcher.hpp"
#include "Viewport.hpp"
#include "RenderState.hpp"
#include "TimeState.hpp"

namespace core {

	SMART_PTR(Level)
	SMART_PTR(Scenario)

	class Scenario : public InputListener, public signals::receiver, public enable_shared_from_this<Scenario> {
	public:

		Scenario();
		virtual ~Scenario();

		virtual void setup(){}
		virtual void cleanup(){}
		virtual void resize( ivec2 size );
		virtual void step( const time_state &time );
		virtual void update( const time_state &time );
		virtual void clear( const render_state &state );
		virtual void draw( const render_state &state );

		const Viewport& getCamera() const { return _camera; }
		Viewport& getCamera() { return _camera; }

		// time state used for animation
		const time_state &getTime() const { return _time; }

		// time state used for physics
		const time_state &getStepTime() const { return _stepTime; }

		const render_state &getRenderState() const { return _renderState; }

		void setRenderMode( RenderMode::mode mode );
		RenderMode::mode getRenderMode() const { return _renderState.mode; }

		/**
		 Save a screenshot as PNG to @a path
		 */
		void screenshot( const ci::fs::path &folderPath, const std::string &namingPrefix, const std::string format = "png" );

		void setLevel(LevelRef level);
		LevelRef getLevel() const { return _level; }

	protected:
		friend class GameApp;

		virtual void _dispatchSetup();
		virtual void _dispatchCleanup();
		virtual void _dispatchResize( const ivec2 &size );
		virtual void _dispatchStep();
		virtual void _dispatchUpdate();
		virtual void _dispatchDraw();


	private:

		Viewport _camera;
		time_state _time, _stepTime;
		render_state _renderState;
		LevelRef _level;
		
	};
	
}
#endif /* Scenario_hpp */
