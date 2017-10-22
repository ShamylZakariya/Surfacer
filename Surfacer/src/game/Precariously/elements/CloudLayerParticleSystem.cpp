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
				
				pIt->angle = M_PI_2 + atan2(worldOnCircle.y, worldOnCircle.x);
			} else {
				pIt->angle = M_PI_2 + a;
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
	 gl::GlslProgRef _shader;
	 vector<particle_vertex> _particles;
	 gl::VboRef _particlesVbo;
	 gl::BatchRef _particlesBatch;
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
						   in vec4 ciColor;

						   out vec2 TexCoord;
						   out vec4 Color;

						   void main(void){
							   gl_Position = ciModelViewProjection * ciPosition;
							   TexCoord = ciTexCoord0;
							   Color = ciColor;
						   }
						   );

		auto fsh = CI_GLSL(150,
						   uniform sampler2D uTex0;

						   in vec2 TexCoord;
						   in vec4 Color;

						   out vec4 oColor;

						   void main(void) {
							   // controlled-additive-blending requires premultiplied alpha
							   // but our texture appears to already be premultiplied after load
							   vec4 texColor = texture( uTex0, TexCoord );
							   oColor = texColor * Color;
						   }
						   );

		_shader = gl::GlslProg::create(gl::GlslProg::Format().vertex(vsh).fragment(fsh));
	}
	
	void CloudLayerParticleSystemDrawComponent::setSimulation(const ParticleSimulationRef simulation) {
		ParticleSystemDrawComponent::setSimulation(simulation);
		auto sim = getSimulation<CloudLayerParticleSimulation>();
		
		// now build our GPU backing. Note, GL_QUADS is deprecated so we need GL_TRIANGLES, which requires 6 vertices to make a quad
		size_t count = sim->getActiveCount();
		_particles.resize(count * 6, particle_vertex());
		
		updateParticles();
		
		// create VBO GPU-side which we can stream to
		_particlesVbo = gl::Vbo::create( GL_ARRAY_BUFFER, _particles, GL_STREAM_DRAW );

		geom::BufferLayout particleLayout;
		particleLayout.append( geom::Attrib::POSITION, 2, sizeof( particle_vertex ), offsetof( particle_vertex, position ) );
		particleLayout.append( geom::Attrib::TEX_COORD_0, 2, sizeof( particle_vertex ), offsetof( particle_vertex, texCoord ) );
		particleLayout.append( geom::Attrib::COLOR, 4, sizeof( particle_vertex ), offsetof( particle_vertex, color ) );

		// pair our layout with vbo.
		auto mesh = gl::VboMesh::create( static_cast<uint32_t>(_particles.size()), GL_TRIANGLES, { { particleLayout, _particlesVbo } } );
		_particlesBatch = gl::Batch::create( mesh, _shader );
	}

	
	void CloudLayerParticleSystemDrawComponent::draw(const render_state &renderState) {
		updateParticles();
		
		gl::ScopedTextureBind tex(_config.textureAtlas, 0);
		gl::ScopedBlendPremult blender;

		_particlesBatch->draw();
	}

	int CloudLayerParticleSystemDrawComponent::getLayer() const {
		return DrawLayers::EFFECTS;
	}
	
	void CloudLayerParticleSystemDrawComponent::updateParticles() {

		// walk the simulation particle state and write to our vertices
		ParticleSimulationRef sim = getSimulation();
		const bool Rotates = sim->rotatesParticles();
		
		const ParticleAtlasType::Type AtlasType = _config.atlasType;
		const vec2* AtlasOffsets = ParticleAtlasType::AtlasOffsets(AtlasType);
		const float AtlasScaling = ParticleAtlasType::AtlasScaling(AtlasType);

		vec2 shape[4];
		mat2 rotator;
		auto vertex = _particles.begin();
		
		for (auto particle = sim->getStorage().begin(), end = sim->getStorage().end(); particle != end; ++particle) {
			const float Radius = particle->radius,
				XScale = Radius * particle->xScale,
				YScale = Radius * particle->yScale;
			
			// TODO: Fast path for when particle Radius == 0 - just pack everything into particle->position
			
			shape[0].x = -XScale;
			shape[0].y = +YScale;
			
			shape[1].x = +XScale;
			shape[1].y = +YScale;
			
			shape[2].x = +XScale;
			shape[2].y = -YScale;
			
			shape[3].x = -XScale;
			shape[3].y = -YScale;
			
			if (Rotates) {
				mat2WithRotation(rotator, static_cast<float>(particle->angle));
				shape[0] = rotator * shape[0];
				shape[1] = rotator * shape[1];
				shape[2] = rotator * shape[2];
				shape[3] = rotator * shape[3];
			}
			
			//
			//	For each vertex, assign position, color and texture coordinate
			//	Note, GL_QUADS is deprecated so we have to draw two TRIANGLES
			//
			
			vec2 worldPosition = vec2(particle->position + particle->displacement);
			ci::ColorA pc = particle->color;
			ci::ColorA additiveColor( pc.r * pc.a, pc.g * pc.a, pc.b * pc.a, pc.a * ( 1 - static_cast<float>(particle->additivity)));
			vec2 atlasOffset = AtlasOffsets[particle->atlasIdx];
			
			// GL_TRIANGLE
			vertex->position = worldPosition + shape[0];
			vertex->texCoord = (TexCoords[0] * AtlasScaling) + atlasOffset;
			vertex->color = additiveColor;
			++vertex;

			vertex->position = worldPosition + shape[1];
			vertex->texCoord = (TexCoords[1] * AtlasScaling) + atlasOffset;
			vertex->color = additiveColor;
			++vertex;

			vertex->position = worldPosition + shape[2];
			vertex->texCoord = (TexCoords[2] * AtlasScaling) + atlasOffset;
			vertex->color = additiveColor;
			++vertex;
			
			// GL_TRIANGLE
			vertex->position = worldPosition + shape[0];
			vertex->texCoord = (TexCoords[0] * AtlasScaling) + atlasOffset;
			vertex->color = additiveColor;
			++vertex;
			
			vertex->position = worldPosition + shape[2];
			vertex->texCoord = (TexCoords[2] * AtlasScaling) + atlasOffset;
			vertex->color = additiveColor;
			++vertex;
			
			vertex->position = worldPosition + shape[3];
			vertex->texCoord = (TexCoords[3] * AtlasScaling) + atlasOffset;
			vertex->color = additiveColor;
			++vertex;
		}
		
		if (_particlesVbo) {
			// transfer to GPU
			void *gpuMem = _particlesVbo->mapReplace();
			memcpy( gpuMem, _particles.data(), _particles.size() * sizeof(particle_vertex) );
			_particlesVbo->unmap();
		}
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
