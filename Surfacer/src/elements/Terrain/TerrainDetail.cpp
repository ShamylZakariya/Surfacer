//
//  TerrainDetail.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 10/7/17.
//

#include "TerrainDetail.hpp"

#include <list>
#include <queue>
#include <functional>

#include "MathHelpers.hpp"
#include "ContourSimplification.hpp"

namespace terrain { namespace detail {
	
	const double MIN_SHAPE_AREA = 1;
	const double RDP_CONTOUR_OPTIMIZATION_THRESHOLD = 0.125;
	
#pragma mark - Helpers
		
	bool is_wound_clockwise(const PolyLine2d &contour) {
		// http://stackoverflow.com/questions/1165647/how-to-determine-if-a-list-of-polygon-points-are-in-clockwise-order
		// Sum over the edges, (x2 − x1)(y2 + y1)
		
		double sum = 0;
		for (size_t i = 0, j = 1, n = contour.getPoints().size(); i < n; i++, j++) {
			dvec2 a = contour.getPoints()[i];
			dvec2 b = contour.getPoints()[j % n];
			double k = (b.x - a.x) * (b.y + a.y);
			sum += k;
		}
		
		return sum >= 0;
	}
	
	void wind_clockwise(PolyLine2d &contour) {
		if (!is_wound_clockwise(contour)) {
			contour.reverse();
		}
	}
	
	void wind_counter_clockwise(PolyLine2d &contour) {
		if (is_wound_clockwise(contour)) {
			contour.reverse();
		}
	}
	
	PolyLine2d transformed(const PolyLine2d &pl, const dmat4 &m) {
		PolyLine2d transformed;
		for (auto &p : pl) {
			transformed.push_back(m * p);
		}
		return transformed;
	}
	
	dpolygon2 transformed(const dpolygon2 &src, const dmat4 &m) {
		dpolygon2 transformed;
		
		for( auto outerIt = src.outer().begin(); outerIt != src.outer().end(); ++outerIt ) {
			dvec2 p(boost::geometry::get<0>(*outerIt), boost::geometry::get<1>(*outerIt));
			dvec2 tp = m * p;
			transformed.outer().push_back(boost::geometry::make<dpoint2>(tp.x, tp.y));
		}
		
		for (auto innerIt = src.inners().begin(); innerIt != src.inners().end(); ++innerIt) {
			dpolygon2::ring_type ring;
			for (auto pIt = innerIt->begin(); pIt != innerIt->end(); ++pIt) {
				dvec2 p(boost::geometry::get<0>(*pIt), boost::geometry::get<1>(*pIt));
				dvec2 tp = m * p;
				ring.push_back(boost::geometry::make<dpoint2>(tp.x, tp.y));
			}
			transformed.inners().push_back(ring);
		}
		
		return transformed;
	}
	
	void transform(PolyLine2d &pl, const dmat4 &m) {
		for (auto &p : pl) {
			dvec2 tp = m * p;
			p.x = tp.x;
			p.y = tp.y;
		}
	}
	
