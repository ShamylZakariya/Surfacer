//
//  Scenario.hpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/14/17.
//
//

#ifndef Scenario_hpp
#define Scenario_hpp

#include "Core.hpp"

namespace core {

	SMART_PTR(Scenario)

	class Scenario : public InputListener, public signals::receiver {
	public:

		Scenario();
		virtual ~Scenario();

		virtual void setup(){}
		virtual void cleanup(){}
		virtual void resize( ivec2 size );
		virtual void step( const time_state &time );
		virtual void update( const time_state &time );
		virtual void draw( const render_state &state );

		const Viewport& camera() const { return _camera; }
		Viewport& camera() { return _camera; }

		// time state used for animation
		const time_state &time() const { return _time; }

		// time state used for physics
		const time_state &stepTime() const { return _stepTime; }

		const render_state &renderState() const { return _renderState; }

		void setRenderMode( RenderMode::mode mode );
		RenderMode::mode renderMode() const { return _renderState.mode; }

		/**
		 Save a screenshot as PNG to @a path
		 */
		void screenshot( const ci::fs::path &folderPath, const std::string &namingPrefix, const std::string format = "png" );

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
		
	};
	
}
#endif /* Scenario_hpp */
