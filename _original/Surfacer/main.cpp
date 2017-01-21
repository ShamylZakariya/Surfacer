#include <cinder/app/App.h>
#include <cinder/app/RendererGL.h>
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

CINDER_APP( Main, RendererGl( RendererGl::Options().msaa( 4 ) ), Main::prepareSettings );
