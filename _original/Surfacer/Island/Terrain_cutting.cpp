//
//  Terrain_cutting.cpp
//  Surfacer
//
//  Created by Zakariya Shamyl on 9/27/11.
//  Copyright 2011 TomorrowPlusX. All rights reserved.
//

#include "Terrain.h"
#include <cinder/app/App.h>

#include "FloodFill.h"
#include "Level.h"
#include "LineChunking.h"
#include "Stopwatch.h"


using namespace ci;
using namespace core;
namespace game { namespace terrain {

namespace {
    
	const real 
        VoxelRadiusLocal = 0.707106781;
    

	// apply a line to a voxel to remove its occupation component, disconnecting if emptied
	unsigned int LineVoxelCut( 
		Voxel *voxel, 
		const util::line_segment &line, 
		real minDist, 
		real maxDist, 
		real oneOverMaxMinusMin,		
		real cutStrength )
	{
		real dist = line.distance( voxel->centroidRelativePosition );
		unsigned int effectMask = 0;

		//
		//	subtract occupation from the vertex based on its overlap with the line segment
		//

		if ( dist <= maxDist )
		{
			//
			//	If this voxel is fixed, early exit
			//

			if ( voxel->fixed() )
			{
				return Terrain::CUT_HIT_FIXED_VOXELS;
			}
		
			// this is the maximum possible occupation of the voxel given overlap and a hypothetical infinitely strong cut
			int maxPossibleOccupation = std::min( lrintf( (dist - minDist) * oneOverMaxMinusMin * 255.0 ), 255L ),			
			    ablation = 255 * (((255 - voxel->strength)/real(255)) * cutStrength); // 0->255 

			ablation = std::max( std::min( ablation, voxel->occupation - maxPossibleOccupation ), 0 );
			
			if ( ablation > 0 )
			{
				voxel->occupation -= ablation;
				effectMask |= Terrain::CUT_AFFECTED_VOXELS;
			}				
						
			//
			//	If we actually changed the voxel occupation
			//

			if ( effectMask && voxel->occupation < MinimumVoxelOccupation ) 
			{
				voxel->disconnect();
				effectMask |= Terrain::CUT_AFFECTED_ISLAND_CONNECTIVITY;
			}
		}

		//
		//	Cut neighbor connections which intersect the cutting line
		//

		for ( int dir = 0; dir < 8; ++dir )
		{
			Voxel *neighbor = voxel->neighbors[dir];
			if ( neighbor && line.intersects(voxel->centroidRelativePosition, neighbor->centroidRelativePosition ))
			{
				voxel->disconnect(dir);
				effectMask |= Terrain::CUT_AFFECTED_ISLAND_CONNECTIVITY;
			}
		}		
				
		return effectMask;
	}
		
}


#pragma mark -
#pragma Terrain Cutting

unsigned int Terrain::cutLine( 
	const Vec2 &start, 
	const Vec2 &end, 
	real thickness, 
	real strength, 
	TerrainCutType::cut_type cutType,
	Island *restrictToIsland )
{
	//
	//	Sanity check
	//

	if( thickness < Epsilon || strength < Epsilon ) 
	{
		return 0;
	}

	strength = std::min( strength, real(1));

	//
	//	We're treating the voxel as a circle enclosing the voxel square, so we're expanding the voxel
	//	radius by center-to-corner diagonal.
	//

	const real 
		VoxelRadius = _voxels.scale() * VoxelRadiusLocal,
		MaxDist = (thickness * 0.5) + VoxelRadius,
		MinDist = (thickness * 0.5) - VoxelRadius,
		OneOverMaxMinusMin = 1.0 / ( MaxDist - MinDist );

	//
	//	Filter the islands down to those which intersect the macro line bounds - later we'll filter to chunk bounds
	//

	cpBB lineBounds;
	cpBBNewLineSegment( lineBounds, start, end );

	std::vector< Island* > islands;
	
	if ( restrictToIsland )
	{
		islands.push_back( restrictToIsland );
	}
	else
	{
		islands.reserve( _allIslands.size() );		
		for( std::vector< Island* >::const_iterator it(_allIslands.begin()),end(_allIslands.end()); it != end; ++it )
		{
			if ( cpBBIntersects( (*it)->aabb(), lineBounds )) islands.push_back( *it );
		}	
	}

	//
	//	Get the storage for touched voxel positions for this cut
	//

	Vec2iSet &touchedScaledWorldPositions = _touchedScaledWorldPositionsByCut[cutType];

	//
	//	Chunk the input line to sublines inside terrain sectors
	//	

	_chunkedCuttingLine.clear();
	core::util::ChunkLine( start, end, _initializer.sectorSize, _chunkedCuttingLine );
	
	unsigned int effectMask = 0;
	
	Vec2 a(_chunkedCuttingLine.front());
	for ( Vec2rVec::const_iterator it( _chunkedCuttingLine.begin() + 1 ), end( _chunkedCuttingLine.end()); it != end; ++it )
	{
		Vec2 b = *it;
		cpBB chunkBB;
		cpBBNewLineSegment( chunkBB, a, b );
		
		for( std::vector< Island* >::const_iterator islandIt(islands.begin()), islandEnd(islands.end()); islandIt != islandEnd; ++islandIt )
		{
			Island *island = *islandIt;

			if ( !cpBBIntersects( chunkBB, island->aabb() )) continue;
			
			//
			// convert world space points to island-group-local
			//

			util::line_segment lineInGroupSpace(
				island->group()->modelviewInverse() * a,
				island->group()->modelviewInverse() * b);

			//
			// we will want to know if this island actually was modified or not
			//

			unsigned int lineCutEffectMask = 0;
			for( std::vector< Voxel* >::iterator voxelIt(island->_voxels.begin()), voxelEnd(island->_voxels.end()); voxelIt != voxelEnd; ++voxelIt )
			{
				Voxel *voxel = *voxelIt;

				unsigned int result = LineVoxelCut( 
					voxel, 
					lineInGroupSpace, 
					MinDist, MaxDist, OneOverMaxMinusMin, 
					strength );

				if ( result )
				{
					lineCutEffectMask |= result;					
					touchedScaledWorldPositions.insert( 
						_upscalePosition( island->group()->fixed() ? voxel->centroidRelativePosition
							: island->group()->modelview() * voxel->centroidRelativePosition ));
				}				
			}
			
			//
			//	if this island was touched by the line, so we need to mark it as affected
			//

			if ( lineCutEffectMask )
			{
				effectMask |= lineCutEffectMask;
				_dirtyIslands.insert( island );
			}			
		}		
		
		a = b;
	}
	
	//
	//	If any voxels were touched, we need to mark an update is needed
	//

	if ( effectMask & CUT_AFFECTED_VOXELS )
	{
		_markDeferredGeometryUpdateNeeded();
	}
	
	return effectMask;
}

unsigned int Terrain::cutDisk(
	const Vec2 &position,
	real radius,
	real strength,
	TerrainCutType::cut_type cutType,
	Island *restrictToIsland )
{
	//
	//	Sanity check
	//

	if( radius < Epsilon || strength < Epsilon ) 
	{
		return 0;
	}

	strength = std::min( strength, real(1));

	//
	//	Get the storage for touched voxel positions for this cut
	//

	Vec2iSet &touchedScaledWorldPositions = _touchedScaledWorldPositionsByCut[cutType];
	Mat4 islandModelview = restrictToIsland->group()->modelview();

	const real
		VoxelRadius = _voxels.scale() * VoxelRadiusLocal,
		MinDistToTouch = radius + VoxelRadius,
        MinDistForCompleteOverlap = radius - VoxelRadius,
        MinDistSquaredToTouch = MinDistToTouch * MinDistToTouch;

	//
	//	Now we're going to iterate the island's voxels.
    //  Move disk position to group space of Island.
	//
    	
	Vec2 positionInGroupSpace = restrictToIsland->group()->modelviewInverse() * position;
	unsigned int effectMask = 0;
	

	for( std::vector< Voxel* >::iterator voxelIt(restrictToIsland->_voxels.begin()), voxelEnd(restrictToIsland->_voxels.end()); voxelIt != voxelEnd; ++voxelIt )
	{
		Voxel *voxel = *voxelIt;		
		real distSquared = positionInGroupSpace.distanceSquared( voxel->centroidRelativePosition );
				
		if ( distSquared < MinDistSquaredToTouch )
		{
            //
			//  disregard uncuttable voxels
            //  note: we have to check for fixed voxels that could have been touched for CUT_HIT_VOXELS to be meaningful
            //

			if ( voxel->fixed() ) 
			{
				effectMask |= CUT_HIT_FIXED_VOXELS;
				continue;
			}


            //
            //  overlap goes from 0 for no overlap to 1 for complete overlap
            //

			real dist = std::sqrt( distSquared ),
				 overlap = real(1) - saturate( (dist - MinDistForCompleteOverlap) / ( MinDistToTouch - MinDistForCompleteOverlap ));
            
            //
			//  ablation is the amount that could be removed based on overlap, cut strength and the voxel's strength, 0->255
            //  note that ablation falls to zero as voxel strength goes to 255
            //

			int ablation = 255 * (((255 - voxel->strength)/real(255)) * overlap * strength);
			
			if ( ablation > 0 )
			{
				voxel->occupation = std::max( voxel->occupation - ablation, 0 );
				effectMask |= CUT_AFFECTED_VOXELS;

				touchedScaledWorldPositions.insert( 
					_upscalePosition( 
						islandModelview * voxel->centroidRelativePosition ));
				
				//
				//	Now if the cutting disc completely overlaps the voxel, we disconnect its outgoing connections
				//
				
				if ( voxel->occupation < MinimumVoxelOccupation || (dist < MinDistForCompleteOverlap + Epsilon) )
				{
					effectMask |= CUT_AFFECTED_ISLAND_CONNECTIVITY;
					voxel->disconnect();
					voxel->occupation = 0;
				}				
			}
		}				
	}
	
	/////////////////
	
	if ( effectMask & CUT_AFFECTED_VOXELS )
	{
		//
		//	Add this island to the dirty set and mark an update is needed
		//

		_dirtyIslands.insert( restrictToIsland );
		_markDeferredGeometryUpdateNeeded();
	}

	return effectMask;
}

#pragma mark - Voxel->Voxel Cutting

namespace {

