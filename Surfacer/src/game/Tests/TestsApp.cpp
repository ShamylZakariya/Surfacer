//
//  TestsApp.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 10/9/17.
//

#include "cinder/app/RendererGl.h"
#include "App.hpp"

#include "TerrainTestScenario.hpp"
#include "TerrainAttachmentsTestScenario.hpp"
#include "SvgTestScenario.hpp"
#include "PerlinWorldTestScenario.hpp"
#include "IPTestsScenario.hpp"

#define SCENARIO IPTestsScenario

class TestsApp : public core::App {
public:

    static void prepareSettings(Settings *settings) {
        App::prepareSettings(settings);
        settings->setTitle("Tests");
    }

public:

    TestsApp() {
    }

    virtual void setup() override {
        App::setup();
        setScenario(make_shared<SCENARIO>());
    }

};

CINDER_APP(TestsApp, ci::app::RendererGl, TestsApp::prepareSettings)
