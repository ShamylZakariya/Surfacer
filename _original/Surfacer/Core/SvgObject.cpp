//
//  SvgObject.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 11/27/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#include "SvgObject.h"

#include "StringLib.h"
#include "SvgParsing.h"

#include <cstdlib>
#include <cinder/app/AppBasic.h>
#include <cinder/Triangulate.h>
#include <cinder/Utilities.h>

using namespace ci;
namespace core {

using namespace util;

#pragma mark - Parsing

namespace {

	std::string indent( int l )
	{
		return std::string( l, '\t' );
	}
	
	template< typename TRANSFORMER >
	void transform( 
		const ci::TriMesh2d &sourceMesh, ci::TriMesh2d &destMesh, 
		const SvgShape::strokevec &sourceStrokes, SvgShape::strokevec &destStrokes,
		const TRANSFORMER &transformer )
	{
		//
		//	transform vertices from sourceMesh into destMesh, copying over same indices
		//

		const std::vector<Vec2> &vertices = sourceMesh.getVertices();
		const std::vector<size_t> &indices = sourceMesh.getIndices();
		
		std::vector<Vec2> destVertices;
		destVertices.reserve(vertices.size());
		for( std::vector<Vec2>::const_iterator v(vertices.begin()),end(vertices.end()); v!=end; ++v )
		{
			destVertices.push_back( transformer(*v) );
		}
		
		// appendIndices takes uint32_t, not size_t! WTF!
		std::vector< uint32_t > destIndices;
		destIndices.reserve(indices.size());
		for( std::vector<size_t>::const_iterator i(indices.begin()),end(indices.end()); i!=end; ++i )
		{
			destIndices.push_back(*i);
		}
		
		//
		//	transfer new vertices and same indices over to worldMesh, and then assign worldMesh to this->mesh 
		//
		
		destMesh.clear();
		destMesh.appendVertices( &(destVertices.front()), destVertices.size() );
		destMesh.appendIndices( &(destIndices.front()), destIndices.size() );
		
			
		//
		//	transform vertices in sourceStrokes into destStrokes
		//
		
		destStrokes.clear();
		for( SvgShape::strokevec::const_iterator s(sourceStrokes.begin()),end(sourceStrokes.end()); s != end; ++s )
		{
			destStrokes.push_back( SvgShape::stroke() );
			SvgShape::stroke &back( destStrokes.back());

			back.closed = s->closed;
			for( std::vector< Vec2 >::const_iterator v(s->vertices.begin()),vEnd(s->vertices.end()); v!=vEnd; ++v )
			{
				back.vertices.push_back( transformer(*v) );
			}
		}
	}

	struct to_world_transformer
	{
		real documentScale;
		Vec2 documentSize;
		Matrix44f toWorld;
		
		to_world_transformer( const Vec2 &dSize, real dScale, const Matrix44f tw ):
			documentScale( dScale ),
			documentSize( dSize ),
			toWorld( tw )
		{}
		
		inline Vec2 operator()( const Vec2 &v ) const
		{
			Vec2 world = ( toWorld * v ) * documentScale;
			world.y = documentSize.y - world.y;
			
			return world;
		}
	};
	
	struct to_local_transformer
	{
		Vec2 localOrigin;
		to_local_transformer( const Vec2 &lo ):
			localOrigin(lo)
		{}
		
