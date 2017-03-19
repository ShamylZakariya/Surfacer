//
//  Island.cpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 3/1/17.
//
//

#include "Terrain.hpp"

#include <cinder/Log.h>
#include <cinder/Triangulate.h>
#include <cinder/gl/gl.h>
#include <cinder/Rand.h>

#include <functional>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/multi/multi.hpp>

#include "ContourSimplification.hpp"

using namespace core;

namespace terrain {

	namespace {

		const float RDP_CONTOUR_OPTIMIZATION_THRESHOLD = 0.1f;
		const float MIN_POLYLINE_AREA = 1.f;
		const float MIN_TRIANGLE_AREA = 1.f;

#pragma mark - Helpers

		Rand colorRand(456);

		Color next_random_color() {
			return Color(CM_HSV, colorRand.nextFloat(), 0.5 + colorRand.nextFloat(0.5), 0.5 + colorRand.nextFloat(0.5));
		}

		vec2 rotate_cw(const vec2 &v) {
			return vec2(v.y, -v.x);
		}

		bool is_wound_clockwise(const PolyLine2f &contour) {
			// http://stackoverflow.com/questions/1165647/how-to-determine-if-a-list-of-polygon-points-are-in-clockwise-order
			// Sum over the edges, (x2 − x1)(y2 + y1)

			double sum = 0;
			for (size_t i = 0, j = 1, n = contour.getPoints().size(); i < n; i++, j++) {
				vec2 a = contour.getPoints()[i];
				vec2 b = contour.getPoints()[j % n];
				double k = (b.x - a.x) * (b.y + a.y);
				sum += k;
			}

			return sum >= 0;
		}

		void wind_clockwise(PolyLine2f &contour) {
			if (!is_wound_clockwise(contour)) {
				contour.reverse();
			}
		}

		void wind_counter_clockwise(PolyLine2f &contour) {
			if (is_wound_clockwise(contour)) {
				contour.reverse();
			}
		}

		PolyLine2f transformed(const PolyLine2f &pl, const mat4 &m) {
			PolyLine2f transformed;
			for (auto &p : pl) {
				transformed.push_back(m * p);
			}
			return transformed;
		}

		void transform(PolyLine2f &pl, const mat4 &m) {
			for (auto &p : pl) {
				vec2 tp = m * p;
				p.x = tp.x;
				p.y = tp.y;
			}
		}

		void transform(const PolyLine2f &a, PolyLine2f &b, const mat4 &m) {
			if (b.getPoints().size() != a.getPoints().size()) {
				b.getPoints().clear();
				b.getPoints().resize(a.getPoints().size());
			}

			auto ai = begin(a);
			auto bi = begin(b);
			const auto end = ::end(a);
			for (; ai != end; ++ai, ++bi) {
				*bi = m * (*ai);
			}
		};

		PolyLine2f optimize(PolyLine2f p) {
			return simplify(p, RDP_CONTOUR_OPTIMIZATION_THRESHOLD);
		}

		vector<Shape::contour_pair> optimize(vector<PolyLine2f> ps) {
			vector<Shape::contour_pair> ret;
			for (auto &p : ps) {
				ret.push_back(simplify(p, RDP_CONTOUR_OPTIMIZATION_THRESHOLD));
			}

			return ret;
		}


#pragma mark - Boost::Geometry - Shape Interop


		typedef boost::geometry::model::polygon<boost::geometry::model::d2::point_xy<double> > polygon;

		std::vector<ShapeRef> convertBoostGeometryToIslandShapes( std::vector<polygon> &polygons, const mat4 &modelview) {
			const float Dist2Epsilon = 1e-2;
			std::vector<ShapeRef> result;

			for( std::vector<polygon>::iterator outIt = polygons.begin(); outIt != polygons.end(); ++outIt ) {
				typedef polygon::inner_container_type::const_iterator RingIterator;
				typedef polygon::ring_type::const_iterator PointIterator;

				boost::geometry::correct(*outIt);

				bool polygonOuterContourIsSane = true;
				PolyLine2f shapeContour;
				vector<PolyLine2f> holeContours;

				// convert the boost geometry to PolyLine2f, creating the shapeContour and holeContours.
				// note: we apply the modelview BEFORE calling updateModelview on the PolyShape. This
				// is to move the polylines to the world space of the shape they were created from,
				// and then move them to their own model space. this is a small loss of precision
				// (transforming and storing vertices!) but it solves problem of cutting a rotated poly

				for( PointIterator pt = outIt->outer().begin(); pt != outIt->outer().end(); ++pt ) {
					const double x = boost::geometry::get<0>(*pt);
					const double y = boost::geometry::get<1>(*pt);
					if ( isinf(x) || isinf(y)) {
						CI_LOG_E("failed isinf() test on outer shape of polygon: " << (size_t)(outIt - polygons.begin()) << " vertex: " << (size_t)(pt - outIt->outer().begin()));
						polygonOuterContourIsSane = false;
						break;
					} else {
						shapeContour.push_back( vec2(x,y) );
					}
				}

				if (!polygonOuterContourIsSane) {
					CI_LOG_E("Skipping this polygon entirely as outer contour failed");
					break;
				}

				// excise double first-last vertices
				if (distanceSquared(shapeContour.getPoints().front(), shapeContour.getPoints().back()) < Dist2Epsilon) {
					shapeContour.getPoints().pop_back();
				}

				if (shapeContour.getPoints().empty() || shapeContour.calcArea() < MIN_POLYLINE_AREA) {
					CI_LOG_E("Polygon is sane, but empty, or area too small to justify creating a PolyShape");
					break;
				}

				transform(shapeContour, modelview);

				for( RingIterator crunk = outIt->inners().begin(); crunk != outIt->inners().end(); ++crunk ) {
					PolyLine2f contour;
					bool innerContourIsSane = true;
					for( PointIterator pt = crunk->begin(); pt != crunk->end(); ++pt ) {
						const double x = boost::geometry::get<0>(*pt);
						const double y = boost::geometry::get<1>(*pt);
						if ( isinf(x) || isinf(y)) {
							innerContourIsSane = false;
							CI_LOG_E("failed isinf() test on inner shape of polygon: " << (size_t)(outIt - polygons.begin())
									 << " ring: " << (size_t)(crunk - outIt->inners().begin())
									 << " vertex: " << (size_t)(pt - crunk->begin()));
							break;
						} else {
							contour.push_back( vec2(x,y) );
						}
					}

					if (!innerContourIsSane) {
						CI_LOG_E("Skipping this inner contour");
						break;
					}

					// excise double first-last vertices
					if (distanceSquared(contour.getPoints().front(), contour.getPoints().back()) < Dist2Epsilon) {
						contour.getPoints().pop_back();
					}

					if (!contour.getPoints().empty() && contour.calcArea() >= MIN_POLYLINE_AREA ) {
						transform(contour, modelview);
						holeContours.push_back( contour );
					}
				}

				// if we're here we're good
				result.push_back(make_shared<Shape>(shapeContour, holeContours));
			}

			return result;
		}

