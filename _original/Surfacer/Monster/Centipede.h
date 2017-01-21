#pragma once

//
//  Centipede.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 6/2/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Monster.h"

namespace game {

class FrondEngine;

class Centipede : public Monster
{
	public:
	
		struct init : public Monster::init {
			real length;
			real thickness;
			real speed;
			real density;
			Vec2 position;
			ci::ColorA color;
			ci::fs::path modulationTexture;

			init():
				length(4),
				thickness(1),
				speed(5),
				density(1),
				position(0,0),
				color(0.8,0.8,0.8,1),
				modulationTexture("CentipedeSkin.png")
			{
				health.initialHealth = health.maxHealth = 10;
				health.crushingInjuryMultiplier = 0.5;
				health.resistance = 0.5;
				attackStrength = 2;
			}
			
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				Monster::init::initialize(v);
				JSON_READ(v,length);
				JSON_READ(v,thickness);
				JSON_READ(v,speed);
				JSON_READ(v,density);
				JSON_READ(v,position);
				JSON_READ(v,color);
				JSON_READ(v,modulationTexture);
			}
				
		};

	public:
	
		Centipede();
		virtual ~Centipede();

		JSON_INITIALIZABLE_INITIALIZE();
		
		// Barnacle
		virtual void initialize( const init & );
		const init &initializer() const { return _initializer; }
		
		// GameObject
		virtual void addedToLevel( core::Level *level );
		virtual void update( const core::time_state & );
		
		// Monster
		virtual void died( const HealthComponent::injury_info &info );
		virtual Vec2 position() const { return v2(cpBBCenter(aabb())); }
		virtual Vec2 up() const;
		virtual Vec2 eyePosition() const;
		virtual cpBody *body() const { return _headBody; }

		// Centipede
		cpBody *headBody() const { return _headBody; }
		cpBody *tailBody() const { return _tailBody; }
		real length() const { return _initializer.length; }
		real thickness() const { return _initializer.thickness; }
		const cpShapeVec &shapes() const { return _shapes; }
		const cpShapeVec &circles() const { return _circles; }
		const cpShapeVec &segments() const { return _segments; }
		const cpConstraintVec &constraints() const { return _constraints; }
		const cpBodyVec &bodies() const { return _bodies; }
			
	private:
	
		init _initializer;
		cpShapeVec _shapes, _circles, _segments;
		cpConstraintVec _constraints, _gearJoints, _pivotJoints;
		cpBodyVec _bodies;
		cpBody *_headBody, *_tailBody;
		std::vector< real > _radii, _thicknesses;
		real _mass, _weight;
		FrondEngine *_whiskers;
};

class CentipedeRenderer : public core::DrawComponent
{
	public:
	
		CentipedeRenderer();
		virtual ~CentipedeRenderer();
		
		virtual void draw( const core::render_state &state );
		
		Centipede *centipede() const { return (Centipede*) owner(); }

	protected:
	
		void _tesselate( const core::render_state &state );
		void _drawBody( const core::render_state &state );
		void _drawHeadAndTail( const core::render_state &state );
	
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
		ci::gl::GlslProg _bodyShader, _svgShader;
		ci::gl::Texture _modulationTexture;
		vertex_vec _triangleStrip;
		core::SvgObject _head;
		std::vector< core::SvgObject > _jaws;
		Vec2 _headScale;

};

class CentipedeController : public GroundBasedMonsterController
{
	public:
	
		CentipedeController();
		virtual ~CentipedeController(){}
		
		Centipede *centipede() const { return (Centipede*) owner(); }		
		virtual void update( const core::time_state &time );
		
	private:

		virtual void _getMotionProbeRays( cpVect &leftOrigin, cpVect &leftEnd, cpVect &rightOrigin, cpVect &rightEnd );


};

}
