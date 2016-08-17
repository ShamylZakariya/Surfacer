//
//  Island_perimeterGreebling.cpp
//  Surfacer
//
//  Created by Zakariya Shamyl on 8/30/11.
//  Copyright 2011 TomorrowPlusX. All rights reserved.
//

#include "Terrain.h"
#include "LineSegment.h"

using namespace ci;
using namespace core;
namespace game { namespace terrain {

namespace {

	inline Vec2r snapToVoxel( const Vec2r &ordinal )
	{
		return Vec2r( std::floor( ordinal.x + real(0.5) ) + real(0.5), std::floor( ordinal.y + real(0.5) ) + real(0.5));
	}
	
	inline bool edgelike( const Voxel const *v, const Island const *thisIsland )
	{
		for ( int i = 0; i < 8; i++ )
		{
			Voxel *ov = v->neighbors[i];
			if ( !ov || ov->occupation <= MinimumVoxelOccupation ) return true;
		}

		return false;
	}
	
	inline void seeds( int randValue, int &a, int &b, int &c, int &d )
	{
		a = randValue >> 24;
		b = (randValue & 0x00FF0000) >> 16;
		c = (randValue & 0x0000FF00) >> 8;
		d = (randValue & 0x000000FF);
	}

	const static Vec2f ParticleShape[4] = 
	{ 
		Vec2f( -1, -1 ),
		Vec2f( +1, -1 ),
		Vec2f( +1, +1 ),
		Vec2f( -1, +1 )
	};

	const float r4 = 1.0f / 4.0f,
	            pi_8 = M_PI / 8.0f,
				pi_16 = M_PI / 16.0f,
				pi_32 = M_PI / 32.0f;

	const static Vec2f TexCoords[4] = 
	{ 
		Vec2f( r4, 0 ),
		Vec2f( 0 , 0 ),
		Vec2f( 0 , r4 ),
		Vec2f( r4, r4 )
	};
	
	
	const static Vec2f AtlasOffset_FourByFour[16] = 
	{ 
		Vec2f( 0*r4, 3*r4), Vec2f( 1*r4, 3*r4), Vec2f( 2*r4, 3*r4), Vec2f( 3*r4, 3*r4),
		Vec2f( 0*r4, 2*r4), Vec2f( 1*r4, 2*r4), Vec2f( 2*r4, 2*r4), Vec2f( 3*r4, 2*r4),
		Vec2f( 0*r4, 1*r4), Vec2f( 1*r4, 1*r4), Vec2f( 2*r4, 1*r4), Vec2f( 3*r4, 1*r4),
		Vec2f( 0*r4, 0*r4), Vec2f( 1*r4, 0*r4), Vec2f( 2*r4, 0*r4), Vec2f( 3*r4, 0*r4)
	};
		
	inline float range( int r, float min, float max )
	{
		return min + (float(r)/255.0f) * (max - min);
	}
	
	inline Vec2r invertY( const Vec2r &v )
	{
		return Vec2r( v.x, real(1) - v.y );
	}

	void add_greeble_particle( 
		std::vector< perimeter_greeble_vertex > &vertices, 
		const Voxel const *vertex,
	    const Vec2f &position, 
		float radius,
		const util::line_segment &segment,
		const Vec2r &Offset,
		const Vec2r &TotalOrdinalSizeReciprocal,
		bool isMask )
	{
		int a,b,c,d;
		seeds( vertex->rand, a,b,c,d );

		// modulate radius slightly by vertex's random factor
		radius = range( c, radius * 0.5, radius * 2 );

		// modulate position along segment by vertex's random factor
		Vec2f origin( position + range( d, -radius * 0.5, radius * 0.5 ) * segment.dir ),
		      right( radius * segment.dir.x, radius * segment.dir.y ),
		      up( -right.y, right.x );

			  
		Vec2f corners[4];
		corners[2] = right + up;

		corners[3].x = -corners[2].y;
		corners[3].y = +corners[2].x;

		corners[0].x = -corners[3].y;
		corners[0].y = +corners[3].x;

		corners[1].x = -corners[0].y;
		corners[1].y = +corners[0].x;

		float upYSign = up.y < 0 ? -1.0f : 1.0f;

		int atlasCol = std::min( std::floor((upYSign * up.y * up.y + 1) * 2), 3.0f ),
		    atlasRow = c % 4,
		    atlasIdx = atlasRow * 4 + atlasCol;	
						
		float modulation = isMask ? 1 : float(1) - ( vertex->strength / float(255));
			
		for ( int i = 0; i < 4; i++ )
		{
			perimeter_greeble_vertex vertex;
			vertex.position = position + corners[i];
			vertex.texCoord = invertY((vertex.position - Offset ) * TotalOrdinalSizeReciprocal);
			vertex.maskTexCoord = TexCoords[i] + AtlasOffset_FourByFour[atlasIdx];
			vertex.color.x = modulation;
			vertex.color.y = modulation;
			vertex.color.z = modulation;
			vertex.color.w = 1;

			vertices.push_back( vertex );
		}			 
	}

}

  
void Island::_createPerimeterGreebling( Vec2r const *ordinalToCentroidRelativeOffset )
{
	const real terrainScale = _store->scale();

	const Vec2r 
		Offset = ordinalToCentroidRelativeOffset ? *ordinalToCentroidRelativeOffset : Vec2r(0,0),
		TotalOrdinalSizeReciprocal( real(1) / real( _store->width() * terrainScale ), real(1) / real(_store->height() * terrainScale) );

	const real 
		TerrainScale = _store->scale(),
		ReciprocalTerrainScale = 1 / TerrainScale,
		VoxelSize = TerrainScale,
		SegmentEndThreshold = VoxelSize * 0.25,
		GreebleSize = _group->terrain()->initializer().greebleSize,
		GreebleRadius = GreebleSize * 0.5;
		
	const bool	
		isMask = _group->terrain()->initializer().greebleTextureIsMask;
	
	_perimeterGreebleVertices.clear();
	foreach( Vec2rVec &voxelPerimeter, _voxelPerimeters )
	{
		if ( voxelPerimeter.size() > 2 )
		{
			Vec2r b = voxelPerimeter.back(),
			      snappedB = snapToVoxel(b * ReciprocalTerrainScale);

			for ( int i = 0, N = voxelPerimeter.size(); i < N; i++ )
			{
				Vec2r a = voxelPerimeter[i],
				      snappedA = snapToVoxel(a * ReciprocalTerrainScale );

				util::line_segment 
					perimeterSegment( a, b ),
					snappedSegment( snappedA, snappedB );

				for ( real d = SegmentEndThreshold, end = perimeterSegment.length - SegmentEndThreshold; d <= end; d += GreebleSize )
				{
					Vec2r perimeterPoint = perimeterSegment.lrp(d),
					      snappedPoint = snappedSegment.lrp(d);

					Voxel *v = _store->voxelAt( snappedPoint );
					
					if ( v && (v->numIslands == 1 || edgelike(v, this) ))
					{
						Vec2r localPointOnSegment = perimeterPoint + Offset;
						add_greeble_particle( 
							_perimeterGreebleVertices, 
							v, 
							localPointOnSegment, 
							GreebleRadius, 
							perimeterSegment,
							Offset,
							TotalOrdinalSizeReciprocal,
							isMask );	
					}
					
				}
				
				b = a;
				snappedB = snappedA;
			}
		}
	}
}
  
}} // end namespace game::terrain