		polygon convertIslandShapeToBoostGeometry( const Shape &a ) {
			polygon result;

			for (auto &p : a.getOuterContour().model.getPoints()) {
				result.outer().push_back( boost::geometry::make<boost::geometry::model::d2::point_xy<double> >( p.x, p.y ) );
			}

			for (auto &holeContour : a.getHoleContours()) {
				polygon::ring_type ring;
				for( auto &p : holeContour.model.getPoints() ) {
					ring.push_back( boost::geometry::make<boost::geometry::model::d2::point_xy<double> >( p.x, p.y ) );
				}
				result.inners().push_back( ring );
			}

			boost::geometry::correct( result );
			return result;
		}

		polygon convertPolyLineToBoostGeometry( const PolyLine2f &a )
		{
			polygon result;

			for (auto &p : a.getPoints()) {
				result.outer().push_back( boost::geometry::make<boost::geometry::model::d2::point_xy<double> >( p.x, p.y ) );
			}

			boost::geometry::correct( result );
			return result;
		}


#pragma mark - Contour Soup


		class contour_tree_node;
		typedef shared_ptr<contour_tree_node> contour_tree_node_ref;

		struct contour_tree_node {
			PolyLine2f contour;
			vector<contour_tree_node_ref> children;
		};

		/**
		 Given a "soup" of contours (an unordered mess of closed polylines) build a tree (with possibly
		 multiple roots) which describes their nesting in a manner useful for constructing PolyShapes
		 */
		vector<contour_tree_node_ref> build_contour_tree(const vector<PolyLine2f> &contourSoup) {

			// simple cases
			if (contourSoup.size() == 0) {
				return vector<contour_tree_node_ref>();
			} else if (contourSoup.size() == 1) {
				contour_tree_node_ref node = make_shared<contour_tree_node>();
				node->contour = contourSoup.front();
				vector<contour_tree_node_ref> ret = { node };
				return ret;
			}


			// create a set of pointers to our polylines
			set<const PolyLine2f*> contours;
			for (auto &pl : contourSoup) {
				contours.insert(&pl);
			}

			vector<contour_tree_node_ref> rootNodes;
			while(!contours.empty()) {


				contour_tree_node_ref node = make_shared<contour_tree_node>();
				rootNodes.push_back(node);

				// find first polyline which is not a hole in another polyline. remove it
				for (auto candidateIt = begin(contours); candidateIt != end(contours); ++candidateIt) {
					bool isHole = false;
					for (auto it2 = begin(contours); it2 != end(contours); ++it2) {
						if (candidateIt != it2) {
							if ((*it2)->contains(*(*candidateIt)->begin())) {
								isHole = true;
								break;
							}
						}
					}

					// we found a non-hole contour; remove candidate from set and break loop
					if (!isHole) {
						node->contour = **candidateIt;
						contours.erase(candidateIt);
						break;
					}
				}

				// find each polyline which is inside the one we just found
				vector<PolyLine2f> innerContours;
				for (auto candidateHoleIt = begin(contours); candidateHoleIt != end(contours);) {
					if (node->contour.contains(*(*candidateHoleIt)->begin())) {
						innerContours.push_back(**candidateHoleIt);
						candidateHoleIt = contours.erase(candidateHoleIt);
					} else {
						++candidateHoleIt;
					}
				}

				// now we know innerContours are children of node->outerContour - but
				// only those with no children may become holes. Any which have children
				// themselves must become new root nodes

				auto children = build_contour_tree(innerContours);
				node->children.insert(end(node->children), begin(children), end(children));
			}

			return rootNodes;
		}

