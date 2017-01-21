/*
 *  Island_generatePerimeter.cpp
 *  ConvexDecomposition2
 *
 *  Created by Shamyl Zakariya on 3/2/11.
 *  Copyright 2011 Shamyl Zakariya. All rights reserved.
 *
 */

#include "Terrain.h"
#include "MarchingSquares.h"
#include "ShapeOptimization.h"

#include <cinder/app/AppBasic.h>

namespace ms = marching_squares; 

using namespace ci;
using namespace core;

namespace game { namespace terrain {

namespace {

	class IslandVoxelSpaceAdapter
	{
		private:
		
			Island *_island;
			const OrdinalVoxelStore &_voxelsByOrdinalPosition;
			const std::vector< Voxel* > &_voxels;
			Vec2i _min, _max;
	
		public:
		
			IslandVoxelSpaceAdapter( Island *island, const OrdinalVoxelStore &voxelsByOrdinalPosition, const std::vector< Voxel* > &voxels ):
				_island(island),
				_voxelsByOrdinalPosition(voxelsByOrdinalPosition),
				_voxels(voxels)
			{
				Recti bounds = island->voxelBoundsOrdinal();
				_min.x = bounds.x1;
				_min.y = bounds.y1;
				_max.x = bounds.x2;
				_max.y = bounds.y2;			
			}

			~IslandVoxelSpaceAdapter(){}

			/**
				The origin is the minimum ordinal coordinate of the island
			*/
			Vec2 origin() const { return Vec2( _min.x, _min.y ); }
							
			/**
				To march edges correctly, we need to outset by one
			*/
			Vec2i min() const { return Vec2i( _min.x - 1, _min.y - 1 ); }

			/**
				To march edges correctly, we need to outset by one
			*/
			Vec2i max() const { return Vec2i( _max.x + 1, _max.y + 1 ); }
			
			/**
				Get the occupation value at a voxel coord. We return 0 for values outside the island, and
				we ignore Z, producing an extrusion along the Z axis.
			*/

			real valueAt( int x, int y ) const
			{
				if ( x >= _min.x && x < _max.x && y >= _min.y && y < _max.y )
				{
					Voxel *v = _voxelsByOrdinalPosition.voxelAtUnsafe( x,y );
					if ( v && v->partOfIsland( _island )) 
					{
						return v->volume();
					}
				}
			
				return 0;
			}
	};
	
	// minimum real delta from MC
	const real V_EPSILON = 1.0 / 256.0;
	const real V_SCALE = 256.0;
	
	struct PerimeterGenerator
	{
		public: 
		
			enum winding {
				CLOCKWISE,
				COUNTER_CLOCKWISE
			};
	
		private:
		
			typedef std::pair< Vec2i, Vec2i > Edge;
		
			std::map< Vec2i, Edge, Vec2iComparator > _edgesByFirstVertex;
			winding _winding;
			
		public:
		
			PerimeterGenerator( winding w ):
				_winding( w )
			{}
			
			inline void operator()( int x, int y, const ms::segment &seg )
			{
				switch( _winding )
				{
					case CLOCKWISE:
					{
						Edge e( 
							scaleUp( seg.a ), 
							scaleUp( seg.b )); 

						if ( e.first != e.second ) _edgesByFirstVertex[e.first] = e;
						break;
					}
					
					case COUNTER_CLOCKWISE:
					{
						Edge e( 
							scaleUp( seg.b ), 
							scaleUp( seg.a )); 

						if ( e.first != e.second ) _edgesByFirstVertex[e.first] = e;
						break;
					}
				}
			}
			
			/*
				Populate a vector of Vec2rVec with every perimeter computed for the isosurface
				Exterior perimeters will be in the current winding direction, interior perimeters
				will be in the opposite winding.
			*/

			int generate( std::vector< Vec2rVec > &perimeters, real scale )
			{	
				perimeters.clear();
				
				while( !_edgesByFirstVertex.empty() )
				{
					std::map< Vec2i, Edge, Vec2iComparator >::iterator 
						begin = _edgesByFirstVertex.begin(),
						end = _edgesByFirstVertex.end(),
						it = begin;
						
					int count = _edgesByFirstVertex.size();					
					perimeters.push_back( Vec2rVec() );

					do 
					{
						perimeters.back().push_back( scaleDown(it->first) * scale );
						_edgesByFirstVertex.erase(it);

						it = _edgesByFirstVertex.find(it->second.second);
						count--;
					
					} while( it != begin && it != end && count > 0 );
				}
				
				return perimeters.size();
			}
			
		private:
		
			inline Vec2i scaleUp( const Vec2 &v ) const
			{
				return Vec2i( lrintf( V_SCALE * v.x ), lrintf( V_SCALE * v.y ) );
			}
			
			inline Vec2 scaleDown( const Vec2i &v ) const
			{
				return Vec2( real(v.x) / V_SCALE, real(v.y) / V_SCALE );
			}			
	};
	
	struct collapsed_point_catcher
	{
		Vec2 lastPoint;
		collapsed_point_catcher():
			lastPoint( -10000, -10000 )
		{}
		
		inline bool operator()( const Vec2 &p )
		{
			if ( lastPoint.distanceSquared(p) < real(0.0025) ) 
			{
				return true;
			}

			lastPoint = p;
			return false;
		}
	};
	
	bool generate( IslandVoxelSpaceAdapter &voxels, 
		real scale,
		std::vector< Vec2rVec > &optimizedPerimeters )
	{
		//
		//	March and collect into PerimeterGenerator
		//

		PerimeterGenerator pgen( PerimeterGenerator::CLOCKWISE );
		ms::march( voxels, pgen, IsoLevel );
		
		// make optimization threshold track the scale of the terrain
		const real LinearDistanceOptimizationThreshold = PerimeterOptimizationLinearDistanceThreshold * scale;
		
		//
		//	Now, generate our perimeter polylines -- note that since our shapes are
		//	guaranteed closed, and since a map using Vec2iComparator will consider the 
		//	first entry to be lowest ordinally, it will represent the outer perimeter
		//

		std::vector< Vec2rVec > perimeters;
		if ( pgen.generate( perimeters, scale ) )
		{
			foreach( Vec2rVec &perimeter, perimeters )
			{
				if ( !perimeter.empty())
				{
					if ( LinearDistanceOptimizationThreshold > 0 )
					{
						optimizedPerimeters.push_back( Vec2rVec() );
						util::shape_optimization::rdpSimplify( 
							perimeter, 
							optimizedPerimeters.back(), 
							LinearDistanceOptimizationThreshold );
							
						// now filter out any runs of identical points
						optimizedPerimeters.back().erase( 
							std::remove_if( 
								optimizedPerimeters.back().begin(), 
								optimizedPerimeters.back().end(), 
								collapsed_point_catcher()), 
							optimizedPerimeters.back().end());
					}
					else
					{
						optimizedPerimeters.push_back( perimeter );
					}
				}
			}
		}
		
		//
		//	return whether we got any usable perimeters
		//

		return !optimizedPerimeters.empty() && !optimizedPerimeters.front().empty();
	}
}


bool Island::_createVoxelPerimeters()
{
	//
	//	Clean up
	//

	_voxelPerimeters.clear();

	//
	//	Create the voxel store
	//

	IslandVoxelSpaceAdapter voxels( this, *_store, _voxels );
	
	
	_usable = generate( voxels, _store->scale(), _voxelPerimeters );
	
	return _usable;
}

}} // end namespace game::terrain