	bool voxel_voxel_cut( Voxel *a, Voxel *b, real radiusWorld, real strength, unsigned int &aCutResultMask, unsigned int &bCutResultMask )
	{		
		const real 
			Distance2 = a->worldPosition.distanceSquared( b->worldPosition ),
			Overlap = 1 - ( Distance2 / ((2*radiusWorld)*(2*radiusWorld))),
			//Distance = a->worldPosition.distance(b->worldPosition),
			//Overlap = 1 - (Distance/(2*radiusWorld)),
			AStrength = static_cast<real>(a->strength)/real(255),
			BStrength = static_cast<real>(b->strength)/real(255),
			CutStrength = strength * Overlap * std::max( AStrength, BStrength );
	
		if ( CutStrength > ALPHA_EPSILON )
		{
			if ( a->occupation > 0 )
			{
				if ( a->strength < 255 )
				{
					aCutResultMask |= Terrain::CUT_AFFECTED_VOXELS;
					a->occupation -= 255 * ((1 - AStrength) * CutStrength);

					if ( a->occupation < MinimumVoxelOccupation ) 
					{
						a->occupation = 0;	
						a->disconnect();
						aCutResultMask |= Terrain::CUT_AFFECTED_ISLAND_CONNECTIVITY;			
					}
				}
				else
				{
					aCutResultMask |= Terrain::CUT_HIT_FIXED_VOXELS;
				}
			}

			if ( b->occupation > 0 )
			{
				if ( b->strength < 255 )
				{
					bCutResultMask |= Terrain::CUT_AFFECTED_VOXELS;
					b->occupation -= 255 * ((1 - BStrength) * CutStrength);

					if ( b->occupation < MinimumVoxelOccupation ) 
					{
						b->occupation = 0;
						b->disconnect();
						bCutResultMask |= Terrain::CUT_AFFECTED_ISLAND_CONNECTIVITY;			
					}
				}
				else
				{
					bCutResultMask |= Terrain::CUT_HIT_FIXED_VOXELS;
				}
			}
		}
			
		return aCutResultMask && bCutResultMask;
	}

}

unsigned int Terrain::cutTerrain( Island *cuttingIsland, real strength, TerrainCutType::cut_type cutType, Island *restrictToIsland )
{
	if ( strength < ALPHA_EPSILON ) return 0;
	strength = std::min( strength, real(1));

	unsigned int cutResultEffectMask = 0;

	const real 
		Scale = 0.5,
		VoxelRadiusWorld = Scale * 0.707106781 * _voxels.scale(),
		VoxelRadiusWorld2 = VoxelRadiusWorld * VoxelRadiusWorld;
		

	unsigned int cuttingIslandCutEffectMask = 0;
	IslandGroup *cuttingIslandGroup = cuttingIsland->group();
	cpBB cuttingIslandBounds = cuttingIsland->aabb();
	std::vector< Island* > islands;
	
	if ( restrictToIsland )
	{
		islands.push_back( restrictToIsland );
	}
	else
	{
		islands.reserve( _allIslands.size() );		
		for( std::vector< Island* >::const_iterator it(_allIslands.begin()),end(_allIslands.end()); it != end; ++it )
		{
			Island *testIsland = *it;		
			if ( !cuttingIslandGroup->hasIsland( testIsland ) && cpBBIntersects( testIsland->aabb(), cuttingIslandBounds ) )
			{
				islands.push_back( testIsland );
			}
		}	
	}
	
	//
	//	Update world positions of cutting island, if it's dynamic. If it's static the worldPosition
	//	is already correct from initialization.
	//
	
	if ( !cuttingIslandGroup->fixed() )
	{
		Mat4 mv = cuttingIslandGroup->modelview();
		for( std::vector< Voxel* >::iterator 
			cuttingVoxel( cuttingIsland->voxels().begin()), 
			cuttingVoxelEnd( cuttingIsland->voxels().end());
			cuttingVoxel != cuttingVoxelEnd;
			++cuttingVoxel )
		{
			(*cuttingVoxel)->worldPosition = mv * (*cuttingVoxel)->centroidRelativePosition;
		}
	}

	//
	//	Get the storage for touched voxel positions for this cut, so we can deferred emit cutting effects
	//

	Vec2iSet &touchedScaledWorldPositions = _touchedScaledWorldPositionsByCut[cutType];
	
	//
	//	Now, for each Island which the cuttingIsland might intersect...
	//
	
	foreach( Island *targetIsland, islands )
	{
		unsigned int targetIslandCutEffectMask = 0;
		
		IslandGroup *targetIslandGroup = targetIsland->group();
		Mat4 islandGroupModelView = targetIslandGroup->modelview();

		//
		//	Holy On^2 batman - each voxel has to test against the other. At least we're pruning to the minimum overlapping subset.
		//

		cpBB intersectionBounds;
		if ( !cpBBIntersection( cuttingIslandBounds, targetIsland->aabb(), intersectionBounds )) continue;


		for( std::vector< Voxel* >::iterator 
			targetVoxelIt( targetIsland->voxels().begin()), 
			targetVoxelItEnd( targetIsland->voxels().end());
			targetVoxelIt != targetVoxelItEnd;
			++targetVoxelIt )
		{
			Voxel *targetVoxel = *targetVoxelIt;
		
			//
			//	If the group is dynamic, we need to update the worldPosition - if it's static the world
			//	position is already correct.
			//

			if ( !targetIslandGroup->fixed() )
			{
				targetVoxel->worldPosition = targetVoxel->centroidRelativePosition * islandGroupModelView;
			}
			
			//
			//	Only process voxels which are in the overlapping region
			//

			if ( cpBBContainsCircle( intersectionBounds, targetVoxel->worldPosition, VoxelRadiusWorld ))
			{
				for( std::vector< Voxel* >::iterator 
					cuttingVoxelIt( cuttingIsland->voxels().begin()), 
					cuttingVoxelItEnd( cuttingIsland->voxels().end());
					cuttingVoxelIt != cuttingVoxelItEnd;
					++cuttingVoxelIt )
				{
					Voxel *cuttingVoxel = *cuttingVoxelIt;
				
					if ( 
						cpBBContainsCircle( intersectionBounds, cuttingVoxel->worldPosition, VoxelRadiusWorld ) && 
						cuttingVoxel->worldPosition.distanceSquared( targetVoxel->worldPosition ) < VoxelRadiusWorld2 &&
						voxel_voxel_cut( cuttingVoxel, targetVoxel, VoxelRadiusWorld, strength, cuttingIslandCutEffectMask, targetIslandCutEffectMask )
					)
					{
						touchedScaledWorldPositions.insert( _upscalePosition( targetVoxel->worldPosition ));
					}
				}
			}			
		}
		
		//
		//	If this island was touched by the cuttingIsland, we need to mark it as affected
		//

		if ( targetIslandCutEffectMask )
		{
			cutResultEffectMask |= targetIslandCutEffectMask;
			_dirtyIslands.insert( targetIsland );
		}			
	}
	
	//
	//	If the cuttingIsland was affected by the cut, we ned to mark it
	//

	if ( cuttingIslandCutEffectMask )
	{
		cutResultEffectMask |= cuttingIslandCutEffectMask;
		_dirtyIslands.insert( cuttingIsland );
	}
	
	//
	//	Finally, if any of the cutting affected anything, we need to queue a deferred geometry update
	//
	
	if ( (cutResultEffectMask & CUT_AFFECTED_VOXELS) || (cutResultEffectMask & CUT_AFFECTED_ISLAND_CONNECTIVITY) )
	{
		_markDeferredGeometryUpdateNeeded();
	}

	return cutResultEffectMask;
}

#pragma mark - Partitioning

namespace {