		vector<ShapeRef> build_from_contour_tree(contour_tree_node_ref rootNode, int depth) {
			if (depth % 2) {
				CI_LOG_E("build_from_contour_tree MUST only be called on even depths");
				assert(false);
			}

			// simplest case, no child nodes
			if (rootNode->children.empty()) {
				vector<ShapeRef> r = { make_shared<Shape>(rootNode->contour) };
				return r;
			}

			// we can make a PolyShape from this node's contour and each child which has no children
			// each child which does have children we have to recurse

			vector<ShapeRef> result;

			vector<PolyLine2f> holes;
			for (auto &childNode : rootNode->children) {
				// each child becomes a hole contour
				holes.push_back(childNode->contour);

				// for each grandchild, we're creating a new PolyShape
				for (auto &grandChild : childNode->children) {
					auto childResult = build_from_contour_tree(grandChild, depth+2);
					result.insert(end(result), begin(childResult), end(childResult));
				}
			}

			result.push_back(make_shared<Shape>(rootNode->contour, holes));
			
			return result;
		}

#pragma mark - Set Merging

		template<typename T>
		bool intersects(const set<T> &a, const set<T> &b) {
			for (auto &i : b) {
				if (a.find(i) != a.end()) {
					return true;
				}
			}
			return false;
		}

		template<typename T>
		vector<set<T>> merge(vector<set<T>> buckets) {
			vector<set<T>> merged;

			while (buckets.size() > 1) {

				// grab first bucket and find any buckets with shared items
				auto first = buckets.begin();
				for (auto it(buckets.begin()+1), e(buckets.end()); it != e; ++it) {
					if (intersects(*first, *it)) {
						// copy elements over to first, and clear
						first->insert(it->begin(), it->end());
						it->clear();
					}
				}

				// copy over to merged and clear
				merged.push_back(*first);
				first->clear();

				// now remove any empty buckets
				auto new_end = remove_if(buckets.begin(), buckets.end(), [](const set<T> &bucket) {
					return bucket.empty();
				});

				buckets.erase(new_end, end(buckets));
			}

			// copy over any remainders
			for (auto &bucket : buckets) {
				if (!bucket.empty()) {
					merged.push_back(bucket);
				}
			}

			return merged;
		}

#pragma mark - 

		template<typename T>
		class sorted_ref_pair {
		public:
			sorted_ref_pair(const shared_ptr<T> &A, const shared_ptr<T> &B) {
				if (A < B) {
					a = A;
					b = B;
				} else {
					a = B;
					b = A;
				}
			}

			friend bool operator == (const sorted_ref_pair<T> &a, const sorted_ref_pair<T> &b) {
				return a.a == b.a && a.b == b.b;
			}

			friend bool operator < (const sorted_ref_pair<T> &a, const sorted_ref_pair<T> &b) {
				if (a.a != b.a) {
					return a.a < b.a;
				}
				return a.b < b.b;
			}

		private:
			shared_ptr<T> a,b;
		};

#pragma mark -

		bool shared_edges(const ShapeRef &a, const ShapeRef &b) {
			const float bbFudge = 4.f;
			if (cpBBIntersects(a->getWorldSpaceContourEdgesBB(), b->getWorldSpaceContourEdgesBB(), bbFudge)) {
				const auto &aShapeEdges = a->getWorldSpaceContourEdges();
				const auto &bShapeEdges = b->getWorldSpaceContourEdges();
				for (const auto &pe : bShapeEdges) {
					if (aShapeEdges.find(pe) != aShapeEdges.end()) {
						return true;
					}
				}
			}
			return false;
		}

	}
	
#pragma mark - World

	/*
		material _material;
		cpSpace *_space;
		set<GroupRef> _groups;
		set<Anchor> _anchors;

	 */
	
	World::World(cpSpace *space, material m):
	_material(m),
	_space(space)
	{}

	World::~World() {
	}

	void World::build(const vector<ShapeRef> &shapes) {
		build(shapes, map<ShapeRef,GroupRef>());
	}

	void World::build(const vector<ShapeRef> &shapes, const vector<AnchorRef> &anchors) {
		// copy over the anchors
		_anchors.insert(_anchors.end(), anchors.begin(), anchors.end());

		// now build
		build(shapes, map<ShapeRef,GroupRef>());
	}

	void World::cut(vec2 a, vec2 b, float radius) {
		const float MinLength = 1e-2;
		const float MinRadius = 1e-2;

		vec2 dir = b - a;
		float l = length(dir);

		if (l > MinLength && radius > MinRadius) {
			dir /= l;

			vec2 right = radius * rotate_cw(dir);
			vec2 left = -right;

			vec2 ca = a + right;
			vec2 cb = a + left;
			vec2 cc = b + left;
			vec2 cd = b + right;

			PolyLine2f contour;
			contour.push_back(ca);
			contour.push_back(cb);
			contour.push_back(cc);
			contour.push_back(cd);
			contour.setClosed();

			CI_LOG_D("Performing cut from " << a << " to " << b);

			struct cut_collector {
				set<ShapeRef> shapes;
				set<GroupRef> groups;

				void clear() {
					shapes.clear();
					groups.clear();
				}
			};

			cut_collector collector;
			cpSpaceSegmentQuery(_space, cpv(a), cpv(b), radius, _material.filter,
								[](cpShape *collisionShape, cpVect point, cpVect normal, cpFloat alpha, void *data){
									cut_collector *collector = static_cast<cut_collector*>(data);
									ShapeRef shape = static_cast<Shape*>(cpShapeGetUserData(collisionShape))->shared_from_this();
									collector->shapes.insert(shape);
									collector->groups.insert(shape->getGroup());
								}, &collector);

			CI_LOG_D("Collected " << collector.shapes.size() << " shapes (" << collector.groups.size() << " bodies) to cut" );

			//
			// Build vector of Shapes affected by this cut (because of group stuff) but not directly cut
			//

			vector<ShapeRef> affectedShapes;
			for (auto &body : collector.groups) {
				for (auto &shape : body->getShapes()) {
					if (collector.shapes.find(shape) == collector.shapes.end()) {
						affectedShapes.push_back(shape);
					}
				}
			}

			//
			//	Perform cut, adding results to affectedShapes and assigning parentage
			//	so we can apply lin/ang vel to new bodies
			//

			map<ShapeRef,GroupRef> parentage;
			for (auto &shapeToCut : collector.shapes) {
				GroupRef parentGroup = shapeToCut->getGroup();

				auto result = shapeToCut->subtract(contour);
				affectedShapes.insert(end(affectedShapes), begin(result), end(result));

				for (auto &newShape : result) {
					parentage[newShape] = parentGroup;
				}

				_groups.erase(parentGroup);
			}

			collector.clear();
			build(affectedShapes, parentage);

		} else {
			CI_LOG_E("Either length and/or radius were below minimum thresholds");
		}
	}
	
