//
//  TerrainWorld.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/1/17.
//
//

#ifndef TerrainWorld_hpp
#define TerrainWorld_hpp

#include <cinder/app/App.h>
#include <boost/functional/hash.hpp>
#include <unordered_set>

#include "Core.hpp"

using namespace ci;
using namespace std;

namespace terrain {

	SMART_PTR(World);
	SMART_PTR(GroupBase);
	SMART_PTR(StaticGroup);
	SMART_PTR(DynamicGroup);
	SMART_PTR(Shape);
	SMART_PTR(Anchor);


	/**
	 poly_edge represents an edge in a PolyLine2f. It is not meant to represent the specific vertices in
	 a coordinate system but rather it is meant to make it easy to determine if two PolyShapes share an edge.
	 As such the coordinates a and b are integers, representing the world space coords of the PolyShape outer
	 contour at a given level of precision.
	 */
	struct poly_edge {
		static const int SNAP_CELL_SIZE = 2;
		ivec2 a, b;

		poly_edge(vec2 m, vec2 n) {
			a.x = static_cast<int>(lround(m.x / SNAP_CELL_SIZE));
			a.y = static_cast<int>(lround(m.y / SNAP_CELL_SIZE));
			b.x = static_cast<int>(lround(n.x / SNAP_CELL_SIZE));
			b.y = static_cast<int>(lround(n.y / SNAP_CELL_SIZE));

			// we want consistent ordering
			if (a.x == b.x) {
				if (b.y < a.y) {
					swap(a,b);
				}
			} else {
				if (b.x < a.x) {
					swap(a,b);
				}
			}
		}

		friend bool operator == (const poly_edge &e0, const poly_edge &e1) {
			return e0.a == e1.a && e0.b == e1.b;
		}

		friend bool operator < (const poly_edge &e0, const poly_edge &e1) {
			if (e0.a.x != e1.a.x) {
				return e0.a.x < e1.a.x;
			} else if (e0.a.y != e1.a.y) {
				return e0.a.y < e1.a.y;
			} else if (e0.b.x != e1.b.x) {
				return e0.b.x < e1.b.x;
			}
			return e0.b.y < e1.b.y;
		}

		friend std::ostream& operator<<( std::ostream& lhs, const poly_edge& rhs ) {
			return lhs << "(poly_edge " << vec2(rhs.a.x * SNAP_CELL_SIZE, rhs.a.y * SNAP_CELL_SIZE) << " : " << vec2(rhs.b.x * SNAP_CELL_SIZE, rhs.b.y * SNAP_CELL_SIZE) << ")";
		}
	};

}


#pragma mark -

namespace std {

	template <>
	struct hash<terrain::poly_edge> {

		// make island::poly_edge hashable
		std::size_t operator()(const terrain::poly_edge& e) const {
			std::size_t seed = 0;
			boost::hash_combine(seed, e.a.x);
			boost::hash_combine(seed, e.a.y);
			boost::hash_combine(seed, e.b.x);
			boost::hash_combine(seed, e.b.y);
			return seed;
		}
	};

}



namespace terrain {

#pragma mark - Material

	/**
	 Describes basic physics material properties for a collision shape
	 */
	struct material {
		float density;
		float friction;
		cpShapeFilter filter;

		material(float d, float f, cpShapeFilter flt):
		density(d),
		friction(f),
		filter(flt)
		{}
	};

#pragma mark - World


	/**
	 @class World
	 World "owns" and manages Group instances, which in turn own and manage Shape instances.
	 */
	class World {
	public:

		/**
		 Partitions shapes in `shapes to a grid with origin at partitionOrigin, with chunks of size paritionSize.
		 The purpose of this is simple: You might have a large level. You want to divide the geometry of the level into manageable
		 chunks for visibility culling and collision/physics performance.
		 */
		static vector<ShapeRef> partition(const vector<ShapeRef> &shapes, vec2 partitionOrigin, float partitionSize);

	public:
		World(cpSpace *space, material m);
		~World();

		/**
		 Build a world of dynamic and static shapes. Any shape overlapping an Anchor will be static.
		 Pieces cut off a static shape will become dynamic provided they don't overlap another anchor.
		 */

		void build(const vector<ShapeRef> &shapes, const vector<AnchorRef> &anchors = vector<AnchorRef>());

		/**
		 Perform a cut in world space from a to b, with half-thickness of radius
		 */
		void cut(vec2 a, vec2 b, float radius, cpShapeFilter filter);

