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
#include "ViewportController.hpp"
#include "RenderState.hpp"
#include "TimeState.hpp"

namespace core {

	SMART_PTR(Level)
	SMART_PTR(Scenario)

	class Scenario : public InputListener, public signals::receiver, public enable_shared_from_this<Scenario> {
	public:

		Scenario();
		virtual ~Scenario();

		bool isListening() const override;

		virtual void setup(){}
		virtual void cleanup(){}
		virtual void resize( ivec2 size );
		virtual void step( const time_state &time );
		virtual void update( const time_state &time );

		/**
		 Clear the screen. No drawing should be done in here.
		 */
		virtual void clear( const render_state &state );

		/**
		 performs draw with viewport set with its current scale, look, etc.
		 */
		virtual void draw( const render_state &state );

		/**
		 performs draw with a top-left 1-to-1 ortho projection suitable for UI.
		 Note, there's no ViewportRef available in this pass since it's screen-direct.
		 */
		virtual void drawScreen( const render_state &state );

		const ViewportRef& getViewport() const { return _viewport; }
		const ViewportControllerRef getViewportController() const { return _viewportController; }
		void setViewportController(ViewportControllerRef vp);

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

	protected:

		class ScreenViewport : public IViewport {
		public:
			ScreenViewport(){}
			virtual ~ScreenViewport(){}

			void setSize( int width, int height ) { _width = width; _height = height; }
			ivec2 getSize() const override { return ivec2(_width, _height); }
			int getWidth() const override { return _width; }
			int getHeight() const override { return _height; }
			dvec2 getCenter() const override { return dvec2(_width/2.0, _height/2.0); }

			virtual double getScale() const override { return 1.0; };
			virtual double getReciprocalScale() const override { return 1.0; };

			virtual dmat4 getViewMatrix() const override { return mat4(); };
			virtual dmat4 getInverseViewMatrix() const override { return mat4(); };
			virtual dmat4 getProjectionMatrix() const override { return mat4(); };
			virtual dmat4 getInverseProjectionMatrix() const override { return mat4(); };
			virtual dmat4 getViewProjectionMatrix() const override { return mat4(); };
			virtual dmat4 getInverseViewProjectionMatrix() const override { return mat4(); };

			virtual cpBB getFrustum() const override {
				return cpBBNew(0, 0, _width, _height);
			};

		private:

			int _width, _height;

		};

	private:

		ViewportRef _viewport;
		shared_ptr<ScreenViewport> _screenViewport;
		ViewportControllerRef _viewportController;
		time_state _time, _stepTime;
		render_state _renderState, _screenRenderState;
		LevelRef _level;
		int _width, _height;
		
	};
	
}
#endif /* Scenario_hpp */