	void World::draw(const render_state &renderState) {
		switch(renderState.mode) {
			case RenderMode::GAME:
				drawGame(renderState);
				break;

			case RenderMode::DEVELOPMENT:
				drawDebug(renderState);
				break;

			case RenderMode::COUNT:
				break;
		}
	}

	void World::step(const time_state &timeState) {
		for (auto &body : _groups) {
			body->step(timeState);
		}
	}

	void World::update(const time_state &timeState) {
		for (auto &body : _groups) {
			body->update(timeState);
		}
	}

	void World::build(const vector<ShapeRef> &shapes, const map<ShapeRef,GroupRef> &parentage) {

		vector<set<ShapeRef>> shapeGroups;
		set<sorted_ref_pair<Shape>> handledPairs;

		//
		// finds the first set in groups which has either `a or `b, and adds the other to it. if none exists, creates a new one
		//

		auto engroup = [&shapeGroups](const ShapeRef &a, const ShapeRef &b) {
			for (auto &group : shapeGroups) {
				if (group.find(a) != group.end()) {
					group.insert(b);
					return;
				} else if (group.find(b) != group.end()) {
					group.insert(a);
					return;
				}
			}

			// if we're here, make a new group
			set<ShapeRef> newGroup = { a, b };
			shapeGroups.push_back(newGroup);
		};

		//
		// find which shapes are neighbors and must be stitched.
		// Did some timing measurements and found that brute force is faster until the shapes count gets pretty big.
		//

		const bool UseSpatialIndex = shapes.size() > 400;

		if (UseSpatialIndex) {

			SpatialIndex<ShapeRef> broadphase(100);

			for (auto shape : shapes) {
				broadphase.insert(shape->getWorldSpaceContourEdgesBB(), shape);
			}

			for (auto queryShape : shapes) {
				const auto &queryShapeEdges = queryShape->getWorldSpaceContourEdges();
				auto neighbors = broadphase.sweep(queryShape->getWorldSpaceContourEdgesBB());
				bool hasGroup = false;

				for (auto neighbor : neighbors) {
					const auto &neighborShapeEdges = neighbor->getWorldSpaceContourEdges();
					for (const auto &pe : neighborShapeEdges) {
						if (queryShapeEdges.find(pe) != queryShapeEdges.end()) {
							engroup(queryShape, neighbor);
							hasGroup = true;
						}
					}
				}

				if (!hasGroup) {
					// so lonely
					set<ShapeRef> singletonGroup = { queryShape };
					shapeGroups.push_back(singletonGroup);
				}

			}

		} else {
			// this is the brute-force approach
			for (auto queryShape : shapes) {
				bool hasNeighbors = false;
				for (auto testShape : shapes) {
					if (testShape != queryShape) {
						const sorted_ref_pair<Shape> pair(queryShape,testShape);
						if (handledPairs.find(pair) == end(handledPairs) && shared_edges(queryShape,testShape)) {
							engroup(queryShape, testShape);
							handledPairs.insert(pair);
							hasNeighbors = true;
						}
					}
				}

				if (!hasNeighbors) {
					// so lonely
					set<ShapeRef> singletonGroup = { queryShape };
					shapeGroups.push_back(singletonGroup);
				}
			}
		}

		//
		// since we can't predict the order of collection, we may have
		// groups which need to be merged to supergroups.
		//

		shapeGroups = merge(shapeGroups);

		//
		//	Each group repfesents an Body, so build them
		//

		for (auto &shapeGroup : shapeGroups) {

			//
			// find the parent body for the new body (if there is one)
			//

			GroupRef parentGroup;
			if (!parentage.empty()) {
				for (auto &shape : shapeGroup) {
					auto search = parentage.find(shape);
					if (search != parentage.end()) {
						parentGroup = search->second;
						break;
					}
				}
			}

			if (parentGroup) {
				// sanity check: parent should have been removed from our active bodies set
				assert(_groups.find(parentGroup) == _groups.end());
			}

			GroupRef group = make_shared<Group>(_space, _material);
			if (group->build(shapeGroup, parentGroup, _anchors)) {
				_groups.insert(group);
			}
		}
	}

	void World::drawGame(const render_state &renderState) {
		const cpBB frustum = renderState.viewport.getFrustum();
		for (const auto &body : _groups) {
			if (cpBBIntersects(frustum, body->getBB())) {
				gl::ScopedModelMatrix smm;
				gl::multModelMatrix(body->getModelview());

				for (const auto &shape : body->getShapes()) {
					gl::color(shape->getColor());
					gl::draw(*shape->getTriMesh());
				}
			}
		}

		for (const auto &anchor : _anchors) {
			if (cpBBIntersects(frustum, anchor->getBB())) {
				// anchors are already in world space so we just draw them
				gl::color(ColorA(1,1,1));
				gl::draw(*anchor->getTriMesh());
			}
		}
	}

