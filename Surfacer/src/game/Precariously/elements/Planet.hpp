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
			
			std::size_t noiseOctaves;
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
	
	
}

#endif /* Planet_hpp */
