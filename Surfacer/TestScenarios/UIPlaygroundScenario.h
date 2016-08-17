#pragma once

//
//  UIPlaygroundScenario.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/28/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Game.h"


using namespace core;
using namespace ci;

namespace game {

	class PlayerHud;
	class RotaryDial;
	class ImageView;
	class SelectionList;
	class RichTextBox;

}

/**
	@class MonsterPlaygroundScenario
	This is just a very simple duplication of milestone4, to put Level 
	and InputListener through their paces
*/

class UIPlaygroundScenario : public game::GameScenario, public InputListener
{
	public:
	
		UIPlaygroundScenario();
		virtual ~UIPlaygroundScenario();

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
	
		game::RotaryDial *_testDial;
		game::ImageView *_background;
		game::SelectionList *_testList;
		game::RichTextBox *_richTextBox;
		DamagedMonitorFilter *_crtFilter;
		PalettizingFilter *_palettizerFilter;
	
};