//
//  ParticleSystem.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 10/18/17.
//

#include "ParticleSystem.hpp"
#include "Strings.hpp"

using namespace core;
namespace particles {
	
#pragma mark - Data
	
	
	const vec2 TexCoords[4] =
	{
		vec2( 0, 0 ),
		vec2( 1, 0 ),
		vec2( 1, 1 ),
		vec2( 0, 1 )
	};
	
	namespace {

		const float r2 = 0.5f;
		const float r4 = 0.25f;
		
		const vec2 AtlasOffset_None[1] = { vec2(0,0) };
		
		const vec2 AtlasOffset_TwoByTwo[4] =
		{
			vec2( 0*r2, 0*r2), vec2( 1*r2, 0*r2),
			vec2( 0*r2, 1*r2), vec2( 1*r2, 1*r2)
		};
		
		const vec2 AtlasOffset_FourByFour[16] =
		{
			vec2( 0*r4, 0*r4), vec2( 1*r4, 0*r4), vec2( 2*r4, 0*r4), vec2( 3*r4, 0*r4),
			vec2( 0*r4, 1*r4), vec2( 1*r4, 1*r4), vec2( 2*r4, 1*r4), vec2( 3*r4, 1*r4),
			vec2( 0*r4, 2*r4), vec2( 1*r4, 2*r4), vec2( 2*r4, 2*r4), vec2( 3*r4, 2*r4),
			vec2( 0*r4, 3*r4), vec2( 1*r4, 3*r4), vec2( 2*r4, 3*r4), vec2( 3*r4, 3*r4)
		};
		
		const vec2 *AtlasOffsetsByAtlasType[3] = {
			AtlasOffset_None,
			AtlasOffset_TwoByTwo,
			AtlasOffset_FourByFour
		};
		
		const float AtlasScalingByAtlasType[3] = {
			1,r2,r4
		};
	}
	
	
#pragma mark - ParticleAtlasType
	
	ParticleAtlasType::Type ParticleAtlasType::fromString(std::string typeStr) {
		typeStr = core::strings::lowercase(typeStr);
		if (typeStr == "twobytwo" || typeStr == "2x2") {
			return TwoByTwo;
		} else if (typeStr == "fourbyfour" || typeStr == "4x4") {
			return FourByFour;
		}
		return None;
	}
	
	std::string ParticleAtlasType::toString(particles::ParticleAtlasType::Type t) {
		switch (t) {
			case None:
				return "None";
			case TwoByTwo:
				return "TwoByTwo";
			case FourByFour:
				return "FourByFour";
		}
		return "None";
	}
	
	const vec2* ParticleAtlasType::AtlasOffsets(Type atlasType) {
		return AtlasOffsetsByAtlasType[atlasType];
	}

	float ParticleAtlasType::AtlasScaling(Type atlasType) {
		return AtlasScalingByAtlasType[atlasType];
	}

	
#pragma mark - ParticleSimulation

	ParticleSimulation::ParticleSimulation()
	{}

#pragma mark - ParticleSystemDrawComponent
	
	ParticleSystemDrawComponent::config ParticleSystemDrawComponent::config::parse(const core::util::xml::XmlMultiTree &node) {
		config c;
		c.drawLayer = util::xml::readNumericAttribute<int>(node, "drawLayer", c.drawLayer);
		return c;
	}
	
	ParticleSystemDrawComponent::ParticleSystemDrawComponent(config c):
	_config(c)
	{}
	
	cpBB ParticleSystemDrawComponent::getBB() const {
		if (ParticleSimulationRef s = _simulation.lock()) {
			return s->getBB();
		}
		return cpBBInvalid;
	}
	
	int ParticleSystemDrawComponent::getLayer() const {
		return _config.drawLayer;
	}
	
#pragma mark - ParticleSystem
	
	ParticleSystem::ParticleSystem(string name):
	Object(name)
	{}
	
	void ParticleSystem::onReady(StageRef stage) {
		CI_ASSERT_MSG(_simulation, "Expect a ParticleSimulation to be among installed components at time of onReady()");
		Object::onReady(stage);
		for (auto dc : getDrawComponents()) {
			if (ParticleSystemDrawComponentRef psdc = dynamic_pointer_cast<ParticleSystemDrawComponent>(dc)) {
				psdc->setSimulation(_simulation);
			}
		}
	}
	
	void ParticleSystem::addComponent(ComponentRef component) {
		Object::addComponent(component);
		if (ParticleSimulationRef sim = dynamic_pointer_cast<ParticleSimulation>(component)) {
			_simulation = sim;
		}
	}
	
}
