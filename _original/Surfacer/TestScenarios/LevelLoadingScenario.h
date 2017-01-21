#pragma once

//
//  LevelLoadingScenario.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 6/12/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
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

class LevelLoadingScenario : public game::GameScenario, public core::InputListener
{
	public:
	
		LevelLoadingScenario();
		virtual ~LevelLoadingScenario();

		void setup();
		void shutdown();
		void resize( Vec2i size );
		void update( const time_state &time );
		void draw( const render_state &state );

		// input
		bool mouseDrag( const app::MouseEvent &event, const Vec2 &delta );
		bool keyDown( const app::KeyEvent &event );

	protected:
					
};
