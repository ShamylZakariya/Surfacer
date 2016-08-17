#pragma once

//
//  CuttingBeam.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 2/8/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Weapon.h"
#include "Flicker.h"


#include <cinder/gl/gl.h>
#include <cinder/gl/Texture.h>
#include <cinder/gl/GlslProg.h>

#include <cinder/Channel.h>

namespace game {

class CuttingBeam : public BeamWeapon 
{
	public:
	
		struct init : public BeamWeapon::init
		{
			unsigned int maxBounces;	//	the max number of mirror bounces we support
			real cutWidth;				//	the width of the cut when shooting terrain
			real cutStrength;			//	the strength of the cut when shooting terrain
			real overcut;				//	fudge factor for cutting

			ci::ColorA beamColor;
			ci::ColorA beamHaloColor;

			ci::ColorA aimColor;
			ci::ColorA aimHaloColor;
						
			init():
				maxBounces(16),
				cutWidth(1),
				cutStrength(0.01),
				overcut(0.25),
				beamColor(1.0,0.5,1.0,0.5),
				beamHaloColor(0,0,1,0.25),
				aimColor(1.0,0.5,0.0,0.5),
				aimHaloColor(1.0,0.0,0.0,0.25)
			{
				width = 1;
				maxRange = 100;
			}
			
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				BeamWeapon::init::initialize(v);
				JSON_READ(v,maxBounces);
				JSON_READ(v,cutWidth);
				JSON_READ(v,cutStrength);
				JSON_READ(v,overcut);
				JSON_READ(v,beamColor);
				JSON_READ(v,beamHaloColor);
				JSON_READ(v,aimColor);
				JSON_READ(v,aimHaloColor);
			}

									
		};
		
		struct beam_segment {
			Vec2r start, end, dir, right;
			real length;
			
			beam_segment(){}
			beam_segment( const Vec2r &s, const Vec2r &e ):
				start(s),
				end(e)
			{
				length = s.distance(e);
				dir = (e-s) / length;
				right = rotateCW(dir);
			}
		};
		
		typedef std::vector< beam_segment > beam_segment_vec;
		
		static void reflective_raycast( 			
			beam_segment_vec &segments,
			core::Level *level,
			Weapon *weapon,
			Vec2r start,
			Vec2r dir,
			real range,
			raycast_query_filter filter = raycast_query_filter(),
			cpShape *ignore = NULL,
			unsigned int maxBounces = 1000 );
			
		static raycast_query_filter defaultQueryFilter();
		
	public:
	
		CuttingBeam();
		virtual ~CuttingBeam();
		
		// Initialization
		virtual void initialize( const init & );
		const init &initializer() const { return _initializer; }
		
		// GameObject
		virtual void update( const core::time_state & );
		
		// Weapon
		virtual void setFiring( bool firing );
		virtual void setRange( real range );
		virtual WeaponType::type weaponType() const { return WeaponType::CUTTING_BEAM; }
		
		// BeamWeapon
		// Since CuttingBeam suports reflections, these only represent the first bounce
		virtual Vec2r beamOrigin() const { return _beamSegments.front().start; }
		virtual Vec2r beamEnd() const { return _beamSegments.front().end; }
		virtual real beamLength() const { return _beamSegments.front().length; }
		
		
		// CuttingBeam
		const beam_segment_vec &beamSegments() const { return _beamSegments; }
		unsigned int maxBeamSegments() const { return _initializer.maxBounces; }
		
		void setAiming( bool aiming );
		bool aiming() const { return _aiming; }

	protected:
	
		virtual void _cutTerrain( const beam_segment &seg );

	private:
	
		init _initializer;
		bool _aiming;
		beam_segment_vec _beamSegments;
};

class CuttingBeamRenderer : public core::DrawComponent
{
	public:
	
		CuttingBeamRenderer();
		virtual ~CuttingBeamRenderer();

		CuttingBeam *cuttingBeam() const { return (CuttingBeam*) owner(); }

		// DrawComponent
		virtual void addedToGameObject( core::GameObject * );
		virtual void update( const core::time_state &time );

	private:
	
		struct vertex {
			Vec2r position;
			Vec4r color;
			
			vertex(){}
			vertex( const Vec2r &p, const Vec4f &c ):
				position(p),
				color(c)
			{}
		};
		
		typedef std::vector< vertex > vertex_vec;

		enum { TRIANGLES_PER_DISC = 16, TRIANGLES_PER_LINE = (16+4) };
	
		void _drawGame( const core::render_state &state );
		void _drawDebug( const core::render_state &state );
		
		void _renderCircle( const Vec2r &position, real radius, const Vec4r &color, vertex_vec::iterator &vertex ) const;
		void _renderLine( const Vec2r &start, const Vec2r &dir, real length, real width, const Vec4r &startColor, const Vec4r &endColor, vertex_vec::iterator &vertex ) const;
		

	private:
	
		GLuint _vbo;
		ci::gl::GlslProg _shader;
		vertex_vec _vertices;
		std::vector< Vec2r > _normalizedCircle;
		
		core::util::Flicker _innerFlicker, _outerFlicker;
				
		Vec4r _colors[4];
		bool _wasFiring, _wasAiming;
		real _onOffTransition;
	
};

class CuttingBeamController : public core::InputComponent
{
	public:
	
		CuttingBeamController();
		virtual ~CuttingBeamController();
		
		CuttingBeam *cuttingBeam() const { return (CuttingBeam*) owner(); }
		bool active() const { return cuttingBeam()->active(); }
		
		virtual void update( const core::time_state &time );
		virtual bool mouseDown( const ci::app::MouseEvent &event );
		virtual bool mouseUp( const ci::app::MouseEvent &event );
		virtual void monitoredKeyDown( int keyCode );
		virtual void monitoredKeyUp( int keyCode );
		
};


} // end namespace game