		inline Vec2 operator()( const Vec2 &v ) const
		{
			return v - localOrigin;
		}
	};
		
}

#pragma mark - SvgShape::stroke

SvgShape::stroke::~stroke()
{
	if ( vbo )
	{
		glDeleteBuffers( 1, &vbo );
	}
}

#pragma mark - SvgShape

/*
		bool _origin, _filled, _stroked;
		Mat4 _svgTransform;
		ci::ColorA _fillColor, _strokeColor;
		real _strokeWidth;
		std::map< std::string, std::string > _attributes;
		std::string _type, _name, _label;
		ci::Triangulator::Winding _fillRule;
		
		ci::TriMesh2d _svgMesh, _worldMesh, _localMesh;
		strokevec _svgStrokes, _worldStrokes, _localStrokes;
		ci::Rectf _worldBounds, _localBounds;
		
		GLuint _vertexVbo, _indexVbo;
*/

SvgShape::SvgShape():
	_origin(false),
	_filled(false),
	_stroked(false),
	_fillColor(0,0,0,1),
	_strokeColor(0,0,0,1),
	_strokeWidth(0),
	_fillRule( Triangulator::WINDING_NONZERO ),
	_vertexVbo(0),
	_indexVbo(0)
{}

SvgShape::~SvgShape()
{
	if ( _vertexVbo )
	{
		glDeleteBuffers( 1, &_vertexVbo );
	}
	
	if ( _indexVbo )
	{
		glDeleteBuffers( 1, &_indexVbo );
	}
}

void SvgShape::parse( const const ci::XmlTree &shapeNode )
{
	_type = shapeNode.getTag();
	
	if ( shapeNode.hasAttribute( "id" ))
	{
		_name = shapeNode.getAttribute( "id" ).getValue();
	}
	
	if ( shapeNode.hasAttribute( "inkscape:label" ))
	{
		_label = shapeNode.getAttribute( "inkscape:label" ).getValue();
	}
	
	//
	//	Cache all attributes, in case we want to query them later
	//

	foreach( const XmlTree::Attr &attr, shapeNode.getAttributes() )
	{
		_attributes[ attr.getName() ] = attr.getValue();
	}	

	//
	//	Parse style declarations
	//
		
	{
		svg::svg_style style = svg::parseStyle( shapeNode );

		_filled = style.filled;
		_stroked = style.stroked;
		_fillColor = ColorA( style.fillColor, style.fillOpacity * style.opacity );
		_strokeColor = ColorA( style.strokeColor, style.strokeOpacity * style.opacity );
		_strokeWidth = style.strokeWidth;
		_fillRule = style.fillRule;
	}
	
	//
	//	Get the transform
	//

	if ( shapeNode.hasAttribute( "transform" ))
	{
		_svgTransform = svg::parseTransform( shapeNode.getAttribute( "transform" ).getValue() );
	}
	
	//
	//	determine if this shape is the origin shape for its parent SvgObject
	//	

	if ( shapeNode.hasChild( "title" ))
	{
		std::string title = shapeNode.getChild( "title" ).getValue();
		title = stringlib::strip( stringlib::lowercase( title ));
		if ( title == "origin" ) _origin = true;
	}
	
	if ( shapeNode.hasAttribute( "origin" ))
	{
		_origin = true;
	}
	
	if ( shapeNode.hasAttribute( "class" ))
	{
		std::string classes = stringlib::lowercase(shapeNode.getAttribute( "class" ).getValue());
		if ( classes.find( "origin" ) != std::string::npos )
		{
			_origin = true;
		}
	}
	
	// any name starting with "origin" makes it an origin node
	if ( name().find( "origin" ) == 0 || label() == "origin" )
	{
		_origin = true;
	}
	

	//
	//	parse svg shape into a ci::Shape2d, and then convert that to _svgMesh in document space.
	//	later, in SvgObject::_normalize we'll project to world space, and then to local space.
	//

	Shape2d shape2d;
	svg::parseShape( shapeNode, shape2d );	
	_build( shape2d );
}

void SvgShape::draw( const render_state &state, SvgObject *owner, real opacity )
{
	switch( state.mode )
	{
		case RenderMode::GAME:
		case RenderMode::DEVELOPMENT:
			if ( !origin() ) _drawGame(state, owner, opacity, _localMesh, _localStrokes );
			break;
			
		case RenderMode::DEBUG:
			_drawDebug(state, owner, opacity, _localMesh, _localStrokes );
			break;
			
		case RenderMode::COUNT:
			break;
	}
}

void SvgShape::_build( const ci::Shape2d &shape )
{
	_svgMesh.clear();
	_worldMesh.clear();
	_localMesh.clear();
	_svgStrokes.clear();
	_worldStrokes.clear();
	_localStrokes.clear();
	
	const real ApproximationScale = 0.5;

	//
	//	we only need to triangulate filled shapes
	//

	if ( filled() )
	{
		_svgMesh = Triangulator(shape, ApproximationScale).calcMesh( _fillRule );
	}

	//
	// we only need to gather the perimeter vertices if our shape is stroked
	//

	if ( stroked() )
	{
		foreach( const Path2d &path, shape.getContours() )
		{
			_svgStrokes.push_back( stroke() );
			_svgStrokes.back().closed = path.isClosed();
			_svgStrokes.back().vertices = path.subdivide( ApproximationScale );
		}
	}
}

Rectf SvgShape::_projectToWorld( const Vec2 &documentSize, real documentScale, const Mat4 &worldTransform )
{
	Mat4 toWorld = worldTransform * _svgTransform;
	transform( _svgMesh, _worldMesh, _svgStrokes, _worldStrokes, to_world_transformer( documentSize, documentScale, toWorld ));

	_worldBounds = _worldMesh.calcBoundingBox();
	return _worldBounds;
}

void SvgShape::_makeLocal( const ci::Vec2 &originWorld )
{
	transform( _worldMesh, _localMesh, _worldStrokes, _localStrokes, to_local_transformer( originWorld ));

	// free up memory we're not using anymore
	_worldMesh.clear();
	_svgMesh.clear();
	_worldStrokes.clear();
	_svgStrokes.clear();
}

void SvgShape::_drawDebug( const render_state &state, SvgObject *owner, real opacity, ci::TriMesh2d &mesh, strokevec &strokes )
{
	if ( origin() )
	{
		Mat4 worldTransform;
		owner->_worldTransform( worldTransform );
		
		real screenSize = 20;
		Vec2 xAxis( worldTransform.getColumn(0).xy()),
		      yAxis( worldTransform.getColumn(1).xy());
		
		Vec2 scale( screenSize / xAxis.length(), screenSize / yAxis.length());
		scale *= state.viewport.reciprocalZoom();

		gl::color(1,0,0);
		gl::drawLine( Vec2(0,0), Vec2(scale.x,0));
		gl::color(0.25,0,0);
		gl::drawLine( Vec2(0,0), Vec2(-scale.x,0));

		gl::color(0,1,0);
		gl::drawLine( Vec2(0,0), Vec2(0,scale.y));
		gl::color(0,0.25,0);
		gl::drawLine( Vec2(0,0), Vec2(0,-scale.y));
	}
	else
	{
		if ( filled() )
		{
			const std::vector<size_t> &indices( mesh.getIndices() );
			const std::vector<Vec2> &vertices( mesh.getVertices() );
			const size_t numTriangles = mesh.getNumTriangles();
			
			//
			//	draw triangles twice, once will a low-alpha fill and once with full opacity wireframe
			//

			ci::ColorA c(fillColor());		
			gl::color( ColorA( c.r, c.g, c.b, c.a * 0.3 * opacity ));
			glBegin( GL_TRIANGLES );
			for ( size_t triIndex = 0; triIndex < numTriangles; triIndex++ )
			{
				gl::vertex( vertices[indices[triIndex*3+0]] );
				gl::vertex( vertices[indices[triIndex*3+1]] );
				gl::vertex( vertices[indices[triIndex*3+2]] );
			}
			glEnd();

			
			gl::color( ColorA( c.r, c.g, c.b, opacity ));
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
			glBegin( GL_TRIANGLES );
			for ( size_t triIndex = 0; triIndex < numTriangles; triIndex++ )
			{
				gl::vertex( vertices[indices[triIndex*3+0]] );
				gl::vertex( vertices[indices[triIndex*3+1]] );
				gl::vertex( vertices[indices[triIndex*3+2]] );
			}
			glEnd();
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}

		if ( stroked() )
		{	
			ci::ColorA sc = strokeColor();
			gl::color( sc.r, sc.g, sc.b, sc.a * opacity );
			foreach( const stroke &s, strokes )
			{
				glBegin( s.closed ? GL_LINE_LOOP : GL_LINE_STRIP );

				for( std::vector< ci::Vec2 >::const_iterator v(s.vertices.begin()),end(s.vertices.end()); v!=end; ++v )
				{
					gl::vertex( *v );
				}
				
				glEnd();	
			}
		}
	}
}

void SvgShape::_drawGame( const render_state &state, SvgObject *owner, real opacity, ci::TriMesh2d &mesh, strokevec &strokes )
{
	//
	//	render solid fill
	//
	
	if ( filled() )
	{
		ci::ColorA fc = fillColor();
		gl::color( fc.r, fc.g, fc.b, fc.a * opacity );

		const std::vector< Vec2 > &vertices( mesh.getVertices() );
		const std::vector< size_t > &indices( mesh.getIndices() );

		if ( !_vertexVbo )
		{
			glGenBuffers( 1, &_vertexVbo );
			glBindBuffer( GL_ARRAY_BUFFER, _vertexVbo );
			glBufferData( GL_ARRAY_BUFFER, 
						  vertices.size() * sizeof( Vec2 ), 
						  &( vertices.front() ),
						  GL_STATIC_DRAW );
		}
		
		if ( !_indexVbo )
		{
			glGenBuffers( 1, &_indexVbo );
			glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, _indexVbo );
			glBufferData( GL_ELEMENT_ARRAY_BUFFER, 
						  indices.size() * sizeof( size_t ), 
						  &(indices.front()),
						  GL_STATIC_DRAW );
		}
		
		glBindBuffer( GL_ARRAY_BUFFER, _vertexVbo );
		glEnableClientState( GL_VERTEX_ARRAY );
		glVertexPointer( 2, GL_REAL, 0, NULL );
		
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, _indexVbo );
		glDrawElements( GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, NULL );

		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
		glBindBuffer( GL_ARRAY_BUFFER, 0 );
		
