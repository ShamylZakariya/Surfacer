//
//  System.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 10/23/17.
//

#include "ParticleSystem.hpp"

#include <chipmunk/chipmunk_unsafe.h>

using namespace core;
namespace particles {
	
	namespace {
		
		double MinKinematicParticleRadius = 0.05;
		
		dvec2 safe_normalize(const dvec2 dir) {
			double l2 = lengthSquared(dir);
			if (l2 > 1e-3) {
				double l = sqrt(l2);
				return dir / l;
			}
			return dir;
		}
		
	}
	
#pragma mark - ParticleSimulation

	/*
	 size_t _count;
	 cpBB _bb;
	 vector<particle_prototype> _prototypes;
	 core::SpaceAccessRef _spaceAccess;
	 */
		
	ParticleSimulation::ParticleSimulation():
	BaseParticleSimulation(),
	_count(0),
	_bb(cpBBInvalid)
	{}
	
	// Component
	void ParticleSimulation::onReady(core::ObjectRef parent, core::StageRef stage) {
		BaseParticleSimulation::onReady(parent, stage);
		_spaceAccess = stage->getSpace();
	}

	void ParticleSimulation::onCleanup() {
		BaseParticleSimulation::onCleanup();
	}

	void ParticleSimulation::update(const core::time_state &time) {
		BaseParticleSimulation::update(time);
		_prepareForSimulation(time);
		_simulate(time);
	}
	
	// BaseParticleSimulation
	
	void ParticleSimulation::setParticleCount(size_t count) {
		BaseParticleSimulation::setParticleCount(count);
		_prototypes.resize(count);
	}
	
	size_t ParticleSimulation::getFirstActive() const {
		// ALWAYS zero
		return 0;
	}

	size_t ParticleSimulation::getActiveCount() const {
		return min(_count, _state.size());
	}

	cpBB ParticleSimulation::getBB() const {
		return _bb;
	}
	
	// ParticleSimulation
	
	void ParticleSimulation::emit(const particle_prototype &particle, const dvec2 &world, const dvec2 &dir) {
		_pending.push_back(particle);
		_pending.back().position = world;
		_pending.back()._velocity = dir * particle.initialVelocity;
	}
	
	void ParticleSimulation::_prepareForSimulation(const core::time_state &time) {
		// run a first pass where we update age and completion, then if necessary perform a compaction pass
		size_t expiredCount = 0;
		const size_t activeCount = getActiveCount();
		const size_t storageSize = _prototypes.size();
		bool sortSuggested = false;
		
		auto state = _state.begin();
		auto end = state + activeCount;
		auto prototype = _prototypes.begin();
		for (; state != end; ++state, ++prototype) {
			
			//
			// update age and completion
			//
			
			prototype->_age += time.deltaT;
			prototype->_completion = prototype->_age / prototype->lifespan;
			
			if (prototype->_completion > 1) {
				
				//
				// This particle is expired. Clean it up, and note how many expired we have
				//
				
				prototype->destroy();
				expiredCount++;
			}
		}
		
		if (expiredCount > activeCount / 2) {
			
			//
			// parition templates such that expired ones are at the end; then reset _count accordingly
			// note: We don't need to sort _particleState because it's ephemeral; we'll overwrite what's
			// needed next pass to update()
			//
			
			auto end = partition(_prototypes.begin(), _prototypes.begin() + activeCount, [](const particle_prototype &prototype){
				return prototype._completion <= 1;
			});
			
			_count = end - _prototypes.begin();
			sortSuggested = true;
		}
		
		//
		//	Process any particles that were emitted
		//

		if (!_pending.empty()) {
		
			for (auto &particle : _pending) {

				//
				// if a particle already lives at this point, perform any cleanup needed
				//
				
				const size_t idx = _count % storageSize;
				_prototypes[idx].destroy();
				
				//
				//	Assign template, and if it's kinematic, create chipmunk physics backing
				//
				
				_prototypes[idx] = particle;
				
				if (particle.kinematics) {
					double mass = particle.mass.getInitialValue();
					double radius = max(particle.radius.getInitialValue(), MinKinematicParticleRadius);
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
					cpBodySetVelocity(body, cpv(particle._velocity));
					cpShapeSetFilter(shape, particle.kinematics.filter);
					cpShapeSetFriction(shape, particle.kinematics.friction);
					
					_prototypes[idx]._body = body;
					_prototypes[idx]._shape = shape;
				}
				
				_prototypes[idx].prepare();
				
				_count++;
				
				// we've round-robined, which means a sort might be needed
				if (_count >= storageSize) {
					sortSuggested = true;
				}
			}
			
			_pending.clear();
		}
		
		if (sortSuggested && _keepSorted) {
			// sort so oldest particles are at front of _prototypes
			sort(_prototypes.begin(), _prototypes.begin() + getActiveCount(), [](const particle_prototype &a, const particle_prototype &b){
				return a._age > b._age;
			});
		}
		
		// we'll re-enable in _simulate
		// TODO: Consider a more graceful handling of this, since re-enablement below is awkward
		for (auto &state : _state) {
			state.active = false;
		}
	}

