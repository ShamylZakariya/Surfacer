//
//  TestsApp.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 10/9/17.
//

#include "cinder/app/RendererGl.h"
#include "App.hpp"

#include "UniversalParticleSystem.hpp"
#include "TerrainTestScenario.hpp"
#include "SvgTestScenario.hpp"

namespace {
	
	// simple tests of the particles::interpolator mechanism
	void testInterpolators() {
		using particles::interpolator;
		
		auto dInterp = interpolator<double>({ 0, 10, 0, -10, 0 });
		for (double i = 0; i <= 1 + 1e-3; i += 0.05) {
			cout << "progress: " << i << " value: " << dInterp.value(i) << endl;
		}

		auto dvec2Interp = interpolator<dvec2>({ dvec2(0,0), dvec2(0,10), dvec2(0,0), dvec2(-10,0), dvec2(0,0) });
		for (double i = 0; i <= 1 + 1e-3; i += 0.05) {
			cout << "progress: " << i << " value: " << dvec2Interp.value(i) << endl;
		}

	}
	
}

class TestsApp : public core::App {
public:
	
	static void prepareSettings(Settings *settings) {
		App::prepareSettings(settings);
		settings->setTitle( "Tests" );
	}
	
public:
	
	TestsApp(){
		testInterpolators();
	}
	
	virtual void setup() override {
		App::setup();
		//setScenario(make_shared<SvgTestScenario>());
	}
	
};

CINDER_APP( TestsApp, ci::app::RendererGl, TestsApp::prepareSettings )