	void transform(const PolyLine2d &a, PolyLine2d &b, const dmat4 &m) {
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
	
	PolyLine2d optimize(PolyLine2d p) {
		if (RDP_CONTOUR_OPTIMIZATION_THRESHOLD > 0) {
			return core::util::simplify(p, RDP_CONTOUR_OPTIMIZATION_THRESHOLD);
		} else {
			p.setClosed();
			return p;
		}
	}
	
	vector<PolyLine2d> optimize(vector<PolyLine2d> ps) {
		vector<PolyLine2d> ret;
		
		if (RDP_CONTOUR_OPTIMIZATION_THRESHOLD > 0) {
			for (auto &p : ps) {
				ret.push_back(core::util::simplify(p, RDP_CONTOUR_OPTIMIZATION_THRESHOLD));
			}
		} else {
			for (auto &p : ps) {
				p.setClosed();
				ret.push_back(p);
			}
		}
		
		return ret;
	}
	
	PolyLine2f polyline2d_to_2f(const PolyLine2d &p2d) {
		PolyLine2f p2f;
		for (dvec2 dv2 : p2d) {
			p2f.push_back(dv2);
		}
		return p2f;
	}
	
	PolyLine2d polyline2f_to_2d(const PolyLine2f &p2f) {
		PolyLine2d p2d;
		for (dvec2 v2 : p2f) {
			p2d.push_back(v2);
		}
		return p2d;
	}
	
	cpBB polygon_bb(const dpolygon2 poly) {
		cpBB bb = cpBBInvalid;
		for (auto outer : poly.outer()) {
			dvec2 p(boost::geometry::get<0>(outer), boost::geometry::get<1>(outer));
			bb = cpBBExpand(bb, p);
		}
		
		return bb;
	}
	
#pragma mark - Conversion to/from Boost::Geometry, PolyLine2d & Shape
	
	typedef dpolygon2::inner_container_type::const_iterator RingIterator;
	typedef dpolygon2::ring_type::const_iterator PointIterator;
	
	vector<polyline_with_holes> dpolygon2_to_polyline_with_holes(std::vector<dpolygon2> &polygons, const dmat4 &modelview) {
		const double Dist2Epsilon = 1e-2;
		std::vector<polyline_with_holes> result;
		
		for( std::vector<dpolygon2>::iterator outIt = polygons.begin(); outIt != polygons.end(); ++outIt ) {
			
			boost::geometry::correct(*outIt);
			
			bool polygonOuterContourIsSane = true;
			PolyLine2d shapeContour;
			vector<PolyLine2d> holeContours;
			
			// convert the boost geometry to PolyLine2d, creating the shapeContour and holeContours.
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
					shapeContour.push_back( dvec2(x,y) );
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
			
			if (shapeContour.getPoints().empty()) {
				CI_LOG_E("Polygon is empty");
				break;
			}
			
			transform(shapeContour, modelview);
			
			for( RingIterator crunk = outIt->inners().begin(); crunk != outIt->inners().end(); ++crunk ) {
				PolyLine2d contour;
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
						contour.push_back( dvec2(x,y) );
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
				
				if (!contour.getPoints().empty() && contour.calcArea() >= MIN_SHAPE_AREA ) {
					transform(contour, modelview);
					holeContours.push_back( contour );
				}
			}
			
			// if we're here we're good
			result.emplace_back( shapeContour, holeContours );
		}
		
		return result;
	}
	
	std::vector<ShapeRef> dpolygon2_to_shape(std::vector<dpolygon2> &polygons, const dmat4 &modelview) {
		
		const auto plhs = dpolygon2_to_polyline_with_holes(polygons, modelview);
		std::vector<ShapeRef> result;
		for (const auto &plh : plhs) {
			result.push_back(make_shared<Shape>(plh.contour, plh.holes));
		}
		
		return result;
	}
	
	dpolygon2 shape_to_dpolygon2( ShapeRef shape ) {
		dpolygon2 result;
		
		for (auto &p : shape->getOuterContour().model.getPoints()) {
			result.outer().push_back( boost::geometry::make<dpoint2>( p.x, p.y ) );
		}
		
		for (auto &holeContour : shape->getHoleContours()) {
			dpolygon2::ring_type ring;
			for( auto &p : holeContour.model.getPoints() ) {
				ring.push_back( boost::geometry::make<dpoint2>( p.x, p.y ) );
			}
			result.inners().push_back( ring );
		}
		
		boost::geometry::correct( result );
		return result;
	}
	
	dpolygon2 polyline2d_to_dpolygon2( const PolyLine2d &polyLine )
	{
		dpolygon2 result;
		
		for (auto &p : polyLine.getPoints()) {
			result.outer().push_back( boost::geometry::make<dpoint2>( p.x, p.y ) );
		}
		
		boost::geometry::correct( result );
		return result;
	}
	
	PolyLine2d dpolygon2_to_polyline2d(dpolygon2 &poly)
	{
		const double Dist2Epsilon = 1e-2;
		boost::geometry::correct(poly);
		
		bool polygonOuterContourIsSane = true;
		PolyLine2d shapeContour;
		vector<PolyLine2d> holeContours;
		
		for( PointIterator pt = poly.outer().begin(); pt != poly.outer().end(); ++pt ) {
			const double x = boost::geometry::get<0>(*pt);
			const double y = boost::geometry::get<1>(*pt);
			if ( isinf(x) || isinf(y)) {
				CI_LOG_E("failed isinf() test on outer shape of polygon vertex: " << (size_t)(pt - poly.outer().begin()));
				polygonOuterContourIsSane = false;
				break;
			} else {
				shapeContour.push_back( dvec2(x,y) );
			}
		}
		
		if (!polygonOuterContourIsSane) {
			CI_LOG_E("Skipping this polygon entirely as outer contour failed");
			return PolyLine2d();
		}
		
		// excise double first-last vertices
		if (distanceSquared(shapeContour.getPoints().front(), shapeContour.getPoints().back()) < Dist2Epsilon) {
			shapeContour.getPoints().pop_back();
		}
		
		return shapeContour;
	}
	
	
#pragma mark - Contour Soup
	
	/**
	 Given a "soup" of contours (an unordered mess of closed polylines) build a tree (with possibly
	 multiple roots) which describes their nesting in a manner useful for constructing PolyShapes
	 */
	vector<contour_tree_node_ref> build_contour_tree(const vector<PolyLine2d> &contourSoup) {
		
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
		set<const PolyLine2d*> contours;
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
			vector<PolyLine2d> innerContours;
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
		CI_ASSERT_MSG((depth % 2) == 0, "build_from_contour_tree MUST only be called on even depths");
		
		// simplest case, no child nodes
		if (rootNode->children.empty()) {
			vector<ShapeRef> r = { make_shared<Shape>(rootNode->contour) };
			return r;
		}
		
		// we can make a PolyShape from this node's contour and each child which has no children
		// each child which does have children we have to recurse
		
		vector<ShapeRef> result;
		
		vector<PolyLine2d> holes;
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
	
#pragma mark - Edge Test
	
	bool shared_edges(const ShapeRef &a, const ShapeRef &b) {
		const double bbFudge = 4.f;
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
	
#pragma mark - Flood Fill
	
	namespace floodfill {
		
		GroupBaseRef shape_get_parent_group(const ShapeRef &shape, const map<ShapeRef,GroupBaseRef> &parentage) {
			if (shape->getGroup()) {
				return shape->getGroup();
			} else {
				auto pos = parentage.find(shape);
				if (pos != parentage.end()) {
					return pos->second;
				}
			}
			
			return nullptr;
		};
		
		template<typename T>
		bool contains(const set<T> &s, const T &v) {
			return s.find(v) != s.end();
		}

	}
	
	set<ShapeRef> find_contact_group(ShapeRef origin, const set<ShapeRef> &all, const map<ShapeRef,GroupBaseRef> &parentage) {
		/*
		 Very loosely adapted from Wikipedia's FloodFill page:
		 http://en.wikipedia.org/wiki/Flood_fill
		 */
		
		queue<ShapeRef> Q;
		Q.push(origin);
		set<ShapeRef> group;
		
		while(!Q.empty()) {
			ShapeRef currentShape = Q.front();
			Q.pop();
			
			if (!floodfill::contains(group, currentShape)) {
				group.insert(currentShape);
				const auto currentShapeParent = floodfill::shape_get_parent_group(currentShape, parentage);
				
				// now find all connected neighbors with shared parentage, add them to group and Q
				
				for (auto queryShape : all) {
					if (queryShape != currentShape && !floodfill::contains(group,queryShape)) {
						const auto queryShapeParent = floodfill::shape_get_parent_group(queryShape, parentage);
						if (queryShapeParent == currentShapeParent && shared_edges(queryShape, currentShape)) {
							Q.push(queryShape);
						}
					}
				}
			}
		}
		
		return group;
	}
	
#pragma mark - Mitering
	
	/**
	 Assuming a clockwise winding of a->b->c (where b is the corner to miter) get the inner miter vertex at a given depth.
	 In this case inner means "inside the polygon".
	 This is a helper function for computing cpSegmentShapes to perimeter a non-convex polygon to prevent small/fast objects
	 from penetrating the cracks between adjacent convex triangles assembled to a non-convex aggregate shape.
	 */
	dvec2 get_inner_miter_for_corner(dvec2 a, dvec2 b, dvec2 c, double depth) {
		auto a2b = normalize(b-a);
		auto b2c = normalize(c-b);
		auto a2b_right = rotateCW(a2b);
		auto b2c_right = rotateCW(b2c);
		auto miter = b + (depth * normalize(a2b_right + b2c_right));
		return miter;
	}
	
	PolyLine2d inset_contour(const PolyLine2d &contour, double depth) {
		// TODO: This doesn't actually work, mitering is a much more subtle thing than this
		
		const auto contourPoints = contour.getPoints();
		const auto N = contourPoints.size();
		vector<dvec2> insetPoints;
		insetPoints.reserve(N);
		
		for (size_t i = 0; i < N; i++) {
			dvec2 a = contourPoints[(i+N-1) % N];
			dvec2 b = contourPoints[i];
			dvec2 c = contourPoints[(i+1) % N];
			insetPoints.push_back(get_inner_miter_for_corner(a,b,c,depth));
		}
		
		return PolyLine2d(insetPoints);
	}
	
	//	PolyLine2d outset_contour(const PolyLine2d &contour, double outsetDepth) {
	//	This is not the same as inset_contour with negative depth, or is it?
	//	}
	
	
}} // namespace terrain::detail
