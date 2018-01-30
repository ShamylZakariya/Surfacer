//
//  TerrainDetail_Svg.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 1/30/18.
//

#include "TerrainDetail_Svg.hpp"

#include "MathHelpers.hpp"
#include "ContourSimplification.hpp"
#include "SvgParsing.hpp"


namespace terrain { namespace detail {
	
#pragma mark - Svg
	
	void svg_traverse_elements(XmlTree node, dmat4 worldTransform, vector<pair<string,PolyLine2d>> &idsAndContours) {
		
		if ( node.hasAttribute( "transform" )) {
			worldTransform = worldTransform * core::util::svg::parseTransform(node.getAttribute( "transform" ).getValue());
		}
		
		const std::string tag = node.getTag();
		if (tag == "g") {
			
			//
			//	Descend
			//
			
			for ( auto childNode = node.begin(); childNode != node.end(); ++childNode ) {
				svg_traverse_elements(*childNode, worldTransform, idsAndContours);
			}
		} else if (core::util::svg::canParseShape(tag)) {
			
			Shape2d shape;
			core::util::svg::parseShape(node,shape);
			
			//
			//	Elements can have ONE contour each. We use the first,
			//	and move it to world space while generating it
			//
			
			PolyLine2d contour;
			for (const dvec2 v : shape.getContour(0).subdivide()) {
				contour.push_back(worldTransform * v);
			}
			
			string id = node.getAttribute("id").getValue();
			idsAndContours.push_back(make_pair(id,contour));
		}
	}
	
	void svg_traverse_anchors(XmlTree node, dmat4 worldTransform, vector<PolyLine2d> &contours) {
		
		if ( node.hasAttribute( "transform" )) {
			worldTransform = worldTransform * core::util::svg::parseTransform(node.getAttribute( "transform" ).getValue());
		}
		
		const std::string tag = node.getTag();
		if (tag == "g") {
			
			//
			//	Descend
			//
			
			for ( auto childNode = node.begin(); childNode != node.end(); ++childNode ) {
				svg_traverse_anchors(*childNode, worldTransform, contours);
			}
		} else if (core::util::svg::canParseShape(tag)) {
			
			Shape2d shape;
			core::util::svg::parseShape(node,shape);
			
			//
			//	Anchors can have ONE contour each. We use the first,
			//	and move it to world space while generating it
			//
			
			PolyLine2d contour;
			for (const dvec2 v : shape.getContour(0).subdivide()) {
				contour.push_back(worldTransform * v);
			}
			
			contours.push_back(contour);
		}
	}
	
	void svg_traverse_world(XmlTree node, dmat4 worldTransform, vector<PolyLine2d> &contours) {
		if ( node.hasAttribute( "transform" )) {
			worldTransform = worldTransform * core::util::svg::parseTransform(node.getAttribute( "transform" ).getValue());
		}
		
		const std::string tag = node.getTag();
		if (tag == "g") {
			
			//
			//	Descend
			//
			
			for ( auto childNode = node.begin(); childNode != node.end(); ++childNode ) {
				svg_traverse_world(*childNode, worldTransform, contours);
			}
		} else if (core::util::svg::canParseShape(tag)) {
			
			//
			//	This is a shape node ('rect', etc), parse it to contours, appending them to the soup
			//
			
			Shape2d shape;
			core::util::svg::parseShape(node,shape);
			
			for (const auto &path : shape.getContours()) {
				PolyLine2d contour;
				for (dvec2 v : path.subdivide()) {
					contour.push_back(worldTransform * v);
				}
				contours.push_back(contour);
			}
		}
	}
	
	
	void svg_descend(XmlTree node, dmat4 worldTransform,
					 vector<PolyLine2d> &shapeContours,
					 vector<PolyLine2d> &anchorContours,
					 vector<pair<string,PolyLine2d>> &elementIdsAndContours)
	{
		if ( node.hasAttribute( "transform" )) {
			worldTransform = worldTransform * core::util::svg::parseTransform(node.getAttribute( "transform" ).getValue());
		}
		
		if (node.hasAttribute("id")) {
			auto id = node.getAttribute("id").getValue();
			if (id == "world") {
				for ( auto childNode = node.begin(); childNode != node.end(); ++childNode ) {
					svg_traverse_world(*childNode, worldTransform, shapeContours);
				}
				return;
			} else if (id == "anchors") {
				for ( auto childNode = node.begin(); childNode != node.end(); ++childNode ) {
					svg_traverse_anchors(*childNode, worldTransform, anchorContours);
				}
				return;
			} else if (id == "elements" ) {
				for ( auto childNode = node.begin(); childNode != node.end(); ++childNode ) {
					svg_traverse_elements(*childNode, worldTransform, elementIdsAndContours);
				}
				return;
			}
		}
		
		for ( auto childNode = node.begin(); childNode != node.end(); ++childNode ) {
			svg_descend(*childNode, worldTransform, shapeContours, anchorContours, elementIdsAndContours);
		}
		
	}
	
	void svg_load(XmlTree node, dmat4 worldTransform, vector<ShapeRef> &outShapes, vector<AnchorRef> &outAnchors, vector<ElementRef> &outElements) {
		
		//
		// collect world shapes and anchors contours. Note, the world is made up
		// of a soup of contours, whereas each contour describes a single anchor.
		//
		
		vector<PolyLine2d> shapeContours, anchorContours;
		vector<pair<string,PolyLine2d>> elementIdsAndContours;
		svg_descend(node, worldTransform, shapeContours, anchorContours, elementIdsAndContours);
		
		auto shapes = Shape::fromContours(shapeContours);
		outShapes.insert(outShapes.end(), shapes.begin(), shapes.end());
		
		auto anchors = Anchor::fromContours(anchorContours);
		outAnchors.insert(outAnchors.end(), anchors.begin(), anchors.end());
		
		for (auto e : elementIdsAndContours) {
			outElements.push_back(Element::fromContour(e.first, e.second));
		}
	}
	
	
}}
