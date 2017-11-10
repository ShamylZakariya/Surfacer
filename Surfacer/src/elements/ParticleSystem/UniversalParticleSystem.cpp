//
//  UniversalParticleSystem.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 10/23/17.
//

#include "UniversalParticleSystem.hpp"

#include <chipmunk/chipmunk_unsafe.h>

using namespace core;
namespace particles {
	
#pragma mark - UniversalParticleSimulation

	/*
	 size_t _count;
	 cpBB _bb;
	 vector<particle_template> _templates;
	 core::SpaceAccessRef _spaceAccess;
	 */
		
	UniversalParticleSimulation::UniversalParticleSimulation():
	ParticleSimulation(),
	_count(0),
	_bb(cpBBInvalid)
	{}
	
	// Component
	void UniversalParticleSimulation::onReady(core::ObjectRef parent, core::LevelRef level) {
		ParticleSimulation::onReady(parent, level);
		_spaceAccess = level->getSpace();
	}

	void UniversalParticleSimulation::onCleanup() {
		ParticleSimulation::onCleanup();
	}

	void UniversalParticleSimulation::update(const core::time_state &time) {
		ParticleSimulation::update(time);
		_prepareForSimulation(time);
		_simulate(time);
	}
	
	// ParticleSimulation
	
	void UniversalParticleSimulation::setParticleCount(size_t count) {
		ParticleSimulation::setParticleCount(count);
		_templates.resize(count);
	}
	
	size_t UniversalParticleSimulation::getFirstActive() const {
		// ALWAYS zero
		return 0;
	}

	size_t UniversalParticleSimulation::getActiveCount() const {
		return min(_count, _state.size());
	}

	cpBB UniversalParticleSimulation::getBB() const {
		return _bb;
	}
	
	// UniversalParticleSimulation
	
	void UniversalParticleSimulation::emit(const particle_template &particle) {
		_pending.push_back(particle);
	}
	
	void UniversalParticleSimulation::_prepareForSimulation(const core::time_state &time) {
		// run a first pass where we update age and completion, then if necessary perform a compaction pass
		size_t expiredCount = 0;
		const size_t activeCount = getActiveCount();
		
		auto state = _state.begin();
		auto end = state + activeCount;
		auto templ = _templates.begin();
		for (; state != end; ++state, ++templ) {
			
			//
			// update age and completion
			//
			
			templ->_age += time.deltaT;
			templ->_completion = templ->_age / templ->lifespan;
			
			if (templ->_completion > 1) {
				
				//
				// This particle is expired. Clean it up, and note how many expired we have
				//
				
				templ->destroy();
				expiredCount++;
			}
		}
		
		if (expiredCount > activeCount / 2) {
			
			//
			// parition templates such that expired ones are at the end; then reset _count accordingly
			// note: We don't need to sort _particleState because it's ephemeral; we'll overwrite what's
			// needed next pass to update()
			//
			
			auto end = partition(_templates.begin(), _templates.begin() + activeCount, [](const particle_template &templ){
				return templ._completion <= 1;
			});
			
			_count = end - _templates.begin();
			
			CI_LOG_D("COMPACTED, went from: " << activeCount << " to " << getActiveCount() << " active particles");
		}
		
		//
		//	Process any particles that were emitted
		//

		if (!_pending.empty()) {
		
			for (auto &particle : _pending) {

				//
				// if a particle already lives at this point, perform any cleanup needed
				//
				
				const size_t idx = _count % _templates.size();
				_templates[idx].destroy();
				
				//
				//	Assign template, and if it's kinematic, create chipmunk physics backing
				//
				
				_templates[idx] = particle;
				
				if (particle.kinematics) {
					double mass = particle.mass.getInitialValue();
					double radius = particle.radius.getInitialValue();
					double moment = cpMomentForCircle(mass, 0, radius, cpvzero);
					cpBody *body = cpBodyNew(mass, moment);
					cpShape *shape = cpCircleShapeNew(body, radius, cpvzero);
					
					// set up user data, etc to play well with our "engine"
					cpBodySetUserData(body, this);
					cpShapeSetUserData(shape, this);
					_spaceAccess->addBody(body);
					_spaceAccess->addShape(shape);
					
					// set initial state
					cpBodySetPosition(body, cpv(particle.position));
					cpBodySetVelocity(body, cpv(particle.velocity));
					cpShapeSetFilter(shape, particle.kinematics.filter);
					cpShapeSetFriction(shape, particle.kinematics.friction);
					
					_templates[idx]._body = body;
					_templates[idx]._shape = shape;
				}
				
				_templates[idx].prepare();
				
				_count++;
			}
			
			_pending.clear();
		}
		
		// we'll re-enable in _simulate
		for (auto &state : _state) {
			state.active = false;
		}
	}

