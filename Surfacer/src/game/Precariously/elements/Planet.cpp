//
//  Planet.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 10/6/17.
//

#include "Planet.hpp"

#include "TerrainDetail.hpp"
#include "PerlinNoise.hpp"

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

		c.noiseOctaves = util::xml::readNumericAttribute<std::size_t>(configNode, "noiseOctaves", c.noiseOctaves);
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
	
}
