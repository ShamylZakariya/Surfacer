//
//  Background.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 10/7/17.
//

#include "Background.hpp"

using namespace core;
namespace precariously {

	BackgroundFillDrawComponent::config BackgroundFillDrawComponent::config::parse(core::util::xml::XmlMultiTree node) {
		config c;
		
		c.spaceColor = util::xml::readColorAttribute(node, "spaceColor", c.spaceColor);
		c.atmosphereColor = util::xml::readColorAttribute(node, "atmosphereColor", c.atmosphereColor);
		c.innerRadius = util::xml::readNumericAttribute<double>(node, "innerRadius", c.innerRadius);
		c.outerRadius = util::xml::readNumericAttribute<double>(node, "outerRadius", c.outerRadius);
		c.steps = util::xml::readNumericAttribute<int>(node, "steps", c.steps);
		
		return c;
	}
	
	/*
	 config _config;
	 gl::GlslProgRef _shader;
	 gl::BatchRef _batch;
	 */
	BackgroundFillDrawComponent::BackgroundFillDrawComponent(config c):
	_config(c)
	{
		auto vsh = CI_GLSL(150,
						   uniform mat4 ciModelViewProjection;
						   uniform mat4 ciModelMatrix;
						   
						   in vec4 ciPosition;
						   
						   out vec2 worldPosition;
						   
						   void main(void){
							   gl_Position = ciModelViewProjection * ciPosition;
							   worldPosition = (ciModelMatrix * ciPosition).xy;
						   }
						   );
		
		auto fsh = CI_GLSL(150,
						   uniform float InnerRadius;
						   uniform float OuterRadius;
						   uniform vec4 SpaceColor;
						   uniform vec4 AtmosphereColor;
						   uniform int Steps;
						   
						   in vec2 worldPosition;
						   
						   out vec4 oColor;
						   
						   void main(void) {
							   float d = (length(worldPosition) - InnerRadius) / (OuterRadius - InnerRadius);
							   d = clamp(d, 0, 1); // d is in range 0 -> 1
							   float aa = round(d * Steps) / float(Steps);
							   oColor = mix(AtmosphereColor, SpaceColor, aa);
						   }
						   );
		
		_shader = gl::GlslProg::create(gl::GlslProg::Format().vertex(vsh).fragment(fsh));
		_shader->uniform("InnerRadius", static_cast<float>(_config.innerRadius));
		_shader->uniform("OuterRadius", static_cast<float>(_config.outerRadius));
		_shader->uniform("AtmosphereColor", ColorA(_config.atmosphereColor, 1));
		_shader->uniform("SpaceColor", ColorA(_config.spaceColor, 1));
		_shader->uniform("Steps", _config.steps);

		_batch = gl::Batch::create(geom::Rect().rect(Rectf(-1,-1,1,1)), _shader);
	}
	
	BackgroundFillDrawComponent::~BackgroundFillDrawComponent()
	{}
	
	void BackgroundFillDrawComponent::draw(const core::render_state &state) {		
		// set up a scale to fill viewport with plane
		cpBB frustum = state.viewport->getFrustum();
		dvec2 centerWorld = v2(cpBBCenter(frustum));
		dvec2 scale = centerWorld - dvec2(frustum.l, frustum.t);
		
		gl::ScopedModelMatrix smm;
		dmat4 modelMatrix = glm::translate(dvec3(centerWorld,0)) * glm::scale(dvec3(scale,1));
		gl::multModelMatrix(modelMatrix);
		
		_batch->draw();
	}
	
	Background::config Background::config::parse(core::util::xml::XmlMultiTree node) {
		config c;
		
		c.backgroundFill = BackgroundFillDrawComponent::config::parse(node.getChild("fill"));
		
		return c;
	}
	
	BackgroundRef Background::create(core::util::xml::XmlMultiTree backgroundNode) {
		config c = config::parse(backgroundNode);
		BackgroundRef bg = make_shared<Background>();
		
		bg->addComponent(make_shared<BackgroundFillDrawComponent>(c.backgroundFill));
		
		return bg;
	}
	
	Background::Background():
	Object("Background")
	{}
	
	Background::~Background()
	{}

	
}
