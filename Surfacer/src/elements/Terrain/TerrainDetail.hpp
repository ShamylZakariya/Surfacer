//
//  TerrainDetail.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/30/17.
//
//

#ifndef TerrainDetail_h
#define TerrainDetail_h

#include "TerrainWorld.hpp"

#include <cinder/Log.h>
#include <cinder/Triangulate.h>
#include <cinder/gl/gl.h>
#include <cinder/Rand.h>
#include <cinder/Xml.h>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/multi/multi.hpp>

namespace terrain { namespace detail {
	
	const extern double MIN_SHAPE_AREA;

	
#pragma mark - Helpers
	
	Color next_random_color();
	
	bool is_wound_clockwise(const PolyLine2d &contour);
	
	void wind_clockwise(PolyLine2d &contour);
	
	void wind_counter_clockwise(PolyLine2d &contour);
	
	PolyLine2d transformed(const PolyLine2d &pl, const dmat4 &m);

	dpolygon2 transformed(const dpolygon2 &pl, const dmat4 &m);

	void transform(PolyLine2d &pl, const dmat4 &m);
	
	void transform(const PolyLine2d &a, PolyLine2d &b, const dmat4 &m);
	
	PolyLine2d optimize(PolyLine2d p);
	
	vector<PolyLine2d> optimize(vector<PolyLine2d> ps);
	
	PolyLine2f polyline2d_to_2f(const PolyLine2d &p2d);
	
	PolyLine2d polyline2f_to_2d(const PolyLine2f &p2f);
	
	cpBB polygon_bb(const dpolygon2 poly);
	
#pragma mark - Conversion to/from Boost::Geometry, PolyLine2d & Shape
	
	struct polyline_with_holes {
		PolyLine2d contour;
		vector<PolyLine2d> holes;
		
		polyline_with_holes(const PolyLine2d c, const vector<PolyLine2d> &h): contour(c), holes(h){}
	};
	
	vector<polyline_with_holes> dpolygon2_to_polyline_with_holes(std::vector<dpolygon2> &polygons, const dmat4 &modelview);
	
	std::vector<ShapeRef> dpolygon2_to_shape(std::vector<dpolygon2> &polygons, const dmat4 &modelview);
	
	dpolygon2 shape_to_dpolygon2( ShapeRef shape );
	
	dpolygon2 polyline2d_to_dpolygon2( const PolyLine2d &polyLine );
	
	PolyLine2d dpolygon2_to_polyline2d(dpolygon2 &poly);
	
	
#pragma mark - Contour Soup
	
	
	class contour_tree_node;
	typedef shared_ptr<contour_tree_node> contour_tree_node_ref;
	
	struct contour_tree_node {
		PolyLine2d contour;
		vector<contour_tree_node_ref> children;
	};
	
	/**
	 Given a "soup" of contours (an unordered mess of closed polylines) build a tree (with possibly
	 multiple roots) which describes their nesting in a manner useful for constructing PolyShapes
	 */
	vector<contour_tree_node_ref> build_contour_tree(const vector<PolyLine2d> &contourSoup);
	
	vector<ShapeRef> build_from_contour_tree(contour_tree_node_ref rootNode, int depth);
	
#pragma mark - Sorted Ref Pair
	
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
	
#pragma mark - Edge Test
	
	bool shared_edges(const ShapeRef &a, const ShapeRef &b);
	
#pragma mark - Flood Fill
		
	// flood fill, finding all shapes which can reach origin
	set<ShapeRef> find_contact_group(ShapeRef origin, const set<ShapeRef> &all, const map<ShapeRef,GroupBaseRef> &parentage);
	
#pragma mark - Svg
	
	void svg_traverse_elements(XmlTree node, dmat4 worldTransform, vector<pair<string,PolyLine2d>> &idsAndContours);
	
	void svg_traverse_anchors(XmlTree node, dmat4 worldTransform, vector<PolyLine2d> &contours);
	
	void svg_traverse_world(XmlTree node, dmat4 worldTransform, vector<PolyLine2d> &contours);
	
	void svg_descend(XmlTree node, dmat4 worldTransform,
					 vector<PolyLine2d> &shapeContours,
					 vector<PolyLine2d> &anchorContours,
					 vector<pair<string,PolyLine2d>> &elementIdsAndContours);
	
	void svg_load(XmlTree node, dmat4 worldTransform, vector<ShapeRef> &outShapes, vector<AnchorRef> &outAnchors, vector<ElementRef> &outElements);
	
#pragma mark - Mitering
	
	/**
	 Assuming a clockwise winding of a->b->c (where b is the corner to miter) get the inner miter vertex at a given depth.
	 In this case inner means "inside the polygon".
	 This is a helper function for computing cpSegmentShapes to perimeter a non-convex polygon to prevent small/fast objects
	 from penetrating the cracks between adjacent convex triangles assembled to a non-convex aggregate shape.
	 */
	dvec2 get_inner_miter_for_corner(dvec2 a, dvec2 b, dvec2 c, double depth);
	
	PolyLine2d inset_contour(const PolyLine2d &contour, double depth);
	
	
	
}} // namespace terrain::detail

#endif /* TerrainDetail_h */