		glDisableClientState( GL_VERTEX_ARRAY );
	}

	//
	//	render strokes
	//

	if ( stroked() )
	{
		ci::ColorA sc = strokeColor();
		gl::color( sc.r, sc.g, sc.b, sc.a * opacity );

		glLineWidth(1);

		for( strokevec::iterator stroke(strokes.begin()),end(strokes.end()); stroke != end; ++stroke )
		{
			if ( !stroke->vbo )
			{
				glGenBuffers( 1, &stroke->vbo );
				glBindBuffer( GL_ARRAY_BUFFER, stroke->vbo );
				glBufferData( GL_ARRAY_BUFFER, 
							  stroke->vertices.size() * sizeof( Vec2 ), 
							  &( stroke->vertices.front() ),
							  GL_STATIC_DRAW );
							  
				stroke->vboVertexCount = stroke->vertices.size();
			}
			
			glBindBuffer( GL_ARRAY_BUFFER, stroke->vbo );
			glEnableClientState( GL_VERTEX_ARRAY );
			glVertexPointer( 2, GL_REAL, 0, NULL );
			
			glDrawArrays( stroke->closed ? GL_LINE_LOOP : GL_LINE_STRIP, 0, stroke->vboVertexCount );
			
			glBindBuffer( GL_ARRAY_BUFFER, 0 );
			glDisableClientState( GL_VERTEX_ARRAY );			
		}
	}
}