	//
	//	Partitioning
	//

	struct gathering_visitor 
	{	
		std::vector<Voxel*> &_voxels;

		gathering_visitor( std::vector<Voxel*> &voxels ):
			_voxels( voxels )
		{}
		
		inline bool operator()( Voxel *v )
		{
			_voxels.push_back(v);
			return true;
		}	
	};
	
	/**
		We want all voxels which are occupied, or which have an occupied neighbor.
		This causes us to gather to antialiased fringe around an island, which results
		in a higher quality tesselation.
	*/
	inline bool ShouldGatherVoxel( Voxel *v )
	{
		if ( v->occupation >= MinimumVoxelOccupation ) return true;
		
		// check if this voxel isn't occupied, but has a neighbor which is
		for ( int i = 0; i < 8; i++ )
		{
			Voxel *n = v->neighbors[i];
			if ( n && n->occupation >= MinimumVoxelOccupation ) return true;
		}
		
		return false;
	}
	
	struct island_membership_tester
	{
		Island *_island;
		island_membership_tester( Island *i ):
			_island(i)
		{}

		inline bool operator()( Voxel *v ) const
		{
			return v && ShouldGatherVoxel(v) && v->partOfIsland(_island);
		}
	};

	inline void GatherVoxels( Island *island, Voxel *origin, std::vector< Voxel* > &voxels )
	{
		gathering_visitor visitor(voxels);
		island_membership_tester test(island);
		floodfill::visit( origin, visitor, test );
	}

}

void Terrain::_markDeferredGeometryUpdateNeeded()
{
	if ( _deferredGeometryUpdateTime < 0 )
	{
		//	defer update by _geometryUpdateDeferralTime + a random tiny fudge factor - this factor should help mitigate
		//	sitations where syncopation of various deferred actions hits the CPU at the same time

		seconds_t fudge = Rand::randFloat(_geometryUpdateDeferralTime * 0.1);
		_deferredGeometryUpdateTime = level()->time().time + _geometryUpdateDeferralTime + fudge;
	}
}

void Terrain::_partitionIslands( std::set< Island* > &affectedIslands )
{	
	//
	//	Record the group dynamics of the affected islands before 
	//	removing them from their respective groups.
	//
	
	foreach( Island *moribundIsland, affectedIslands )
	{
		IslandGroup *group = moribundIsland->group();

		moribundIsland->_setGroupDynamics( island_group_dynamics(group));
	}
	
	//
	//	Remove the islands which will be cut from the static group, and from the various dynamic groups.
	//	Delete any dynamic groups which are emptied. 
	//
	
	_staticGroup->removeIslands( affectedIslands );

	std::set< DynamicIslandGroup* > remainingDynamicGroups;
	foreach( DynamicIslandGroup *dg, _dynamicGroups )
	{
		dg->removeIslands( affectedIslands );
		
		if (!dg->empty())
		{
			remainingDynamicGroups.insert(dg);
		}
		else
		{
			removeChild( dg );
			delete dg;
		}
	}
	
	//
	//	swap over our dynamic groups
	//

	_dynamicGroups = remainingDynamicGroups;
	remainingDynamicGroups.clear();

	//
	//	Perform the cutting against affected islands; collecting newly created islands in newIslands
	//

	std::set< Island* > newIslands;
	foreach( Island *island, affectedIslands )
	{
		//
		//	Perform the cut, populating newIslands with the newly created islands.
		//	note: _partitionIsland() deletes the passed in island.
		//
		_partitionIsland( island, newIslands );
	}

	//
	//	Clear the list of affected islands -- they've been deleted in _partitionIsland()
	//

	affectedIslands.clear();

	//
	//	Now that the partitioning is performed, and affected islands' destructors have run, we need to sanitize our
	//	remaining DynamicIslandGroups to see if the changes to voxel connectivity split them into child groupings.
	//	We also want to discard any dynamic groups which have been emptied.
	//	

	foreach( DynamicIslandGroup *dg, _dynamicGroups )
	{
		std::vector< DynamicIslandGroup* > newDynamicGroups;
		dg->sanitize( newDynamicGroups );
		
		if ( newDynamicGroups.empty() )
		{
			remainingDynamicGroups.insert( dg );
		}
		else
		{
			removeChild( dg );
			delete dg;

			foreach( DynamicIslandGroup *ndg, newDynamicGroups )
			{
				addChild( ndg );
				remainingDynamicGroups.insert( ndg );
			}
		}
	}
	
	_dynamicGroups = remainingDynamicGroups;

	
	//
	//	Update groups and 
	//
	
	_updateIslandGroups( newIslands );		
}

void Terrain::_partitionIsland( Island *moribundIsland, std::set< Island* > &newIslands )
{

	//
	//	The approach is simple -- we'll walk through the voxels of this island and each
	//	time we reach a voxel that is owned by the parent island and is both occupied && !disconnected,
	//	we perform a flood fill to grab all voxels reachable from this one. That group will be made into
	//	a new island, who add tenantship of those voxels, and remove tenantship from the moribund island. 
	//	Then we continue the search for voxels owned by the original parent island.
	//	

	foreach( Voxel *v, moribundIsland->voxels() )
	{
		//
		//	note that creating a new Island changes vertex island tenantship
		//

		if ( v->partOfIsland(moribundIsland) && v->hasNeighbors() )
		{
			std::vector< Voxel * > connectedVoxels;
			GatherVoxels( moribundIsland, v, connectedVoxels );
			
			//
			//	Remove these voxels from the moribund island
			//

			foreach( Voxel *cv, connectedVoxels )
			{
				cv->removeFromIsland(moribundIsland);
			}

			//
			//	If we have at least one voxel, create a new Island. The new
			//	island inherits the island_group_dynamics from the parent Island.
			//	This is used to transfer coordinate spaces and momentum in
			//	DynamicIslandGroup::updatePhysics
			//

			if ( connectedVoxels.size() > 1 )
			{
				Island *newIsland = new Island( &_voxels, connectedVoxels );
				newIsland->setBatchDrawDelegate( this );
				newIsland->_setGroupDynamics( moribundIsland->_groupDynamics() );
				newIslands.insert( newIsland );
			}
		}
	}
	
	//
	//	Now, remove the island from its group, and delete it. This will disconnect orphaned voxels.
	//	Note: at runtime all islands have a group, but during the initial partitioning
	//	after loaded the level data, the islands are all orphans.	
	//

	if ( moribundIsland->group() )
	{
		moribundIsland->group()->removeIsland( moribundIsland );	
	}
	
	moribundIsland->releaseVoxels();
	delete moribundIsland;
}


}} // end namespace game::terrain
