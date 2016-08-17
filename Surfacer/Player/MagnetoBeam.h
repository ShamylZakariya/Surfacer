#pragma once

//
//  MagnetoBeam.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 2/8/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Weapon.h"

#include <cinder/gl/gl.h>
#include <cinder/gl/Texture.h>
#include <cinder/gl/GlslProg.h>
#include <cinder/Channel.h>


namespace game {

class Player;
class MagnetoBeam;
class MagnetoBeamController;

/**
	Objects which may be towed, and which want to be notified when
	they are towed can implement this interface.
	
	Note: towability is denoted by GameObjectType and mass, etc.
	
*/
class TowListener
{
	public:
	
		TowListener(){}
		virtual ~TowListener(){}

		// invoked when a MagnetoBeam instance latches onto an object to tow it
		virtual void aboutToBeTowed( MagnetoBeam *beam ) = 0;

		// invoked when a MagnetoBeam instanceis about to release an object being towed
		virtual void aboutToBeUnTowed( MagnetoBeam *beam ) = 0;
		
};



class MagnetoBeam : public BeamWeapon 
{
	public:
	
		enum untowable_reason
		{
			NONE,
			UNTOWABLE_BECAUSE_NULL,
			UNTOWABLE_BECAUSE_STATIC,
			UNTOWABLE_BECAUSE_TOO_HEAVY,
			UNTOWABLE_BECAUSE_WRONG_MATERIAL
		};
	
	
		struct init : public BeamWeapon::init
		{
			real maxTowMass;
		
			init():
				maxTowMass(5000)
			{
				width = 0.05;
				maxRange = 500;
				powerPerSecond = 0.1;
			}	
			
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				BeamWeapon::init::initialize(v);
				JSON_READ(v,maxTowMass);
			}

										
		};
		
		struct tow_handle {
			raycast_target target;				//	the object being towed
			cpConstraint *positionConstraint;	//	positioning constraint 
			cpConstraint *rotationConstraint;	//	rotation constraint
			bool contracting;					//	if true, towing is in initial contraction phase
			bool towable;						//	if true, the object is superficially towable, if not, see untowableReason
			untowable_reason untowableReason;	//	the reason not towable, if not towable
			real initialDistance;				//	the distance between the object's center of mass and the magneto beam target
			real contraction;					//	goes from 0 to 1, where 1 means the contraction phase has finished (think "fully contracted" )
			
			tow_handle():
				positionConstraint(NULL),
				rotationConstraint(NULL),
				contracting(false),
				towable(false),
				untowableReason(NONE),
				initialDistance(0),
				contraction(0)
			{}
			
			operator bool() const { return bool(target); }
			
			void release();
			bool allowTowing( Player *player ) const;
		};
		
		typedef std::map< GameObject*, tow_handle > TowHandlesByObject;
		
		// emitted when a new towing session is begun
		signals::signal< void(MagnetoBeam*, const tow_handle &) > towBegun;
		
		// emitted when a towing session is voluntarily ended by user
		signals::signal< void(MagnetoBeam*, const tow_handle &) > towReleased;
		
		// emitted when a towing session is involuntarily ended by occlusion of towing beam
		signals::signal< void(MagnetoBeam*, const tow_handle &) > towSevered;

	public:
	
		MagnetoBeam();
		virtual ~MagnetoBeam();

		// Initialization
		virtual void initialize( const init & );
		const init &initializer() const { return _initializer; }
		
		// GameObject
		virtual void addedToLevel( core::Level * );
		virtual void removedFromLevel( core::Level * );
		virtual void update( const core::time_state & );
		
		// Weapon
		virtual void setFiring( bool on );
		virtual WeaponType::type weaponType() const { return WeaponType::MAGNETO_BEAM; }
		virtual void wasPutAway();
		virtual bool drawingPower() const { return towing(); }
		virtual real drawingPowerPerSecond() const;
		
