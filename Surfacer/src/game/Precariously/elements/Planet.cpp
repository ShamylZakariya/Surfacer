//
//  Planet.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 10/6/17.
//

#include "Planet.hpp"

#include "TerrainDetail.hpp"
#include "PerlinNoise.hpp"
#include "PrecariouslyConstants.hpp"

#include "cinder/Rand.h"

using namespace core;
using namespace terrain;

namespace {
	
	using terrain::detail::polygon;
	polygon circle(dvec2 origin, double radius, double arcPrecisionDegrees, util::PerlinNoise *permuter, double noiseRoughness, double noiseOffset) {
		permuter->setScale(noiseRoughness);
		polygon contour;
		for (double angle = 0; angle < 360; angle += arcPrecisionDegrees)
		{
			double radians = angle * M_PI / 180;
			double noise = permuter->noise(angle) * noiseOffset;
			double vertexRadius = radius + noise;
			
			vec2 v(origin.x + vertexRadius * cos(radians), origin.y + vertexRadius * sin(radians));
			contour.outer().push_back(boost::geometry::make<boost::geometry::model::d2::point_xy<double> >( v.x, v.y ));
		}
		boost::geometry::correct( contour );
		return contour;
	}
	
}

namespace precariously {
	
	Planet::config Planet::config::parse(core::util::xml::XmlMultiTree configNode) {
		config c;
		
		c.seed = util::xml::readNumericAttribute<int>(configNode, "seed", c.seed);
		
		c.radius = util::xml::readNumericAttribute<double>(configNode, "radius", c.radius);
		c.arcPrecisionDegrees = util::xml::readNumericAttribute<double>(configNode, "arcPrecisionDegrees", c.arcPrecisionDegrees);

		c.noiseOctaves = util::xml::readNumericAttribute<size_t>(configNode, "noiseOctaves", c.noiseOctaves);
		c.noiseFalloff = util::xml::readNumericAttribute<double>(configNode, "noiseFalloff", c.noiseFalloff);
		c.noiseOffset = util::xml::readNumericAttribute<double>(configNode, "noiseOffset", c.noiseOffset);
		c.noiseRoughness = util::xml::readNumericAttribute<double>(configNode, "noiseRoughness", c.noiseRoughness);
		
		return c;
	}
	
	PlanetRef Planet::create(string name, terrain::WorldRef world, core::util::xml::XmlMultiTree planetNode, int drawLayer) {
		dvec2 origin = util::xml::readPointAttribute(planetNode, "origin", dvec2(0,0));
		double partitionSize = util::xml::readNumericAttribute<double>(planetNode, "partitionSize", 250);
		config surfaceConfig = config::parse(planetNode.getChild("surface"));
		config coreConfig = config::parse(planetNode.getChild("core"));
		return create(name, world, surfaceConfig, coreConfig, origin, partitionSize, drawLayer);
	}
	
	PlanetRef Planet::create(string name, terrain::WorldRef world, const config &surfaceConfig, const config &coreConfig, dvec2 origin, double partitionSize, int drawLayer) {
		
		util::PerlinNoise surfacePermuter(surfaceConfig.noiseOctaves, surfaceConfig.noiseFalloff, 1, surfaceConfig.seed);
		util::PerlinNoise corePermuter(coreConfig.noiseOctaves, coreConfig.noiseFalloff, 1, coreConfig.seed);
		
		// create basic planet outer contour
		vector<polygon> contourPolygons = {
			circle(origin, surfaceConfig.radius, surfaceConfig.arcPrecisionDegrees, &surfacePermuter, surfaceConfig.noiseRoughness, surfaceConfig.noiseOffset)
		};

		// build planet core contour
		polygon corePolygon = circle(origin, coreConfig.radius, coreConfig.arcPrecisionDegrees, &corePermuter, surfaceConfig.noiseRoughness, coreConfig.noiseOffset);
		PolyLine2d corePolyline = terrain::detail::convertBoostGeometryToPolyline2d(corePolygon);

		// now convert the contours to a form consumable by terrain::World::build
		vector<ShapeRef> shapes = terrain::detail::convertBoostGeometryToTerrainShapes(contourPolygons, dmat4());
		
		if (partitionSize > 0) {
			shapes = terrain::World::partition(shapes, origin, partitionSize);
		}

		vector<AnchorRef> anchors = { terrain::Anchor::fromContour(corePolyline) };
		world->build(shapes, anchors);
		
		// then perturb the world with additions (mountains, junk, etc) and subtractions (craters, lakebeds, etc)

		// finally create the Planet
		return make_shared<Planet>(name, world, drawLayer);
	}
	