	void World::drawDebug(const render_state &renderState) {
		const cpBB frustum = renderState.viewport.getFrustum();
		for (const auto &body : _groups) {
			if (cpBBIntersects(frustum, body->getBB())) {
				gl::ScopedModelMatrix smm;
				gl::multModelMatrix(body->getModelview());

				for (const auto &shape : body->getShapes()) {
					gl::color(shape->getColor() * 0.25);
					gl::draw(*shape->getTriMesh());

					gl::lineWidth(1);
					gl::color(shape->getColor());
					gl::draw(shape->getOuterContour().model);

					for (auto &holeContour : shape->getHoleContours()) {
						gl::draw(holeContour.model);
					}

					{
						float rScale = renderState.viewport.getReciprocalScale();
						float angle = body->getAngle();
						vec3 modelCentroid = vec3(shape->_modelCentroid, 0);
						gl::ScopedModelMatrix smm2;
						gl::multModelMatrix(glm::translate(modelCentroid) * glm::rotate(-angle, vec3(0,0,1)) * glm::scale(vec3(rScale, -rScale, 1)));
						gl::drawString(shape->getName(), vec2(0,0), shape->getColor());
					}
				}
			}

			// body draws some debug data - note that game render pass ignores Group::draw
			body->draw(renderState);
		}

		for (const auto &anchor : _anchors) {
			if (cpBBIntersects(frustum, anchor->getBB())) {
				// anchors are already in world space so we just draw them
				gl::color(ColorA(1,1,1,0.25));
				gl::draw(*anchor->getTriMesh());

				gl::lineWidth(1);
				gl::color(Color(1,1,1));
				gl::draw(anchor->getContour());
			}
		}
	}


#pragma mark - Body

	/*
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
	 */

	size_t Group::_count = 0;

	Group::Group(cpSpace *space, material m):
	_dynamic(false),
	_material(m),
	_space(space),
	_body(nullptr),
	_position(cpv(0,0)),
	_angle(0),
	_worldBB(cpBBInvalid),
	_modelBB(cpBBInvalid),
	_modelview(1),
	_modelviewInverse(1),
	_name(nextId()) {
		_hash = hash<string>{}(_name);
	}

	Group::~Group() {
		_shapes.clear(); // will run Shape::dtor freeing cpShapes
		cpCleanupAndFree(_body);
	}

	string Group::getName() const {
		stringstream ss;
		ss << "Body[" << _name << "](";
		for (auto &shape : _shapes) {
			ss << shape->getName() << ":";
		}
		ss << ")";
		return ss.str();
	}

	void Group::draw(const render_state &renderState) {
		if (renderState.mode == RenderMode::DEVELOPMENT) {
			const float rScale = renderState.viewport.getReciprocalScale();

			gl::ScopedModelMatrix smm2;
			gl::multModelMatrix(translate(vec3(getPosition() + vec2(0,20),0)) * scale(vec3(rScale, -rScale, 1)));

			gl::drawString(getName(), vec2(0,0), Color(1,1,1));
		}
	}

	void Group::step(const time_state &timeState) {
		syncToCpBody();
	}

	void Group::update(const time_state &timeState) {
	}

