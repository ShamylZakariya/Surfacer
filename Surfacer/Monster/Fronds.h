#pragma once

//
//  Fronds.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 6/6/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "GameObject.h"
#include <cinder/gl/gl.h>
#include <cinder/gl/GlslProg.h>

namespace game {

class FrondEngine : public core::GameObject
{
	public:
	
		struct init : public core::GameObject::init
		{
			ci::ColorA color;
			
			init():
				color(0,0,0,1)
			{}
					
		};
	
		struct frond_init {
			
			real length;
			real thickness;
			real density;
			real stiffness;
			int count;
			
			frond_init():
				length(1),
				thickness(0.1),
				density(0.01),
				stiffness(1),
				count(5)
			{}
			
		};

		struct frond_state {
			cpBodyVec bodies;
			cpConstraintVec constraints;
			Vec2rVec positions;
			real segmentLength;
			real radius;
		};
		
		typedef std::vector< frond_state > frond_state_vec;

	public:
	
		FrondEngine();
		virtual ~FrondEngine();
		
		// Initialization
		virtual void initialize( const init &i );
		const init &initializer() const { return _initializer; }
		
		// GameObject
		virtual void addedToLevel( core::Level *level );
		virtual void update( const core::time_state & );
		
		// FrondEngine
		/*
			Adds a frond described by @a frond to the body @a attachToBody at world coordinates @a positionWorld in direction @a dirWorld
		*/
		virtual void addFrond( const frond_init &frond, cpBody *attachToBody, const Vec2r &positionWorld, const Vec2r &dirWorld );
		
		const frond_state_vec &fronds() const { return _fronds; }
		frond_state_vec &fronds() { return _fronds; }
		
		void setColor( const ci::ColorA &color ) { _color = color; }
		ci::ColorA color() const { return _color; }
		
	protected:

		virtual void _setParent( core::GameObject *p );
		void _parentWillDestroyPhysics( core::GameObject * );
		void _cleanup();

	private:
	
		init _initializer;
		std::vector< frond_state > _fronds;
		ci::ColorA _color;
		
};

class FrondEngineRenderer : public core::DrawComponent
{
	public:
	
		FrondEngineRenderer();
		virtual ~FrondEngineRenderer();

		FrondEngine *frondEngine() const { return static_cast<FrondEngine*>(owner()); }
		
	protected:

		virtual void _drawGame( const core::render_state & );
		virtual void _drawDebug( const core::render_state & );
		
		void _tesselate( const core::render_state & );
		
	private:

		struct vertex {
			ci::Vec2f position;

			vertex()
			{}

			vertex( const ci::Vec2f &p ):
				position(p)
			{}
		};
		
		struct spline_rep {
			Vec2fVec positions;
		};
		
		typedef std::vector<vertex> vertex_vec;
		typedef std::vector<spline_rep> spline_rep_vec;

	private:
	
		spline_rep_vec _splineReps;
		vertex_vec _vertices;
		GLuint _vbo;
		ci::gl::GlslProg _shader;		
};


}