#pragma mark - SvgObject

/*
			std::weak_ptr<obj> parent;
			SvgShape *originShape;

			std::string name;
			real opacity;
			bool transformDirty;
			real localTransformAngle;
			Vec2 localTransformPosition, localTransformScale, documentSize;			
			Mat4 groupTransform, transform;

			std::vector< SvgShape* > shapes;
			std::map< std::string, SvgShape* > shapesByName;
			std::vector< SvgObject > children;
			std::map< std::string, SvgObject > childrenByName;
			std::map< std::string, std::string > attributes;
			std::vector< drawable > childrenToDraw;
*/

SvgObject::obj::obj():
	originShape(NULL),
	opacity(1),
	transformDirty(true),
	localTransformAngle(0),
	localTransformPosition(0,0),
	localTransformScale(1,1),
	documentSize(0,0)
{}

SvgObject::obj::~obj()
{
	originShape = NULL;

	foreach( SvgShape *shape, shapes )
	{
		delete shape;
	}
	
	shapes.clear();
	shapesByName.clear();
	children.clear();
	childrenByName.clear();
	attributes.clear();
	childrenToDraw.clear();
}
		
SvgObject::SvgObject( ci::DataSourceRef svgFile, real scale )
{
	//
	//	We're reading a document, not a <g> group; but we want to load attributes and children and (if any,) shapes
	//

	XmlTree svgDoc = XmlTree( svgFile ).getChild( "svg" );

	parse( svgDoc );

	_obj->documentSize.x = svg::parseNumericAttribute( svgDoc.getAttribute("width").getValue()) * scale;
	_obj->documentSize.y = svg::parseNumericAttribute( svgDoc.getAttribute("height").getValue()) * scale;

	//
	//	Now normalize our generated geometry
	//

	_normalize( _obj->documentSize, scale, Mat4(), Vec2(0,0) );
}