	void UniversalParticleSimulation::_simulate(const core::time_state &time) {
		
		const auto &gravities = getLevel()->getGravities();
		cpBB bb = cpBBInvalid;
		
		auto state = _state.begin();
		auto end = state + getActiveCount();
		auto templ = _templates.begin();
		size_t idx = 0;
		for (; state != end; ++state, ++templ, ++idx) {
			
			const auto size = templ->radius(templ->_completion) * M_SQRT2;
			const auto damping = 1-saturate(templ->damping(templ->_completion));
			const auto additivity = templ->additivity(templ->_completion);
			const auto mass = templ->mass(templ->_completion);
			const auto color = templ->color(templ->_completion);
			
			bool didRotate = false;
			
			if (!templ->kinematics) {
				
				//
				//	Non-kinematic particles get standard velocity + damping + gravity applied
				//
				
				templ->position = templ->position + templ->velocity * time.deltaT;
				
				for (const auto &gravity : gravities) {
					auto force = gravity->calculate(templ->position);
					templ->velocity += mass * force.magnitude * force.dir * time.deltaT;
				}
				
				if (damping < 1) {
					templ->velocity *= damping;
				}
				
			} else {
				
				//
				// Kinematic bodies are simulated by chipmunk; extract position, rotation etc
				//
				
				cpShape *shape = templ->_shape;
				cpBody *body = templ->_body;
				
				templ->position = v2(cpBodyGetPosition(body));
				
				if (damping < 1) {
					cpBodySetVelocity(body, cpvmult(cpBodyGetVelocity(body), damping));
					cpBodySetAngularVelocity(body, damping * cpBodyGetAngularVelocity(body));
				}
				
				templ->velocity = v2(cpBodyGetVelocity(body));
				
				if (!templ->orientToVelocity) {
					dvec2 rotation = v2(cpBodyGetRotation(body));
					state->right = rotation * size;
					state->up = rotateCCW(state->right);
					didRotate = true;
				}
				
				// now update chipmunk's representation
				cpBodySetMass(body, max(mass, 0.0));
				cpCircleShapeSetRadius(shape, size);
			}
			
			if (templ->orientToVelocity) {
				dvec2 dir = templ->velocity;
				double len2 = lengthSquared(dir);
				if (templ->orientToVelocity && len2 > 1e-2) {
					state->right = (dir/sqrt(len2)) * size;
					state->up = rotateCCW(state->right);
					didRotate = true;
				}
			}
			
			if (!didRotate) {
				// default rotation
				state->right.x = size;
				state->right.y = 0;
				state->up.x = 0;
				state->up.y = size;
			}
			
			state->active = true;
			state->atlasIdx = templ->atlasIdx;
			state->age = templ->_age;
			state->completion = templ->_completion;
			state->position = templ->position;
			state->color = color;
			state->additivity = additivity;
			
			bb = cpBBExpand(bb, templ->position, size);
		}
		
		//
		// update BB and notify
		//
		
		_bb = bb;
		notifyMoved();
	}
	
#pragma mark - UniversalParticleSystemDrawComponent
	
	/*
	 config _config;
	 gl::GlslProgRef _shader;
	 vector<particle_vertex> _particles;
	 gl::VboRef _particlesVbo;
	 gl::BatchRef _particlesBatch;
	 GLsizei _batchDrawStart, _batchDrawCount;
	 */
	
