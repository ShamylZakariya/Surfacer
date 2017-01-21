#pragma once

/*
 *  Terrain.h
 *  CuttingExperiments
 *
 *  Created by Shamyl Zakariya on 12/14/10.
 *  Copyright 2010 Shamyl Zakariya. All rights reserved.
 *
 */

#include <cinder/Surface.h>
#include <cinder/Color.h>
#include <cinder/Vector.h>
#include <cinder/Matrix.h>
#include <cinder/gl/Texture.h>
#include <cinder/gl/GlslProg.h>

#include <cinder/Function.h>
#include <boost/bind.hpp>

#include "Common.h"
#include "GameObject.h"
#include "LineSegment.h"
#include "Viewport.h"
#include "GameConstants.h"

#include "Voxel.h"

namespace game { namespace terrain {

class Island;
class IslandGroup;
class StaticIslandGroup;
class DynamicIslandGroup;
class Terrain;

enum terrain_render_pass {

	SOLID_GEOMETRY_PASS		= 0,
	GREEBLING_PASS			= 1

};


#pragma mark -
#pragma mark triangle

struct triangle 
{
	struct vertex {
		ci::Vec2 position, texCoord;

		vertex(){}

		vertex( const Vec2 &p, const Vec2 &tc ):
			position(p),
			texCoord(tc)
		{}
	};

	vertex a,b,c;

		
	triangle()
	{}

	inline
	triangle( const Vec2 &A, const Vec2 &tcA,
			  const Vec2 &B, const Vec2 &tcB,
			  const Vec2 &C, const Vec2 &tcC ):
		a( A, tcA ),
		b( B, tcB ),
		c( C, tcC )
	{}
	
	~triangle()
	{}
	
	inline void vertices( cpVect *verts ) const
	{
		verts->x = a.position.x;
		verts->y = a.position.y;
		verts++;

		verts->x = b.position.x;
		verts->y = b.position.y;
		verts++;

		verts->x = c.position.x;
		verts->y = c.position.y;
	}
						
};

#pragma mark -
#pragma mark island_group_dynamics

/**
	Represents the transform and velocity of an IslandGroup
	Used to pass on position and momentum from one DynamicIslandGroup to
	its children.
*/

struct island_group_dynamics {

	Vec2 position, linearVelocity;
	real angle, angularVelocity;

	island_group_dynamics():
		position( 0,0 ), 
		linearVelocity( 0,0 ),
		angle( 0 ),
		angularVelocity( 0 )
	{}
	
	island_group_dynamics( class IslandGroup *group );

};

inline std::ostream &operator << ( std::ostream &os, const island_group_dynamics &igd )
{
	return os << "[position (" << igd.position.x << ", " << igd.position.y << ") angle: " << igd.angle << "]";
}

/**
	@struct perimeter_greeble_particle
	Island draws greebling shapes around its perimeter. The perimeter_greeble_particle
	represents a single greebling particle, to be rendered by IslandRenderer after the Island itself is rendered.
*/

struct perimeter_greeble_vertex {
	ci::Vec2 position;
	ci::Vec2 texCoord;
	ci::Vec2 maskTexCoord;
	ci::Vec4 color;
};

#pragma mark -
#pragma mark Island

class Island : public core::GameObject
{
	public:

		/**
			Initialize an island using pixel data in a bitmap
			
			@param data The image data to build from.
			@param store The OrdinalVoxelStore into which new Voxel instances will be added
			@param origin The sector origin
			@param size The sector size
			
			The param @a data is an RGB image where RED is the occupation level of matter for a particular location,
			GREEN is the strength of that matter, and for now BLUE is ignored. GREEN values run from 0, totally weak and
			easy for the player to cut, to 254 which is hard but still cuttable. GREEN of 255 is impervious to cutting, 
			and is referred to as "fixed", since it anchors any attached matter to the static game world.			
		*/
		Island( ci::Surface &data, OrdinalVoxelStore *store, const Vec2i &origin, const Vec2i &size );

		/**            			Initialize an island using a subset of voxels from another island.
			This new island takes ownership of the subset of the previous island's voxels
		*/
		Island( OrdinalVoxelStore *store, const std::vector<Voxel*> &voxels );

		~Island();
		
		/**
			Call before destroying an Island - lets go of this Island's voxels.
			We can't call this in Island::~Island because when Terrain is destroyed,
			its voxel store goes away before its child Groups & Islands are deleted. 			
		*/
		void releaseVoxels();
		
		/**
			Get all this Island's voxels
		*/
		inline std::vector< Voxel* > &voxels() { return _voxels; }
		
		inline const std::vector< Voxel* > &voxels() const { return _voxels; }
		
		/**
			Get the shared ordinal voxel store
		*/
		const OrdinalVoxelStore *voxelStore() const { return _store; }
		
