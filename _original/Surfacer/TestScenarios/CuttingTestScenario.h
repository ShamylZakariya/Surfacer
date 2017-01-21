#pragma once

//
//  CuttingTestScenario.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/14/11.
//  Copyright 2011 Shamyl Zakariya. All rights reserved.
//

#include "Game.h"


using namespace core;
using namespace ci;

namespace game {
	class ViewportController;
}

/**
	@class CuttingTestScenario
	This is just a very simple duplication of milestone4, to put Level 
	and InputListener through their paces
*/

class CuttingTestScenario : public game::GameScenario, public core::InputListener
{
	public:
	
		CuttingTestScenario();
		virtual ~CuttingTestScenario();

		void setup();
		void shutdown();
		void resize( Vec2i size );
		void update( const time_state &time );
		void draw( const render_state &state );

		// input
		bool mouseDown( const app::MouseEvent &event );
		bool mouseUp( const app::MouseEvent &event );
		bool mouseWheel( const app::MouseEvent &event );
		bool mouseMove( const app::MouseEvent &event, const Vec2 &delta );
		bool mouseDrag( const app::MouseEvent &event, const Vec2 &delta );
		bool keyDown( const app::KeyEvent &event );
		bool keyUp( const app::KeyEvent &event );

	protected:

		void _appendToCut( const Vec2 &p )
		{
			if ( _cut.size() < 2 ) _cut.push_back( p );
			else _cut.back() = p;
		}
		
	protected:
	
		cpBody *_mouseBody;
		cpConstraint *_mouseJoint;
		Vec2 _mouseWorld, _previousMouseWorld, _mouseScreen;
				
		Vec2rVec _cut;			
		Font _font;
};
