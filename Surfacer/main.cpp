#include <cinder/app/AppBasic.h>
#include <chipmunk/chipmunk.h>

#include "SurfacerApp.h"
#include "Scenario.h"

///////////////////////////////////////////////////////////////////////
// Testing Scenarios

#include "CuttingTestScenario.h"
#include "LevelLoadingScenario.h"
#include "MonsterPlaygroundScenario.h"
#include "UIPlaygroundScenario.h"
#include "SensorTestScenario.h"

///////////////////////////////////////////////////////////////////////

using namespace ci;
using namespace ci::app;

#pragma mark -
#pragma mark App

class Main : public game::SurfacerApp 
{
	public:
	
		Main()
		{
			// sanity checking...
			assert( sizeof( real ) == sizeof( cpFloat ) );
		}
		
		~Main()
		{}
	
		void prepareSettings( Settings *settings )
		{
			settings->setWindowSize( 800, 600 );
			SurfacerApp::prepareSettings( settings );
		}
		
		void setup()
		{
			//setGameScenario( new CuttingTestScenario() );
			//setGameScenario( new MonsterPlaygroundScenario() );
			//setGameScenario( new UIPlaygroundScenario() );
			setGameScenario( new SensorTestScenario() );
			//setGameScenario( new LevelLoadingScenario() );
			
			SurfacerApp::setup();
		}
		
};

#pragma mark -
#pragma Bootstrap

namespace
{
	/*
		Antialiasing is enabled for an important reason - some bug
		in Cinder, or Apple's GL is preventing glClear from working
		correctly when in aliased rendering, preventing proper compositing
		of UI components. I can't say why, only that it took a long time 
		to track this down.
	*/
	const bool Antialiased = true;
}

CINDER_APP_BASIC( Main, RendererGl(Antialiased) )
