#pragma once

//
//  SensorTestScenario.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/10/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Game.h"

using namespace core;
using namespace ci;

namespace game {
	class ViewportController;
	class Monster;
}

/**
	@class SensorTestScenario
	This is just a very simple duplication of milestone4, to put Level 
	and InputListener through their paces
*/

class SensorTestScenario : public game::GameScenario, public InputListener
{
	public:
	
		SensorTestScenario();
		virtual ~SensorTestScenario();

		void setup();
		void shutdown();
		void resize( Vec2i size );
		void update( const time_state & );
		void draw( const render_state & );

		// input
		bool mouseDown( const app::MouseEvent &event );
		bool mouseUp( const app::MouseEvent &event );
		bool mouseWheel( const app::MouseEvent &event );
		bool mouseMove( const app::MouseEvent &event, const Vec2r &delta );
		bool mouseDrag( const app::MouseEvent &event, const Vec2r &delta );
		bool keyDown( const app::KeyEvent &event );
		bool keyUp( const app::KeyEvent &event );


	protected:

		real _zoom() const;
		void _setZoom( real z );
				
	protected:
	
		game::ViewportController *_cameraController;		
		Font _font;
		
		cpBody *_mouseBody, *_draggingBody;
		cpConstraint *_mouseJoint;						
		Vec2r _mouseWorld, _previousMouseWorld;
};