		/**
			Get the rectangle bounding the voxels used in this Island.
			The rectangle bounds the Island's voxels' ordinalPositions. The ordinalPositions
			are set during level initialization, and do not change at runtime.
		*/
		ci::Recti voxelBoundsOrdinal() const { return _vertexBoundsOrdinal; }
		
		/**
			FInds the voxel owned by this island which is closest to @a worldPosition and closer than
			@a distThreshold
		*/
		Voxel *findVoxelClosestTo( const Vec2 &worldPosition, real distThreshold ) const;
		
		/**
			Get this Island's perimeter triangulation
		*/
		const std::vector< triangle > &triangulation() const { return _triangulation; }
		
		/**
			Get this island's perimeter greebling particle array, in centroid-relative coordinate space
		*/
		const std::vector< perimeter_greeble_vertex > &perimeterGreebleVertices() const { return _perimeterGreebleVertices; }

		/**
			Return true if this Island can be used, e.g., has a viable triangulation
		*/
		bool usable() const { return _usable; }		
		
		/**
			Return true if this Island has any fixed voxels, which means
			any Voxel who's strength is >= 255.
		*/
		bool fixed() const { return _fixed; }
		
		/**
			Return true iff this Island is made up entirely of fixed voxels
		*/
		bool entirelyFixed() const { return _entirelyFixed; }
		
		/**
			Set the IslandGroup membership of this Island
		*/
		void setGroup( IslandGroup *group );
		
		/**
			Get this Island's group membership
		*/
		IslandGroup* group() const { return _group; }
		
		/**
			Create triangulation for this Island's shape.
			Frees any previous triangulation.
			@return true if the Island is usable
		*/
		bool triangulate( Vec2 const *ordinalToCentroidRelativeOffset = NULL );

	private:
	
		friend class IslandGroup;
		friend class Terrain;

		bool _createVoxelPerimeters(); // implemented in Island_perimeter.cpp
		bool _triangulate( Vec2 const *ordinalToCentroidRelativeOffset );
		void _triangulatePerimeters( const std::vector< Vec2rVec > &ordinalSpacePerimeter, 
		                             const Vec2 &offset, 
		                             std::vector< triangle > &triangulation );

		void _createPerimeterGreebling( Vec2 const *ordinalToCentroidRelativeOffset );

		
		void _setGroupDynamics( const island_group_dynamics &igd )
		{
			_gd = igd;
		}
		
		const island_group_dynamics &_groupDynamics() const
		{
			return _gd;
		}
			
	private:
		
		class IslandRenderer *_renderer;
		IslandGroup *_group;
		OrdinalVoxelStore *_store;
		island_group_dynamics _gd;

		bool _usable, _fixed, _entirelyFixed;
		ci::Recti _vertexBoundsOrdinal;
		std::vector<Voxel*> _voxels;
		std::vector< Vec2rVec > _voxelPerimeters;
		std::vector< triangle > _triangulation;
		
		// perimeter greeble particle voxels, in centroid-relative space
		std::vector< perimeter_greeble_vertex > _perimeterGreebleVertices;
		
};

#pragma mark -
#pragma mark IslandGroups

class IslandGroup : public core::GameObject
{
	public:
	
		IslandGroup( cpSpace *space, Terrain *world, int gameObjectType );
		virtual ~IslandGroup();
		
		Terrain *terrain() const { return _terrain; }
		cpSpace *space() const { return _space; }
		cpBody *body() const { return _body; }
		
		virtual bool fixed() const = 0;
		
		real area() const { return _area; }
		real mass() const { return _mass; }
		real moment() const { return _moment; }
		bool sleeping() const { return _sleeping; }

		const Mat4 &modelview() const { return _modelview; }
		const Mat4 &modelviewInverse();

		real angle() const;
		real angularVelocity() const;
		Vec2 position() const;
		Vec2 linearVelocity() const;
		
		void freeCollisionShapes();
		virtual void updatePhysics(){}
		virtual void updateAabb();

		/**
			Add an island to this group.
			Return true if it was added, false if it was
			already a member of the group and nothing changed.
		*/
		virtual bool addIsland( Island* island );

		/**
			Remove an island from this group.
			Return true if it was a member of the group and was removed.
			Return false if it was not, and nothing changed.
		*/
		virtual bool removeIsland( Island *island );
		
		const std::set< Island* > &islands() const { return _islands; }
		bool empty() const { return _islands.empty(); }
		std::size_t size() const { return _islands.size(); }
		bool hasIsland( Island *island ) const { return _islands.count(island); }
		
		template < class T >
		void addIslands( const T &islands ) { foreach( Island *island, islands ) { addIsland( island ); }}

