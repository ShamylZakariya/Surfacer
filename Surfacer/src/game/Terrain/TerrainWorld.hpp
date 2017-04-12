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

namespace terrain {

	SMART_PTR(World);
	SMART_PTR(GroupBase);
	SMART_PTR(StaticGroup);
	SMART_PTR(DynamicGroup);
	SMART_PTR(Drawable);
	SMART_PTR(Shape);
	SMART_PTR(Anchor);

	/**
	 If we're considering 1 unit to be 1m, then a precision of 100 means we have a precision of 1cm, e.g.,
	 poly_edges whos' vertices are within 1cm of eachother will be considered congruent. A precision of 1000 would
	 increase precision such that only edges with vertices within one mm of one another would be congruent.
	 */

	#define POLY_EDGE_PRECISION 100.0

	/**
	 poly_edge represents an edge in a PolyLine2d. It is not meant to represent the specific vertices in
	 a coordinate system but rather it is meant to make it easy to determine if two PolyShapes share an edge.
	 As such the coordinates a and b are integers, representing the world space coords of the PolyShape outer
	 contour at a given level of precision.
	 */
	struct poly_edge {
		ivec2 a, b;

		poly_edge(dvec2 m, dvec2 n) {
			a.x = static_cast<int>(lround(m.x * POLY_EDGE_PRECISION));
			a.y = static_cast<int>(lround(m.y * POLY_EDGE_PRECISION));
			b.x = static_cast<int>(lround(n.x * POLY_EDGE_PRECISION));
			b.y = static_cast<int>(lround(n.y * POLY_EDGE_PRECISION));

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
			return lhs << "(poly_edge " << dvec2(rhs.a.x / POLY_EDGE_PRECISION, rhs.a.y / POLY_EDGE_PRECISION)
				<< " : " << dvec2(rhs.b.x / POLY_EDGE_PRECISION, rhs.b.y / POLY_EDGE_PRECISION) << ")";
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
		cpFloat density;
		cpFloat friction;
		cpShapeFilter filter;

		material():
		density(1),
		friction(1),
		filter({0,0,0})
		{}

		material(cpFloat d, cpFloat f, cpShapeFilter flt):
		density(d),
		friction(f),
		filter(flt)
		{}
	};

#pragma mark - DrawDispatcher

	class DrawDispatcher {
	public:

		struct collector
		{
			set<DrawableRef> visible;
			vector<DrawableRef> sorted;

			void remove( const DrawableRef &s  )
			{
				visible.erase(s);
				sorted.erase(std::remove(sorted.begin(),sorted.end(),s), sorted.end());
			}
		};

		DrawDispatcher();
		virtual ~DrawDispatcher();

		void add( const DrawableRef & );
		void remove( const DrawableRef & );
		void moved( const DrawableRef & );
		void moved( Drawable* );

		void cull( const core::render_state & );
		void draw( const core::render_state &, const ci::gl::GlslProgRef &shader);

		void setDrawPasses(size_t passes) { _drawPasses = passes; }
		size_t getPasses() const { return _drawPasses; }

		size_t visibleCount() const { return _collector.sorted.size(); }
		const set<DrawableRef> &getVisible() const { return _collector.visible; }
		const vector<DrawableRef> &getVisibleSorted() const { return _collector.sorted; }

	private:

		/**
			render a run of shapes belonging to a common group
			returns iterator to last shape drawn
		 */
		vector<DrawableRef>::iterator _drawGroupRun( vector<DrawableRef>::iterator first, vector<DrawableRef>::iterator storageEnd, const core::render_state &state, const ci::gl::GlslProgRef &shader );

	private:

		cpSpatialIndex *_index;
		set<DrawableRef> _all;
		collector _collector;
		size_t _drawPasses;
		
	};



#pragma mark - World


	/**
	 @class World
	 World "owns" and manages Group instances, which in turn own and manage Shape instances.
	 */
	class World : public enable_shared_from_this<World> {
	public:

		static void loadSvg(ci::DataSourceRef svgData, dmat4 transform, vector<ShapeRef> &shapes, vector<AnchorRef> &anchors, bool flip = true);

		/**
		 Partitions shapes in `shapes to a grid with origin at partitionOrigin, with chunks of size paritionSize.
		 The purpose of this is simple: You might have a large level. You want to divide the geometry of the level into manageable
		 chunks for visibility culling and collision/physics performance.
		 */
		static vector<ShapeRef> partition(const vector<ShapeRef> &shapes, dvec2 partitionOrigin, double partitionSize);

