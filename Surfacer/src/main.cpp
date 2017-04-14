//
//  main.cpp
//  Milestone3
//
//  Created by Shamyl Zakariya on 2/2/17.
//
//

#include "cinder/app/RendererGl.h"
#include "GameApp.hpp"

#include "TerrainTestScenario.hpp"
#include "LevelTestScenario.hpp"
#include "SvgTestScenario.hpp"
#include "GameLevelTestScenario.hpp"

class DemoApp : public core::GameApp {
public:

	static void prepareSettings(Settings *settings) {
		GameApp::prepareSettings(settings);
		settings->setTitle( "DemoApp" );
	}

public:

	DemoApp(){}

	virtual void setup() override {
		GameApp::setup();
		setScenario(make_shared<GameLevelTestScenario>());
	}

};

CINDER_APP( DemoApp, app::RendererGl, DemoApp::prepareSettings )
