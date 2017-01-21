#pragma once

//
//  ShubNiggurath.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 12/10/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#include "MotileFluidMonster.h"
#include "Grub.h"

namespace game {

class ShubNiggurath;

class Tentacle : public core::GameObject
{
	public:
	
		struct init : public core::GameObject::init
		{
			int numSegments;
			real segmentLength;
			real segmentWidth;
			real segmentDensity;
			
			// these are set by the parent ShubNiggurath
			real _distance;
			real _angleOffset;
			real _z;
			int _idx;
			
			init():
				numSegments(5),
				segmentLength(2),
				segmentWidth(0.5),
				segmentDensity(1),
				_distance(0),
				_angleOffset(0),
				_z(0),
				_idx(0)
			{}
			
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				core::GameObject::init::initialize(v);
				JSON_READ(v,numSegments);
				JSON_READ(v,segmentLength);
				JSON_READ(v,segmentWidth);
				JSON_READ(v,segmentDensity);
			}

			
		};
		
		struct segment {
			cpShape *shape;
			cpBody *body;
			cpConstraint *joint;
			cpConstraint *rotation;
			cpConstraint *angularLimit;

			real width, length, angularRange, torque, heat, scale;
			Vec2 position, dir;
			seconds_t disconnectionTime;
			bool isBeingShot;
			
			segment():
				shape(NULL),
				body(NULL),
				joint(NULL),
				rotation(NULL),
				angularLimit(NULL),
				width(0),
				length(0),
				angularRange(0),
				torque(0),
				heat(0),
				disconnectionTime(0),
				isBeingShot(false)
			{}
			
			bool connected() const { return joint != NULL; }
			
		};
		
		typedef std::vector< segment > segmentvec;

	public:
	
		Tentacle();
		virtual ~Tentacle();

		virtual void initialize( const init & );
		const init &initializer() const { return _initializer; }

		virtual void addedToLevel( core::Level *level );
		virtual void update( const core::time_state & );		

		ShubNiggurath *shubNiggurath() const { return (ShubNiggurath*) parent(); }

		const segmentvec &segments() const { return _segments; }
		segmentvec &segments() { return _segments; }
		
		void die() { _dead = true; }
		bool dead() const { return _dead; }

		/**
			Get the index of the segment whos collision shape is @a shape, or -1 if none such exists
		*/
		int indexOfSegmentForShape( cpShape *shape ) const;
		
		/**
			Cause the tentacle to be broken at the base of the segment @a idx
		*/
		void disconnectSegment( int idx );

		void setColor( const Vec4 &c ) { _color = c; }
		Vec4 color() const { return _color; }	
			
		real length() const;

	private:
	
		void _destroySegments( segmentvec::iterator firstToDestroy );
				
	private:
	
		bool _dead;
		init _initializer;
		cpSpace *_space;
		segmentvec _segments;
		std::map< cpShape*, int > _segmentIndexForShape;
		real _fudge;
		Vec4 _color;
};

class ShubNiggurath : public MotileFluidMonster
{
	public:
	
		static const bool TENTACLES_CAN_BE_SEVERED = false;	
	
		struct init : public MotileFluidMonster::init
		{
			// Grubs are ShubNiggurath's 'ammo'
			Grub::init grubInit;
			real fireGrubsPerSecond;
			real growGrubsPerSecond;
			real maxGrubAttackDistance;
			int maxGrubs;

			std::vector< Tentacle::init > tentacles;
			ci::fs::path tentacleTexture;
			
			init():
				fireGrubsPerSecond(0.5),
				growGrubsPerSecond(0.25),
				maxGrubAttackDistance(50),
				maxGrubs(8),
				tentacleTexture("ShubNiggurathTentacleSkin.png")
			{
				speed = 2;

				health.initialHealth = health.maxHealth = 100;
				health.resistance = 0.9;
				introTime = fluidInit.introTime = 4;
				extroTime = fluidInit.extroTime = 6;
			
				fluidInit.pulsePeriod = 2;
				fluidInit.pulseMagnitude = 0.25;
				fluidInit.friction = 1;
				fluidInit.elasticity = 0;
				fluidInit.radius = 4;
				fluidInit.numParticles = 16;
				fluidInit.fluidParticleTexture = "ShubNiggurathParticle.png";
				fluidInit.fluidToneMapTexture = "ShubNiggurathToneMap.png";
				fluidInit.fluidHighlightColor = ci::ColorA(1,1,1,0.1);

				attackStrength = 100;
			}
				
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				MotileFluidMonster::init::initialize(v);
				JSON_READ(v,grubInit);
				JSON_READ(v,fireGrubsPerSecond);
				JSON_READ(v,growGrubsPerSecond);
				JSON_READ(v,maxGrubAttackDistance);
				JSON_READ(v,maxGrubs);
				JSON_READ(v,tentacleTexture);

