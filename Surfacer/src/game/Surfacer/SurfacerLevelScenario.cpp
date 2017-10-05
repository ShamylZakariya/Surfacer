//
//  SurfacerLevelScenario.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 10/5/17.
//

#include "SurfacerLevelScenario.hpp"

#include "App.hpp"
#include "SurfacerLevel.hpp"
#include "Strings.hpp"
#include "Terrain.hpp"

#include "DevComponents.hpp"


using namespace core;
namespace surfacer {
	
	SurfacerLevelScenario::SurfacerLevelScenario(string levelXmlFile):
	_levelXmlFile(levelXmlFile)
	{}
	
	SurfacerLevelScenario::~SurfacerLevelScenario() {
		
	}
	
	void SurfacerLevelScenario::setup() {
		surfacer::SurfacerLevelRef level = make_shared<surfacer::SurfacerLevel>();
		setLevel(level);
		
		level->load(app::loadAsset(_levelXmlFile));
		auto terrain = level->getTerrain();
		auto player = level->getPlayer();
		
		if (!player) {
			
			// build camera controller, dragger, and cutter, with input dispatch indices 0,1,2 meaning CC gets input first
			
			auto cc = Object::with("CameraController", {make_shared<ManualViewportControlComponent>(getViewportController(), 0)});
			getLevel()->addObject(cc);
			
			auto dragger = Object::with("Dragger", {
				make_shared<MousePickComponent>(surfacer::ShapeFilters::GRABBABLE, 1),
				make_shared<MousePickDrawComponent>()
			});
			getLevel()->addObject(dragger);
			
			if (terrain) {
				auto cutter = Object::with("Cutter", {
					make_shared<terrain::MouseCutterComponent>(terrain, 4, 2),
					make_shared<terrain::MouseCutterDrawComponent>()
				});
				getLevel()->addObject(cutter);
			}
			
		}
		
		auto grid = Object::with("Grid", { WorldCartesianGridDrawComponent::create() });
		getLevel()->addObject(grid);
		
	}
	
	void SurfacerLevelScenario::cleanup() {
		setLevel(nullptr);
	}
	
	void SurfacerLevelScenario::resize( ivec2 size ) {
	}
	
	void SurfacerLevelScenario::step( const time_state &time ) {
	}
	
	void SurfacerLevelScenario::update( const time_state &time ) {
	}
	
	void SurfacerLevelScenario::clear( const render_state &state ) {
		gl::clear( Color( 0.2, 0.2, 0.2 ) );
	}
	
	void SurfacerLevelScenario::drawScreen( const render_state &state ) {
		
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
	
	bool SurfacerLevelScenario::keyDown( const ci::app::KeyEvent &event ) {
		if (event.getChar() == 'r') {
			reset();
			return true;
		} else if (event.getCode() == ci::app::KeyEvent::KEY_BACKQUOTE) {
			setRenderMode( RenderMode::mode( (int(getRenderMode()) + 1) % RenderMode::COUNT ));
		}
		return false;
	}
	
	
	void SurfacerLevelScenario::reset() {
		cleanup();
		setup();
	}
	
}