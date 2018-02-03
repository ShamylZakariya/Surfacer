//
//  PlanetGenerator.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 2/1/18.
//

#ifndef PlanetGenerator_hpp
#define PlanetGenerator_hpp


#include "Core.hpp"
#include "TerrainWorld.hpp"

namespace precariously { namespace planet_generation {
    
    struct params {
        int size;
        int seed;
        
        int noiseOctaves;
        float noiseFrequencyScale;
        
        bool pruneFloaters;
        dmat4 transform;
        
        params():
        size(512),
        seed(1234),
        noiseOctaves(4),
        noiseFrequencyScale(1),
        pruneFloaters(true)
        {}
    };
    
    void generate_terrain_map(const params &p, ci::Channel8u &terrain);
    void generate_anchors_map(const params &p, ci::Channel8u &terrain);

    void generate_maps(const params &p, ci::Channel8u &terrain, ci::Channel8u &anchors);
    
    pair<ci::Channel8u, ci::Channel8u> generate(const params &p, vector <terrain::ShapeRef> &shapes, vector <terrain::AnchorRef> &anchors);

    ci::Channel8u generate(const params &p, vector <terrain::ShapeRef> &shapes);
   
    pair<terrain::WorldRef, pair<ci::Channel8u, ci::Channel8u>> generate(const params &p, core::SpaceAccessRef space, terrain::material terrainMaterial, terrain::material anchorMaterial);

    pair<terrain::WorldRef, ci::Channel8u> generate(const params &p, core::SpaceAccessRef space, terrain::material terrainMaterial);


    
}} // end namespace precariously::planet_generation

#endif /* PlanetGenerator_hpp */
