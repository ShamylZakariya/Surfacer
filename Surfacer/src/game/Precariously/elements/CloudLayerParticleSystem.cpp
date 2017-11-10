//
//  CloudLayerParticleSystem.cpp
//  Precariously
//
//  Created by Shamyl Zakariya on 10/18/17.
//

#include "CloudLayerParticleSystem.hpp"

#include <cinder/Rand.h>

using namespace core;
using namespace particles;

namespace precariously {
	
	CloudLayerParticleSimulation::particle_template CloudLayerParticleSimulation::particle_template::parse(const util::xml::XmlMultiTree &node) {
		particle_template pt;
		pt.minRadius = util::xml::readNumericAttribute<double>(node, "minRadius", pt.minRadius);
		pt.maxRadius = util::xml::readNumericAttribute<double>(node, "maxRadius", pt.maxRadius);
		pt.minRadiusNoiseValue = util::xml::readNumericAttribute<double>(node, "minRadiusNoiseValue", pt.minRadiusNoiseValue);
		pt.color = util::xml::readColorAttribute(node, "color", pt.color);

		return pt;
	}

	
	CloudLayerParticleSimulation::config CloudLayerParticleSimulation::config::parse(const util::xml::XmlMultiTree &node) {
		config c;
		
		c.generator = util::PerlinNoise::config::parse(node.getChild("generator"));
		c.particle = particle_template::parse(node.getChild("particle"));
		c.origin = util::xml::readPointAttribute(node, "origin", c.origin);
		c.radius = util::xml::readNumericAttribute<double>(node, "radius", c.radius);
		c.count = util::xml::readNumericAttribute<size_t>(node, "count", c.count);
		c.period = util::xml::readNumericAttribute<seconds_t>(node, "period", c.period);
		c.displacementForce = util::xml::readNumericAttribute<double>(node, "displacementForce", c.displacementForce);
		c.returnForce = util::xml::readNumericAttribute<double>(node, "returnForce", c.returnForce);

		return c;
	}

	/*
	 config _config;
	 util::PerlinNoise _generator;
	 seconds_t _time;
	 cpBB _bb;
	 */
	
	CloudLayerParticleSimulation::CloudLayerParticleSimulation(const config &c):
	_config(c),
	_generator(util::PerlinNoise(c.generator)),
	_time(0)
	{}

	void CloudLayerParticleSimulation::onReady(ObjectRef parent, StageRef stage) {
		_generator.setScale(2*M_PI);
		
		// create store, and run simulate() once to populate
		setParticleCount(_config.count);

		// set some invariants and default state
		double dr = 2 * M_PI / getActiveCount();
		double a = 0;
		auto state = _state.begin();
		auto physics = _physics.begin();
		for (size_t i = 0, N = getParticleCount(); i < N; i++, ++state, ++physics, a += dr) {
			
			// set up initial physics state
			physics->position = physics->previous_position = physics->home = _config.origin + _config.radius * dvec2(cos(a), sin(a));
			physics->damping = Rand::randFloat(0.4, 0.7);
			physics->velocity = dvec2(0,0);
			physics->radius = 0;

			state->atlasIdx = 0;
			state->color = _config.particle.color;
			state->additivity = 0;
			state->position = physics->home;
			state->age = 0;
			state->completion = 0;
			state->active = true; // always active
		}
		
		simulate(stage->getTimeState());
	}
	
	void CloudLayerParticleSimulation::update(const time_state &timeState) {
		_time += timeState.deltaT;
		simulate(timeState);
	}
	
	void CloudLayerParticleSimulation::setParticleCount(size_t count) {
		ParticleSimulation::setParticleCount(count);
		_physics.resize(count);
	}
	
	void CloudLayerParticleSimulation::addGravityDisplacement(const RadialGravitationCalculatorRef &gravity) {
		_displacements.push_back(gravity);
	}
	
