//
//  PlanetGenerator.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 2/1/18.
//

#include "PlanetGenerator.hpp"

#include <cinder/Rand.h>
#include <cinder/Perlin.h>

#include "PerlinNoise.hpp"
#include "MarchingSquares.hpp"
#include "ContourSimplification.hpp"
#include "ImageProcessing.hpp"

using namespace ci;
using namespace core;

namespace precariously { namespace planet_generation {

    void generate_terrain_map(const params &p, Channel8u &terrain) {
        terrain = Channel8u(p.size, p.size);
        
        //
        //    Initilialize our terrain with perlin noise, fading out to black with radial vignette
        //
        
        const float size = terrain.getWidth();
        const vec2 center(size/2, size/2);

        {
            const uint8_t octaves = 4;
            ci::Perlin pn(octaves, p.seed);
            
            const float frequency = p.size / 32;
            
            Channel8u::Iter iter = terrain.getIter();
            while (iter.line()) {
                while (iter.pixel()) {
                    float noise = pn.fBm(frequency * iter.x() / size, frequency * iter.y() / size);
                    iter.v() = noise < -0.05 || noise > 0.05 ? 255 : 0;
                }
            }
        }
        
        //
        // now perform radial samples from inside out, floodfilling
        // white blobs to grey. grey will be our marker for "solid land"
        //
        
        const uint8_t landValue = 128;
        {
            uint8_t *data = terrain.getData();
            int32_t rowBytes = terrain.getRowBytes();
            int32_t increment = terrain.getIncrement();
            
            auto get = [&](const ivec2 &p) -> uint8_t {
                return data[p.y * rowBytes + p.x * increment];
            };
            
            double ringThickness = 16;
            int ringSteps = ((p.size / 2) * 0.5) / ringThickness;
            const ivec2 center(p.size / 2, p.size / 2);
            
            for (int ringStep = 0; ringStep < ringSteps; ringStep++) {
                double radius = ringStep * ringThickness;
                
                double rads = 0;
                int radsSteps = 180 - 5 * ringStep;
                double radsIncrement = 2 * M_PI / radsSteps;
                
                for (int radsStep = 0; radsStep < radsSteps; radsStep++, rads += radsIncrement) {
                    double px = center.x + radius * cos(rads);
                    double py = center.y + radius * sin(rads);
                    ivec2 plot(static_cast<int>(round(px)), static_cast<int>(round(py)));
                    if (get(plot) == 255) {
                        util::ip::in_place::floodfill(terrain, plot, 255, landValue);
                    }
                }
            }
        }
        
        //
        // Remap to make "land" white, and eveything else black. Then a blur pass
        // which will cause nearby landmasses to touch when marching_squares is run.
        // We don't need to threshold because marching_squares' isoLevel param is our pinion
        //
        
        util::ip::in_place::remap(terrain, landValue, 255, 0);
        terrain = util::ip::blur(terrain, 15);
        
        //
        //  Now apply vignette to prevent blobs from touching edges
        //
        
        {
            const float outerVignetteRadius = size * 0.5f;
            const float innerVignetteRadius = outerVignetteRadius * 0.9f;
            const float innerVignetteRadius2 = innerVignetteRadius * innerVignetteRadius;
            const float vignetteThickness = outerVignetteRadius - innerVignetteRadius;

            Channel8u::Iter iter = terrain.getIter();
            while (iter.line()) {
                while (iter.pixel()) {
                    float radius2 = lengthSquared(vec2(iter.getPos()) - center);
                    if (radius2 > innerVignetteRadius2) {
                        float radius = sqrt(radius2);
                        float vignette = 1 - min<float>(((radius - innerVignetteRadius) / vignetteThickness), 1);
                        float val = static_cast<float>(iter.v()) / 255.f;
                        iter.v() = static_cast<uint8>(vignette * val * 255);
                    }
                }
            }
        }
    }

    void generate_anchors_map(const params &p, Channel8u &terrain) {
        
    }

    
    void generate_maps(const params &p, Channel8u &terrain, Channel8u &anchors) {
        generate_terrain_map(p, terrain);
        generate_anchors_map(p, anchors);
    }

    
}} // end namespace precariously::planet_generation
