//
//  main.cpp
//  Milestone3
//
//  Created by Shamyl Zakariya on 2/2/17.
//
//

#include "cinder/app/RendererGl.h"
#include "App.hpp"

#include "TerrainTestScenario.hpp"
#include "SvgTestScenario.hpp"
#include "GameLevelTestScenario.hpp"

class DemoApp : public core::App {
public:

	static void prepareSettings(Settings *settings) {
		App::prepareSettings(settings);
		settings->setTitle( "DemoApp" );
	}

public:

	DemoApp(){}

	virtual void setup() override {
		App::setup();
		setScenario(make_shared<GameLevelTestScenario>());
	}
 
};

CINDER_APP( DemoApp, app::RendererGl, DemoApp::prepareSettings )