		static size_t nextId() {
			return _idCounter++;
		}

	public:
		World(cpSpace *space, material worldMaterial, material anchorMaterial);
		~World();

		/**
		 Build a world of dynamic and static shapes. Any shape overlapping an Anchor will be static.
		 Pieces cut off a static shape will become dynamic provided they don't overlap another anchor.
		 */

		void build(const vector<ShapeRef> &shapes, const vector<AnchorRef> &anchors = vector<AnchorRef>());

		/**
		 Perform a cut in world space from a to b, with half-thickness of radius
		 */
		void cut(dvec2 a, dvec2 b, double radius, cpShapeFilter filter);

		void draw(const core::render_state &renderState);
		void step(const core::time_state &timeState);
		void update(const core::time_state &timeState);

		const set<DynamicGroupRef> getDynamicGroups() const { return _dynamicGroups; }
		StaticGroupRef getStaticGroup() const { return _staticGroup; }
		const vector<AnchorRef> getAnchors() const { return _anchors; }

		DrawDispatcher &getDrawDispatcher() { return _drawDispatcher; }
		const DrawDispatcher &getDrawDispatcher() const { return _drawDispatcher; }

	protected:

		void build(const vector<ShapeRef> &shapes, const map<ShapeRef,GroupBaseRef> &parentage);
		vector<set<ShapeRef>> findShapeGroups(const vector<ShapeRef> &shapes, const map<ShapeRef,GroupBaseRef> &parentage);
		bool isShapeGroupStatic(const set<ShapeRef> shapeGroup, const GroupBaseRef &parentGroup);

	private:

		static size_t _idCounter;

		material _worldMaterial, _anchorMaterial;
		cpSpace *_space;
		StaticGroupRef _staticGroup;
		set<DynamicGroupRef> _dynamicGroups;
		vector<AnchorRef> _anchors;

		DrawDispatcher _drawDispatcher;
		ci::gl::GlslProgRef _shader;
	};


#pragma mark - GroupBase


	class GroupBase {
	public:

		GroupBase(cpSpace *space, material m, DrawDispatcher &dispatcher);
		virtual ~GroupBase();

		DrawDispatcher &getDrawDispatcher() const { return _drawDispatcher; }
		cpSpace *getSpace() const { return _space; }
		const material &getMaterial() const { return _material; }
		virtual string getName() const { return _name; }
		virtual Color getColor() const { return _color; }
		virtual cpHashValue getHash() const { return _hash; }

		virtual cpBody* getBody() const = 0;
		virtual cpBB getBB() const = 0;
		virtual dmat4 getModelview() const = 0;
		virtual dmat4 getModelviewInverse() const = 0;
		virtual dvec2 getPosition() const = 0;
		virtual double getAngle() const = 0;
		virtual set<ShapeRef> getShapes() const = 0;
		virtual void releaseShapes() = 0;
		virtual size_t getDrawingBatchId() const { return _drawingBatchId;}
		virtual void draw(const core::render_state &renderState) = 0;
		virtual void step(const core::time_state &timeState) = 0;
		virtual void update(const core::time_state &timeState) = 0;


		cpTransform getModelviewTransform() const {
			dmat4 mv = getModelview();
			return cpTransform { mv[0].x, mv[0].y, mv[1].x, mv[1].y, mv[3].x, mv[3].y };
		}

		cpTransform getModelviewInverseTransform() const {
			dmat4 mvi = getModelviewInverse();
			return cpTransform { mvi[0].x, mvi[0].y, mvi[1].x, mvi[1].y, mvi[3].x, mvi[3].y };
		}

	protected:

		DrawDispatcher &_drawDispatcher;
		size_t _drawingBatchId;
		cpSpace *_space;
		material _material;
		string _name;
		Color _color;
		cpHashValue _hash;

	};

#pragma mark - StaticGroup

	class StaticGroup : public GroupBase, public enable_shared_from_this<StaticGroup> {
	public:
		StaticGroup(cpSpace *space, material m, DrawDispatcher &dispatcher);
		virtual ~StaticGroup();