	bool Group::build(set<ShapeRef> shapes, const GroupRef parentGroup, const vector<AnchorRef> &anchors) {

		set<ShapeRef> garbage;
		auto emptyGarbage = [&garbage, &shapes]() {
			for (auto &s : garbage) {
				s->setGroup(nullptr);
				shapes.erase(s);
			}
			garbage.clear();
		};

		_modelBB = cpBBInvalid;

		// PRECONDITIONS: It is assumed that each shape's contour model forms are in world space.
		// This is already true for any new shape, but any shape re-attached or reparented needs
		// to have its world contours updated from the model and transform, and then reassigned to model
		for (auto &shape : shapes) {
			// check if this object has a parent (and that the parent isn't us)
			GroupRef body = shape->getGroup();
			if (body && body.get() != this) {

				const mat4 T = shape->getModelview();
				transform(shape->_outerContour.model, shape->_outerContour.world, T);
				shape->_outerContour.model = shape->_outerContour.world;

				for (auto &holeContour : shape->_holeContours) {
					transform(holeContour.model, holeContour.world, T);
					holeContour.model = holeContour.world;
				}
			}

			// assign parentage AFTER accessing shape's modelview
			shape->setGroup(shared_from_this());
		}


		// the static-dynamic rule is this: a body may check to see if its static
		// if has NOT been dynamic in previous parentage
		bool shouldSearchForAnchors = true;
		if (parentGroup && parentGroup->isDynamic()) {
			shouldSearchForAnchors = false;
		}

		if (shouldSearchForAnchors) {
			bool found = false;
			for (auto &shape : shapes) {
				for (auto &anchor : anchors) {
					const PolyLine2f &shapeContour = shape->getOuterContour().world;
					if (cpBBIntersects(anchor->getBB(), shape->getWorldSpaceContourEdgesBB())) {
						// check if any vertices in the anchor's contours are in shape's outer contour
						for (const auto &p : anchor->getContour().getPoints()) {
							if (shapeContour.contains(p)) {
								found = true;
								break;
							}
						}
					}
				}

				if (found) {
					break;
				}
			}

			_dynamic = !found;

		} else {
			// if not searching for anchors we are by default dynamic
			_dynamic = true;
		}

		//
		// 1) first pass, gather up all shapes for attached bodies and compute common centroid, then apply a model space transform
		// 2) compute group total for areas, masses, moments, etc
		// 3) create the body
		// 4) then transform each shape's contours to local space, attach colliders, etc
		//


		if (_dynamic) {

			//
			// first compute the averaged centroid
			//

			vec2 averageModelSpaceCentroid(0,0);
			for (auto &shape : shapes) {
				shape->_modelCentroid = shape->_outerContour.model.calcCentroid();
				averageModelSpaceCentroid += shape->_modelCentroid;
			}

			averageModelSpaceCentroid /= shapes.size();

			//
			// move contours from their own private model space to shared model space
			//

			for (auto &shape : shapes) {
				shape->_modelCentroid -= averageModelSpaceCentroid;
				shape->_outerContour.model.offset(-averageModelSpaceCentroid);
				for (auto &hole : shape->_holeContours) {
					hole.model.offset(-averageModelSpaceCentroid);
				}
			}

			_modelview = _modelview * translate(vec3(averageModelSpaceCentroid.x, averageModelSpaceCentroid.y, 0.f));
			_modelviewInverse = inverse(_modelview);

			//
			//	Triangulate - any which fail should be collected to garbage
			//

			for (auto &shape : shapes) {
				if (!shape->triangulate()) {
					garbage.insert(shape);
				}
			}
			
			emptyGarbage();

			//
			//	Compute collective mass and moment
			//

			float totalArea = 0, totalMass = 0, totalMoment = 0;
			for (auto &shape : shapes) {
				float area = 0, mass = 0, moment = 0;
				shape->computeMassAndMoment(_material.density, mass, moment, area);
				totalArea += area;
				totalMass += mass;
				totalMoment += moment;
			}


			if (totalArea > MIN_POLYLINE_AREA) {

				_body = cpBodyNew(totalMass, totalMoment);
				cpBodySetUserData(_body, this);

				// glm::mat4 is column major
				vec2 position;
				position.x = _modelview[3][0];
				position.y = _modelview[3][1];
				cpBodySetPosition(_body, cpv(position));

				for (auto &shape : shapes) {
					cpBB modelBB = cpBBInvalid;

					// destroy any lingering collision shapes from previous tessellations and build new shapes
					// TODO: When dealing with static bodies, e.g., large environment terrain, we can keep collision shapes since they'll be in world space anyway

					shape->destroyCollisionShapes();
					vector<cpShape*> collisionShapes = shape->createCollisionShapes(_body, modelBB);

					if (!collisionShapes.empty() && cpBBIsValid(modelBB)) {
						cpBBExpand(_modelBB, modelBB);

						for (cpShape *collisionShape : collisionShapes) {
							cpShapeSetFilter(collisionShape, _material.filter);
							cpShapeSetFriction(collisionShape, _material.friction);
							cpSpaceAddShape(_space, collisionShape);
						}
					} else {
						garbage.insert(shape);
					}
				}

				emptyGarbage();

				if (!shapes.empty()) {

					//
					//	We're good to go
					//

					_shapes = shapes;
					cpSpaceAddBody(_space, _body);
					syncToCpBody();

					if (parentGroup) {

						//
						//	Compute world linearvel and angularvel for parent and apply to self
						//

						const float angularVel = cpBodyGetAngularVelocity(parentGroup->getBody());
						const cpVect linearVel = cpBodyGetVelocityAtWorldPoint(parentGroup->getBody(), cpv(getPosition()));
						cpBodySetVelocity(_body, linearVel);
						cpBodySetAngularVelocity(_body, angularVel);
					}

					return true;
				}
			}

			//
			//	We didn't have enough total area, or none of our shapes created collision shapes
			//

			return false;

		} else {

			//
			//	Static bodies have identity transform, model contour geometry lives in world space.
			//

			float totalArea = 0;
			for (auto &shape : shapes) {
				shape->_modelCentroid = shape->_outerContour.model.calcCentroid();
				if (shape->triangulate()) {
					totalArea += shape->computeArea();
				} else {
					garbage.insert(shape);
				}
			}

			emptyGarbage();

			_modelview = mat4(1);
			_modelviewInverse = mat4(1);

			_position.x = 0;
			_position.y = 0;
			_angle = 0;

			//
			//	Compute total area
			//

			if (totalArea > MIN_POLYLINE_AREA) {

				_body = cpBodyNewStatic();
				cpBodySetUserData(_body, this);
				cpBodySetPosition(_body, _position);

				for (auto &shape : shapes) {
					cpBB modelBB = cpBBInvalid;

					// destroy any lingering collision shapes from previous tessellations and build new shapes
					// TODO: When dealing with static bodies, e.g., large environment terrain, we can keep collision shapes since they'll be in world space anyway

					shape->destroyCollisionShapes();
					vector<cpShape*> collisionShapes = shape->createCollisionShapes(_body, modelBB);

					if (!collisionShapes.empty() && cpBBIsValid(modelBB)) {
						cpBBExpand(_modelBB, modelBB);

						for (cpShape *collisionShape : collisionShapes) {
							cpShapeSetFilter(collisionShape, _material.filter);
							cpShapeSetFriction(collisionShape, _material.friction);
							cpSpaceAddShape(_space, collisionShape);
						}
					} else {
						garbage.insert(shape);
					}
				}

				emptyGarbage();

				if (!shapes.empty()) {

					//
					//	We're good to go
					//

					_shapes = shapes;
					_worldBB = cpTransformbBB(getModelviewTransform(), _modelBB);
					cpSpaceAddBody(_space, _body);

					// TODO: Confirm setting contour edges dirty flag on static shapes is necessary
					for (auto &shape : _shapes) {
						shape->_worldSpaceShapeContourEdgesDirty = true;
					}


					return true;
				}
			}

			//
			//	Looks like we weren't able to generate any usable collision geometry
			//

			return false;
		}
	}

