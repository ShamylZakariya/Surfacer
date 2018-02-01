//
//  PlanetGenerator.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 2/1/18.
//

#ifndef PlanetGenerator_hpp
#define PlanetGenerator_hpp


#include "Core.hpp"

namespace precariously { namespace planet_generation {
    
    struct params {
        int size;
        int seed;
        
        params():
        size(512),
        seed(1234)
        {}
    };
    
    void generate_terrain_map(const params &p, ci::Channel8u &terrain);
    void generate_anchors_map(const params &p, ci::Channel8u &terrain);

    void generate_maps(const params &p, ci::Channel8u &terrain, ci::Channel8u &anchors);
    
}} // end namespace precariously::planet_generation

#endif /* PlanetGenerator_hpp */
