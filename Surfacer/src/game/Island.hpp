//
//  Island.hpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 3/1/17.
//
//

#ifndef Island_hpp
#define Island_hpp

#include <cinder/app/App.h>
#include <boost/functional/hash.hpp>
#include <unordered_set>

#include "Core.hpp"

using namespace ci;
using namespace std;

namespace island {

	SMART_PTR(World);
	SMART_PTR(Body);
	SMART_PTR(Shape);
	SMART_PTR(Anchor);


	/**
	 poly_edge represents an edge in a PolyLine2f. It is not meant to represent the specific vertices in
	 a coordinate system but rather it is meant to make it easy to determine if two PolyShapes share an edge.
	 As such the coordinates a and b are integers, representing the world space coords of the PolyShape outer
	 contour at a given level of precision.
	 */
	struct poly_edge {
		static const int PRECISION = 0;
		ivec2 a, b;

		poly_edge(vec2 m, vec2 n) {
			const int power = powi(10, PRECISION);
			a.x = static_cast<int>(lround(m.x * power));
			a.y = static_cast<int>(lround(m.y * power));
			b.x = static_cast<int>(lround(n.x * power));
			b.y = static_cast<int>(lround(n.y * power));

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
			float scale = powi(10, PRECISION);
			return lhs << "(poly_edge " << vec2(rhs.a.x/scale, rhs.a.y/scale) << " : " << vec2(rhs.b.x/scale, rhs.b.y/scale) << ")";
		}
	};

}


#pragma mark -

namespace std {

	template <>
	struct hash<island::poly_edge> {

		// make island::poly_edge hashable
		std::size_t operator()(const island::poly_edge& e) const {
			std::size_t seed = 0;
			boost::hash_combine(seed, e.a.x);
			boost::hash_combine(seed, e.a.y);
			boost::hash_combine(seed, e.b.x);
			boost::hash_combine(seed, e.b.y);
			return seed;
		}
	};

}


#pragma mark -

namespace island {

	/**
	 Describes basic physics material properties for a collision shape
	*/
	struct material {
		float density;
		float friction;
		cpShapeFilter filter;
	};


	/**
	@class World
	World "owns" and manages Body instances, which in turn own and manage Shape instances.
	*/
	class World {
	public:
		World(cpSpace *space, material m);
		~World();

		/**
		 Build a world of dynamic shapes.
		 */
		void build(const vector<ShapeRef> &shapes);

		/**
		 Build a world of dynamic and static shapes. Any shape overlapping an Anchor will be static. 
		 Pieces cut off a static shape will become dynamic provided they don't overlap another anchor.
		 */

		void build(const vector<ShapeRef> &shapes, const vector<AnchorRef> &anchors);

		/**
		Perform a cut in world space from a to b, with half-thickness of radius
		 */
		void cut(vec2 a, vec2 b, float radius);

		void draw(const core::render_state &renderState);
		void step(const core::time_state &timeState);
		void update(const core::time_state &timeState);

		const set<BodyRef> getBodies() const { return _bodies; }
		const vector<AnchorRef> getAnchors() const { return _anchors; }

	protected:

		void build(const vector<ShapeRef> &shapes, const map<ShapeRef,BodyRef> &parentage);

		void drawGame(const core::render_state &renderState);
		void drawDebug(const core::render_state &renderState);

	private:

		material _material;
		cpSpace *_space;
		set<BodyRef> _bodies;
		vector<AnchorRef> _anchors;
	};



	class Body : public enable_shared_from_this<Body>{
	public:
		Body(cpSpace *space, material m);
		~Body();

		string getName() const;
		const cpHashValue getHash() const { return _hash; }
		bool isDynamic() const { return _dynamic; }

		set<ShapeRef> getShapes() const { return _shapes; }

		const mat4 getModelview() const { return _modelview; }

		const cpTransform getModelviewTransform() const {
			return cpTransform { _modelview[0].x, _modelview[0].y, _modelview[1].x, _modelview[1].y, _modelview[3].x, _modelview[3].y };
		}

		const mat4 getModelviewInverse() const { return _modelviewInverse; }

