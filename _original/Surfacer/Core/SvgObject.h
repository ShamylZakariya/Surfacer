#pragma once

//
//  SvgObject.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 11/27/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#include "BlendMode.h"
#include "Common.h"
#include "RenderState.h"
#include "Transform.h"

#include "cinder/DataSource.h"
#include <cinder/Path2D.h>
#include <cinder/Rect.h>
#include <cinder/Triangulate.h>
#include <cinder/gl/Vbo.h>

namespace cinder {

	class XmlTree;

}

namespace core {

class SvgObject;

class SvgShape
{
	public:
	
		struct stroke 
		{
			std::vector< Vec2 > vertices;
			bool closed;
			GLuint vbo, vboVertexCount;
			
			stroke():
				closed(false),
				vbo(0),
				vboVertexCount(0)
			{}
			
			~stroke();
			
		};
		
		typedef std::vector< stroke > strokevec;

	public:
	
		SvgShape();
		~SvgShape();
		
		void parse( const ci::XmlTree &shapeNode );
		void draw( const render_state &, SvgObject *owner, real opacity );		
		
		void setFillColor( const ci::ColorA &color ) { _fillColor = color; _filled = _fillColor.a > ALPHA_EPSILON; }
		ci::ColorA fillColor() const { return _fillColor; }

		void setFillAlpha( real a ) { _fillColor.a = a; _filled = _fillColor.a > ALPHA_EPSILON; }
		bool fillAlpha() const { return _fillColor.a; }

		void setStrokeColor( const ci::ColorA &color ) { _strokeColor = color; _stroked = (_strokeWidth > 0 && _strokeColor.a > ALPHA_EPSILON ); }
		ci::ColorA strokeColor() const { return _strokeColor; }

		void setStrokeWidth( real w ) { _strokeWidth = std::max( w, real(0)); _stroked = (_strokeWidth > 0 && _strokeColor.a > ALPHA_EPSILON ); }
		real strokeWidth() const { return _strokeWidth; }
		
		inline bool stroked() const { return _stroked; }
		inline bool filled() const { return _filled; }
		
		void setBlendMode( const BlendMode &bm ) { _blendMode = bm; }
		const BlendMode &blendMode() const { return _blendMode; }
		
		/**
			return true if this shape represents the origin shape for the parent SvgObject
			origin shapes aren't drawn, they're used to position the origin for the parent object's shapes and children
			
			to mark a shape as origin, do any of the following
				- add a title node to the shape with the text "origin"
				- set attriubute origin="true" to the shape node
				- add class "origin" to the shape node's classes
			
		*/
		inline bool origin() const { return _origin; }
		
		// returns 'path', 'rect', etc
		std::string type() const { return _type; }
		
		// returns the shape's id
		std::string name() const { return _name; }
										
		// returns the inkscape:label attribute
		std::string label() const { return _label; }
		
		// get all the attribute pairs on this shape
		const std::map< std::string, std::string > &attributes() const { return _attributes; }
										
	private:
	
		friend class SvgObject;
		
		void _build( const ci::Shape2d &shape );
		ci::Rectf _projectToWorld( const Vec2 &documentSize, real documentScale, const Mat4 &worldTransform );		
		void _makeLocal( const Vec2 &originWorld );
		void _drawDebug( const render_state &state, SvgObject *owner, real opacity, ci::TriMesh &mesh, strokevec &strokes );
		void _drawGame( const render_state &state, SvgObject *owner, real opacity, ci::TriMesh &mesh, strokevec &strokes );
		
	private:

		bool _origin, _filled, _stroked;
		Mat4 _svgTransform;
		ci::ColorA _fillColor, _strokeColor;
		real _strokeWidth;
		std::map< std::string, std::string > _attributes;
		std::string _type, _name, _label;
		ci::Triangulator::Winding _fillRule;
		
		ci::TriMesh _svgMesh, _worldMesh, _localMesh;
		strokevec _svgStrokes, _worldStrokes, _localStrokes;
		ci::Rectf _worldBounds, _localBounds;
		
		GLuint _vertexVbo, _indexVbo;
		
		BlendMode _blendMode;
		
};

/**
	@class SvgObject
	SvgObject corresponds to an SVG group <g> element. Each group gets a transform, name, and has child SVGObjects and child SVGShapes.
*/
class SvgObject
{
	public:
	
		SvgObject(){}
		
		/**
			Load an SVG file.
			@param svgData SVG byte stream
			@param documentScale A scaling factor to apply to the SVG geometry.
		*/
		SvgObject( ci::DataSourceRef svgData, real documentScale = 1 );
		
		SvgObject( const SvgObject &copy ):_obj(copy._obj){}
		
		SvgObject &operator=( const SvgObject &rhs )
		{
			_obj = rhs._obj;
			return *this;
		}
		
	private:

		typedef std::pair< SvgObject,SvgShape*> drawable;

