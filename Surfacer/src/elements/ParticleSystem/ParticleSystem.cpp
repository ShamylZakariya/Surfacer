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
	
	ParticleSimulation::ParticleSimulation()
	{}

#pragma mark - ParticleSystemDrawComponent
	
	ParticleSystemDrawComponent::config ParticleSystemDrawComponent::config::parse(const core::util::xml::XmlMultiTree &node) {
		config c;
		// PLACEHOLDER
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
	
#pragma mark - ParticleSystem
	
	ParticleSystem::ParticleSystem(string name):
	Object(name)
	{}
	
	void ParticleSystem::onReady(LevelRef level) {
		CI_ASSERT_MSG(_simulation, "Expect a ParticleSimulation to be among installed components at time of onReady()");
		Object::onReady(level);
		for (auto dc : getDrawComponents()) {
			if (ParticleSystemDrawComponentRef psdc = dynamic_pointer_cast<ParticleSystemDrawComponent>(dc)) {
				psdc->setParticleSimulation(_simulation);
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