				if ( v.hasChild("tentacles"))
				{
					const ci::JsonTree ts( v["tentacles"] );
					for ( ci::JsonTree::ConstIter tentacle(ts.begin()),end(ts.end()); tentacle != end; ++tentacle )
					{
						Tentacle::init tentacleInit;
						tentacleInit.initialize(*tentacle);
						tentacles.push_back(tentacleInit);
					}
				}
			}
		};
		
	public:
	
		ShubNiggurath();
		virtual ~ShubNiggurath();

		JSON_INITIALIZABLE_INITIALIZE();

		// Initialization
		virtual void initialize( const init & );
		const init &initializer() const { return _initializer; }

		// GameObject
		virtual void addedToLevel( core::Level *level );
		virtual void draw( const core::render_state & );
		virtual void update( const core::time_state & );		
		
		// Monster
		virtual void injured( const HealthComponent::injury_info &info );
		virtual void died( const HealthComponent::injury_info &info );
		virtual bool allowWeaponHit( Weapon *weapon, cpShape *shape ) const;

	protected:
	
		friend class Tentacle;

		// MotileFluidMonster
		virtual MotileFluid::fluid_type _fluidType() const { return MotileFluid::AMORPHOUS_TYPE; }

		// ShubNiggurath
		void _handleBodyInjury( const HealthComponent::injury_info &info );
		void _handleTentacleInjury( const HealthComponent::injury_info &info );
		void _tentacleSevered( Tentacle *tentacle, int idx );
		void _emitGrub();
		
		bool _aim( const Vec2 &targetPosition, real &linearVel, Vec2 &dir, int triesRemaining );
		
	private:
	
		init _initializer;
		bool _firing;
		std::vector< Tentacle* > _tentacles;
		real _aggressionDistance, _grubs;
		seconds_t _lastGrubEmissionTime;
		cpShape *_tentacleAttachmentShape;
};

class ShubNiggurathController : public MotileFluidMonsterController
{
	public:
	
		ShubNiggurathController();
		virtual ~ShubNiggurathController(){}
		
		ShubNiggurath *shubNiggurath() const { return (ShubNiggurath*) owner(); }
		
		virtual void update( const core::time_state &time );

};


#pragma mark - Renderers

class TentacleRenderer : public core::DrawComponent
{
	public:
	
		TentacleRenderer();
		virtual ~TentacleRenderer();
		
		Tentacle *tentacle() const { return (Tentacle*) owner(); }
		virtual void draw( const core::render_state & );

	protected:
	
		virtual void _drawGame( const core::render_state & );
		virtual void _drawDebug( const core::render_state & );

		void _updateSplineReps();
		void _decompose();
		
	private:
	
		struct vertex {
			ci::Vec2 position;
			ci::Vec2 texCoord;
			float heat, opacity;
			
			vertex( const ci::Vec2 &p, const ci::Vec2 &tc, float h, float o ):
				position(p),
				texCoord(tc),
				heat(h),
				opacity(o)
			{}
		};
		
		struct spline_rep {
			std::vector< ci::Vec2 > segmentVertices, splineVertices;
			float startWidth, endWidth, startHeat, endHeat, startScale, endScale;	
		};
		
		typedef std::vector<vertex> vertex_vec;
		typedef std::vector< spline_rep > spline_rep_vec;
				
	private:
	
		GLuint _vbo;
		ci::gl::GlslProg _shader;
		ci::gl::Texture _tentacleTexture, _toneMapTexture;
		GLint _heatAttrLoc, _opacityAttrLoc;

		spline_rep_vec _splineReps;
		vertex_vec _quads;
};


} // end namespace game