		struct obj
		{
			obj();
			~obj();
		
			std::weak_ptr<obj> parent;
			SvgShape *originShape;

			std::string name, label;
			real opacity;
			bool transformDirty;
			real localTransformAngle;
			Vec2 localTransformPosition, localTransformScale, documentSize;			
			Mat4 groupTransform, transform;
			BlendMode blendMode;

			std::vector< SvgShape* > shapes;
			std::map< std::string, SvgShape* > shapesByName, shapesByLabel;
			std::vector< SvgObject > children;
			std::map< std::string, SvgObject > childrenByName, childrenByLabel;
			std::map< std::string, std::string > attributes;
			std::vector< drawable > childrenToDraw;
		};

		std::shared_ptr< obj > _obj;

		SvgObject( const std::shared_ptr< obj > &objRef );

	public:

		void parse( const ci::XmlTree &svgGroupNode );
		
		const std::string &name() const { return _obj->name; }
		const std::string &label() const { return _obj->label; }
		const std::map< std::string, std::string > &attributes() const { return _obj->attributes; }
				
		const std::vector< SvgShape* > shapes() const { return _obj->shapes; }
		const std::vector< SvgObject > children() const { return _obj->children; }
		
		// get this SvgObject's parent. May be empty.
		SvgObject parent() const { return SvgObject( _obj->parent.lock() ); }

		SvgObject root() const;
		
		bool isRoot() const;

		/**
			Find the child SvgObject of this SvgObject with id=name, or inkscape:label=name
		*/
		SvgObject childNamed( const std::string &name ) const;	
			
		/**
			Find the child SvgShape of this SvgObject with id=name, or inkscape:label=name
		*/
		SvgShape* shapeNamed( const std::string &name ) const;

		/**
			Find a child SvgObject given a path, in form of path/to/child.
			If @pathToChild starts with the separator, it is considered an absolute path and is evaluated from the root SvgObject.
			find() uses the name() and label() for name comparison, trying name() first, and then label(). This is an artifact
			of Inkscape making it really hard to assign ids, but easy to assign labels to groups. 			
		*/
		SvgObject find( const std::string &pathToChild, char separator = '/' ) const;

		void draw( const render_state &state );
		
		/**
			Dumps human-readable tree to stdout
		*/
		void trace( int depth = 0 ) const;
		
		Vec2 documentSize() const;
		
		void setPosition( const Vec2 &position );
		Vec2 position() const { return _obj->localTransformPosition; }
		
		void setAngle( real a );
		real angle() const { return _obj->localTransformAngle; }
		
		void setScale( real s ) { setScale( s,s ); }
		void setScale( real xScale, real yScale ) { setScale( Vec2(xScale,yScale)); }
		void setScale( const Vec2 &scale );
		Vec2 scale() const { return _obj->localTransformScale; }
		
		/**
			Set the opacity for this SVGObject.
			When drawn, each child SVGShape will draw with its fill opacity * parent SVGObject.alpha. The alpha
			is inherited down the line by child SVGObjects.
		*/
		void setOpacity( real a ) { _obj->opacity = saturate( a ); }
		real opacity( void ) const { return _obj->opacity; }

		/**
			Set blend mode for this layer. This blend mode affects all
			children of the layer which don't specify their own blend modes.
			
			Note: default BlendMode is GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA			
		*/
		void setBlendMode( const BlendMode &bm );
		const BlendMode &blendMode() const { return _obj->blendMode; }
				
		/**
			Project a point in this SvgObject's local space to world
		*/
		Vec2 localToGlobal( const Vec2 &p );

		/**
			Project a point in world space to this object's local space
		*/
		Vec2 globalToLocal( const Vec2 &p );
		
	private:
		
		friend class SvgShape;
	
		void _draw( const render_state &state, real opacity );
		void _parseGroupAttributes( const ci::XmlTree &groupNode );
		void _loadChildrenAndShapes( const ci::XmlTree &fromNode );
		void _updateTransform();
	
		/**
			walk down tree, moving vertices of child shapes to world space, then walk back up tree transforming them to space local to their transform.
		*/
		void _normalize( const Vec2 &documentSize, real documentScale, const Mat4 &worldTransform, const Vec2 &parentWorldOrigin );
		
		/**
			get the world transform of this SvgObject
			The world transform is the concatenation of the matrices from root down to this object
		*/
		void _worldTransform( Mat4 &m );
								
  public:

	//@{
	//! Emulates shared_ptr-like behavior
	typedef std::shared_ptr< obj > SvgObject::*unspecified_bool_type;
	operator unspecified_bool_type() const { return ( _obj.get() == 0 ) ? 0 : &SvgObject::_obj; }
	void reset() { _obj.reset(); }
	//@}  	

};


}


std::ostream &operator << ( std::ostream &os, const core::BlendMode &blend );
