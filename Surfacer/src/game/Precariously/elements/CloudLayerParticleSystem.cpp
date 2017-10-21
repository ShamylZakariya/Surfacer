//
//  CloudLayerParticleSystem.cpp
//  Precariously
//
//  Created by Shamyl Zakariya on 10/18/17.
//

#include "CloudLayerParticleSystem.hpp"

#include "PrecariouslyConstants.hpp"

using namespace core;
using namespace particles;

namespace precariously {
	
	CloudLayerParticleSimulation::particle_template CloudLayerParticleSimulation::particle_template::parse(const core::util::xml::XmlMultiTree &node) {
		particle_template pt;
		pt.minRadius = util::xml::readNumericAttribute<double>(node, "minRadius", pt.minRadius);
		pt.maxRadius = util::xml::readNumericAttribute<double>(node, "maxRadius", pt.maxRadius);
		pt.minRadiusNoiseValue = util::xml::readNumericAttribute<double>(node, "minRadiusNoiseValue", pt.minRadiusNoiseValue);
		return pt;
	}

	
	CloudLayerParticleSimulation::config CloudLayerParticleSimulation::config::parse(const core::util::xml::XmlMultiTree &node) {
		config c;
		
		c.generator = util::PerlinNoise::config::parse(node.getChild("generator"));
		c.particle = particle_template::parse(node.getChild("particle"));
		c.origin = util::xml::readPointAttribute(node, "origin", c.origin);
		c.radius = util::xml::readNumericAttribute<double>(node, "radius", c.radius);
		c.count = util::xml::readNumericAttribute<size_t>(node, "count", c.count);
		c.period = util::xml::readNumericAttribute<seconds_t>(node, "period", c.period);
		c.displacementForce = util::xml::readNumericAttribute<double>(node, "displacementForce", c.displacementForce);
		
		return c;
	}

	/*
	 config _config;
	 core::util::PerlinNoise _generator;
	 core::seconds_t _time;
	 cpBB _bb;
	 */
	
	CloudLayerParticleSimulation::CloudLayerParticleSimulation(const config &c):
	_config(c),
	_generator(util::PerlinNoise(c.generator)),
	_time(0)
	{}

	void CloudLayerParticleSimulation::onReady(core::ObjectRef parent, core::LevelRef level) {
		_generator.setScale(2*M_PI);
		
		// create store, and run simulate() once to populate
		initialize(_config.count);

		// set some invariants and default state
		double dr = 2 * M_PI / getActiveCount();
		double a = 0;
		auto pIt = _storage.begin();
		for (size_t i = 0, N = getStorageSize(); i < N; i++, ++pIt, a += dr) {
			pIt->idx = i;
			pIt->atlasIdx = 0;
			pIt->xScale = 1;
			pIt->yScale = 1;
			pIt->color = ci::ColorA(1,1,1,1);
			pIt->additivity = 0;
			pIt->position = _config.origin + _config.radius * dvec2(cos(a), sin(a));
			pIt->displacement = vec2(0,0);
		}
		
		simulate(level->getTimeState());
	}
	
	void CloudLayerParticleSimulation::update(const core::time_state &timeState) {
		_time += timeState.deltaT;
		simulate(timeState);
	}
	
	void CloudLayerParticleSimulation::addGravityDisplacement(const core::RadialGravitationCalculatorRef &gravity) {
		_displacements.push_back(gravity);
	}
	
	void CloudLayerParticleSimulation::simulate(const core::time_state &timeState) {
		pruneDisplacements();
		
		// distribute particles in an even circle, and apply radius based on value of generator at a given angle
		
		double dr = 2 * M_PI / getActiveCount();
		double a = 0;
		double particleRadiusDelta = _config.particle.maxRadius - _config.particle.minRadius;
		double particleMinRadius = _config.particle.minRadius;
		double radius = 0;
		double noiseYAxis = _time / _config.period;
		double noiseMin = _config.particle.minRadiusNoiseValue;
		double rNoiseRange = 1.0 / (1.0 - noiseMin);
		cpBB bounds = cpBBInvalid;
		
		for (auto pIt = _storage.begin(), pEnd = _storage.end(); pIt != pEnd; ++pIt, a += dr) {
			
			pIt->displacement *= 0.999;
			
			if (!_displacements.empty()) {
				for (const auto &g : _displacements) {
					auto force = g->calculate(pIt->position + pIt->displacement, g->getCenterOfMass(), g->getMagnitude(), 1);
					pIt->displacement += _config.displacementForce * force.magnitude * force.dir * timeState.deltaT;
				}
			}
			
			// Adjust displacement to keep the clouds on their circle
			if (lengthSquared(pIt->displacement) > 1e-2) {
				dvec2 world = pIt->position + pIt->displacement;
				dvec2 worldOnCircle = _config.origin + _config.radius * normalize(world - _config.origin);
				pIt->displacement = worldOnCircle - pIt->position;
				
				pIt->angle = atan2(worldOnCircle.y, worldOnCircle.x);
			} else {
				pIt->angle = a;
			}
			

			double noise = _generator.noiseUnit(a,noiseYAxis);
			if (noise > noiseMin) {
				double remappedNoiseVale = (noise - noiseMin) * rNoiseRange;
				radius = particleMinRadius + remappedNoiseVale * particleRadiusDelta;
			} else {
				radius = 0;
			}
			pIt->radius = lrp(0.25, pIt->radius, radius);
			bounds = cpBBExpand(bounds, pIt->position + pIt->displacement, pIt->radius);
		}

		_bb = bounds;
	}
	
