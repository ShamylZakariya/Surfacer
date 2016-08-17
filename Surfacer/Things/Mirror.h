#pragma once

//
//  Mirror.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/11/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Core.h"
#include "Flicker.h"
#include "CuttingBeam.h" // for reflection bouncing code
#include "MagnetoBeam.h"

namespace game {

class MagnetoBeam;
class MirrorRotationOverlay;

class Mirror : public core::GameObject, public TowListener
{
	public:
	
		struct init : public core::GameObject::init {
			real size;
			Vec2r position;
			
			init():
				size(2),
				position(-100,-100)
			{}
			
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				core::GameObject::init::initialize(v);
				JSON_READ(v,size);
				JSON_READ(v,position);
			}

						
		};
		
	public:
	
		Mirror();
		virtual ~Mirror();
		
		JSON_INITIALIZABLE_INITIALIZE();
		
		// initialization
		virtual void initialize( const init & );
		const init &initializer() const { return _initializer; }
		
		// GameObject
		virtual void addedToLevel( core::Level *level );
		virtual void update( const core::time_state &time );
		virtual Vec2r position() const { return v2r( cpBodyGetPos(_body) ); }

		// Mirror
		cpBody *body() const { return _body; }
		cpShape *caseShape() const { return _caseShape; }
		cpShape *mirrorShape() const { return _mirrorShape; }
		cpGearJoint *rotator() const { return (cpGearJoint*) _rotationConstraint; }
		real angle() const { return cpBodyGetAngle( _body ); }
		
		// TowListener
		virtual void aboutToBeTowed( MagnetoBeam *beam );
		virtual void aboutToBeUnTowed( MagnetoBeam *beam );
		bool beingTowed() const { return _beingTowed; }
		
	private:
	
		init _initializer;
		cpBody *_body;
		cpShape *_caseShape, *_mirrorShape;
		cpConstraint *_rotationConstraint;
		bool _beingTowed;
		MirrorRotationOverlay *_rotationOverlay;

};

class MirrorRenderer : public core::DrawComponent
{
	public:
	
		MirrorRenderer();
		virtual ~MirrorRenderer();
		
		Mirror *mirror() const { return (Mirror*) owner(); }
		
	protected:

		virtual void _drawGame( const core::render_state & );
		virtual void _drawDebug( const core::render_state & );		
				
	private:

		ci::gl::GlslProg _svgShader;
		core::SvgObject _svg, _halo;
		Vec2r _svgScale;
		core::util::Flicker _flicker;
};

class MirrorRotationOverlay : public core::GameObject
{
	public:
	
		MirrorRotationOverlay( Mirror *mirror );
		virtual ~MirrorRotationOverlay();
		
		virtual void draw( const core::render_state & );

	private:

		void _renderReflectionPlane( const core::render_state & );
		void _renderBeamPath( const core::render_state & );
		
	private:
	
		struct vertex {
			Vec2r position;
			real distanceFromOrigin;
			
			vertex(){}
			vertex( const Vec2r &p, real d ):
				position(p),
				distanceFromOrigin(d)
			{}
		};
		
		typedef std::vector< vertex > vertex_vec;
	
	private:
	
		Mirror *_mirror;
		vertex_vec _planeVertices, _beamPathVertices;
		ci::gl::GlslProg _shader;		
		GLint _distanceFromOriginAttrLoc;
		GLuint _planeVbo;
		CuttingBeam::beam_segment_vec _beamPathSegments;
		CuttingBeam::raycast_query_filter _rqf;
		
};

}