	UniversalParticleSystemDrawComponent::config UniversalParticleSystemDrawComponent::config::parse(const util::xml::XmlMultiTree &node) {
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
	
	UniversalParticleSystemDrawComponent::UniversalParticleSystemDrawComponent(config c):
	ParticleSystemDrawComponent(c),
	_config(c),
	_batchDrawStart(0),
	_batchDrawCount(0)
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
							   float alpha = round(texture( uTex0, TexCoord ).r);
							   
							   // NOTE: additive blending requires premultiplication
							   oColor.rgb = Color.rgb * alpha;
							   oColor.a = Color.a * alpha;
						   }
						   );
		
		_shader = gl::GlslProg::create(gl::GlslProg::Format().vertex(vsh).fragment(fsh));
	}
	
	void UniversalParticleSystemDrawComponent::setSimulation(const ParticleSimulationRef simulation) {
		ParticleSystemDrawComponent::setSimulation(simulation);
		
		// now build our GPU backing. Note, GL_QUADS is deprecated so we need GL_TRIANGLES, which requires 6 vertices to make a quad
		size_t count = simulation->getParticleCount();
		_particles.resize(count * 6, particle_vertex());
		
		updateParticles(simulation);
		
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
	
	void UniversalParticleSystemDrawComponent::draw(const render_state &renderState) {
		auto sim = getSimulation();
		if (updateParticles(sim)) {
			gl::ScopedTextureBind tex(_config.textureAtlas, 0);
			gl::ScopedBlendPremult blender;
			
			_particlesBatch->draw(_batchDrawStart, _batchDrawCount);
			
			if (renderState.mode == RenderMode::DEVELOPMENT) {
				// draw BB
				cpBB bb = getBB();
				const ColorA bbColor(1,0.2,1,0.5);
				gl::color(bbColor);
				gl::drawStrokedRect(Rectf(bb.l, bb.b, bb.r, bb.t), 1);
			}
		}
	}
	
	bool UniversalParticleSystemDrawComponent::updateParticles(const ParticleSimulationRef &sim) {
		
		// walk the simulation particle state and write to our vertices		
		const ParticleAtlasType::Type AtlasType = _config.atlasType;
		const vec2* AtlasOffsets = ParticleAtlasType::AtlasOffsets(AtlasType);
		const float AtlasScaling = ParticleAtlasType::AtlasScaling(AtlasType);
		const ci::ColorA transparent(0,0,0,0);
		const vec2 origin(0,0);
		
		vec2 shape[4];
		mat2 rotator;
		const size_t activeCount = sim->getActiveCount();
		auto vertex = _particles.begin();
		auto stateBegin = sim->getParticleState().begin();
		auto stateEnd = stateBegin + activeCount;
		int verticesWritten = 0;
		
		for (auto state = stateBegin; state != stateEnd; ++state) {
			
			// Check if particle is active && visible before writing geometry
			if (state->active && state->color.a >= ALPHA_EPSILON) {
				
				shape[0] = state->position - state->right + state->up;
				shape[1] = state->position + state->right + state->up;
				shape[2] = state->position + state->right - state->up;
				shape[3] = state->position - state->right - state->up;
				
				//
				//	For each vertex, assign position, color and texture coordinate
				//	Note, GL_QUADS is deprecated so we have to draw two TRIANGLES
				//
				
				ci::ColorA pc = state->color;
				ci::ColorA additiveColor( pc.r * pc.a, pc.g * pc.a, pc.b * pc.a, pc.a * ( 1 - static_cast<float>(state->additivity)));
				vec2 atlasOffset = AtlasOffsets[state->atlasIdx];
				
				// GL_TRIANGLE
				vertex->position = shape[0];
				vertex->texCoord = (TexCoords[0] * AtlasScaling) + atlasOffset;
				vertex->color = additiveColor;
				++vertex;
				
				vertex->position = shape[1];
				vertex->texCoord = (TexCoords[1] * AtlasScaling) + atlasOffset;
				vertex->color = additiveColor;
				++vertex;
				
				vertex->position = shape[2];
				vertex->texCoord = (TexCoords[2] * AtlasScaling) + atlasOffset;
				vertex->color = additiveColor;
				++vertex;
				
				// GL_TRIANGLE
				vertex->position = shape[0];
				vertex->texCoord = (TexCoords[0] * AtlasScaling) + atlasOffset;
				vertex->color = additiveColor;
				++vertex;
				
				vertex->position = shape[2];
				vertex->texCoord = (TexCoords[2] * AtlasScaling) + atlasOffset;
				vertex->color = additiveColor;
				++vertex;
				
				vertex->position = shape[3];
				vertex->texCoord = (TexCoords[3] * AtlasScaling) + atlasOffset;
				vertex->color = additiveColor;
				++vertex;
				
				verticesWritten += 6;

			} else {
				//
				// active == false or alpha == 0, so this particle isn't visible:
				// write 2 triangles which will not be rendered
				//
				for (int i = 0; i < 6; i++) {
					vertex->position = origin;
					vertex->color = transparent;
					++vertex;
				}
			}
		}
		
		if (_particlesVbo && verticesWritten > 0) {
			
			// TODO: Only submit written * 6 particles...
			
			// transfer to GPU
			_batchDrawStart = 0;
			_batchDrawCount = static_cast<GLsizei>(activeCount) * 6;
			void *gpuMem = _particlesVbo->mapReplace();
			memcpy( gpuMem, _particles.data(), _batchDrawCount * sizeof(particle_vertex) );
			_particlesVbo->unmap();
			
			return true;
		}
		
		return false;
	}


#pragma mark - UniversalParticleSystem
	
	UniversalParticleSystem::config UniversalParticleSystem::config::parse(const util::xml::XmlMultiTree &node) {
		config c;
		c.maxParticleCount = util::xml::readNumericAttribute<size_t>(node, "count", c.maxParticleCount);
		c.drawConfig = UniversalParticleSystemDrawComponent::config::parse(node.getChild("draw"));
		return c;
	}
	
	UniversalParticleSystemRef UniversalParticleSystem::create(string name, const config &c) {
		auto simulation = make_shared<UniversalParticleSimulation>();
		simulation->setParticleCount(c.maxParticleCount);
		auto draw = make_shared<UniversalParticleSystemDrawComponent>(c.drawConfig);
		return Object::create<UniversalParticleSystem>(name, { draw, simulation });
	}
	
	UniversalParticleSystem::UniversalParticleSystem(string name):
	ParticleSystem(name)
	{}
	
	
}