		virtual cpBody* getBody() const override { return _body; }
		virtual cpBB getBB() const override;
		virtual dmat4 getModelview() const override { return dmat4(1); }
		virtual dmat4 getModelviewInverse() const override { return dmat4(1); }
		virtual dvec2 getPosition() const override { return dvec2(0); }
		virtual double getAngle() const override { return 0; }
		virtual set<ShapeRef> getShapes() const override { return _shapes; }
		virtual void releaseShapes() override;

		virtual void draw(const core::render_state &renderState) override {}
		virtual void step(const core::time_state &timeState) override {}
		virtual void update(const core::time_state &timeState) override {}

		void addShape(ShapeRef shape);
		void removeShape(ShapeRef shape);

	protected:

		void updateWorldBB();

	protected:
		cpBody *_body;
		set<ShapeRef> _shapes;
		mutable cpBB _worldBB;
	};


#pragma mark - DynamicGroup


	class DynamicGroup : public GroupBase, public enable_shared_from_this<DynamicGroup>{
	public:
		DynamicGroup(cpSpace *space, material m, DrawDispatcher &dispatcher);
		virtual ~DynamicGroup();

		virtual string getName() const override;
		virtual cpBody* getBody() const override { return _body; }
		cpBB getBB() const override { return _worldBB; }
		virtual dmat4 getModelview() const override { return _modelview; }
		virtual dmat4 getModelviewInverse() const override { return _modelviewInverse; }
		virtual dvec2 getPosition() const override { return v2(_position); }
		virtual double getAngle() const override { return static_cast<double>(_angle); }
		virtual set<ShapeRef> getShapes() const override { return _shapes; }
		virtual void releaseShapes() override;

		virtual void draw(const core::render_state &renderState) override;
		virtual void step(const core::time_state &timeState) override;
		virtual void update(const core::time_state &timeState) override;


	protected:
		friend class World;

		bool build(set<ShapeRef> shapes, const GroupBaseRef &parentGroup);
		void syncToCpBody();

	private:

		cpBody *_body;
		cpVect _position;
		cpFloat _angle;
		cpBB _worldBB, _modelBB;
		dmat4 _modelview, _modelviewInverse;

		set<ShapeRef> _shapes;
	};

#pragma mark - Drawable

	class Drawable : public enable_shared_from_this<Drawable> {
	public:
		Drawable();
		virtual ~Drawable();

		size_t getId() const { return _id; }

		virtual cpBB getBB() const = 0;
		virtual size_t getDrawingBatchId() const = 0;
		virtual size_t getLayer() const = 0;
		virtual dmat4 getModelview() const = 0;
		virtual double getAngle() const = 0;
		virtual dvec2 getModelCentroid() const = 0;
		virtual const TriMeshRef &getTriMesh() const = 0;
		virtual const gl::VboMeshRef &getVboMesh() const = 0;
		virtual Color getColor() const = 0;

		// get typed shared_from_this, e.g., shared_ptr<Shape> = shared_from_this<Shape>();
		template<typename T>
		shared_ptr<T> shared_from_this() const {
			return dynamic_pointer_cast<T>(enable_shared_from_this<Drawable>::shared_from_this());
		}

		// get typed shared_from_this, e.g., shared_ptr<Shape> = shared_from_this<Shape>();
		template<typename T>
		shared_ptr<T> shared_from_this() {
			return dynamic_pointer_cast<T>(enable_shared_from_this<Drawable>::shared_from_this());
		}

	private:
		size_t _id;
	};


#pragma mark - Anchor

	/**
	 @class Anchor
	 An Anchor is a single contour in world space which "anchors" shapes to be static and not dynamic bodies.
	 A body is static if one of its shapes overlaps an anchor, and if the body's parentage has never been dynamic.
	 This is to say, once a shape is severed from a static body and becomes dynamic, it can never be static again.
	 */
	class Anchor : public Drawable {
	public:

		static const size_t DRAWING_BATCH_ID = ~0;
		static const size_t LAYER = 1;

		static AnchorRef fromContour(const PolyLine2d &contour) {
			return make_shared<Anchor>(contour);
		}

		static vector<AnchorRef> fromContours(const vector<PolyLine2d> &contours) {
			vector<AnchorRef> anchors;
			anchors.reserve(contours.size());
			for (auto c : contours) {
				anchors.push_back(make_shared<Anchor>(c));
			}
			return anchors;
		}

		static AnchorRef fromShape(const Shape2d &shape);

	public:

		Anchor(const PolyLine2d &contour);
		~Anchor();

		const PolyLine2d &getContour() const { return _contour; }