		const cpTransform getModelviewInverseTransform() const {
			return cpTransform { _modelviewInverse[0].x, _modelviewInverse[0].y, _modelviewInverse[1].x, _modelviewInverse[1].y, _modelviewInverse[3].x, _modelviewInverse[3].y };
		}


		vec2 getPosition() const { return v2(_position); }
		float getAngle() const { return static_cast<float>(_angle); }

		cpSpace *getSpace() const { return _space; }
		cpBody* getBody() const { return _body; }
		cpBB getBB() const { return _worldBB; }

		void draw(const core::render_state &renderState);
		void step(const core::time_state &timeState);
		void update(const core::time_state &timeState);


	protected:
		friend class World;

		string nextId();
		bool build(set<ShapeRef> shapes, const BodyRef parentBody, const vector<AnchorRef> &anchors);
		void syncToCpBody();

	private:
		static size_t _count;

		bool _dynamic;
		material _material;
		cpSpace *_space;
		cpBody *_body;
		cpVect _position;
		cpFloat _angle;
		cpBB _worldBB, _modelBB;
		mat4 _modelview, _modelviewInverse;

		set<ShapeRef> _shapes;

		string _name;
		cpHashValue _hash;
	};



	/**
	@class Anchor
	 An Anchor is a single contour in world space which "anchors" shapes to be static and not dynamic bodies.
	 A body is static if one of its shapes overlaps an anchor, and if the body's parentage has never been dynamic.
	 This is to say, once a shape is severed from a static body and becomes dynamic, it can never be static again.
	 */
	class Anchor : public enable_shared_from_this<Anchor> {
	public:

		static AnchorRef fromContour(const PolyLine2f &contour) {
			return make_shared<Anchor>(contour);
		}

	public:

		Anchor(const PolyLine2f &contour);

		const PolyLine2f &getContour() const { return _contour; }
		const cpBB getBB() const { return _bb; }
		const TriMeshRef &getTriMesh() const { return _trimesh; }

	private:

		cpBB _bb;
		PolyLine2f _contour;
		TriMeshRef _trimesh;


	};


	class Shape : public enable_shared_from_this<Shape>{
	public:

		struct contour_pair {
			PolyLine2f world, model;

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
		Color getColor() const { return _color; }
		const contour_pair &getOuterContour() const { return _outerContour; }
		const vector<contour_pair> &getHoleContours() const { return _holeContours; }

		BodyRef getBody() const { return _body.lock(); }

		const mat4 getModelview() const {
			if (BodyRef body = _body.lock()) {
				return body->getModelview();
			} else {
				return mat4();
			}
		}

		const mat4 getModelviewInverse() const {
			if (BodyRef body = _body.lock()) {
				return body->getModelviewInverse();
			} else {
				return mat4();
			}
		}

		cpSpace *getSpace() const {
			if (BodyRef body = _body.lock()) {
				return body->getSpace();
			} else {
				return nullptr;
			}
		}

		const vector<cpShape*> &getShapes() const { return _shapes; }
		const TriMeshRef &getTriMesh() const { return _trimesh; }

		const unordered_set<poly_edge> &getWorldSpaceContourEdges();
		cpBB getWorldSpaceContourEdgesBB();

		vector<ShapeRef> subtract(const PolyLine2f &contourToSubtract) const;

	protected:

		friend class Body;
		friend class World;

		string nextId();
		void updateWorldSpaceContourAndBB();
		void setIslandBody(BodyRef body);

		// build the trimesh, returning true iff we got > 0 triangles
		bool triangulate();

		float computeArea();
		void computeMassAndMoment(float density, float &mass, float &moment, float &area);

		void destroyCollisionShapes();
		const vector<cpShape*> &createCollisionShapes(cpBody *body, cpBB &modelBB);

	private:
		static size_t _count;

		bool _worldSpaceShapeContourEdgesDirty;
		cpHashValue _hash;
		string _name;
		Color _color;
		contour_pair _outerContour;
		vector<contour_pair> _holeContours;
		TriMeshRef _trimesh;
		vec2 _modelCentroid;

		vector<cpShape*> _shapes;
		BodyWeakRef _body;

		unordered_set<poly_edge> _worldSpaceContourEdges;
		cpBB _worldSpaceContourEdgesBB;
	};

}

#endif /* Island_hpp */