		void draw(const core::render_state &renderState);
		void step(const core::time_state &timeState);
		void update(const core::time_state &timeState);

		const set<DynamicGroupRef> getDynamicGroups() const { return _dynamicGroups; }
		StaticGroupRef getStaticGroup() const { return _staticGroup; }
		const vector<AnchorRef> getAnchors() const { return _anchors; }

	protected:

		void build(const vector<ShapeRef> &shapes, const map<ShapeRef,GroupBaseRef> &parentage);
		vector<set<ShapeRef>> findShapeGroups(const vector<ShapeRef> &shapes, const map<ShapeRef,GroupBaseRef> &parentage);
		bool isShapeGroupStatic(const set<ShapeRef> shapeGroup, const GroupBaseRef &parentGroup);

		void drawGame(const core::render_state &renderState);
		void drawDebug(const core::render_state &renderState);

	private:

		material _material;
		cpSpace *_space;
		StaticGroupRef _staticGroup;
		set<DynamicGroupRef> _dynamicGroups;
		vector<AnchorRef> _anchors;
	};


#pragma mark - GroupBase


	class GroupBase {
	public:
		virtual ~GroupBase();

		virtual string getName() const { return _name; }
		virtual Color getColor() const { return _color; }
		virtual const cpHashValue getHash() const { return _hash; }

		virtual cpBody* getBody() const = 0;
		virtual cpBB getBB() const = 0;
		virtual mat4 getModelview() const = 0;
		virtual mat4 getModelviewInverse() const = 0;
		virtual vec2 getPosition() const = 0;
		virtual float getAngle() const = 0;
		virtual set<ShapeRef> getShapes() const = 0;
		virtual void draw(const core::render_state &renderState) = 0;
		virtual void step(const core::time_state &timeState) = 0;
		virtual void update(const core::time_state &timeState) = 0;

		cpTransform getModelviewTransform() const {
			mat4 mv = getModelview();
			return cpTransform { mv[0].x, mv[0].y, mv[1].x, mv[1].y, mv[3].x, mv[3].y };
		}

		cpTransform getModelviewInverseTransform() const {
			mat4 mvi = getModelviewInverse();
			return cpTransform { mvi[0].x, mvi[0].y, mvi[1].x, mvi[1].y, mvi[3].x, mvi[3].y };
		}

	protected:
		string _name;
		Color _color;
		cpHashValue _hash;

	};

#pragma mark - StaticGroup

	class StaticGroup : public GroupBase, public enable_shared_from_this<StaticGroup> {
	public:
		StaticGroup(cpSpace *space, material m);
		virtual ~StaticGroup();

		virtual cpBody* getBody() const override { return _body; }
		virtual cpBB getBB() const override;
		virtual mat4 getModelview() const override { return mat4(1); }
		virtual mat4 getModelviewInverse() const override { return mat4(1); }
		virtual vec2 getPosition() const override { return vec2(0); }
		virtual float getAngle() const override { return 0; }
		virtual set<ShapeRef> getShapes() const override { return _shapes; }

		virtual void draw(const core::render_state &renderState) override {}
		virtual void step(const core::time_state &timeState) override {}
		virtual void update(const core::time_state &timeState) override {}

		void addShape(ShapeRef shape);
		void removeShape(ShapeRef shape);

	protected:

		void updateWorldBB();

	protected:
		cpSpace *_space;
		cpBody *_body;
		material _material;
		set<ShapeRef> _shapes;
		mutable cpBB _worldBB;
	};


#pragma mark - DynamicGroup


	class DynamicGroup : public GroupBase, public enable_shared_from_this<DynamicGroup>{
	public:
		DynamicGroup(cpSpace *space, material m);
		virtual ~DynamicGroup();

		virtual string getName() const override;
		virtual cpBody* getBody() const override { return _body; }
		cpBB getBB() const override { return _worldBB; }
		virtual mat4 getModelview() const override { return _modelview; }
		virtual mat4 getModelviewInverse() const override { return _modelviewInverse; }
		virtual vec2 getPosition() const override { return v2(_position); }
		virtual float getAngle() const override { return static_cast<float>(_angle); }
		virtual set<ShapeRef> getShapes() const override { return _shapes; }

		virtual void draw(const core::render_state &renderState) override;
		virtual void step(const core::time_state &timeState) override;
		virtual void update(const core::time_state &timeState) override;


	protected:
		friend class World;

		string nextId();
		bool build(set<ShapeRef> shapes, const GroupBaseRef &parentGroup);
		void syncToCpBody();

