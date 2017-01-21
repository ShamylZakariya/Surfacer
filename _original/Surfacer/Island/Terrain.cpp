/*
 *  Island.cpp
 *  CuttingExperiments
 *
 *  Created by Shamyl Zakariya on 12/14/10.
 *  Copyright 2010 Shamyl Zakariya. All rights reserved.
 *
 */

#include "Terrain.h"

#include <queue>

#include <cinder/app/App.h>
#include <cinder/ImageIO.h>
#include <cinder/Rand.h>
#include <cinder/gl/gl.h>
#include <cinder/ip/Resize.h>

#include "Level.h"
#include "Transform.h"
#include "TerrainRendering.h"
#include "GameConstants.h"
#include "Stopwatch.h"


using namespace ci;
using namespace core;
namespace game { namespace terrain {

namespace 
{

	enum GatherMode {
		GatherMode_All,
		GatherMode_EarlyExitForFixed
	};
	
	/**
		Gather all the islands reachable from @a island into @a group, removing those islands from the pool @a all
		@return true if any island in the group is fixed
	*/
	
	bool GatherIslandGroupRecursive( Island *island, std::set< Island* > &group, std::set< Island* > &all, GatherMode mode )
	{
		bool fixed = island->fixed();

		all.erase( island );
		group.insert( island );

		if ( fixed && mode == GatherMode_EarlyExitForFixed ) return true;

		//
		// get this island's neighbors
		//
		
		std::set< Island* > neighbors;
		foreach( Voxel *v, island->voxels() )
		{
			if ( v->numIslands > 1 ) 
			{
				for ( int i = 0, n = v->numIslands; i < n; i++ )
				{
					if ( v->islands[i] && (v->islands[i] != island))
					{
						neighbors.insert( v->islands[i] );
					}
				}
			}
		}
		
		//
		//	for each neighbor we found, add to our group and recurse if that neighbor hasn't been reached before
		//	note that if any neighbor is fixed, this group must be fixed
		//

		foreach( Island *neighbor, neighbors )
		{
			if ( group.insert( neighbor ).second )
			{
				if ( GatherIslandGroupRecursive( neighbor, group, all, mode ) ) 
				{
					fixed = true;
					if ( mode == GatherMode_EarlyExitForFixed ) return true;
				}
			}
		}	
		
		return fixed;
	}

	bool GatherIslandGroupIterative( Island *island, std::set< Island* > &group, std::set< Island* > &all, GatherMode mode )
	{
		bool fixed = false;
		std::queue< Island* > stack;
		stack.push( island );
		
		while( !stack.empty() )
		{
			Island *island = stack.front();
			stack.pop();

			if ( island->fixed() ) 
			{
				fixed = true;
			}

			all.erase( island );
			group.insert( island );
			
			//
			//	If fixed, and we're early exiting for fixed, get out of here
			//

			if ( fixed && mode == GatherMode_EarlyExitForFixed ) return true;
			
			foreach( Voxel *voxel, island->voxels())
			{
				if ( voxel->numIslands > 1 ) 
				{
					for ( int i = 0, n = voxel->numIslands; i < n; i++ )
					{
						Island *neighbor = voxel->islands[i];
						if ( neighbor && (neighbor != island) && group.insert( neighbor ).second )
						{
							stack.push(neighbor);
						}
					}
				}
			}
		}
		
		return fixed;
	}	
	
	inline bool GatherIslandGroup( Island *island, std::set< Island* > &group, std::set< Island* > &all, GatherMode mode )
	{
		return GatherIslandGroupRecursive(island, group, all, mode );
	}
	
}

#pragma mark -
#pragma mark Island


/*
		IslandGroup *_group;
		OrdinalVoxelStore *_store;
		island_group_dynamics _gd;

		bool _usable, _fixed, _entirelyFixed;
		std::vector<Voxel*> _voxels;
		std::vector< Vec2rVec > _voxelPerimeters;
		std::vector< triangle > _triangulation;
*/

Island::Island( 
	Surface &surface, 
	OrdinalVoxelStore *store, 
	const Vec2i &origin, 
	const Vec2i &size ):

