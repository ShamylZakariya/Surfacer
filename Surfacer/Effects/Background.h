#pragma once

//
//  Background.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 7/8/11.
//  Copyright 2011 Shamyl Zakariya. All rights reserved.
//

#include "Core.h"

#include <cinder/Surface.h>
#include <cinder/gl/GlslProg.h>
#include <cinder/gl/Texture.h>
#include <cinder/gl/Vbo.h>

namespace game {

class Background : public core::GameObject
{
	public:
	
		struct layer : public core::util::JsonInitializable {
			ci::fs::path texture;
			real scale;
			ci::ColorA color; 
			
			layer():
				scale(1),
				color(1,1,1,1)
			{}
			
			layer( const ci::fs::path &t, real s, const ci::ColorA &c ):
				texture(t),
				scale(std::max(s, real(1))),
				color(c)
			{}

			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				JSON_READ(v, texture);
				JSON_READ(v, scale);
				JSON_READ(v, color);
			}
			
		};
	
		struct init : public core::GameObject::init {
			
			ci::Color clearColor;
			std::vector< layer > layers;
			int sectorSize;
			
			init():
				clearColor(0,0,0),
				sectorSize(64)
			{}
			
			void addLayer( const ci::fs::path &p, real s, const ci::ColorA &c = ci::ColorA(1,1,1,1) );
			
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				GameObject::init::initialize(v);

				JSON_READ(v, clearColor);
				JSON_READ(v, sectorSize);
				
				const ci::JsonTree Layers = v["layers"];
				for ( ci::JsonTree::ConstIter layer(Layers.begin()),end(Layers.end()); layer != end; ++layer )
				{
					Background::layer layerInit;
					layerInit.initialize( *layer );
					
					layers.push_back( layerInit );
				}
			}
			
		};

	public:
	
		Background();
		virtual ~Background();

		JSON_INITIALIZABLE_INITIALIZE();
		
		const init &initializer() const { return _initializer; }		
		void initialize( const init & );
		
	protected:
	
		friend class BackgroundRenderer;	

		init _initializer;
};

class Foreground : public Background
{
	public:

		Foreground();
		virtual ~Foreground();
	
};

class BackgroundRenderer : public core::DrawComponent
{
	public:

		BackgroundRenderer();
		virtual ~BackgroundRenderer();
		
		void draw( const core::render_state &state );

	protected:
	
		void _drawParallaxTexturedBackground( const core::render_state &state, const ci::Rectf &viewportWorld );
		void _drawSectorOverlay( const core::render_state &state, const ci::Rectf &viewportWorld );
	
		ci::gl::GlslProg _shader;
		ci::Vec3f _vertices[4];
		std::vector< ci::gl::Texture > _layerTextures;
		
};

} // end namespace game