	private:
		static size_t _count;

		material _material;
		cpSpace *_space;
		cpBody *_body;
		cpVect _position;
		cpFloat _angle;
		cpBB _worldBB, _modelBB;
		mat4 _modelview, _modelviewInverse;

		set<ShapeRef> _shapes;
	};


#pragma mark - Anchor

	/**
	 @class Anchor
	 An Anchor is a single contour in world space which "anchors" shapes to be static and not dynamic bodies.
	 A body is static if one of its shapes overlaps an anchor, and if the body's parentage has never been dynamic.
	 This is to say, once a shape is severed from a static body and becomes dynamic, it can never be static again.
	 */
	class Anchor : public enable_shared_from_this<Anchor> {
	public:

		static AnchorRef fromContour(const PolyLine2f &contour, material m) {
			return make_shared<Anchor>(contour, m);
		}

	public:

		Anchor(const PolyLine2f &contour, material m);
		~Anchor();

		const PolyLine2f &getContour() const { return _contour; }
		const cpBB getBB() const { return _bb; }
		const TriMeshRef &getTriMesh() const { return _trimesh; }

	protected:

		friend class World;
		bool build(cpSpace *space);

	private:

		cpBody *_staticBody;
		vector<cpShape*> _shapes;

		cpBB _bb;
		material _material;
		PolyLine2f _contour;
		TriMeshRef _trimesh;

	};

#pragma mark - Shape

	class Shape : public enable_shared_from_this<Shape>{
	public:

		struct contour_pair {
			PolyLine2f world, model;

			// when constructed model and world are both equal
			contour_pair(const PolyLine2f &w):
			world(w),
			model(w)
			{}
		};

		static ShapeRef fromContour(const PolyLine2f outerContour);
		static vector<ShapeRef> fromContours(const vector<PolyLine2f> &contourSoup);
		static vector<ShapeRef> fromShapes(const vector<Shape2d> &shapeSoup);

	public:

		Shape(const PolyLine2f &shapeContour);
		Shape(const PolyLine2f &shapeContour, const std::vector<PolyLine2f> &holeContours);

		~Shape();

		const string &getName() const { return _name; }
		const cpHashValue getHash() const { return _hash; }
		const contour_pair &getOuterContour() const { return _outerContour; }
		const vector<contour_pair> &getHoleContours() const { return _holeContours; }

		GroupBaseRef getGroup() const { return _group.lock(); }

		const mat4 getModelview() const {
			if (GroupBaseRef group = _group.lock()) {
				return group->getModelview();
			} else {
				return mat4();
			}
		}

		const mat4 getModelviewInverse() const {
			if (GroupBaseRef group = _group.lock()) {
				return group->getModelviewInverse();
			} else {
				return mat4();
			}
		}

		const vector<cpShape*> &getShapes(cpBB &shapesModelBB) const {
			shapesModelBB = _shapesBB;
			return _shapes;
		}

		const TriMeshRef &getTriMesh() const { return _trimesh; }

		bool hasValidTriMesh() const {
			return _trimesh && _trimesh->getNumTriangles() > 0;
		}

		const unordered_set<poly_edge> &getWorldSpaceContourEdges();
		cpBB getWorldSpaceContourEdgesBB();

		vector<ShapeRef> subtract(const PolyLine2f &contourToSubtract) const;

	protected:

		friend class StaticGroup;
		friend class DynamicGroup;
		friend class World;

		string nextId();
		void updateWorldSpaceContourAndBB();
		void setGroup(GroupBaseRef group) { _group = group; }

		// build the trimesh, returning true iff we got > 0 triangles
		bool triangulate();

		float computeArea();
		void computeMassAndMoment(float density, float &mass, float &moment, float &area);

		void destroyCollisionShapes();
		const vector<cpShape*> &createCollisionShapes(cpBody *body, cpBB &shapesModelBB);

	private:

		static size_t _count;

		bool _worldSpaceShapeContourEdgesDirty;
		cpHashValue _hash;
		string _name;
		contour_pair _outerContour;
		vector<contour_pair> _holeContours;
		TriMeshRef _trimesh;
		vec2 _modelCentroid;

		cpBB _shapesBB;
		vector<cpShape*> _shapes;
		GroupBaseWeakRef _group;
		
		unordered_set<poly_edge> _worldSpaceContourEdges;
		cpBB _worldSpaceContourEdgesBB;
	};
	
}

#endif /* TerrainWorld_hpp */
