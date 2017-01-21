#pragma once

//
//  Grub.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 1/28/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//


#include "Monster.h"

namespace game {

class Grub : public Monster
{
	public:
	
		struct init : public Monster::init {
			real size;
			real speed;
			real density;
			seconds_t lifespan;
			ci::ColorA color;
			ci::fs::path modulationTexture;

			init():
				size(2),
				speed(5),
				density(1),
				lifespan(0),
				color(0.9,0.9,0.9,1),
				modulationTexture("GrubSkin.png")
			{
				health.initialHealth = health.maxHealth = 5;
				health.resistance = 0.1;
				health.crushingInjuryMultiplier = INFINITY;
				health.stompable = true;
				
				attackStrength = 1;
			}
			
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				Monster::init::initialize(v);
				JSON_READ(v, size);
				JSON_READ(v, speed);
				JSON_READ(v, density);
				JSON_READ(v, lifespan);
				JSON_READ(v, color);
				JSON_READ(v, modulationTexture);
			}

				
		};

	public:
	
		Grub();
		virtual ~Grub();

		JSON_INITIALIZABLE_INITIALIZE();
		
		// Barnacle
		virtual void initialize( const init & );
		const init &initializer() const { return _initializer; }
		
		// GameObject
		virtual void addedToLevel( core::Level *level );
		virtual void update( const core::time_state & );
		
		// Monster
		virtual void died( const HealthComponent::injury_info &info );
		virtual Vec2 position() const { return v2(cpBodyGetPos( _headBody )); }
		virtual Vec2 up() const;
		virtual Vec2 eyePosition() const { return position(); }

		virtual cpBody *body() const { return _headBody; }

		// Grub
		real size() const { return _initializer.size; }
		const cpShapeVec &shapes() const { return _shapes; }
		const cpConstraintVec &constraints() const { return _constraints; }
		const cpBodyVec &bodies() const { return _bodies; }
			
	private:
	
		init _initializer;
		cpShapeVec _shapes;
		cpConstraintVec _constraints, _gearJoints;
		cpBodyVec _bodies;
		cpBody *_headBody;
		std::vector< real > _radii;
		real _speed;

};

class GrubRenderer : public core::DrawComponent
{
	public:
	
		GrubRenderer();
		virtual ~GrubRenderer();
		
		virtual void draw( const core::render_state &state );
		
		Grub *grub() const { return (Grub*) owner(); }

	protected:
	
		void _tesselate( const core::render_state &state );
	
		void _drawGame( const core::render_state &state );
		void _drawDebug( const core::render_state &state );

	private:

		struct vertex {
			ci::Vec2 position;
			ci::Vec2 texCoord;

			vertex()
			{}

			vertex( const ci::Vec2 &p, const ci::Vec2 &tc ):
				position(p),
				texCoord(tc)
			{}
		};

		typedef std::vector<vertex> vertex_vec;
		
	private:
	
		GLuint _vbo;
		ci::gl::GlslProg _shader;
		ci::gl::Texture _modulationTexture;
		vertex_vec _triangleStrip;

};

class GrubController : public GroundBasedMonsterController
{
	public:
	
		GrubController();
		virtual ~GrubController(){}
		
		Grub *grub() const { return (Grub*) owner(); }		
		virtual void update( const core::time_state &time );
		
	private:

		virtual void _getMotionProbeRays( cpVect &leftOrigin, cpVect &leftEnd, cpVect &rightOrigin, cpVect &rightEnd );

};


} // end namespace game