		cpBB getBB() const override { return _bb; }
		size_t getDrawingBatchId() const override { return DRAWING_BATCH_ID; }
		size_t getLayer() const override { return LAYER; }
		dmat4 getModelview() const override { return dmat4(1); } // identity
		double getAngle() const override { return 0; };
		dvec2 getModelCentroid() const override;
		const TriMeshRef &getTriMesh() const override { return _trimesh; }
		const gl::VboMeshRef &getVboMesh() const override { return _vboMesh; }
		Color getColor() const override { return Color(0,0,0); }

	protected:

		friend class World;
		bool build(cpSpace *space, material m);

	private:

		cpBody *_staticBody;
		vector<cpShape*> _shapes;

		cpBB _bb;
		material _material;
		PolyLine2d _contour;

		TriMeshRef _trimesh;
		ci::gl::VboMeshRef _vboMesh;

	};

#pragma mark - Shape

	class Shape : public Drawable {
	public:

		static const size_t LAYER = 0;

		struct contour_pair {
			PolyLine2d world, model;

			// when constructed model and world are both equal
			contour_pair(const PolyLine2d &w):
			world(w),
			model(w)
			{}
		};

		static ShapeRef fromContour(const PolyLine2d &outerContour);
		static vector<ShapeRef> fromContours(const vector<PolyLine2d> &contourSoup);
		static vector<ShapeRef> fromShapes(const vector<Shape2d> &shapeSoup);

	public:

		Shape(const PolyLine2d &shapeContour);
		Shape(const PolyLine2d &shapeContour, const std::vector<PolyLine2d> &holeContours);

		~Shape();

		const contour_pair &getOuterContour() const { return _outerContour; }
		const vector<contour_pair> &getHoleContours() const { return _holeContours; }

		GroupBaseRef getGroup() const { return _group.lock(); }
		cpHashValue getGroupHash() const { return _groupHash; }

		dmat4 getModelview() const override {
			if (GroupBaseRef group = _group.lock()) {
				return group->getModelview();
			} else {
				return dmat4();
			}
		}

		dmat4 getModelviewInverse() const {
			if (GroupBaseRef group = _group.lock()) {
				return group->getModelviewInverse();
			} else {
				return dmat4();
			}
		}

		const vector<cpShape*> &getShapes(cpBB &shapesModelBB) const {
			shapesModelBB = _shapesModelBB;
			return _shapes;
		}

		bool hasValidTriMesh() const {
			return _trimesh && _trimesh->getNumTriangles() > 0;
		}

		cpBB getBB() const override;

		size_t getDrawingBatchId() const override {
			return _groupDrawingBatchId;
		}

		size_t getLayer() const override { return LAYER; }

		double getAngle() const override { return getGroup()->getAngle(); }
		dvec2 getModelCentroid() const override { return _modelCentroid; }

		const TriMeshRef &getTriMesh() const override { return _trimesh; }
		const gl::VboMeshRef &getVboMesh() const override { return _vboMesh; }

		Color getColor() const override { return getGroup()->getColor(); }

		const unordered_set<poly_edge> &getWorldSpaceContourEdges();
		cpBB getWorldSpaceContourEdgesBB();

		vector<ShapeRef> subtract(const PolyLine2d &contourToSubtract) const;


	protected:

		friend class StaticGroup;
		friend class DynamicGroup;
		friend class World;

		void updateWorldSpaceContourAndBB();
		void setGroup(GroupBaseRef group);

		// build the trimesh, returning true iff we got > 0 triangles
		bool triangulate();

		double computeArea();
		void computeMassAndMoment(double density, double &mass, double &moment, double &area);

		void destroyCollisionShapes();
		const vector<cpShape*> &createCollisionShapes(cpBody *body, cpBB &shapesModelBB);

	private:

		bool _worldSpaceShapeContourEdgesDirty;
		contour_pair _outerContour;
		vector<contour_pair> _holeContours;
		dvec2 _modelCentroid;

		cpBB _shapesModelBB;
		vector<cpShape*> _shapes;
		GroupBaseWeakRef _group;
		size_t _groupDrawingBatchId;
		cpHashValue _groupHash;
		
		unordered_set<poly_edge> _worldSpaceContourEdges;
		cpBB _worldSpaceContourEdgesBB;

		TriMeshRef _trimesh;
		ci::gl::VboMeshRef _vboMesh;
	};
	
}

#endif /* TerrainWorld_hpp */
