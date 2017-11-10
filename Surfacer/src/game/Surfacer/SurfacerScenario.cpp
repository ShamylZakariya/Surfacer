//
//  SurfacerScenario.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 10/5/17.
//

#include "SurfacerScenario.hpp"

#include "App.hpp"
#include "SurfacerStage.hpp"
#include "Strings.hpp"
#include "Terrain.hpp"

#include "DevComponents.hpp"


using namespace core;
namespace surfacer {
	
	SurfacerScenario::SurfacerScenario(string stageXmlFile):
	_stageXmlFile(stageXmlFile)
	{}
	
	SurfacerScenario::~SurfacerScenario() {
		
	}
	
	void SurfacerScenario::setup() {
		surfacer::SurfacerStageRef stage = make_shared<surfacer::SurfacerStage>();
		setStage(stage);
		
		stage->load(app::loadAsset(_stageXmlFile));
		auto terrain = stage->getTerrain();
		auto player = stage->getPlayer();
		
		if (!player) {
			
			// build camera controller, dragger, and cutter, with input dispatch indices 0,1,2 meaning CC gets input first
			
			auto cc = Object::with("CameraController", {make_shared<ManualViewportControlComponent>(getViewportController(), 0)});
			getStage()->addObject(cc);
			
			auto dragger = Object::with("Dragger", {
				make_shared<MousePickComponent>(surfacer::ShapeFilters::GRABBABLE, 1),
				make_shared<MousePickDrawComponent>()
			});
			getStage()->addObject(dragger);
			
			if (terrain) {
				auto cutter = Object::with("Cutter", {
					make_shared<terrain::MouseCutterComponent>(terrain, 4, 2),
					make_shared<terrain::MouseCutterDrawComponent>()
				});
				getStage()->addObject(cutter);
			}
			
		}
		
		auto grid = Object::with("Grid", { WorldCartesianGridDrawComponent::create() });
		getStage()->addObject(grid);
		
	}
	
	void SurfacerScenario::cleanup() {
		setStage(nullptr);
	}
	
	void SurfacerScenario::resize( ivec2 size ) {
	}
	
	void SurfacerScenario::step( const time_state &time ) {
	}
	
	void SurfacerScenario::update( const time_state &time ) {
	}
	
	void SurfacerScenario::clear( const render_state &state ) {
		gl::clear( Color( 0.2, 0.2, 0.2 ) );
	}
	
	void SurfacerScenario::drawScreen( const render_state &state ) {
		
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
	
	bool SurfacerScenario::keyDown( const ci::app::KeyEvent &event ) {
		if (event.getChar() == 'r') {
			reset();
			return true;
		} else if (event.getCode() == ci::app::KeyEvent::KEY_BACKQUOTE) {
			setRenderMode( RenderMode::mode( (int(getRenderMode()) + 1) % RenderMode::COUNT ));
		}
		return false;
	}
	
	
	void SurfacerScenario::reset() {
		cleanup();
		setup();
	}
	
}