	void Group::syncToCpBody() {
		if (_dynamic) {
			// extract position and rotation from body and apply to _modelview
			cpVect position = cpBodyGetPosition(_body);
			cpVect rotation = cpBodyGetRotation(_body);
			cpFloat angle = cpBodyGetAngle(_body);

			// determine if we moved/rotated since last step
			const cpFloat Epsilon = 1e-3;
			bool moved = (cpvlengthsq(cpvsub(position, _position)) > Epsilon || abs(angle - _angle) > Epsilon);
			_position = position;
			_angle = angle;

			// mat4 - column major, each column is a vec4
			_modelview = mat4(
							  vec4(rotation.x, rotation.y, 0, 0),
							  vec4(-rotation.y, rotation.x, 0, 0),
							  vec4(0,0,1,0),
							  vec4(position.x, position.y, 0, 1));

			_modelviewInverse = inverse(_modelview);

			// update our world BB
			_worldBB = cpTransformbBB(getModelviewTransform(), _modelBB);

			if (moved) {
				for (auto &shape : _shapes) {
					shape->_worldSpaceShapeContourEdgesDirty = true;
				}
			}
		}
	}

#pragma mark - Anchor

	/*
		cpBB _bb;
		PolyLine2f _contour;
		TriMeshRef _trimesh;
	 */

	Anchor::Anchor(const PolyLine2f &contour):
	_bb(cpBBInvalid),
	_contour(contour) {

		for (auto &p : _contour.getPoints()) {
			cpBBExpand(_bb, p);
		}

		Triangulator triangulator;
		triangulator.addPolyLine(_contour);
		_trimesh = triangulator.createMesh();
	}



#pragma mark - Shape

	size_t Shape::_count = 0;

	string Shape::nextId() {
		stringstream ss;
		ss << _count++;
		return ss.str();
	}

	ShapeRef Shape::fromContour(const PolyLine2f outerContour) {
		return make_shared<Shape>(outerContour);
	}

	vector<ShapeRef> Shape::fromContours(const vector<PolyLine2f> &contourSoup) {

		vector<contour_tree_node_ref> rootNodes = build_contour_tree(contourSoup);
		vector<ShapeRef> shapes;
		for (auto &rn : rootNodes) {
			vector<ShapeRef> built = build_from_contour_tree(rn, 0);
			shapes.insert(end(shapes), begin(built), end(built));
		}

		return shapes;
	}

	vector<ShapeRef> Shape::fromShapes(const vector<Shape2d> &shapeSoup) {
		vector<PolyLine2f> contours;
		for (auto &shape : shapeSoup) {
			for (auto &path : shape.getContours()) {
				contours.push_back(path.subdivide());
			}
		}

		return fromContours(contours);
	}

	/*
		bool _worldSpaceShapeContourEdgesDirty;
		cpHashValue _hash;
		string _name;
		Color _color;
		PolyLine2f _worldSpaceOuterContour, _modelSpaceOuterContour;
		vector<PolyLine2f> _worldSpaceHoleContours, _modelSpaceHoleContours;
		TriMeshRef _trimesh;
		vec2 _modelCentroid;

		vector<cpShape*> _shapes;
		GroupWeakRef _body;

		set<poly_edge> _worldSpaceContourEdges;
		cpBB _worldSpaceContourEdgesBB;
	 */


	Shape::Shape(const PolyLine2f &sc):
	_worldSpaceShapeContourEdgesDirty(true),
	_name(nextId()),
	_color(next_random_color()),
	_outerContour(optimize(sc)),
	_modelCentroid(0,0),
	_worldSpaceContourEdgesBB(cpBBInvalid) {
		_hash = hash<string>{}(_name);
		wind_clockwise(_outerContour.world);
	}

	Shape::Shape(const PolyLine2f &sc, const std::vector<PolyLine2f> &hcs):
	_worldSpaceShapeContourEdgesDirty(true),
	_name(nextId()),
	_color(next_random_color()),
	_outerContour(optimize(sc)),
	_holeContours(optimize(hcs)),
	_modelCentroid(0,0),
	_worldSpaceContourEdgesBB(cpBBInvalid) {
		_hash = hash<string>{}(_name);

		wind_clockwise(_outerContour.world);
		for (auto &hc : _holeContours) {
			wind_counter_clockwise(hc.world);
		}
	}

	Shape::~Shape() {
		destroyCollisionShapes();
		_group.reset();
	}

	const unordered_set<poly_edge> &Shape::getWorldSpaceContourEdges() {
		updateWorldSpaceContourAndBB();
		return _worldSpaceContourEdges;
	}

	cpBB Shape::getWorldSpaceContourEdgesBB() {
		updateWorldSpaceContourAndBB();
		return _worldSpaceContourEdgesBB;
	}

	vector<ShapeRef> Shape::subtract(const PolyLine2f &contourToSubtract) const {
		if (!contourToSubtract.getPoints().empty()) {

			//
			// move the shape to subtract from world space to the our model space, and convert to a boost poly
			//

			PolyLine2f transformedShapeToSubtract = transformed(contourToSubtract, getModelviewInverse());
			polygon polyToSubtract = convertPolyLineToBoostGeometry( transformedShapeToSubtract );

			//
			// convert self (outerContour & holeContours) to a boost poly in model space
			//

			polygon thisPoly = convertIslandShapeToBoostGeometry( *this );

			//
			// now subtract - results are in model space
			//

			std::vector<polygon> output;
			boost::geometry::difference( thisPoly, polyToSubtract, output );

			//
			// convert output to Shapes - they need to have our modelview applied to them to move them back to world space
			//

			vector<ShapeRef> newIslandShapes = convertBoostGeometryToIslandShapes( output, getModelview() );

			if (!newIslandShapes.empty()) {
				return newIslandShapes;
			}
		}

		// handle failure case
		vector<ShapeRef> result = { const_cast<Shape*>(this)->shared_from_this() };
		return result;
	}

