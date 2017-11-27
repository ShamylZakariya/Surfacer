//
//  TestsApp.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 10/9/17.
//

#include "cinder/app/RendererGl.h"
#include "App.hpp"

#include "ParticleSystem.hpp"
#include "TerrainTestScenario.hpp"
#include "SvgTestScenario.hpp"
#include "ParticleSystemTestScenario.hpp"
#include "PerlinWorldTest.hpp"

class TestsApp : public core::App {
public:
	
	static void prepareSettings(Settings *settings) {
		App::prepareSettings(settings);
		settings->setTitle( "Tests" );
	}
	
public:
	
	TestsApp(){
	}
	
	virtual void setup() override {
		App::setup();
		setScenario(make_shared<PerlinWorldTestScenario>());
	}
	
};

CINDER_APP( TestsApp, ci::app::RendererGl, TestsApp::prepareSettings )