	void CloudLayerParticleSimulation::simulate(const time_state &timeState) {
		
		if (!_displacements.empty()) {
			pruneDisplacements();
			applyGravityDisplacements(timeState);
		}
		
		const double dr = 2 * M_PI / getActiveCount();
		double a = 0;
		const double cloudLayerRadius = _config.radius;
		const double cloudLayerRadius2 = cloudLayerRadius * cloudLayerRadius;
		const double particleRadiusDelta = _config.particle.maxRadius - _config.particle.minRadius;
		const double particleMinRadius = _config.particle.minRadius;
		const double noiseYAxis = _time / _config.period;
		const double noiseMin = _config.particle.minRadiusNoiseValue;
		const double rNoiseRange = 1.0 / (1.0 - noiseMin);
		const double returnForce = _config.returnForce;
		const double deltaT2 = timeState.deltaT * timeState.deltaT;
		const dvec2 origin = _config.origin;
		cpBB bounds = cpBBInvalid;
		auto physics = _physics.begin();
		auto state = _state.begin();
		const auto end = _state.end();
		
		for (;state != end; ++state, ++physics, a += dr) {

			//
			// simplistic verlet integration
			//

			physics->velocity = (physics->position - physics->previous_position) * physics->damping;
			physics->previous_position = physics->position;
			
			dvec2 acceleration = (physics->home - physics->position) * returnForce;
			physics->position += physics->velocity + (acceleration * deltaT2);
			
			//
			// move position back to circle
			//

			dvec2 dirToPosition = physics->position - origin;
			double d2 = lengthSquared(dirToPosition);
			if (d2 < cloudLayerRadius2 - 1e-1 || d2 > cloudLayerRadius2 + 1e-1) {
				dirToPosition /= sqrt(d2);
				physics->position = origin + cloudLayerRadius * dirToPosition;
			}
			
			//
			//	now update position scale and rotation
			//

			state->position = physics->position;

			double angle;
			if (lengthSquared(physics->position - physics->home) > 1e-2) {
				angle = M_PI_2 + atan2(physics->position.y, physics->position.x);
			} else {
				angle = M_PI_2 + a;
			}
			
			double noise = _generator.noiseUnit(a,noiseYAxis);
			double radius = 0;
			if (noise > noiseMin) {
				double remappedNoiseVale = (noise - noiseMin) * rNoiseRange;
				radius = particleMinRadius + remappedNoiseVale * particleRadiusDelta;
			}
			physics->radius = lrp(timeState.deltaT, physics->radius, radius);
			
			//
			//	Compute the rotation axes
			//

			double cosa, sina;
			__sincos(angle, &sina, &cosa);
			state->right = physics->radius * dvec2(cosa, sina);
			state->up = rotateCCW(state->right);

			//
			// we're using alpha as a flag to say this particle should or should not be drawn
			//
			
			state->color.a = physics->radius > 1e-2 ? 1 : 0;
			
			//
			//	Update bounds
			//

			bounds = cpBBExpand(bounds, state->position, physics->radius);
		}

		_bb = bounds;
	}
	
	void CloudLayerParticleSimulation::pruneDisplacements() {
		_displacements.erase(std::remove_if(_displacements.begin(), _displacements.end(),[](const GravitationCalculatorRef &g){
			return g->isFinished();
		}), _displacements.end());
	}
	
	void CloudLayerParticleSimulation::applyGravityDisplacements(const time_state &timeState) {
		for (const auto &g : _displacements) {
			dvec2 centerOfMass = g->getCenterOfMass();
			double magnitude = -1 * g->getMagnitude() * timeState.deltaT * _config.displacementForce;
			for (auto physics = _physics.begin(), end = _physics.end(); physics != end; ++physics) {
				dvec2 dir = physics->position - centerOfMass;
				double d2 = length2(dir);
				physics->position += magnitude * dir / d2;
			}
		}
	}

#pragma mark - CloudLayerParticleSystem
	
	CloudLayerParticleSystem::config CloudLayerParticleSystem::config::parse(const util::xml::XmlMultiTree &node) {
		config c;
		c.drawConfig.drawLayer = DrawLayers::EFFECTS;
		c.drawConfig = UniversalParticleSystemDrawComponent::config::parse(node.getChild("draw"));
		c.simulationConfig = CloudLayerParticleSimulation::config::parse(node.getChild("simulation"));
		return c;
	}
	
	CloudLayerParticleSystemRef CloudLayerParticleSystem::create(const config &c) {
		auto simulation = make_shared<CloudLayerParticleSimulation>(c.simulationConfig);
		auto draw = make_shared<UniversalParticleSystemDrawComponent>(c.drawConfig);
		return Object::create<CloudLayerParticleSystem>("CloudLayer", { draw, simulation });
	}
	
	CloudLayerParticleSystem::CloudLayerParticleSystem(string name):
	ParticleSystem(name)
	{}
	
	
}