		template < class T >
		void removeIslands( const T &islands ) { foreach( Island *island, islands ) { removeIsland( island ); }}

		/**
			Prune the store of islands to only contain usable islands. Any non-usable
			islands are removed and deleted.
			
			@return true if this IslandGroup has at least one usable Island after pruning.
		*/
		virtual bool prune();
						
	protected:
	
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
};

class StaticIslandGroup : public IslandGroup
{
	public:
	
		StaticIslandGroup( cpSpace *space, Terrain *world );		
		virtual ~StaticIslandGroup();

		virtual bool fixed() const { return true; }
		virtual void updatePhysics();

		virtual bool addIsland( Island *island );
		virtual bool removeIsland( Island *island );
};


class DynamicIslandGroup : public IslandGroup
{
	public:
	
		DynamicIslandGroup( cpSpace *space, Terrain *world );
		DynamicIslandGroup( cpSpace *space, Terrain *world, const island_group_dynamics &gd );
		
		virtual ~DynamicIslandGroup();

		virtual bool fixed() const { return false; }

		virtual void update( const core::time_state &time );
		virtual void updatePhysics();
		
		virtual bool addIsland( Island *island );
		virtual bool removeIsland( Island *island );
		
		/**
			After removing islands from a dynamic group, call this and @a newGroups
			will be populated with new DynamicIslandGroups, if the removal(s) split this group
			up. After doing so, this group should be deleted and the new groups replace it.
			
			If the removal didn't split group connectivity, @a newGroups will be empty
			
		*/
		void sanitize( std::vector< DynamicIslandGroup* > &newGroups );
		
	protected:
	
		bool _physicsDirty;
		island_group_dynamics _inheritedDynamics;
		
};

#pragma mark -
#pragma mark Terrain


class Terrain : public core::GameObject, public core::BatchDrawDelegate
{
	public:

		struct init : public core::GameObject::init {
			Vec2i sectorSize;
			Vec2i origin;
			Vec2i extent;
			real scale;			
			ci::fs::path levelImage;
			ci::fs::path materialTexture;
			ci::Color materialColor;
			real density;		
			real elasticity;
			real friction;

			ci::fs::path greebleTextureAtlas;
			real greebleSize;
			bool greebleTextureIsMask;
			
			init():
				sectorSize(64,64),
				origin(0,0),
				extent(-1,-1),
				scale(1),
				materialColor(1,0,1),
				density(100),
				elasticity(0),
				friction(0.5),
				greebleSize(0.5),
				greebleTextureIsMask(true)
			{}
						
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				core::GameObject::init::initialize(v);
				JSON_READ(v,sectorSize);
				JSON_READ(v,origin);
				JSON_READ(v,extent);
				JSON_READ(v,scale);
				JSON_READ(v,materialColor);
				JSON_READ(v,levelImage);
				JSON_READ(v,materialTexture);
				JSON_READ(v,density);
				JSON_READ(v,elasticity);
				JSON_READ(v,friction);
				JSON_READ(v,greebleTextureAtlas);
				JSON_READ(v,greebleSize);
				JSON_READ(v,greebleTextureIsMask);				
			}

						
		};
		
		enum CutResult {
			CUT_AFFECTED_VOXELS					= 1,
			CUT_AFFECTED_ISLAND_CONNECTIVITY	= 2,
			CUT_HIT_FIXED_VOXELS				= 4
		};
		
	public:

		/**
			the cutPerformed signal

			This will be emitted when a cut is performed, passing a Vec2rVec of the world space positions
			of the voxels which were touched by the cut and the type of cut passed to Terrain::cutLine. 
			
			You can use this for particle system effects, etc.
		*/
		signals::signal< void(const Vec2rVec &, TerrainCutType::cut_type ) > cutPerformed;


		/**
			Invoked when a cut severs the connection between an island and another island's fixed voxels.
			Such a cut causes one or more Islands to transition from fixed to dynamic.
		*/
		signals::signal< void( Island* ) > cutTransitionedIslandFromStaticToDynamic;

	
	public:
	
		Terrain();
		~Terrain();

		JSON_INITIALIZABLE_INITIALIZE();

		void initialize( const init &initializer );
		const init &initializer() const { return _initializer; }
		
		void addedToLevel( core::Level *level );
		void update( const core::time_state &time );


		virtual void prepareForBatchDraw( const core::render_state &, GameObject *firstInRun );
		virtual void cleanupAfterBatchDraw( const core::render_state &, GameObject *firstInRun, GameObject *lastInRun );


		/**
			Process any pending deferred geometry updates.
			This is called by Level after all GameObjects' update()s are called.
		*/
		void updateGeometry( const core::time_state &time );
		