SvgObject::SvgObject( const std::shared_ptr< obj > &objRef ):
	_obj(objRef)
{}

void SvgObject::parse( const ci::XmlTree &svgGroupNode )
{
	if ( !_obj )
	{
		_obj.reset( new obj() );
	}

	_parseGroupAttributes( svgGroupNode );
	_loadChildrenAndShapes( svgGroupNode );
}

SvgObject SvgObject::find( const std::string &pathToChild, char separator ) const
{
	if ( pathToChild.at(0) == separator )
	{
		return root().find( pathToChild.substr(1), separator );
	}

	SvgObject node = *this;
	stringlib::stringvec path = stringlib::split( pathToChild, separator );
	
	for( stringlib::stringvec::const_iterator childName(path.begin()),end(path.end()); childName != end; ++childName )
	{
		node = node.childNamed( *childName );
		
		//
		//	if the node is empty, return it as a failure. if we're on the last element, return the node
		//
		
		if ( !node || ((childName+1) == end) ) return node;
	}
	
	return SvgObject();
}

void SvgObject::draw( const render_state &state )
{
	_draw( state, 1 );
}

void SvgObject::trace( int depth ) const
{
	std::string 
		ind = indent(depth),
		ind2 = indent( depth+1 );
	
	app::console() 
		<< ind << "[SvgObject name: " << name() << std::endl
		<< ind << " +shapes:" << std::endl;
		
	foreach( SvgShape *shape, shapes() )
	{
		app::console() << ind2 << "[SvgShape name: " << shape->name() 
			<< " type: " << shape->type() 
			<< " origin: " << str(shape->origin())
			<< " filled: " << str(shape->filled()) << " stroked: " << str(shape->stroked())
			<< " fillColor: " << shape->fillColor() 
			<< " strokeColor: " << shape->strokeColor()
			<< "]" << std::endl;
	}
	
	app::console() << ind << " +children:" << std::endl;
	foreach( const SvgObject &obj, children() )
	{
		obj.trace( depth + 1 );
	}
	
	app::console() << ind << "]" << std::endl;		
}

SvgObject SvgObject::root() const
{
	SvgObject r = *this;

	while( r.parent() )
	{
		r = r.parent();
	}
	
	return r;
}

bool SvgObject::isRoot() const
{
	return !parent();
}

SvgObject SvgObject::childNamed( const std::string &name ) const
{
	std::map< std::string, SvgObject >::const_iterator pos( _obj->childrenByName.find( name ));
	if ( pos != _obj->childrenByName.end() ) return pos->second;
	
	pos = _obj->childrenByLabel.find( name );
	if ( pos != _obj->childrenByLabel.end() ) return pos->second;
	
	return SvgObject();
}

SvgShape* SvgObject::shapeNamed( const std::string &name ) const
{
	std::map< std::string, SvgShape* >::const_iterator pos( _obj->shapesByName.find( name ));
	if ( pos != _obj->shapesByName.end() ) return pos->second;

	pos = _obj->shapesByLabel.find( name );
	if ( pos != _obj->shapesByLabel.end() ) return pos->second;

	return NULL;
}


Vec2 SvgObject::documentSize() const
{
	return root()._obj->documentSize;
}

void SvgObject::setPosition( const Vec2 &position )
{
	_obj->localTransformPosition = position;
	_obj->transformDirty = true;
}
		
void SvgObject::setAngle( real a )
{
	_obj->localTransformAngle = a;
	_obj->transformDirty = true;
}

void SvgObject::setScale( const Vec2 &scale )
{
	_obj->localTransformScale = scale;
	_obj->transformDirty = true;
}

void SvgObject::setBlendMode( const BlendMode &bm )
{
	_obj->blendMode = bm;
}