	Planet::Planet(string name, WorldRef world, int drawLayer):
	TerrainObject(name, world, drawLayer)
	{}
	
	Planet::~Planet()
	{}
	
	void Planet::onReady(core::LevelRef level) {
		TerrainObject::onReady(level);
	}
	
#pragma mark - radial_crack_data
	
	/*
	 double thickness;
	 dvec2 origin;
	 vector<dvec2> vertices;
	 vector<pair<size_t,size_t>> lines;
	 */
	radial_crack_template::radial_crack_template(dvec2 origin, int numSpokes, int numRings, double radius, double thickness, double variance):
	thickness(thickness),
	origin(origin)
	{
		// compute max variance that won't result in degenerate geometry
		variance = min(variance, (radius / numSpokes) - 2 * thickness);
		
		auto plotRingVertex = [origin](double ringRadius, double theta) -> dvec2 {
			return origin + ringRadius * dvec2(cos(theta),sin(theta));
		};
		
		Rand rng;
		vertices.push_back(origin);
		for (int i = 1; i <= numRings; i++) {
			double ringRadius = (radius * i) / numRings;
			double arcWidth = distance(plotRingVertex(ringRadius, 0), plotRingVertex(ringRadius, 2 * M_PI / numSpokes));
			double ringVariance = min(variance, arcWidth);
			for (int j = 0; j < numSpokes; j++) {
				double angleRads = (2.0 * M_PI * j) / numSpokes;
				dvec2 v = plotRingVertex(ringRadius, angleRads);
				if (ringVariance > 0) {
					v += ringVariance * dvec2(rng.nextVec2());
				}
				vertices.push_back(v);
			}
		}

		// build spokes
		for (size_t spoke = 0; spoke < numSpokes; spoke++) {
			vector<pair<size_t, size_t>> spokeSegments;
			size_t a = 0;
			for (size_t ring = 1; ring <= numRings; ring++) {
				size_t b = (ring - 1) * numSpokes + 1 + spoke;
				spokeSegments.push_back(make_pair(a,b));
				a = b;
			}
			spokes.push_back(spokeSegments);
		}
		
		// build rings
		for (int ring = 1; ring <= numRings; ring++) {
			vector<pair<size_t, size_t>> ringSegments;
			for (int spoke = 0; spoke < numSpokes; spoke++) {
				size_t a = (ring - 1) * numSpokes + 1 + spoke;
				size_t b = spoke < numSpokes - 1 ? a + 1 : ((ring-1) * numSpokes + 1);
				ringSegments.push_back(make_pair(a,b));
			}
			rings.push_back(ringSegments);
		}
		
	}
	
	polygon radial_crack_template::to_polygon() const {
		vector<polygon> polygons;
		vector<polygon> collector;
		
		// boost::geometry::union only takes two polygons at a time, and returns a vector of polygons.
		// this means, if two polygons dont overlap, the result is two polygons. If they do overlap,
		// the result is one. Note, a polygon has outer contour, and a vector of "holes" so if we
		// carefully order the construction of this polygon, we can always have a single polygon
		// contour with N holes.
		// so we're building the spokes from center out, always maintaining a single polygon. Then
		// we build the rings as single polygons, then merge those rings with the spoke structure.
		
		// connect spoke segments into array of polygons where each is a single spoke
		for (const auto &segments : spokes) {
			polygon spoke = to_polygon(segments[0]);
			for (size_t i = 1, N = segments.size(); i < N; i++) {
				boost::geometry::union_(spoke, to_polygon(segments[i]), collector);
				CI_ASSERT_MSG(collector.size() == 1, "When joining spoke segment polygons expect result of length 1");
				spoke = collector[0];
				collector.clear();
			}
			polygons.push_back(spoke);
		}
		
		// join spokes into single polygon
		polygon joinedSpokes = polygons[0];
		for (size_t i = 1, N = polygons.size(); i < N; i++) {
			boost::geometry::union_(joinedSpokes, polygons[i], collector);
			CI_ASSERT_MSG(collector.size() == 1, "When joining spoke polygons expect result of length 1");
			joinedSpokes = collector[0];
			collector.clear();
		}
		
		// join ring segments into array of polygons where each is a single ring
		polygons.clear();
		collector.clear();
		for (const auto &segments : rings) {
			polygon ring = to_polygon(segments[0]);
			for (size_t i = 1, N = segments.size(); i < N; i++) {
				boost::geometry::union_(ring, to_polygon(segments[i]), collector);
				CI_ASSERT_MSG(collector.size() == 1, "When joining ring segment polygons expect result of length 1");
				ring = collector[0];
				collector.clear();
			}
			polygons.push_back(ring);
		}
		
		// merge rings with the joinedSpokes
		polygon result = joinedSpokes;
		for (const auto &ring : polygons) {
			boost::geometry::union_(result, ring, collector);
			CI_ASSERT_MSG(collector.size() == 1, "When joining ring polygons with spokes, expect result of length 1");
			result = collector[0];
			collector.clear();
		}

		return result;
	}
	