	void CloudLayerParticleSimulation::pruneDisplacements() {
		_displacements.erase(std::remove_if(_displacements.begin(), _displacements.end(),[](const GravitationCalculatorRef &g){
			return g->isFinished();
		}), _displacements.end());
	}

	
#pragma mark - CloudLayerParticleSystemDrawComponent
	/*
	 config _config;
	 gl::Texture2dRef _textureAtlas;
	 gl::GlslProgRef _shader;
	 gl::BatchRef _batch;
	 */
	
	CloudLayerParticleSystemDrawComponent::config CloudLayerParticleSystemDrawComponent::config::parse(const core::util::xml::XmlMultiTree &node) {
		config c = ParticleSystemDrawComponent::config::parse(node);
		
		auto textureName = node.getAttribute("textureAtlas");
		if (textureName) {

			auto image = loadImage(app::loadAsset(*textureName));
			gl::Texture2d::Format fmt = gl::Texture2d::Format().mipmap(false);
			
			c.textureAtlas = gl::Texture2d::create(image, fmt);
			c.atlasType = ParticleAtlasType::fromString(node.getAttribute("atlasType", "None"));
		}

		return c;
	}

	CloudLayerParticleSystemDrawComponent::CloudLayerParticleSystemDrawComponent(config c):
	ParticleSystemDrawComponent(c),
	_config(c)
	{
		auto vsh = CI_GLSL(150,
						   uniform mat4 ciModelViewProjection;
						   
						   in vec4 ciPosition;
						   in vec2 ciTexCoord0;
						   
						   out vec2 TexCoord0;
						   
						   void main(void){
							   gl_Position = ciModelViewProjection * ciPosition;
							   TexCoord0 = ciTexCoord0;
						   }
						   );
		
		auto fsh = CI_GLSL(150,
						   uniform vec4 uColor;
						   uniform sampler2D uTex0;

						   in vec2 TexCoord0;
						   
						   out vec4 oColor;
						   
						   void main(void) {
							   // controlled-additive-blending requires premultiplied alpha
							   vec4 texColor = texture( uTex0, TexCoord0 );
//							   texColor.rgb *= texColor.a;

							   oColor = texColor * uColor;
						   }
						   );
		
		_shader = gl::GlslProg::create(gl::GlslProg::Format().vertex(vsh).fragment(fsh));
		_batch = gl::Batch::create(geom::Rect().rect(Rectf(-1,-1,1,1)).texCoords(vec2(0,0), vec2(0,1), vec2(1,1), vec2(1,0)), _shader);
	}
	
	void CloudLayerParticleSystemDrawComponent::draw(const render_state &renderState) {
		
		dmat4 MM;

		_config.textureAtlas->bind(0);
		_shader->uniform("uTex0", 0);
		
		for (auto &ps : getParticleSimulation()->getStorage()) {
			
			// controlled-additive blending
			ci::ColorA c = ps.color;
			c.r *= c.a;
			c.g *= c.a;
			c.b *= c.a;
			c.a *= 1 - ps.additivity;
			_shader->uniform("uColor", c);
			
			gl::pushModelMatrix();
			
			mat4WithPositionAndRotationAndScale(MM, ps.position + ps.displacement, ps.angle, ps.radius);
			gl::multModelMatrix(MM);
			_batch->draw();
			
			gl::popModelMatrix();
		}
	}

	int CloudLayerParticleSystemDrawComponent::getLayer() const {
		return DrawLayers::EFFECTS;
	}

#pragma mark - CloudLayerParticleSystem
	
	CloudLayerParticleSystem::config CloudLayerParticleSystem::config::parse(const core::util::xml::XmlMultiTree &node) {
		config c;
		c.drawConfig = CloudLayerParticleSystemDrawComponent::config::parse(node.getChild("draw"));
		c.simulationConfig = CloudLayerParticleSimulation::config::parse(node.getChild("simulation"));
		return c;
	}
	
	CloudLayerParticleSystemRef CloudLayerParticleSystem::create(const config &c) {
		auto simulation = make_shared<CloudLayerParticleSimulation>(c.simulationConfig);
		auto draw = make_shared<CloudLayerParticleSystemDrawComponent>(c.drawConfig);
		return Object::create<CloudLayerParticleSystem>("CloudLayer", { draw, simulation });
	}
	
	CloudLayerParticleSystem::CloudLayerParticleSystem(string name):
	ParticleSystem(name)
	{}
	
	
}
