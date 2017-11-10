//
//  SurfacerScenario.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 10/5/17.
//

#include "PrecariouslyScenario.hpp"

#include "App.hpp"
#include "PrecariouslyStage.hpp"
#include "Strings.hpp"
#include "Terrain.hpp"

#include "DevComponents.hpp"


using namespace core;
namespace precariously {
	
	PrecariouslyScenario::PrecariouslyScenario(string stageXmlFile):
	_stageXmlFile(stageXmlFile)
	{}
	
	PrecariouslyScenario::~PrecariouslyScenario() {	
	}
	
	void PrecariouslyScenario::setup() {
		PrecariouslyStageRef stage = make_shared<PrecariouslyStage>();
		setStage(stage);
		
		stage->load(app::loadAsset(_stageXmlFile));
		auto planet = stage->getPlanet();
		
		if (true) {
			
			// build camera controller, dragger, and cutter, with input dispatch indices 0,1,2 meaning CC gets input first
			
			auto cc = Object::with("CameraController", {make_shared<ManualViewportControlComponent>(getViewportController(), 0)});
			getStage()->addObject(cc);
			
			auto dragger = Object::with("Dragger", {
				make_shared<MousePickComponent>(ShapeFilters::GRABBABLE, 1),
				make_shared<MousePickDrawComponent>()
			});
			getStage()->addObject(dragger);
			
			if (planet) {
				auto cutter = Object::with("Cutter", {
					make_shared<terrain::MouseCutterComponent>(planet, 4, 2),
					make_shared<terrain::MouseCutterDrawComponent>()
				});
				getStage()->addObject(cutter);
			}
			
		}
	}
	
	void PrecariouslyScenario::cleanup() {
		setStage(nullptr);
	}
	
	void PrecariouslyScenario::resize( ivec2 size ) {
	}
	
	void PrecariouslyScenario::step( const time_state &time ) {
	}
	
	void PrecariouslyScenario::update( const time_state &time ) {
	}
		
	void PrecariouslyScenario::drawScreen( const render_state &state ) {
		
		//
		// NOTE: we're in screen space, with coordinate system origin at top left
		//
		
		float fps = core::App::get()->getAverageFps();
		float sps = core::App::get()->getAverageSps();
		string info = core::strings::format("%.1f %.1f", fps, sps);
		gl::drawString(info, vec2(10,10), Color(1,1,1));
		
		stringstream ss;
		Viewport::look look = getViewport()->getLook();
		double scale = getViewport()->getScale();
		
		ss << std::setprecision(3) << "world (" << look.world.x << ", " << look.world.y  << ") scale: " << scale << " up: (" << look.up.x << ", " << look.up.y << ")";
		gl::drawString(ss.str(), vec2(10,40), Color(1,1,1));
	}
	
	bool PrecariouslyScenario::keyDown( const ci::app::KeyEvent &event ) {
		if (event.getChar() == 'r') {
			reset();
			return true;
		} else if (event.getCode() == ci::app::KeyEvent::KEY_BACKQUOTE) {
			setRenderMode( RenderMode::mode( (int(getRenderMode()) + 1) % RenderMode::COUNT ));
		}
		return false;
	}
	
	void PrecariouslyScenario::reset() {
		cleanup();
		setup();
	}
	
}
