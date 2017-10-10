//
//  Planet.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 10/6/17.
//

#ifndef Planet_hpp
#define Planet_hpp

#include "Terrain.hpp"
#include "Xml.hpp"

namespace precariously {
	
	SMART_PTR(Planet);
	
	class Planet : public terrain::TerrainObject {
	public:
		
		/**
		 Description to build a planet
		 */
		struct config {
			int seed;

			double radius;
			double arcPrecisionDegrees;
			
			size_t noiseOctaves;
			double noiseFalloff;
			double noiseOffset;
			double noiseRoughness;
			
			config():
			seed(12345),
			radius(500),
			arcPrecisionDegrees(0.5),
			noiseOctaves(4),
			noiseFalloff(0.5),
			noiseOffset(0),
			noiseRoughness(0.25)
			{}
			
			static config parse(core::util::xml::XmlMultiTree node);
			
		};
		
		static PlanetRef create(string name, terrain::WorldRef world, core::util::xml::XmlMultiTree planetNode, int drawLayer);
		static PlanetRef create(string name, terrain::WorldRef world, const config &surfaceConfig, const config &coreConfig, dvec2 origin, double partitionSize, int drawLayer);
		
	public:
		
		Planet(string name, terrain::WorldRef world, int drawLayer);
		virtual ~Planet();
		
		// Object
		void onReady(core::LevelRef level) override;
		
		// Planet
		dvec2 getOrigin() const { return _origin; }
		
	private:
		
		dvec2 _origin;
		
	};
	
	typedef boost::geometry::model::polygon<boost::geometry::model::d2::point_xy<double>> polygon;

	struct radial_crack_template {
		double thickness;
		dvec2 origin;
		vector<dvec2> vertices;
		vector<vector<pair<size_t, size_t>>> spokes, rings;
		
		radial_crack_template(dvec2 origin, int numSpokes, int numRings, double radius, double thickness, double variance);
		
		polygon to_polygon() const;
		
	private:
		
		polygon to_polygon(const pair<std::size_t, std::size_t> &segment) const;
		polygon line_segment_to_polygon(dvec2 a, dvec2 b, double width) const;
	};
	
	class RadialCrackDrawComponent : public core::DrawComponent {
	public:
		
		RadialCrackDrawComponent(const radial_crack_template &crack);
		~RadialCrackDrawComponent();
		
		cpBB getBB() const override;
		void draw(const core::render_state &renderState) override;
		core::VisibilityDetermination::style getVisibilityDetermination() const override;
		int getLayer() const override;

	private:
		radial_crack_template _crackTemplate;
		TriMeshRef _trimesh;
		cpBB _bb;		
	};
	
		
	
}

#endif /* Planet_hpp */