Vec2 SvgObject::localToGlobal( const Vec2 &p )
{
	Mat4 worldTransform;
	_worldTransform( worldTransform );
	
	return worldTransform * p;
}

Vec2 SvgObject::globalToLocal( const Vec2 &p )
{
	Mat4 worldTransform;
	_worldTransform( worldTransform );
	
	return worldTransform.affineInverted() * p;
}

void SvgObject::_draw( const render_state &state, real parentOpacity )
{
	real opacity = _obj->opacity * parentOpacity;

	BlendMode lastBlendMode = this->blendMode();

	glEnable( GL_BLEND );
	lastBlendMode.bind();

	_updateTransform();

	gl::pushModelView();
	gl::multModelView( _obj->transform );
	
	for( std::vector< drawable >::iterator c(_obj->childrenToDraw.begin()),end(_obj->childrenToDraw.end()); c!=end; ++c )
	{
		if ( c->first ) 
		{
			//
			//	Draw child layer
			//	

			c->first._draw( state, opacity );
		}
		else if ( c->second ) 
		{
			//
			//	Draw child shape with its blend mode if one was set, or ours otherwise
			//
			
			if ( c->second->blendMode() != lastBlendMode )
			{
				lastBlendMode = c->second->blendMode();
				lastBlendMode.bind();
			}

			c->second->draw( state, this, opacity );
		}
	}
	
	gl::popModelView();
}

void SvgObject::_parseGroupAttributes( const ci::XmlTree &groupNode )
{
	if ( groupNode.hasAttribute( "id" ))
	{
		_obj->name = groupNode.getAttribute( "id" ).getValue();
	}

	if ( groupNode.hasAttribute( "inkscape:label" ))
	{
		_obj->label = groupNode.getAttribute( "inkscape:label" ).getValue();
	}
	
	if ( groupNode.hasAttribute( "transform" ))
	{
		_obj->groupTransform = svg::parseTransform( groupNode.getAttribute( "transform" ).getValue() );
	}
	
	{
		svg::svg_style style = svg::parseStyle( groupNode );
		_obj->opacity = style.opacity;
	}
		
	//
	//	Now cache all attributes, in case we want to query them later
	//

	foreach( const XmlTree::Attr &attr, groupNode.getAttributes() )
	{
		_obj->attributes[ attr.getName() ] = attr.getValue();
	}	
}

void SvgObject::_loadChildrenAndShapes( const ci::XmlTree &fromNode )
{
	for ( XmlTree::ConstIter childNode = fromNode.begin(); childNode != fromNode.end(); ++childNode )
	{
		const std::string tag = childNode->getTag();
		
		if ( tag == "g" )
		{
			SvgObject child;
			child.parse( *childNode );
			child._obj->parent = _obj;
			
			_obj->children.push_back( child );
			_obj->childrenByName[ child.name() ] = child;
			if ( !child.label().empty() ) _obj->childrenByLabel[ child.label() ] = child;

			_obj->childrenToDraw.push_back( drawable( child, NULL) );
		}
		else if ( svg::canParseShape( tag ))
		{
			SvgShape *shape = new SvgShape();
			shape->parse( *childNode );
			_obj->shapes.push_back( shape );
			_obj->shapesByName[ shape->name() ] = shape;
			if ( !shape->label().empty() ) _obj->shapesByLabel[ shape->label() ] = shape;
			
			if ( shape->origin() )
			{
				_obj->originShape = shape;
			}
			
			_obj->childrenToDraw.push_back( drawable( SvgObject(), shape ));
		}
	}		
}

void SvgObject::_updateTransform()
{
	if ( _obj->transformDirty )
	{
		_obj->transformDirty = false;

		Mat4 
			S = Mat4::createScale( _obj->localTransformScale ),
			T = Mat4::createTranslation( Vec3( _obj->localTransformPosition )),
			R;

		const real
			c = std::cos(_obj->localTransformAngle),
			s = std::sin(_obj->localTransformAngle);

		R.m[0] = c; R.m[4] = -s; R.m[8 ] = 0;  R.m[12] = 0;
		R.m[1] = s; R.m[5] = c;  R.m[9 ] = 0;  R.m[13] = 0;
		R.m[2] = 0; R.m[6] = 0;  R.m[10] = 1;  R.m[14] = 0;
		R.m[3] = 0; R.m[7] = 0;  R.m[11] = 0;  R.m[15] = 1;

		_obj->transform = _obj->groupTransform * ( T * S * R );
	}
}