		// MagnetoBeam
		void setTowPosition( const Vec2r &worldPos );
		Vec2r towPosition() const { return _towPosition; }

		void rotateTowBy( real deltaRadians );
		tow_handle towable( const raycast_target & ) const;

		MagnetoBeamController *controller() const { return _controller; }		
		cpBody *towBody() const { return _towBody; }
		const tow_handle &towing() const { return _towing; }

		
	private:
	
		void _tow( const tow_handle &towable );
		void _untow( GameObject *obj );
				
	private:
	
		init _initializer;
		MagnetoBeamController *_controller;
		
		real _towAngle, _maxConstraintForce;		
		tow_handle _towing;
		cpBody *_towBody;	
		Vec2r _towPosition;
		
		seconds_t _towSeverTime, _minTimeUntilNextTow;
};

class MagnetoBeamRenderer : public core::DrawComponent
{
	public:
	
		MagnetoBeamRenderer();
		virtual ~MagnetoBeamRenderer();
		
		MagnetoBeam *magnetoBeam() const { return (MagnetoBeam*) owner(); }
		virtual void draw( const core::render_state & );

	protected:

		struct vertex {
			Vec2r position;
			Vec4r color;		
			
			vertex(){}
			vertex( const Vec2r &p, const Vec4r &c ):
				position(p),
				color(c)
			{}
			
		};
		
		typedef std::vector< vertex > vertex_vec;
						
		struct beam_segment {
			Vec2r position;
			real tension;
			bool firstFrame;
			
			beam_segment():
				tension(1),
				firstFrame(true)
			{}
		};
		
		typedef std::vector< beam_segment > beam_segment_vec;
		
		struct beam_vertex {
			Vec2r position;
			float edge;
		};

		typedef std::vector< beam_vertex > beam_vertex_vec;
		
		struct perlin_init {
			int octaves, seed;
			double falloff, scale;
			
			perlin_init():
				octaves(4),
				falloff(0.5),
				scale(1),
				seed(123)
			{}
		};
		
		struct beam_state {
			Vec2r start, end;
			real transition, width, noiseScale, radius;
			beam_segment_vec segments;
			beam_vertex_vec vertices;
			ci::Channel32f noise;
			perlin_init perlin;
			
			Vec2r noiseOffset, noiseOffsetRate;
			Vec4r color;	
			
			unsigned int segmentsDrawn;
			GLuint vbo;
			
			beam_state():
				transition(0),
				width(0),
				noiseScale(0),
				radius(0),
				segmentsDrawn(0),
				vbo(0)
			{}
			
			~beam_state();			
		};
		
		typedef std::vector< beam_state > beam_state_vec;
		
				
	protected:
	
		void _drawGame( const core::render_state &state );
		void _drawDevelopment( const core::render_state &s );
		void _drawDebug( const core::render_state &state );

		void _updateBeam( const core::render_state &state, beam_state &beamState );
		void _drawBeam( const core::render_state &state, beam_state &beamState, ci::gl::GlslProg &shader );
		void _debugDrawBeam( const core::render_state &state, beam_state &beamState );

	protected:
	
		ci::gl::GlslProg _towOverlayShader, _queryOverlayShader, _beamShader;		
		beam_state_vec _beamStates;
		GLint _beamShaderEdgeAttrLoc;
		
};

class MagnetoBeamController : public core::InputComponent
{
	public:
	
		MagnetoBeamController();
		virtual ~MagnetoBeamController();
		
		MagnetoBeam *magnetoBeam() const { return (MagnetoBeam*) owner(); }
		bool active() const { return magnetoBeam()->active(); }

		virtual void update( const core::time_state &time );

		virtual bool mouseDown( const ci::app::MouseEvent &event );
		virtual bool mouseUp( const ci::app::MouseEvent &event );
		virtual bool mouseWheel( const ci::app::MouseEvent &event );
		
		real spin() const { return _spin; }
		
	private:

		real _spin, _deltaSpin;
				
};


} // end namespace game