	void ParticleSimulation::_simulate(const core::time_state &time) {
		
		const auto &gravities = getStage()->getGravities();
		cpBB bb = cpBBInvalid;
		
		auto state = _state.begin();
		auto end = state + getActiveCount();
		auto prototype = _prototypes.begin();
		size_t idx = 0;
		for (; state != end; ++state, ++prototype, ++idx) {
			
			// _prepareForSimulation deactivates all particles
			// and requires _simulate to re-activate any which should be active

			state->active = prototype->_completion <= 1;
			if (!state->active) {
				continue;
			}

			const auto radius = prototype->radius(prototype->_completion);
			const auto size = radius * M_SQRT2;
			const auto damping = 1-saturate(prototype->damping(prototype->_completion));
			const auto additivity = prototype->additivity(prototype->_completion);
			const auto mass = prototype->mass(prototype->_completion);
			const auto color = prototype->color(prototype->_completion);
			
			bool didRotate = false;
			
			if (!prototype->kinematics) {
				
				//
				//	Non-kinematic particles get standard velocity + damping + gravity applied
				//
				
				prototype->position = prototype->position + prototype->_velocity * time.deltaT;
				
				for (const auto &gravity : gravities) {
					if (gravity->getGravitationLayer() & prototype->gravitationLayerMask) {
						auto force = gravity->calculate(prototype->position);
						prototype->_velocity += mass * force.magnitude * force.dir * time.deltaT;
					}
				}
				
				if (damping < 1) {
					prototype->_velocity *= damping;
				}
								
			} else {
				
				//
				// Kinematic bodies are simulated by chipmunk; extract position, rotation etc
				//
				
				cpShape *shape = prototype->_shape;
				cpBody *body = prototype->_body;
				
				prototype->position = v2(cpBodyGetPosition(body));
				
				if (damping < 1) {
					cpBodySetVelocity(body, cpvmult(cpBodyGetVelocity(body), damping));
					cpBodySetAngularVelocity(body, damping * cpBodyGetAngularVelocity(body));
				}
				
				prototype->_velocity = v2(cpBodyGetVelocity(body));
				
				if (!prototype->orientToVelocity) {
					dvec2 rotation = v2(cpBodyGetRotation(body));
					state->right = rotation * size;
					state->up = rotateCCW(state->right);
					didRotate = true;
				}
				
				// now update chipmunk's representation
				cpBodySetMass(body, max(mass, 0.0));
				cpCircleShapeSetRadius(shape, max(radius,MinKinematicParticleRadius));
			}
			
			if (prototype->orientToVelocity) {
				dvec2 dir = prototype->_velocity;
				double len2 = lengthSquared(dir);
				if (prototype->orientToVelocity && len2 > 1e-2) {
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
			
			state->atlasIdx = prototype->atlasIdx;
			state->age = prototype->_age;
			state->completion = prototype->_completion;
			state->position = prototype->position;
			state->color = color;
			state->additivity = additivity;
			
			bb = cpBBExpand(bb, prototype->position, size);
		}
		
		//
		// update BB and notify
		//
		
		_bb = bb;
		notifyMoved();
	}
	
#pragma mark - ParticleEmitter
	
	namespace {
		ParticleEmitter::emission_id _emission_id_tracker = 0;
		
		ParticleEmitter::emission_id next_emission_id() {
			return ++_emission_id_tracker;
		}
	}
	
	/*
	 ParticleSimulationWeakRef _simulation;
	 vector<emission> _emissions;
	 vector<emission_prototype> _prototypes;
	 vector<int> _prototypeLookup;
	*/
	
	ParticleEmitter::ParticleEmitter(uint32_t seed):
	_rng(seed)
	{}
	
	void ParticleEmitter::update(const core::time_state &time) {
		if (ParticleSimulationRef sim = _simulation.lock()) {

			//
			//	Process all active emissions, collecting those which have expired
			//

			vector<emission_id> expired;
			for (auto &it : _emissions) {
				emission &e = it.second;
				e.secondsAccumulator += time.deltaT;
				
				// this one's done, collect and move on
				if (e.endTime > 0 && e.endTime < time.time) {
					expired.push_back(e.id);
					continue;
				}
				
				double ramp = 1;
				if (e.startTime > 0 && e.endTime > 0) {
					double life = (time.time - e.startTime) / (e.endTime - e.startTime);
					ramp = e.envelope(life);
				}
				
				if (e.secondsAccumulator > e.secondsPerEmission) {
					// get number of particles to emit
					double particlesToEmit = ramp * e.secondsAccumulator / e.secondsPerEmission;
					//CI_LOG_D("ramp: " << ramp << " particlesToEmit: " << particlesToEmit);
					
					while (particlesToEmit >= 1) {
						
						// emit one particle
						size_t idx = static_cast<size_t>(_rng.nextUint()) % _prototypeLookup.size();
						const emission_prototype &proto = _prototypes[_prototypeLookup[idx]];
						dvec2 position = e.world + e.radius * static_cast<double>(_rng.nextFloat()) * dvec2(_rng.nextVec2());
						dvec2 direction = perturb(e.dir, proto.variance);
						sim->emit(proto.prototype, position, direction);
						
						// remove one particle's worth of seconds from accumulator
						// accumulator will retain leftovers for next step
						particlesToEmit -= 1;
						e.secondsAccumulator -= e.secondsPerEmission;
					}
				}
			}
			
			// dispose of expired emissions
			if (!expired.empty()) {
				for (auto id : expired) {
					_emissions.erase(id);
				}
			}
		}
	}
	
	void ParticleEmitter::onReady(ObjectRef parent, StageRef stage) {
		if (!getSimulation()) {
			ParticleSimulationRef sim = getSibling<ParticleSimulation>();
			if (sim) {
				setSimulation(sim);
			}
		}
	}
	
	// ParticleEmitter
	
	void ParticleEmitter::setSimulation(const ParticleSimulationRef simulation) {
		_simulation = simulation;
	}
	
	ParticleSimulationRef ParticleEmitter::getSimulation() const {
		return _simulation.lock();
	}
	
	void ParticleEmitter::seed(uint32_t seed) {
		_rng.seed(seed);
	}
	
	void ParticleEmitter::add(const particle_prototype &prototype, float variance, int probability) {
		size_t idx = _prototypes.size();
		_prototypes.push_back({prototype, variance});
		for (int i = 0; i < probability; i++) {
			_prototypeLookup.push_back(idx);
		}
	}
	
	size_t ParticleEmitter::emit(dvec2 world, double radius, dvec2 dir, seconds_t duration, double rate, Envelope env) {
		switch (env) {
			case RampUp:
				return emit(world, radius, dir, duration, rate, interpolator<double>({0.0,1.0}));
				break;
			case Sawtooth:
				return emit(world, radius, dir, duration, rate, interpolator<double>({0.0,1.0,0.0}));
				break;
			case Mesa:
				return emit(world, radius, dir, duration, rate, interpolator<double>({0.0,1.0,1.0,1.0,1.0,1.0,1.0,0.0}));
				break;
			case RampDown:
				return emit(world, radius, dir, duration, rate, interpolator<double>({1.0,0.0}));
				break;
		}
		
		// this shouldn't happen
		return 0;
	}
	
	size_t ParticleEmitter::emit(dvec2 world, double radius, dvec2 dir, seconds_t duration, double rate, const interpolator<double> &envelope) {
		emission e;
		e.id = next_emission_id();
		e.startTime = getStage()->getTimeState().time;
		e.endTime = e.startTime + duration;
		e.secondsPerEmission = 1.0 / rate;
		e.secondsAccumulator = 0;
		e.world = world;
		e.dir = safe_normalize(dir);
		e.radius = radius;
		e.envelope = envelope;		
		_emissions[e.id] = e;
		
		return e.id;
	}
	
	size_t ParticleEmitter::emit(dvec2 world, double radius, dvec2 dir, double rate) {
		emission e;
		e.id = next_emission_id();
		e.startTime = e.endTime = -1;
		e.secondsPerEmission = 1.0 / rate;
		e.secondsAccumulator = 0;
		e.world = world;
		e.dir = safe_normalize(dir);
		e.radius = radius;
		e.envelope = 1;
		
		_emissions[e.id] = e;
		
		return e.id;
	}
	
	void ParticleEmitter::setEmissionPosition(emission_id emissionId, dvec2 world, double radius, dvec2 dir) {
		auto pos = _emissions.find(emissionId);
		if (pos != _emissions.end()) {
			pos->second.world = world;
			pos->second.radius = radius;
			pos->second.dir = safe_normalize(dir);
		}
	}

	void ParticleEmitter::cancel(emission_id id) {
		auto pos = _emissions.find(id);
		if (pos != _emissions.end()) {
			_emissions.erase(pos);
		}
	}
	
	void ParticleEmitter::emitBurst(dvec2 world, double radius, dvec2 dir, int count) {
		if (ParticleSimulationRef sim = _simulation.lock()) {
			dir = safe_normalize(dir);

			for (int i = 0; i < count; i++) {
				size_t idx = static_cast<size_t>(_rng.nextUint()) % _prototypeLookup.size();
				const emission_prototype &proto = _prototypes[_prototypeLookup[idx]];
				dvec2 position = world + radius * static_cast<double>(_rng.nextFloat()) * dvec2(_rng.nextVec2());
				dvec2 direction = perturb(dir, proto.variance);
				
				sim->emit(proto.prototype, position, direction);
			}
		}
	}
	
	double ParticleEmitter::nextDouble(double variance) {
		return 1.0 + static_cast<double>(_rng.nextFloat(-variance, variance));
	}

	dvec2 ParticleEmitter::perturb(const dvec2 dir, double variance) {
		double len2 = lengthSquared(dir);
		if (len2 > 0) {
			return dir * (1.0 + nextDouble(variance));
		}
		return dir;
	}
	
	double ParticleEmitter::perturb(double value, double variance) {
		return value * (1.0 + _rng.nextFloat(-variance, +variance));
	}


#pragma mark - ParticleSystemDrawComponent
	
	/*
	 config _config;
	 gl::GlslProgRef _shader;
	 vector<particle_vertex> _particles;
	 gl::VboRef _particlesVbo;
	 gl::BatchRef _particlesBatch;
	 GLsizei _batchDrawStart, _batchDrawCount;
	 */
	
	ParticleSystemDrawComponent::config ParticleSystemDrawComponent::config::parse(const util::xml::XmlMultiTree &node) {
		config c = BaseParticleSystemDrawComponent::config::parse(node);
		
		auto textureName = node.getAttribute("textureAtlas");
		if (textureName) {
			
			auto image = loadImage(app::loadAsset(*textureName));
			gl::Texture2d::Format fmt = gl::Texture2d::Format().mipmap(false);
			
			c.textureAtlas = gl::Texture2d::create(image, fmt);
			c.atlasType = Atlas::fromString(node.getAttribute("atlasType", "None"));
		}
		
		return c;
	}
	
	ParticleSystemDrawComponent::ParticleSystemDrawComponent(config c):
	BaseParticleSystemDrawComponent(c),
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
	
	void ParticleSystemDrawComponent::setSimulation(const BaseParticleSimulationRef simulation) {
		BaseParticleSystemDrawComponent::setSimulation(simulation);
		
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
	
	void ParticleSystemDrawComponent::draw(const render_state &renderState) {
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
	
	bool ParticleSystemDrawComponent::updateParticles(const BaseParticleSimulationRef &sim) {
		
		// walk the simulation particle state and write to our vertices		
		const Atlas::Type AtlasType = _config.atlasType;
		const vec2* AtlasOffsets = Atlas::AtlasOffsets(AtlasType);
		const float AtlasScaling = Atlas::AtlasScaling(AtlasType);
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


#pragma mark - System
	
	ParticleSystem::config ParticleSystem::config::parse(const util::xml::XmlMultiTree &node) {
		config c;
		c.maxParticleCount = util::xml::readNumericAttribute<size_t>(node, "count", c.maxParticleCount);
		c.keepSorted = util::xml::readBoolAttribute(node, "sorted", c.keepSorted);
		c.drawConfig = ParticleSystemDrawComponent::config::parse(node.getChild("draw"));
		return c;
	}
	
	ParticleSystemRef ParticleSystem::create(string name, const config &c) {
		auto simulation = make_shared<ParticleSimulation>();
		simulation->setParticleCount(c.maxParticleCount);
		simulation->setShouldKeepSorted(c.keepSorted);
		auto draw = make_shared<ParticleSystemDrawComponent>(c.drawConfig);
		
		ParticleSystemRef ps = make_shared<ParticleSystem>(name, c);
		ps->addComponent(draw);
		ps->addComponent(simulation);
		
		return ps;
	}
	
	ParticleSystem::ParticleSystem(string name, const config &c):
	BaseParticleSystem(name),
	_config(c)
	{}
	
	ParticleEmitterRef ParticleSystem::createEmitter() {
		ParticleEmitterRef emitter = make_shared<ParticleEmitter>();
		emitter->setSimulation(getComponent<ParticleSimulation>());
		addComponent(emitter);
		return emitter;
	}
	
	size_t ParticleSystem::getGravitationLayerMask(cpBody *body) const {
		return _config.kinematicParticleGravitationLayerMask;
	}

}
