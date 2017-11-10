//
//  main.cpp
//  Milestone3
//
//  Created by Shamyl Zakariya on 2/2/17.
//
//

#include "cinder/app/RendererGl.h"
#include "App.hpp"

#include "PrecariouslyScenario.hpp"

class PrecariouslyApp : public core::App {
public:

	static void prepareSettings(Settings *settings) {
		App::prepareSettings(settings);
		settings->setTitle( "Precariously" );
	}

public:

	PrecariouslyApp(){}

	virtual void setup() override {
		App::setup();
		setScenario(make_shared<precariously::PrecariouslyScenario>("precariously/stages/0.xml"));
	}
 
};

CINDER_APP( PrecariouslyApp, app::RendererGl, PrecariouslyApp::prepareSettings )
