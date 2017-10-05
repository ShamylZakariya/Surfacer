//
//  main.cpp
//  Milestone3
//
//  Created by Shamyl Zakariya on 2/2/17.
//
//

#include "cinder/app/RendererGl.h"
#include "App.hpp"

#include "SurfacerLevelScenario.hpp"

class SurfacerApp : public core::App {
public:

	static void prepareSettings(Settings *settings) {
		App::prepareSettings(settings);
		settings->setTitle( "Surfacer" );
	}

public:

	SurfacerApp(){}

	virtual void setup() override {
		App::setup();
		setScenario(make_shared<surfacer::SurfacerLevelScenario>("levels/test0.xml"));
	}
 
};

CINDER_APP( SurfacerApp, app::RendererGl, SurfacerApp::prepareSettings )
