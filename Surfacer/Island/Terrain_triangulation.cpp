//
//  Terrain_triangulation.cpp
//  Surfacer
//
//  Created by Zakariya Shamyl on 10/4/11.
//  Copyright 2011 TomorrowPlusX. All rights reserved.
//

#include "Terrain.h"
#include "TerrainRendering.h"

#include <cinder/app/App.h>

#define TRIANGULATE_POLY_2_TRI 0

#if TRIANGULATE_POLY_2_TRI
	#include <poly2tri.h>
#else
	#include <cinder/Triangulate.h>
#endif

using namespace ci;
using namespace core;
namespace game { namespace terrain {

namespace {
  
	#if TRIANGULATE_POLY_2_TRI
		inline real Area( const p2t::Point &a, const p2t::Point &b, const p2t::Point &c )
		{
			real sa = ( a - b ).Length(),
				sb = ( b - c ).Length(),
				sc = ( c - a ).Length(),
				s( (sa + sb + sc ) * 0.5f );
			
			return std::sqrt( s * ( s - sa ) * ( s - sb ) * ( s - sc ));
		}
	#else
		inline real Area( const Vec2r &a, const Vec2r &b, const Vec2r &c )
		{
			real sa = a.distance(b),
				sb = b.distance(c),
				sc = c.distance(a),
				s( (sa + sb + sc ) * 0.5f );
			
			return std::sqrt( s * ( s - sa ) * ( s - sb ) * ( s - sc ));
		}
	#endif

	
	inline Vec2r InvertY( const Vec2r &v )
	{
		return Vec2r( v.x, real(1) - v.y );
	}
		
}


bool Island::triangulate( Vec2r const *ordinalToCentroidRelativeOffset )
{
	if ( _createVoxelPerimeters() && _triangulate(ordinalToCentroidRelativeOffset))
	{
		if ( !group()->terrain()->initializer().greebleTextureAtlas.empty() )
		{
			_createPerimeterGreebling(ordinalToCentroidRelativeOffset);
		}

		return true;
	}

	return false;
}


bool Island::_triangulate( Vec2r const *ordinalToCentroidRelativeOffset )
{
	const Vec2r offset = ordinalToCentroidRelativeOffset ? *ordinalToCentroidRelativeOffset : Vec2r(0,0);

	//
	//	Clear any previous triangulation and renderer's FBOs, if any
	//

	_renderer->reset();
	_triangulation.clear();

	//
	//	Triangulate the perimeter
	//

	if ( !_voxelPerimeters.empty() )
	{
		_triangulatePerimeters( _voxelPerimeters, offset, _triangulation );
	}

	//
	//	If we didn't get any triangles, then we're not usable
	//

	_usable = !_triangulation.empty();
	return _usable;
}

#if TRIANGULATE_POLY_2_TRI

void Island::_triangulatePerimeters( 
	const std::vector< Vec2rVec > &ordinalSpacePerimeters, 
	const Vec2r &offset,
	std::vector< triangle > &triangulation )
{
	//
	//	The marching squares algorithm can (and often does ) produce multiple perimeters for a single island.
	//	The trouble is, in some cases some of the perimeters are holes, and in some cases, some of the perimeters
	//	are also outward facing islands. This is counter-intuitive, but it happens when an island narrows
	//	to a very thin single pixel wide strip, which falls below the isosurface threshold. This is a legit
	//	case, not a bug, but as such, we must treat all perimeters as non-holes until I can think of a way
	//	to detect holes...
	//

	typedef std::vector< p2t::Point* > PolyLine;
	typedef std::vector< PolyLine > PolyLineVec;

	PolyLineVec polylines;	
	foreach( const Vec2rVec &osp, ordinalSpacePerimeters )
	{
		if ( osp.size() > 2 )
		{
			polylines.push_back( PolyLine() );
			polylines.back().reserve(osp.size());
			
			foreach( const Vec2r &v, osp )
			{
				polylines.back().push_back( new p2t::Point( offset.x + v.x, offset.y + v.y ));
			}
		}
	}
	
	//
	//	When computing texture coordinates, we will divide the ordinal vertex position by the total world size
	//

	real terrainScale = _store->scale();
	const Vec2r TotalOrdinalSizeReciprocal( real(1) / real( _store->width() * terrainScale ), real(1) / real(_store->height() * terrainScale) );

	//
	//	Now, for each polyline, triangulate
	//

	foreach( PolyLine &polyline, polylines )
	{
		if ( !polyline.empty() )
		{
			p2t::CDT cdt( polyline );
			
			cdt.Triangulate();
			std::vector< p2t::Triangle* > triangles = cdt.GetTriangles();
			
			foreach( p2t::Triangle* t, triangles )
			{
				p2t::Point *a = t->GetPoint(0),
				*b = t->GetPoint(1), 
				*c = t->GetPoint(2);
				
				if ( Area( *a, *b, *c ) > Epsilon )
				{
					//
					//	add a triangle, note that reversed winding of triangles is makes chipmunk happy.
					//	To compute tex coords remove center-of-mass offset to return vertex to ordinal space, and then
					//	divide by the size of the level. This is because we use a single shared texture for all islands.
					//

					triangulation.push_back(triangle(
						Vec2r( c->x, c->y ), InvertY((Vec2r( c->x, c->y ) - offset ) * TotalOrdinalSizeReciprocal),
						Vec2r( b->x, b->y ), InvertY((Vec2r( b->x, b->y ) - offset ) * TotalOrdinalSizeReciprocal),
						Vec2r( a->x, a->y ), InvertY((Vec2r( a->x, a->y ) - offset ) * TotalOrdinalSizeReciprocal)
					));
				}
			}
			
			foreach( p2t::Point *point, polyline ) 
			{
				delete point;
			}
		}
	}
}

#else


void Island::_triangulatePerimeters( 
	const std::vector< Vec2rVec > &ordinalSpacePerimeters, 
	const Vec2r &offset,
	std::vector< triangle > &triangulation )
{
	//
	//	cinder's triangulator uses an approximation scale for quality... not certain what that means on the inside
	//	It defaults to 1, so I'm setting it to the voxel scale, so it will at least follow level size scaling.
	//

	const float TriangulatorApproximationScale = _store->scale();

	//
	//	Populate the triangulator with paths
	//

	Triangulator triangulator;	
	foreach( const Vec2rVec &osp, ordinalSpacePerimeters )
	{
		if ( osp.size() > 2 )
		{
			Path2d path;
			path.moveTo( osp.front() + offset );

			for( Vec2rVec::const_iterator it( osp.begin() + 1 ), end( osp.end()); it != end; ++it )
			{
				path.lineTo( *it + offset );
			}

			path.close();
			triangulator.addPath( path, TriangulatorApproximationScale );
		}
	}

	//
	//	Triangulate
	//

	TriMesh2d mesh( triangulator.calcMesh(Triangulator::WINDING_POSITIVE));

	//
	//	When computing texture coordinates, we will divide the ordinal vertex position by the total world size
	//

	real terrainScale = _store->scale();
	const Vec2r TotalOrdinalSizeReciprocal( real(1) / real( _store->width() * terrainScale ), real(1) / real(_store->height() * terrainScale) );

	for ( std::size_t i = 0, N = mesh.getNumTriangles(); i < N; i++ )
	{
		ci::Vec2f a,b,c;
		mesh.getTriangleVertices( i, &a, &b, &c );
	
		real area = Area( a,b,c );
		if ( area > Epsilon )
		{
			//
			//	To compute tex coords remove center-of-mass offset to return vertex to ordinal space, and then
			//	divide by the size of the level. This is because we use a single shared texture for all islands.
			//

			triangulation.push_back(triangle(
				a, InvertY((a - offset ) * TotalOrdinalSizeReciprocal),
				b, InvertY((b - offset ) * TotalOrdinalSizeReciprocal),
				c, InvertY((c - offset ) * TotalOrdinalSizeReciprocal)
			));			
		}
	}
}

#endif


}} // end namespace core::terrain