		OrdinalVoxelStore &voxelStore() { return _voxels; }
		const OrdinalVoxelStore &voxelStore() const { return _voxels; }
		Vec2i size() const { return _voxels.size(); }
		cpSpace *space() const { return _space; }
						
		const std::vector< Island* > &allIslands() const { return _allIslands; }
		
		/**
			Cut a line through the terrain

			@param start Point in world space of the start of the cutting line
			@param end Point in world space of the end of the cutting line
			@param thickness The width of the cut. Values less than 1 tend to work poorly unless applied repeatedly.
			@param strength The stength of the cut. 0 is completely innefectual, and 1 is maximum strength.
			@param cutType The type of cut performed - this is just a marker passed in the cutPerformed signal so you can create appropriate graphic effects.

			@return mask of CutResult describing the effect the cut had
			
			When applying a cut, all voxels touching the region 1/2 thickness from the cut will be affected. The affect
			is dependant on the strength of the cut. Each voxel has a strength value from 0 to 255. Voxels with
			strength up to but not including 255 are cuttable. 255 means "impossible to cut".
			
			Applying a cut to a voxel works sort of like:

				voxel->occupation -= (cutOverlap * (cutStrength * (1-voxel->strength)))
				
			Where:
				cutOverlap is the amount 0 to 1 that the cut overlaps the voxel
				cutStrength is the strength of the cut you provide
			
			As voxel strength approaches 1, cuts become less and less effective.	
			
			@note The re-partitioning and regeneration of geometry is not performed immediately,
			rather, required updates are batch executed during Level::update() which calls Terrain::updateGeometry.
		*/
		unsigned int cutLine( 
			const Vec2 &start, 
			const Vec2 &end, 
			real thickness, 
			real strength, 
			TerrainCutType::cut_type cutType,
			Island *restrictToIsland = NULL );

		unsigned int cutDisk(
			const Vec2 &position,
			real radius,
			real strength,
			TerrainCutType::cut_type cutType,
			Island *restrictToIsland );

		unsigned int cutTerrain(
			Island *shape,
			real strength,
			TerrainCutType::cut_type cutType,
			Island *restrictToIsland = NULL );
			
		void setRenderVoxelsInDebug( bool rv ) { _renderVoxelsInDebug = rv; }
		bool renderVoxelsInDebug() const { return _renderVoxelsInDebug; }
				
	protected:
			
		void _markDeferredGeometryUpdateNeeded();
			
		/**
			
			Implemented in Terrain_cutting.cpp
		*/
		void _partitionIslands( std::set< Island* > &islands );

		/**
			
			Implemented in Terrain_cutting.cpp
		*/
		void _partitionIsland( Island *island, std::set< Island* > &newIslands );
		
		/**
			Gathers all islands into _allIslands vector
		*/
		void _gatherAllIslands();
		
		/**
			Given a set of Islands in @a all, populates _staticGroup with all the islands which are not dynamic,
			and populates _dynamicGroups with groups of islands which are dynamic.
		*/
		void _createIslandGroups( std::set< Island* > all );
		
		/**
			Given a set of new Islands created by a cut, adds static island groups to _staticGroup, and adds new 
			dynamic island groups to _dynamicGroups, or if any new Islands are part of an existing group, adds them
			to that group.	
		*/
		void _updateIslandGroups( std::set< Island* > newIslands );

		/**
			Create a ci::Surface (which will be used as source for a ci::gl::Texture ) which will 
			modulate the color of the rendered geometry to denote underlying voxel strength.
		*/
		ci::Surface _createModulationSurface( const ci::Surface &levelImage ) const;


		void _prepareBatchDraw_Game( const core::render_state &state );
		void _cleanupAfterBatchDraw_Game( const core::render_state &state );

		void _prepareBatchDraw_Development( const core::render_state &state );
		void _cleanupAfterBatchDraw_Development( const core::render_state &state );

		void _prepareBatchDraw_Debug( const core::render_state &state );
		void _cleanupAfterBatchDraw_Debug( const core::render_state &state );

		
		/**
			Up-scale Vec2 to Vec2i for storage in std::set
		*/
		static inline Vec2i _upscalePosition( const Vec2 &position )
		{
			return Vec2i( position.x * 32, position.y * 32 );
		}

		/**
			Downscale Vec2i scaled up with _upscalePosition when retreiving from std::set
		*/
		static inline Vec2 _downscalePosition( const Vec2i &upscaledPosition )
		{
			return Vec2( upscaledPosition.x / real(32), upscaledPosition.y / real(32) );
		}
			
	protected:
	
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

		Vec2rVec _chunkedCuttingLine, _touchedVoxelWorldPositions;	
		
		std::map< TerrainCutType::cut_type, Vec2iSet > _touchedScaledWorldPositionsByCut;
		
		bool _renderVoxelsInDebug;
};

}} // end namespace core::terrain