	string Group::nextId() {
		stringstream ss;
		ss << _count++;
		return ss.str();
	}

	void Shape::updateWorldSpaceContourAndBB() {
		if (_worldSpaceShapeContourEdgesDirty) {

			assert(_outerContour.model.size() > 1);

			const mat4 mv = getModelview();
			vec2 worldPoint = mv * _outerContour.model.getPoints().front();

			_worldSpaceContourEdges.clear();
			cpBB bb = cpBBInvalid;

			// the shapeContour is in model coordinates - we need to move each vertex to world
			for (auto current = _outerContour.model.begin(), next = _outerContour.model.begin()+1, end = _outerContour.model.end(); current != end; ++current, ++next) {
				if (next == end) {
					next = _outerContour.model.begin();
				}
				const vec2 worldNext = mv * (*next);
				_worldSpaceContourEdges.insert(poly_edge(worldPoint, worldNext));
				worldPoint = worldNext;
				cpBBExpand(bb, worldPoint);
			}

			_worldSpaceContourEdgesBB = bb;
			_worldSpaceShapeContourEdgesDirty = false;
		}
	}

	void Shape::setGroup(GroupRef group) {
		_group = group;
	}

	bool Shape::triangulate() {
		Triangulator triangulator;
		triangulator.addPolyLine(_outerContour.model);

		for (auto holeContour : _holeContours) {
			triangulator.addPolyLine(holeContour.model);
		}

		_trimesh = triangulator.createMesh();

		return _trimesh->getNumTriangles() > 0;
	}

	float Shape::computeArea() {
		float area = 0;
		cpVect triangle[3];

		for (size_t i = 0, N = _trimesh->getNumTriangles(); i < N; i++) {
			vec2 a,b,c;
			_trimesh->getTriangleVertices(i, &a, &b, &c);

			// find the winding with positive area
			triangle[0] = cpv(a);
			triangle[1] = cpv(b);
			triangle[2] = cpv(c);
			float triArea = length(cross(vec3(b-a,0),vec3(c-a,0))) * 0.5f;

			if (triArea < 0) {

				//
				// sanity check. this doesn't seem to ever actually happen
				// but it's no skin off my back to be prepared
				//

				triangle[0] = cpv(c);
				triangle[1] = cpv(b);
				triangle[2] = cpv(a);
				triArea = -triArea;
			}

			area += triArea;
		}

		return area;
	}

	void Shape::computeMassAndMoment(float density, float &mass, float &moment, float &area) {
		mass = 0;
		moment = 0;
		area = 0;

		cpVect triangle[3];
		for (size_t i = 0, N = _trimesh->getNumTriangles(); i < N; i++) {
			vec2 a,b,c;
			_trimesh->getTriangleVertices(i, &a, &b, &c);

			// find the winding with positive area
			triangle[0] = cpv(a);
			triangle[1] = cpv(b);
			triangle[2] = cpv(c);
			float triArea = length(cross(vec3(b-a,0),vec3(c-a,0))) * 0.5f;

			if (triArea < 0) {

				//
				// sanity check. this doesn't seem to ever actually happen
				// but it's no skin off my back to be prepared
				//

				triangle[0] = cpv(c);
				triangle[1] = cpv(b);
				triangle[2] = cpv(a);
				triArea = -triArea;
			}

			const float polyMass = abs(density * triArea);

			if (polyMass > 1e-3) {
				const float polyMoment = cpMomentForPoly(polyMass, 3, triangle, cpvzero, 0);
				if (!isnan(polyMoment)) {
					mass += polyMass;
					moment += abs(polyMoment);
					area += abs(triArea);
				}
			}
		}
	}

	void Shape::destroyCollisionShapes() {
		for (auto shape : _shapes) {
			cpCleanupAndFree(shape);
		}
		_shapes.clear();
	}

	const vector<cpShape*> &Shape::createCollisionShapes(cpBody *body, cpBB &modelBB) {
		assert(_shapes.empty());

		modelBB = cpBBInvalid;
		cpVect triangle[3];

		for (size_t i = 0, N = _trimesh->getNumTriangles(); i < N; i++) {
			vec2 a,b,c;
			_trimesh->getTriangleVertices(i, &a, &b, &c);
			triangle[0] = cpv(a);
			triangle[1] = cpv(b);
			triangle[2] = cpv(c);

			float area = cpAreaForPoly(3, triangle, 0);
			if (area < 0) {
				triangle[0] = cpv(c);
				triangle[1] = cpv(b);
				triangle[2] = cpv(a);
				area = cpAreaForPoly(3, triangle, 0);
			}

			if (area >= MIN_TRIANGLE_AREA) {
				cpShape *polyShape = cpPolyShapeNew(body, 3, triangle, cpTransformIdentity, 0);
				cpShapeSetUserData(polyShape, this);
				_shapes.push_back(polyShape);

				// a,b, & c are in model space
				cpBBExpand(modelBB, a);
				cpBBExpand(modelBB, b);
				cpBBExpand(modelBB, c);
			}
		}

		return _shapes;
	}
	

} // end namespace island
