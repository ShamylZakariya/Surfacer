#pragma once

//
//  Voxel.h
//  Surfacer
//
//  Created by Zakariya Shamyl on 9/28/11.
//  Copyright 2011 TomorrowPlusX. All rights reserved.
//


#include "Common.h"

namespace game { namespace terrain {

#pragma mark -
#pragma mark Constants

/**
	The minimum voxel occupation factor to consider a voxel usable as part of an island.
*/
const static int MinimumVoxelOccupation = 96;

/**
	The iso level to use in the marching squares tesselator
*/
const static real IsoLevel = 0.5;

/**
	Distance threshold used when running simplification of generated terrain perimeter
*/
const real PerimeterOptimizationLinearDistanceThreshold = 0.25;

const real IslandAABBPadding = 0.5;



#pragma mark -
#pragma mark Compass

namespace Compass {
	enum Direction {
		North = 0,
		NorthWest,
		West,
		SouthWest,
		South,
		SouthEast,
		East,
		NorthEast
	};
	
	inline const Vec2i &dir( int d )
	{
		using namespace ci;
		static const Vec2i dirs[8] = {
			Vec2i( +0, +1 ),
			Vec2i( -1, +1 ),
			Vec2i( -1, +0 ),
			Vec2i( -1, -1 ),
			Vec2i( +0, -1 ),
			Vec2i( +1, -1 ),
			Vec2i( +1, +0 ),
			Vec2i( +1, +1 )
		};
		
		return dirs[d % 8];
	}

	inline const Vec2i &dir( Direction d )
	{
		return dir( (int) d );
	}
}

#pragma mark -
#pragma mark Voxel

class Island;

/**
	@class Voxel
	Vertices represent the core voxel data type
*/
class Voxel {

	public:
		
		Vec2	worldPosition;
		Vec2	centroidRelativePosition;
		Vec2i	ordinalPosition;

		int			occupation;
		int			strength;
		int			id;
		std::size_t	tag;
		std::size_t	numIslands;
		int			rand;
		Voxel*		neighbors[8];		
		Island*		islands[8];

	public:
	
		Voxel():
			worldPosition(0,0),
			centroidRelativePosition(0,0),
			ordinalPosition( -1,-1 ),
			occupation(0),
			strength(0),
			id(0),
			tag(0),
			numIslands(0),
			rand(0)
		{
			for ( int i = 0; i < 8; i++ ) 
			{
				neighbors[i] = NULL;
				islands[i] = NULL;
			}
		}
					
		inline real volume() const { return real(occupation) / real(255); }
		inline bool empty() const { return occupation <= 0; }
		inline bool fixed() const { return strength >= 255; }		
				
		inline bool hasNeighbors() const
		{
			return neighbors[0] || neighbors[1] || neighbors[2] || neighbors[3] ||
				   neighbors[4] || neighbors[5] || neighbors[6] || neighbors[7];
		}
						
		// mutual disconnection from neighbors
		inline void disconnect() 
		{
			for ( int i = 0; i < 8; i++ )
			{
				if ( neighbors[i] ) 
				{
					neighbors[i]->neighbors[(i + 4) % 8] = NULL;
				}

				neighbors[i] = NULL;
			}
			
			occupation = 0;
		}

		inline void disconnect(int dir)
		{
			if (neighbors[dir])
			{
				neighbors[dir]->neighbors[(dir+4)%8] = NULL;
				neighbors[dir] = NULL;
			}
			
			// mark unoccupied if we're orphaned
			if ( !hasNeighbors() ) occupation = 0;
		}
		
		inline int addToIsland( Island *i )
		{
			if ( numIslands < 8 && !partOfIsland(i) )
			{
				islands[numIslands++] = i;
			}
			
			return numIslands - 1;
		}
		
		inline void removeFromIsland( Island *i )
		{
			if ( numIslands > 0 && partOfIsland(i))
			{
				std::remove( islands, islands + numIslands, i );
				numIslands--;
				islands[numIslands] = NULL;
			}
		}
		
		inline int islandIndex( Island *island ) const
		{
			for ( int i = 0; i < 8; i++ )
			{
				if ( islands[i] == island ) return i;
			}
			
			return -1;
		}
		
		inline bool partOfIsland( Island *i ) const
		{
			return numIslands > 0 &&
			       (islands[0] == i || 
				    islands[1] == i ||
				    islands[2] == i ||
				    islands[3] == i ||
				    islands[4] == i ||
				    islands[5] == i ||
				    islands[6] == i ||
				    islands[7] == i );
		}
		
};

#pragma mark -
#pragma mark OrdinalVoxelStore

/**
	@class OrdinalVoxelStore
*/
class OrdinalVoxelStore 
{
	protected:
	
		int _width, _height;
		real _scale, _rScale;
		std::vector< Voxel > _store;
		Voxel *_storePtr;

	public:
	
		OrdinalVoxelStore():
			_width(0),
			_height(0),
			_scale(1),
			_storePtr(NULL)
		{}
		
		~OrdinalVoxelStore()
		{}

		/**
			Initialize the vertex store to hold width * height voxels
		*/
		void set( int width, int height, real scale )
		{
			_width = width;
			_height = height;
			_scale = scale;
			_rScale = 1 / scale;

			_store.resize(width * height);
			_storePtr = &(_store[0]);
			
			for ( std::size_t row = 0; row < height; row++ )
			{
				for ( std::size_t col = 0; col < width; col++ )
				{
					Voxel *v = &(_storePtr[row * width + col]);
					v->ordinalPosition.x = col;
					v->ordinalPosition.y = row;
				}
			}
			
			ci::Rand rand;
			foreach( Voxel &v, _store )
			{
				v.rand = rand.nextInt();
				for ( int i = 0; i < 8; i++ )
				{
					v.neighbors[i] = this->voxelAt( v.ordinalPosition + Compass::dir( i ) );
				}
			}			
		}
						
		inline Voxel* voxelAt( int x, int y ) const 
		{
			if ( x >= 0 && x < _width &&
				 y >= 0 && y < _height )
			{
				return &(_storePtr[ (y * _width) + x]);
			}
			
			return NULL;
		}
		
		inline Voxel* voxelAt( const Vec2i &a ) const { return voxelAt( a.x, a.y ); }
		
		inline Voxel* voxelAtWorld( const Vec2 &world ) const
		{
			return voxelAt( int( world.x * _rScale ), int( world.y * _rScale ));
		}
		
		inline Voxel* voxelAtUnsafe( int x, int y ) const 
		{
			return &(_storePtr[ (y * _width) + x]);
		}

		inline Voxel* voxelAtUnsafe( const Vec2i &a ) const { return voxelAtUnsafe( a.x, a.y ); }
		
		int width() const { return _width; }
		int height() const { return _height; }
		real scale() const { return _scale; }
		Vec2i size() const { return Vec2i( _width, _height ); }
		
		const std::vector< Voxel > &voxels() const { return _store; }

};

}} // end namespace core::terrain