	GameObject( GameObjectType::ISLAND ),	
	_renderer( NULL ),
	_group(NULL),
	_store(store),
	_usable(true),
	_fixed(true),
	_entirelyFixed(false)
{
	setName( "Island (Initialization Template)" );
	setVisibilityDetermination( VisibilityDetermination::NEVER_DRAW );

	const Vec2i surfaceSize( surface.getWidth(), surface.getHeight()),
				surfaceExtent( surfaceSize.x - 1, surfaceSize.y - 1 );
				
	const real scale = _store->scale();
		  
	const uint8_t pixelInc = surface.getPixelInc();

	const int xStart = std::max( origin.x - 0, 0 ),
			  xEnd = std::min( origin.x + size.x + 1, surfaceExtent.x ),
			  yStart = std::max( origin.y - 0, 0 ),
			  yEnd = std::min( origin.y + size.y + 1, surfaceExtent.y );

	for ( int y = yStart; y <= yEnd; y++ )
	{
		//
		// get the pixel data for this row - note, we're flipping y-axis
		//

		Vec2i rowOrigin(0,surfaceExtent.y - y);
		uint8_t *rowBytes = surface.getData( rowOrigin ),
				*bytes = rowBytes + (xStart * pixelInc);

		for ( int x = xStart; x <= xEnd; x++ )
		{
			const int 
				Red = bytes[0],
				Green = bytes[1],
				Blue = bytes[2];
				
			// right now:
			// red: occupation, 0 is empty, 255 is fully occupied.
			// green: strength, 0->254 are cuttable and dynamic; 255 is fixed and will eventually be uncuttable.
			// blue: id for Sensor testing
				
			if ( Red > 0 )
			{
				Voxel *voxel = store->voxelAt(x,y);
				
				if ( voxel )
				{
					//
					//	If the voxel has no owner, we know it needs to be initialized. Then add this Island to owners list
					//

					if ( voxel->numIslands == 0 )
					{
						voxel->occupation = Red;
						voxel->strength = Green;
						voxel->id = Blue;
						voxel->centroidRelativePosition = voxel->worldPosition = Vec2( voxel->ordinalPosition.x, voxel->ordinalPosition.y ) * scale;
					}

					//
					//	take posession
					//

					voxel->addToIsland(this);
					_voxels.push_back( voxel );
				}
			}

			//
			//	Update our row bytes by respective pixel increments
			//

			bytes += pixelInc;
		}
	}

	// no need to assign renderers since this is a throwaway island
}

Island::Island( OrdinalVoxelStore *store, const std::vector<Voxel*> &voxels ):
	GameObject( GameObjectType::ISLAND ),
	_renderer( new IslandRenderer() ),
	_group(NULL),
	_store( store ),
	_usable(true),
	_fixed(false),
	_entirelyFixed(true)
{
	setName( "Island" );
	addComponent( _renderer );
	setLayer( RenderLayer::TERRAIN );
	setVisibilityDetermination( VisibilityDetermination::FRUSTUM_CULLING ); // frustum is set by parent IslandGroup

	//
	//	we're computing local min/max with stack locals, because this proved 
	//	to be a hotspot when profiled. Also, for some reason RectT<int>::set() isn't implemented,
	//	so we can't use the RectT<int>(x1,y1,x2,y2) constructor.
	//

	Recti vertexBoundsLocal;
	vertexBoundsLocal.x1 = INT_MAX;
	vertexBoundsLocal.y1 = INT_MAX;
	vertexBoundsLocal.x2 = INT_MIN;
	vertexBoundsLocal.y2 = INT_MIN;

	//
	//	Take ownership of these voxels and compute the vertex ordinal bounds
	//

	_voxels.reserve( voxels.size() );
	foreach( Voxel *v, voxels )
	{
		v->addToIsland(this);
		_voxels.push_back(v);

		//
		//	if any voxels have max strength, this is a static 'fixed' island.
		//	And if any do NOT, this is not an 'entirely fixed' Island
		//

		if ( v->fixed() ) 
		{
			_fixed = true;	
		}
		else
		{
			_entirelyFixed = false;
		}

		vertexBoundsLocal.x1 = std::min( vertexBoundsLocal.x1, v->ordinalPosition.x );
		vertexBoundsLocal.y1 = std::min( vertexBoundsLocal.y1, v->ordinalPosition.y );
		vertexBoundsLocal.x2 = std::max( vertexBoundsLocal.x2, v->ordinalPosition.x );
		vertexBoundsLocal.y2 = std::max( vertexBoundsLocal.y2, v->ordinalPosition.y );
	}

	_vertexBoundsOrdinal = vertexBoundsLocal;		
}


Island::~Island()
{}

void Island::releaseVoxels()
{
	//
	//	Remove self from each of our voxels, disconnecting those who
	//	lose ownership. We do not delete voxels, as they are owned by the world's store.
	//

	foreach( Voxel *v, _voxels )
	{
		v->removeFromIsland(this);
		if ( v->numIslands == 0 ) 
		{
			v->disconnect();
		}
	}
	
	_voxels.clear();
}

Voxel *Island::findVoxelClosestTo( const Vec2 &worldPosition, real distThreshold ) const
{
	Voxel *found = NULL;
	real foundDist2 = FLT_MAX;
	const real distThreshold2 = distThreshold * distThreshold;
	
	Vec2 islandLocalPosition = this->group()->modelviewInverse() * worldPosition;

	for( std::vector< Voxel* >::const_iterator 
		voxel(_voxels.begin()), end( _voxels.end());
		voxel != end; 
		++voxel )
	{
		// use voxel->centroidRelativePosition
		real d2 = islandLocalPosition.distanceSquared( (*voxel)->centroidRelativePosition );
		if ( d2 < distThreshold2 )
		{
			if ( d2 < foundDist2 )
			{
				found = *voxel;
				foundDist2 = d2;
			}
		}
	}
	
	return found;
}

void Island::setGroup( IslandGroup *group ) 
{ 
	_group = group; 
	if ( _group )
	{
		setDrawPasses( _group->terrain()->drawPasses() );
	}
}

#pragma mark -
#pragma mark island_group_dynamics

island_group_dynamics::island_group_dynamics( class IslandGroup *group )
{
	if ( group )
	{
		position = group->position();
		linearVelocity = group->linearVelocity();
		angle = group->angle();
		angularVelocity = group->angularVelocity();
	}
	else
	{
		position = linearVelocity = Vec2(0,0);
		angle = angularVelocity = 0;
	}
}


#pragma mark -
#pragma mark IslandGroup

/*
		friend class Terrain;
	
		typedef std::vector< cpShape* > cpShapeVec;
		typedef std::map< Island*, cpShapeVec > IslandShapeVecMap;
	
		Terrain *_terrain;
		cpSpace *_space;
		cpBody *_body;
		IslandShapeVecMap _shapesByIsland;
		Mat4 _modelview, _modelviewInverse;
		bool _modelviewInverseDirty, _sleeping;
		real _area, _mass, _moment;

		std::set< Island* > _islands;
		
		ci::Color _groupDebugColor;
*/

IslandGroup::IslandGroup( cpSpace *space, Terrain *terrain, int gameObjectType ):
	GameObject( gameObjectType ),
	_terrain( terrain ),
	_space( space ),
	_body(NULL),
	_modelviewInverseDirty(true),
	_sleeping(true),
	_area(0),
	_mass(0),
	_moment(0)
{
	setLayer( RenderLayer::TERRAIN );
	setVisibilityDetermination( VisibilityDetermination::NEVER_DRAW );
}

IslandGroup::~IslandGroup()
{
	aboutToDestroyPhysics(this);

	// GameObject::dtor will delete _children
	freeCollisionShapes();
	
	if ( _body )
	{								
		if ( cpSpaceContainsBody( _space, _body ))
		{
			cpSpaceRemoveBody( _space, _body );
		}

		cpBodyFree( _body );
	}	
}

const Mat4 & IslandGroup::modelviewInverse()
{
	if ( _modelviewInverseDirty )
	{
		_modelviewInverse = _modelview.inverted();
		_modelviewInverseDirty = false;
	}
	
	return _modelviewInverse;
}

real IslandGroup::angle() const
{
	return cpBodyGetAngle( body() );
}

real IslandGroup::angularVelocity() const
{
	return cpBodyGetAngVel( body() );
}

Vec2 IslandGroup::position() const
{
	return v2( cpBodyGetPos( body() ));
}

Vec2 IslandGroup::linearVelocity() const
{
	return v2( cpBodyGetVel( body() ));
}

void IslandGroup::freeCollisionShapes()
{
	//
	//	free the collider shapes
	//

	for ( IslandShapeVecMap::iterator it( _shapesByIsland.begin()), end( _shapesByIsland.end()); it != end; ++it )
	{
		foreach( cpShape *shape, it->second )
		{
			cpSpaceRemoveShape( _space, shape );
			cpShapeFree( shape );
		}
	}

	_shapesByIsland.clear();
	_modelviewInverseDirty = true;
	_modelview.setToIdentity();
}

void IslandGroup::updateAabb()
{
	cpBB bounds = cpBBInvalid;
	
	for ( IslandShapeVecMap::iterator it(_shapesByIsland.begin()),end(_shapesByIsland.end()); it!=end; ++it )
	{
		cpBB islandBB;
		cpBBInvalidate( islandBB );
		
		foreach( cpShape* shape, it->second )
		{
			islandBB = cpBBMerge( islandBB, cpShapeGetBB( shape ));
		}
		
		islandBB.l -= IslandAABBPadding;
		islandBB.b -= IslandAABBPadding;
		islandBB.r += IslandAABBPadding;
		islandBB.t += IslandAABBPadding;

		it->first->setAabb( islandBB );
		bounds = cpBBMerge( bounds, islandBB );
	}
	
	setAabb( bounds );
}

bool IslandGroup::addIsland( Island* island )
{ 
	addChild( island );
	island->setGroup( this );
	return _islands.insert( island ).second; 
}

bool IslandGroup::removeIsland( Island *island ) 
{ 
	removeChild( island );
		
	//
	//	Remove the collision shapes associated with this Island
	//

	IslandShapeVecMap::iterator pos = _shapesByIsland.find( island ); 
	if ( pos != _shapesByIsland.end() )
	{
		foreach( cpShape *shape, pos->second )
		{
			cpSpaceRemoveShape( _space, shape );
			cpShapeFree( shape );
		}		

		_shapesByIsland.erase(pos);
	}
	
	if ( island->group() == this ) island->setGroup( NULL );	
	return _islands.erase( island ) > 0;
}		

bool IslandGroup::prune()
{
	std::set< Island* > remainingIslands;
	foreach( Island *island, _islands )
	{
		if ( island->usable() )
		{
			remainingIslands.insert( island );
		}
		else
		{
			removeChild( island );
			island->releaseVoxels();
			delete island;
		}
	}
	
	_islands = remainingIslands;
	
	return !_islands.empty();
}

#pragma mark -
#pragma mark StaticIslandGroup

StaticIslandGroup::StaticIslandGroup( cpSpace *space, Terrain *world ):
	IslandGroup( space, world, GameObjectType::ISLAND_STATIC_GROUP )
{
	setName( "StaticIslandGroup" );		
	setDebugColor( Color::white() );
}

StaticIslandGroup::~StaticIslandGroup()
{}

void StaticIslandGroup::updatePhysics()
{
	updateAabb();
}

bool StaticIslandGroup::addIsland( Island *island )
{
	//
	//	Create our static body
	//

	if ( !_body )
	{
		_body = cpBodyNewStatic();
		cpBodySetUserData( _body, this );			
	}

	//
	//	Skip if we already have this island
	//

	if ( hasIsland( island ) ) return true;

	//
	//	Add the island, and triangulate. If triangulation fails, cleanup and bail.
	//	Note, this island is static, so it can be triangulated off the default voxels' 
	//	world-space centroidRelativePositions. 
	//	When an island becomes dynamic ( can only go fixed->dynamic, not the other way around ) 
	//	we will have to compute centroidRelativePositions for rigid bodies
	//

	IslandGroup::addIsland( island );

	if ( island->triangulate() )
	{
		//
		//	Create collision shapes for this Island, and mark ownership
		//
		
		cpVect triangleVertices[3];
		
		foreach( const triangle &tri, island->triangulation() )
		{
			tri.vertices( triangleVertices );
			cpShape *polyShape = cpPolyShapeNew( _body, 3, triangleVertices, cpvzero );

			cpShapeSetUserData( polyShape, island );
			cpShapeSetElasticity( polyShape, _terrain->initializer().elasticity );
			cpShapeSetFriction( polyShape, _terrain->initializer().friction );
			cpShapeSetLayers( polyShape, CollisionLayerMask::TERRAIN );
			cpShapeSetCollisionType( polyShape, CollisionType::TERRAIN );
			
			cpSpaceAddShape( _space, polyShape );
			
			_shapesByIsland[island].push_back(polyShape);
		}
	}
	else
	{
		//
		// we can't use this island
		//

		removeIsland( island );
		island->releaseVoxels();
		delete island;

		return false;
	}			

	return true;
}

bool StaticIslandGroup::removeIsland( Island *island )
{
	if ( IslandGroup::removeIsland( island ))
	{
		#warning Chipmunk workaround - activating all bodies touching level body
		cpBodyActivateStatic( _body, NULL );
		
		return true;
	}
	
	return false;
}



#pragma mark -
#pragma mark DynamicIslandGroup

/*
		bool _physicsDirty;
		island_group_dynamics _inheritedDynamics;
*/

DynamicIslandGroup::DynamicIslandGroup( cpSpace *space, Terrain *world ):
	IslandGroup( space, world, GameObjectType::ISLAND_DYNAMIC_GROUP ),
	_physicsDirty( true )
{
	setName( "DynamicIslandGroup");
}

DynamicIslandGroup::DynamicIslandGroup( cpSpace *space, Terrain *world, const island_group_dynamics &parentGD ):
	IslandGroup( space, world, GameObjectType::ISLAND_DYNAMIC_GROUP ),
	_physicsDirty( true ),
	_inheritedDynamics( parentGD )
{
	setName( "DynamicIslandGroup");
}

DynamicIslandGroup::~DynamicIslandGroup()
{}

void DynamicIslandGroup::update( const time_state &time ) 
{
	if ( _body )
	{
		_sleeping = cpBodyIsSleeping( _body );

		if ( !_sleeping )
		{
			//
			//	update modelview for GL rendering, and mark inverse as dirty for lazy computation if needed
			//

			cpBody_ToMatrix( _body, _modelview );
			_modelviewInverseDirty = true;
			
			updateAabb();
		}
	}
}

void DynamicIslandGroup::updatePhysics()
{
	//
	//	Here's the quick and dirty of what's about to happen:
	//	We need to take all the islands which are members of this group and
	//	bring their voxels to a common coordinate space around a common centroid.
	//	We need to UNROTATE them for tesselation to work correctly. And finally,
	//	when creating a new body, we need to set the position and re-rotate, and
	//	add the velocity of the parent island group to maintain linear & angular vel.
	//	This is done by bringing the voxels to world space, then back to centroid-relative
	//
	

	//
	//	Terrain runs updatePhysics on ALL groups after any sort of
	//	cut or other topological change. Groups which didn't have islands
	//	added or removed don't need to update their physics.
	//
	
	if ( _physicsDirty )
	{
		const real density = _terrain->initializer().density,
		           elasticity = _terrain->initializer().elasticity,
		           friction = _terrain->initializer().friction;
	
		//
		//	If this is an existing group being rebuilt, retain our transform;
		//	Otherwise, use what was passed to our constructor.
		//

		if ( body() )
		{
			_inheritedDynamics = island_group_dynamics(this);
		}
	
		//
		//	Destroy current physics representation
		//

		freeCollisionShapes();
		
		//
		//	First, compute worldPosition for each vertex, and while doing so compute a
		//	rough centroid as average of worldPositions. That centroid will be the
		//	body() position.
		//

		Vec2 position;
		
		{
			util::Transform parentTransform( _inheritedDynamics.position, _inheritedDynamics.angle );

			Vec2 accum(0,0);
			std::size_t tick = 0;

			foreach( Island *island, _islands )
			{
				foreach( Voxel *v, island->voxels() )
				{
					if ( v->islands[0] == island )
					{
						v->worldPosition = parentTransform * v->centroidRelativePosition;

						accum += v->worldPosition;
						tick++;
					}
				}
			}
			
			position = accum / tick;			
		}
		
		//
		//	Now, we want to update the centroidRelativePosition of each vertex to reflect the new centroid,
		//	and also we want to rotate them in the inverse of the parent rotation such that the centroidRelativePositions
		//	are back in an unrotated coordinate system. This is necessary for tesselation. We also need to compute the
		//	offset from ordinalSpace to centroidRelativeSpace for tesselation to create voxels in centroidRelative space.
		//

		Vec2 centroidRelativePositionMin( FLT_MAX, FLT_MAX );
		Vec2 ordinalPositionMin( FLT_MAX, FLT_MAX );
		const real scale = _terrain->voxelStore().scale();

		{
			util::Transform unrotate( Vec2(0,0), -_inheritedDynamics.angle );

			foreach( Island *island, _islands )
			{
				foreach( Voxel *v, island->voxels() )
				{
					if ( v->islands[0] == island )
					{
						v->centroidRelativePosition = unrotate * (v->worldPosition - position);

						centroidRelativePositionMin = ::min( centroidRelativePositionMin, v->centroidRelativePosition );
						ordinalPositionMin = ::min( ordinalPositionMin, Vec2(v->ordinalPosition) * scale );
					}
				}
			}
		}

		real 
			mass = 0, 
			moment = 0;
		
		Vec2 
			ordinalToCentroidRelativeOffset = centroidRelativePositionMin - ordinalPositionMin;
		
		//
		//	triangulate our islands and determine mass and moment. 
		//	discard any islands which couldn't triangulate.
		//	

		cpVect triangleVertices[3];
		std::set< Island* > usableIslands;
		foreach( Island *island, _islands )
		{
			if ( island->triangulate( &ordinalToCentroidRelativeOffset ))
			{
				usableIslands.insert( island );

				foreach( const triangle &tri, island->triangulation() )
				{
					tri.vertices( triangleVertices );

					//
					//	For some reason, cpAreaForPoly and cpMomentForPoly are returning negative values.
					//

					real pMass = density * std::abs( cpAreaForPoly( 3, triangleVertices ));
					mass += pMass;

					real pMoment = cpMomentForPoly( pMass, 3, triangleVertices, cpvzero );
					if ( !std::isnan( pMoment ))
					{
						moment += std::abs( pMoment );
					}
				}					
			}
			else
			{
				removeChild( island );
				island->releaseVoxels();
				delete island;
			}
		}
		
		_mass = mass;
		_moment = moment;
		
		//
		//	Now, if any of the triangulations were successful, we can proceed to lazily make
		//	the body and add collision shapes. 
		//	Note, if _islands is empty(), we make no bodies and Terrain, which 
		//	will call this::prune() immediately after this::updatePhysics(), will
		//	discard unusable groups.
		//

		_islands = usableIslands;
		if ( !_islands.empty() )
		{	
			if ( !_body )
			{
				_body = cpBodyNew( this->mass(), this->moment() );
				cpBodySetUserData( _body, this );
				cpSpaceAddBody( _space, _body );
			}
			else
			{
				cpBodySetMass( _body, this->mass() );
				cpBodySetMoment( _body, this->moment() );
			}

			cpBodySetAngle( _body, _inheritedDynamics.angle );
			cpBodySetPos( _body, cpv(position));
			cpBodySetVel( _body, cpv(_inheritedDynamics.linearVelocity));
			cpBodySetAngVel( _body, _inheritedDynamics.angularVelocity );

			//
			//	Now add collision shapes
			//

			cpVect triangleVertices[3];
			foreach( Island *island, _islands )
			{
				foreach( const triangle &tri, island->triangulation() )
				{
					tri.vertices( triangleVertices );
					
					if ( cpPolyValidate( triangleVertices, 3 ) )
					{
						cpShape *polyShape = cpPolyShapeNew( _body, 3, triangleVertices, cpvzero );
						
						cpShapeSetUserData( polyShape, island );
						cpShapeSetElasticity( polyShape, elasticity );
						cpShapeSetFriction( polyShape, friction );
						cpShapeSetLayers( polyShape, CollisionLayerMask::TERRAIN );
						cpShapeSetCollisionType( polyShape, CollisionType::TERRAIN );
											
						cpSpaceAddShape( _space, polyShape );
						_shapesByIsland[island].push_back(polyShape);
					}
				}
			}
		}

		_physicsDirty = false;
		
		//
		//	Update matrices, aabbs, etc.
		//

		update( _terrain->level()->time() );
	}
}


bool DynamicIslandGroup::addIsland( Island *island )
{
	if (IslandGroup::addIsland( island ))
	{
		_physicsDirty = true;
		return true;
	}
	
	return false;
}

bool DynamicIslandGroup::removeIsland( Island *island )
{
	if ( IslandGroup::removeIsland( island ))
	{
		_physicsDirty = true;
		return true;
	}
	
	return false;
}

void DynamicIslandGroup::sanitize( std::vector< DynamicIslandGroup* > &newGroups )
{
	std::set< Island* > all = _islands;
	std::vector< std::set< Island* > > groups;

	while( !all.empty() )
	{
		Island *island = *all.begin();

		std::set< Island* > group;
		GatherIslandGroup( island, group, all, GatherMode_All );
		
		groups.push_back( group );
	}
	
	//
	//	If we collected more than one group, create new child groups
	//	and the caller will be responsible for deleting us
	//

	if ( groups.size() > 1 )
	{
		island_group_dynamics myDynamics( this );

		foreach( std::set< Island* > &group, groups )
		{
			DynamicIslandGroup *dig = new DynamicIslandGroup( _space, _terrain, myDynamics );
			dig->addIslands( group );
			newGroups.push_back( dig );
		}
		
		//
		// we no longer own these islands, since we've handed them off to new dynamic groups
		//

		_islands.clear();
		removeAllChildren();
	}	
}

#pragma mark -
#pragma mark Terrain

CLASSLOAD(Terrain)

/*
		init _initializer;
		cpSpace *_space;

		// voxel representation and island groups
		OrdinalVoxelStore _voxels;
		StaticIslandGroup* _staticGroup;
		std::set< DynamicIslandGroup* > _dynamicGroups;
		std::vector< Island* > _allIslands;
				
		// rendering ivars
		ci::gl::Texture _materialTex, _modulationTex, _greebleTexAtlas;
		ci::gl::GlslProg _solidMaterialShader, _greebleShader;
		
		// deferred cutting state
		seconds_t _deferredGeometryUpdateTime, _geometryUpdateDeferralTime;
		std::set< Island* > _dirtyIslands;
		Vec2rVec _chunkedCuttingLine;	
		
		bool _renderVoxelsInDebug;
*/

Terrain::Terrain():
	GameObject( GameObjectType::TERRAIN ),
	_space( NULL ),
	_staticGroup(NULL),
	_deferredGeometryUpdateTime(-1),
	_geometryUpdateDeferralTime(0.25),
	_renderVoxelsInDebug(false)
{
	setName( "Terrain" );
	setLayer( RenderLayer::TERRAIN );
	setVisibilityDetermination( VisibilityDetermination::NEVER_DRAW );
}

Terrain::~Terrain()
{}

void 
Terrain::initialize( const init &initializer )
{
	GameObject::initialize(initializer);
	_initializer = initializer;
}

void Terrain::addedToLevel( Level *level )
{
	GameObject::addedToLevel(level);

	_space = level->space();
	_geometryUpdateDeferralTime = 10 * level->time().deltaT;
	
	//
	//	Load textures
	//

	ResourceManager *rm = level->resourceManager();

	// make a local copy, since Island needs a non-const Surface &
	ci::Surface levelImage = rm->getSurface( _initializer.levelImage );

	_materialTex = rm->getTexture( _initializer.materialTexture );
	_materialTex.setWrap( GL_REPEAT, GL_REPEAT );
	
	_greebleTexAtlas = rm->getTexture( _initializer.greebleTextureAtlas );
	if ( _greebleTexAtlas )
	{
		_greebleTexAtlas.setWrap( GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE );

		//
		//	mark that we need two render passes when greeblin' - note, Terrain doesn't draw, Islands
		//	do. So islands will sync their draw passes to terrain's.
		//

		setDrawPasses(2);
	}

	gl::Texture::Format mipmappingFormat;
	mipmappingFormat.enableMipmapping(true);
	mipmappingFormat.setMinFilter( GL_LINEAR_MIPMAP_LINEAR );
	mipmappingFormat.setMagFilter( GL_LINEAR );	
	_modulationTex = gl::Texture( _createModulationSurface( levelImage ), mipmappingFormat );
	
	
	//
	//	Initialize the OrdinalVoxelStore
	//	

	int width = _initializer.extent.x > 0 ? _initializer.extent.x : levelImage.getWidth(),
		height = _initializer.extent.y > 0 ? _initializer.extent.y : levelImage.getHeight();

	width = std::min( width, levelImage.getWidth());
	height = std::min( height, levelImage.getHeight());
	
	//
	//	Initialize the voxel store
	//

	_voxels.set( width, height, _initializer.scale );
	
	//
	//	Mark our bounds
	//

	const real AABBInset = 1/_initializer.scale;
	setAabb( cpBBNew(0, 0, width*_initializer.scale - AABBInset, (height-1)*_initializer.scale - AABBInset));

	//
	//	Sanity check
	//
	
	assert( !_staticGroup );
	assert( _dynamicGroups.empty() );
	
	_staticGroup = new StaticIslandGroup(_space, this);
	addChild( _staticGroup );


	//
	//	the island(s) created here via the Surface constructor aren't tesselated, nor are they
	//	given physics bodies; instead, they're immediately passed to _partitionIsland and the real gameplay islands are created
	//
	
	std::set< Island* > islandTemplates;
	if ( _initializer.sectorSize.x > 0 && _initializer.sectorSize.y > 0 )
	{
		for ( int y = _initializer.origin.y; y < height; y += _initializer.sectorSize.y )
		{
			for ( int x = _initializer.origin.x; x < width; x += _initializer.sectorSize.x )
			{
				islandTemplates.insert( 
					new Island( levelImage, &_voxels, Vec2i(x,y), _initializer.sectorSize ) );
			}
		}
		
	}
	else
	{
		islandTemplates.insert( 
			new Island( levelImage, &_voxels, _initializer.origin, Vec2i(width,height) ));
	}

	//
	//	Cut the island templates to make proper islands
	//

	std::set< Island* > newIslands;
	foreach( Island *island, islandTemplates )
	{
		_partitionIsland( island, newIslands );
	}
	
	//
	//	Now walk island connectivity to build an initial set of static and
	//	dynamic island groupings
	//

	_createIslandGroups( newIslands );	
}


void Terrain::update( const time_state &time )
{
	//
	//	Fire signal marking voxels which were touched
	//
	
	if ( !_touchedScaledWorldPositionsByCut.empty())
	{
		for ( std::map< TerrainCutType::cut_type, Vec2iSet >::const_iterator touches(_touchedScaledWorldPositionsByCut.begin()),end(_touchedScaledWorldPositionsByCut.end()); touches != end; ++touches )
		{
			if ( touches->second.empty() ) continue;

			//
			//	convert touched voxels set to array of unique world positions, and emit
			//

			_touchedVoxelWorldPositions.clear();
			for( std::set< Vec2i >::const_iterator scaledPosition(touches->second.begin()), end(touches->second.end()); scaledPosition!=end; ++scaledPosition )
			{
				_touchedVoxelWorldPositions.push_back( _downscalePosition( *scaledPosition ) );
			}
			
			cutPerformed( _touchedVoxelWorldPositions, touches->first );
		}
		
		_touchedScaledWorldPositionsByCut.clear();
	}
}

void Terrain::prepareForBatchDraw( const render_state &state, GameObject * )
{
	switch( state.mode )
	{
		case RenderMode::GAME:
			_prepareBatchDraw_Game(state);
			break;

		case RenderMode::DEVELOPMENT:
			_prepareBatchDraw_Development(state);
			break;

		case RenderMode::DEBUG:
			_prepareBatchDraw_Debug(state);
			break;
			
		case RenderMode::COUNT:
			break;
	}
}

void Terrain::cleanupAfterBatchDraw( const render_state &state, GameObject *, GameObject * )
{
	switch( state.mode )
	{
		case RenderMode::GAME:
			_cleanupAfterBatchDraw_Game(state);
			break;
			
		case RenderMode::DEVELOPMENT:
			_cleanupAfterBatchDraw_Development(state);
			break;

		case RenderMode::DEBUG:
			_cleanupAfterBatchDraw_Debug(state);
			break;
			
		case RenderMode::COUNT:
			break;
	}
}


void Terrain::updateGeometry( const time_state &time )
{
	//
	//	If deferred geometry updates are pending, _deferredGeometryUpdateTime will be > 0; if enough time
	//	has passed since the oldest cut to justify a geometry update, perform it and reset.
	//

	if (_deferredGeometryUpdateTime > 0)
	{
		if ( time.time > _deferredGeometryUpdateTime )
		{
			if ( !_dirtyIslands.empty() )
			{
				_partitionIslands( _dirtyIslands );
				_dirtyIslands.clear();
			}
			
			_deferredGeometryUpdateTime = -1;
		}
	}
}

void Terrain::_gatherAllIslands()
{
	_allIslands.clear();

	for ( std::set< Island* >::const_iterator it( _staticGroup->_islands.begin()),end(_staticGroup->_islands.end()); it != end; ++it )
	{
		_allIslands.push_back( *it );
	}
	
	for ( std::set< DynamicIslandGroup* >::const_iterator it(_dynamicGroups.begin()),end(_dynamicGroups.end()); it != end; ++it )
	{
		for( std::set< Island* >::const_iterator islandIt( (*it)->_islands.begin()),islandEnd((*it)->_islands.end()); islandIt != islandEnd; ++islandIt )
		{
			_allIslands.push_back( *islandIt );
		}
	}	
}

void Terrain::_createIslandGroups( std::set< Island* > all )
{
	while( !all.empty() )
	{
		Island* island = *all.begin();
		std::set< Island* > newGroup;
		bool isFixed = GatherIslandGroup( island, newGroup, all, GatherMode_All );
		
		if ( isFixed )
		{
			_staticGroup->addIslands( newGroup );
		}	
		else
		{
			DynamicIslandGroup *dg = new DynamicIslandGroup( _space, this );
			dg->addIslands( newGroup );

			_dynamicGroups.insert( dg );
			addChild( dg );
		}
	}
	
	//
	//	Now that we're done, update physics representations
	//
	
	_staticGroup->updatePhysics();
	_staticGroup->prune();
		
	std::set< DynamicIslandGroup* > remainingDynamicGroups;
	foreach( DynamicIslandGroup* dg, _dynamicGroups )
	{
		dg->updatePhysics();
		
		if ( dg->prune() )
		{
			remainingDynamicGroups.insert( dg );
		}
		else
		{
			removeChild( dg );
			delete dg;
		}
	}	
	
	_dynamicGroups = remainingDynamicGroups;

	//
	//	Finally update our list of all islands
	//

	_gatherAllIslands();
}

void Terrain::_updateIslandGroups( std::set< Island* > newIslands )
{
	while( !newIslands.empty() )
	{
		Island* island = *newIslands.begin();
		std::set< Island* > newGroup;

		bool isFixed = GatherIslandGroup( island, newGroup, newIslands, GatherMode_EarlyExitForFixed );
								
		if ( isFixed )
		{
			foreach( Island *i, newGroup )
			{
				_staticGroup->addIsland(i);
			}
		}	
		else
		{
			//
			//	Some of these islands may have been in the static group;
			//	remove them, since they're being reparented to dynamic
			//

			foreach( Island *i, newGroup )
			{
				if ( _staticGroup->removeIsland(i))
				{
					cutTransitionedIslandFromStaticToDynamic( i );
				}
			}
		
			//
			//	find if any islands in this group are in an existing dynamic group;
			//	if so, update that group, otherwise, create a new dynamicGroup
			//
			
			DynamicIslandGroup *existingGroup = NULL;
			foreach( DynamicIslandGroup *existingDynamicGroup, _dynamicGroups )
			{
				foreach( Island *ngi, newGroup )
				{
					if ( existingDynamicGroup->hasIsland( ngi ))
					{
						existingGroup = existingDynamicGroup;
						break;
					}
				}
				
				if ( existingGroup ) break;
			}
			
			if ( existingGroup )
			{
				existingGroup->addIslands( newGroup );
			}
			else
			{
				//
				//	Create new DynamicIslandGroup inheriting island_group_dynamics
				//	from the seed island's.
				//

				DynamicIslandGroup *dg = new DynamicIslandGroup( _space, this, island->_groupDynamics() );
				dg->addIslands( newGroup );

				_dynamicGroups.insert( dg );
				addChild( dg );
			}
		}
	}
	
	//
	//	Update physics representations
	//
	
	_staticGroup->updatePhysics();
	_staticGroup->prune();

	std::set< DynamicIslandGroup* > remainingDynamicGroups;
	foreach( DynamicIslandGroup* dg, _dynamicGroups )
	{
		dg->updatePhysics();
		
		if ( dg->prune() )
		{
			remainingDynamicGroups.insert( dg );
		}
		else
		{
			removeChild( dg );
			delete dg;
		}
	}	
	
	_dynamicGroups = remainingDynamicGroups;

	//
	//	Finally update our list of all islands
	//

	_gatherAllIslands();
}

#pragma mark -
#pragma mark Terrain Rendering

Surface Terrain::_createModulationSurface( const Surface &levelImage ) const
{
	//
	//	Create a square, POT surface of max size 256•256 from the green channel of levelImage
	//

	int size = std::min( std::min( levelImage.getWidth(), levelImage.getHeight() ), 256 );		
	if ( !isPow2( size )) size = previousPowerOf2(size);

	ci::Surface s( size, size, false, SurfaceChannelOrder::RGB );
	ci::ip::resize( levelImage, &s );
	
	Surface::Iter iter = s.getIter();

	while( iter.line() )
	{
		while( iter.pixel() )
		{
			iter.r() = iter.g() = iter.b() = 255 - iter.g();
		}
	}

	return s;
}

void Terrain::_prepareBatchDraw_Game( const render_state &state )
{
	const float 
		scale = 0.25,
		materialScale = _initializer.sectorSize.x > 0 ? (_initializer.sectorSize.x * scale) : scale;

	switch( state.pass )
	{
		case SOLID_GEOMETRY_PASS:
		{
			if ( !_solidMaterialShader )
			{
				_solidMaterialShader = level()->resourceManager()->getShader( "IslandShader" );		
			}

		
			_solidMaterialShader.bind();
			_solidMaterialShader.uniform( "MaterialTexture", 0 );
			_solidMaterialShader.uniform( "ModulationTexture", 1 );
			_solidMaterialShader.uniform( "MaterialScale", materialScale );

			//
			//	Bind textures
			//
			
			_materialTex.bind(0);
			_materialTex.setWrap( GL_REPEAT, GL_REPEAT );
			
			_modulationTex.bind(1);
			
			glDisable( GL_BLEND );

			break;
		}
		
		case GREEBLING_PASS:
		{
			if ( !_greebleShader )
			{
				if ( _initializer.greebleTextureIsMask )
				{
					_greebleShader = level()->resourceManager()->getShader( "GreebleMaskShader" );
				}
				else
				{
					_greebleShader = level()->resourceManager()->getShader( "GreebleShader" );
				}
			}

			_greebleShader.bind();
			
			if ( _initializer.greebleTextureIsMask )
			{
				_greebleShader.bind();
				_greebleShader.uniform( "MaterialTexture", 0 );
				_greebleShader.uniform( "ModulationTexture", 1 );
				_greebleShader.uniform( "GreebleMaskTexture", 2 );
				_greebleShader.uniform( "MaterialScale", materialScale );

				//
				//	Bind textures
				//
				
				_materialTex.bind(0);
				_materialTex.setWrap( GL_REPEAT, GL_REPEAT );
				
				_modulationTex.bind(1);
				_greebleTexAtlas.bind(2);
			}
			else
			{
				_greebleShader.uniform( "Texture0", 0 );
				_greebleTexAtlas.bind(0);
			}
			
			glEnable( GL_BLEND );
			glBlendFuncSeparate( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE );

			break;
		}
	}
	
	gl::disableWireframe();
	gl::color( Color::white() );
}

void Terrain::_cleanupAfterBatchDraw_Game( const render_state &state )
{
	switch( state.pass )
	{
		case SOLID_GEOMETRY_PASS:
		{
			_materialTex.unbind(0);
			_modulationTex.unbind(1);

			break;
		}
		
		case GREEBLING_PASS:
		{
			if ( _initializer.greebleTextureIsMask )
			{
				_materialTex.unbind(0);
				_modulationTex.unbind(1);
				_greebleTexAtlas.unbind(2);
			}
			else
			{
				_greebleTexAtlas.unbind(0);
			}

			break;
		}
	}	
	
	glActiveTexture( GL_TEXTURE0 );
	gl::GlslProg::unbind();
}

void Terrain::_prepareBatchDraw_Development( const core::render_state &state )
{
	switch( state.pass )
	{
		case SOLID_GEOMETRY_PASS:
			_prepareBatchDraw_Game(state);
			break;
			
		case GREEBLING_PASS:
			_prepareBatchDraw_Debug(state);
			break;
	}
}

void Terrain::_cleanupAfterBatchDraw_Development( const core::render_state &state )
{
	switch( state.pass )
	{
		case SOLID_GEOMETRY_PASS:
			_cleanupAfterBatchDraw_Game(state);
			break;
			
		case GREEBLING_PASS:
			_cleanupAfterBatchDraw_Debug(state);
			break;
	}
}

void Terrain::_prepareBatchDraw_Debug( const render_state &state )
{
	gl::enableAlphaBlending();
}

void Terrain::_cleanupAfterBatchDraw_Debug( const render_state &state )
{}

}} // end namespace terrain
