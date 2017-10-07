//
//  Background.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 10/7/17.
//

#ifndef Background_hpp
#define Background_hpp

#include "Core.hpp"
#include "Xml.hpp"

#include "PrecariouslyConstants.hpp"

namespace precariously {
	
	SMART_PTR(Background);
	
	class BackgroundFillDrawComponent : public core::DrawComponent {
	public:
		
		struct config {
			Color spaceColor;
			Color atmosphereColor;
			double innerRadius;
			double outerRadius;
			int steps;
			
			config():
			spaceColor(100,100,120),
			atmosphereColor(255,0,255),
			innerRadius(500),
			outerRadius(1000),
			steps(8)
			{}
			
			static config parse(core::util::xml::XmlMultiTree node);
		};
		
	public:
		
		BackgroundFillDrawComponent(config c);
		~BackgroundFillDrawComponent();
		
		// DrawComponent

		int getLayer() const override {
			return DrawLayers::BACKGROUND - 1000;
		}
		
		cpBB getBB() const override {
			return cpBBInfinity;
		}
		
		void draw(const core::render_state &state) override;
		
		core::VisibilityDetermination::style getVisibilityDetermination() const override {
			return core::VisibilityDetermination::ALWAYS_DRAW;
		}

	private:
		
		config _config;
		gl::GlslProgRef _shader;
		gl::BatchRef _batch;
		
	};
	
	class Background : public core::Object {
	public:
		
		struct config {
			BackgroundFillDrawComponent::config backgroundFill;
			
			config()
			{}
			
			static config parse(core::util::xml::XmlMultiTree node);
		};
		
		static BackgroundRef create(core::util::xml::XmlMultiTree backgroundNode);
		
	public:
	
		Background();
		~Background();
		
	};
	
}

#endif /* Background_hpp */