void SvgObject::_normalize( const Vec2 &documentSize, real documentScale, const Mat4 &worldTransform, const Vec2 &parentWorldOrigin )
{
	//
	//	Project shapes to world, and gather their collective bounds in world space
	//

	Mat4 toWorld = worldTransform * _obj->groupTransform;

	Rectf worldBounds(
		+std::numeric_limits<real>::max(),
		+std::numeric_limits<real>::max(),
		-std::numeric_limits<real>::max(),
		-std::numeric_limits<real>::max()
	);

	if ( !_obj->shapes.empty() )
	{
		for ( std::vector< SvgShape* >::iterator shape(_obj->shapes.begin()),end(_obj->shapes.end()); shape != end; ++shape )
		{
			worldBounds.include((*shape)->_projectToWorld( documentSize, documentScale, toWorld ));
		}
	}
	else
	{
		worldBounds.set(0, 0, documentSize.x, documentSize.y );
	}
	
	//
	//	determine the origin of this object in world coordinates. 
	//	if an 'origin' object was specified, use the center of its world bounds.
	//	otherwise use the center of the bounds of all our shapes.
	//

	Vec2 worldOrigin = worldBounds.getCenter();
	
	if ( _obj->originShape )
	{
		worldOrigin = _obj->originShape->_worldBounds.getCenter();
	}
	
	//
	//	Now make the shapes geometry local to our worldOrigin
	//

	for ( std::vector< SvgShape* >::iterator shape(_obj->shapes.begin()),end(_obj->shapes.end()); shape != end; ++shape )
	{
		(*shape)->_makeLocal( worldOrigin );
	}

	//
	//	now set our transform to make our world origin local to our parent's world origin
	//	

	_obj->groupTransform = Mat4::createTranslation( Vec3( worldOrigin - parentWorldOrigin, 0 ));
						
	//
	//	finally pass on to children
	//

	for ( std::vector< SvgObject >::iterator child(_obj->children.begin()),end(_obj->children.end()); child != end; ++child )
	{
		child->_normalize( documentSize, documentScale, toWorld, worldOrigin );
	}	
	
	//
	//	Now, finally, the root object should move its group transform to local, and make
	//	the group transform identity. This is because nobody 'owns' the root object, so
	//	its transform is inherently local.
	//
	
	if ( isRoot() )
	{
		_obj->localTransformPosition = worldOrigin - parentWorldOrigin;
		_obj->groupTransform = Mat4::identity();
	}
}

void SvgObject::_worldTransform( Mat4 &m )
{
	if ( parent() ) parent()._worldTransform( m );

	_updateTransform();
	m = m * _obj->transform;
}

}

namespace {

	std::string blendFactorName( GLenum f )
	{
		switch( f )
		{
			case GL_DST_ALPHA: return "GL_DST_ALPHA";  
			case GL_DST_COLOR: return "GL_DST_COLOR";
			case GL_ONE: return "GL_ONE";
			case GL_ONE_MINUS_DST_ALPHA: return "GL_ONE_MINUS_DST_ALPHA";
			case GL_ONE_MINUS_DST_COLOR: return "GL_ONE_MINUS_DST_COLOR";
			case GL_ONE_MINUS_SRC_ALPHA: return "GL_ONE_MINUS_SRC_ALPHA";
			case GL_ONE_MINUS_SRC_COLOR: return "GL_ONE_MINUS_SRC_COLOR"; 
			case GL_SRC_ALPHA: return "GL_SRC_ALPHA";
			case GL_SRC_ALPHA_SATURATE: return "GL_SRC_ALPHA_SATURATE";
			case GL_SRC_COLOR: return "GL_SRC_COLOR";
			case GL_ZERO: return "GL_ZERO";
			
		}
		
		return "[" + str(f) + " is not a blend factor]";
	}

}

std::ostream &operator << ( std::ostream &os, const core::BlendMode &blend )
{
	return os << "[BlendMode src: " + blendFactorName(blend.src()) + " : " + blendFactorName(blend.dst()) + "]";
}