	polygon radial_crack_template::to_polygon(const pair<std::size_t, std::size_t> &segment) const {
		return line_segment_to_polygon(vertices[segment.first], vertices[segment.second], thickness);
	}
	
	polygon radial_crack_template::line_segment_to_polygon(dvec2 a, dvec2 b, double width) const {
		typedef boost::geometry::model::d2::point_xy<double> point;
		using boost::geometry::make;
		
		double halfWidth = width * 0.5;
		polygon result;
		dvec2 dir = b - a;
		double len = length(dir);
		if (len > 0) {
			dir /= len;
			dvec2 back = -halfWidth * 0.5 * dir;
			dvec2 forward = -back;
			dvec2 right = halfWidth * rotateCW(dir);
			dvec2 left = -right;
			
			dvec2 aRight = a + right;
			dvec2 aBack = a + back;
			dvec2 aLeft = a + left;
			dvec2 bLeft = b + left;
			dvec2 bForward = b + forward;
			dvec2 bRight = b + right;
			
			result.outer().push_back(make<point>( aRight.x, aRight.y ));
			result.outer().push_back(make<point>( aBack.x, aBack.y ));
			result.outer().push_back(make<point>( aLeft.x, aLeft.y ));
			result.outer().push_back(make<point>( bLeft.x, bLeft.y ));
			result.outer().push_back(make<point>( bForward.x, bForward.y ));
			result.outer().push_back(make<point>( bRight.x, bRight.y ));
			
			boost::geometry::correct(result);
		}
		return result;
	}
	
#pragma mark - RadialCrackDrawComponent
	
	/*
	 radial_crack_template _crackTemplate;
	 cpBB _bb;
	 */
	
	RadialCrackDrawComponent::RadialCrackDrawComponent(const radial_crack_template &crack):
	_crackTemplate(crack)
	{
		_bb = cpBBInvalid;
		for (const auto &v : crack.vertices) {
			_bb = cpBBExpand(_bb, v);
		}
		
		// convert polygon to something we can tesselate
		vector<polygon> crackPolys = { crack.to_polygon() };
		vector<terrain::detail::polyline_with_holes> result = terrain::detail::buildPolyLinesFromBoostGeometry(crackPolys, dmat4());

		Triangulator triangulator;
		for (const auto &thing : result) {
			triangulator.addPolyLine(terrain::detail::polyline2d_to_2f(thing.contour));
			for (const auto &hole : thing.holes) {
				triangulator.addPolyLine(terrain::detail::polyline2d_to_2f(hole));
			}
		}
		
		_trimesh = triangulator.createMesh();
	}
	
	RadialCrackDrawComponent::~RadialCrackDrawComponent(){}
	
	cpBB RadialCrackDrawComponent::getBB() const {
		return _bb;
	}

	void RadialCrackDrawComponent::draw(const render_state &renderState) {
		gl::color(1,1,1);
		gl::draw(*_trimesh);

		gl::color(0,0,0);
		for (const auto &spoke : _crackTemplate.spokes) {
			for (const auto &seg : spoke) {
				gl::drawLine(_crackTemplate.vertices[seg.first],_crackTemplate.vertices[seg.second]);
			}
		}
		for (const auto &ring : _crackTemplate.rings) {
			for (const auto &seg : ring) {
				gl::drawLine(_crackTemplate.vertices[seg.first],_crackTemplate.vertices[seg.second]);
			}
		}
		
	}
	
	VisibilityDetermination::style RadialCrackDrawComponent::getVisibilityDetermination() const {
		return VisibilityDetermination::FRUSTUM_CULLING;
	}

	int RadialCrackDrawComponent::getLayer() const {
		return DrawLayers::EFFECTS;
	}

	
}
