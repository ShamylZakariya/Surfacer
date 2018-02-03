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

#include "TerrainDetail_MarchingSquares.hpp"

using namespace ci;
using namespace core;

namespace precariously { namespace planet_generation {
    
    namespace {
        
        void generate_map(const params &p, double vignetteStart, Channel8u &map) {
            map = Channel8u(p.size, p.size);
            
            //
            //    Initilialize our terrain with perlin noise, fading out to black with radial vignette
            //
            
            const float size = map.getWidth();
            const vec2 center(size/2, size/2);
            
            {
                ci::Perlin pn(p.noiseOctaves, p.seed);
                const float frequency = p.noiseFrequencyScale * p.size / 32.f;
                Channel8u::Iter iter = map.getIter();
                
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
                uint8_t *data = map.getData();
                int32_t rowBytes = map.getRowBytes();
                int32_t increment = map.getIncrement();
                
                auto get = [&](const ivec2 &p) -> uint8_t {
                    return data[p.y * rowBytes + p.x * increment];
                };
                
                double ringThickness = p.size / 32.0;
                int ringSteps = static_cast<int>(((p.size / 2) * 0.5) / ringThickness);
                
                for (int ringStep = 0; ringStep < ringSteps; ringStep++) {
                    double radius = ringStep * ringThickness;
                    
                    // TODO: Compute the arc-width of a pixel at a given radius, and step the rads by that amount * some scalar.
                    double rads = 0;
                    int radsSteps = 180 - 5 * ringStep;
                    double radsIncrement = 2 * M_PI / radsSteps;
                    
                    for (int radsStep = 0; radsStep < radsSteps; radsStep++, rads += radsIncrement) {
                        double px = center.x + radius * cos(rads);
                        double py = center.y + radius * sin(rads);
                        ivec2 plot(static_cast<int>(round(px)), static_cast<int>(round(py)));
                        if (get(plot) == 255) {
                            util::ip::in_place::floodfill(map, plot, 255, landValue);
                        }
                    }
                }
            }
            
            //
            // Remap to make "land" white, and eveything else black. Then a blur pass
            // which will cause nearby landmasses to touch when marching_squares is run.
            // We don't need to threshold because marching_squares' isoLevel param is our thresholder
            //
            
            util::ip::in_place::remap(map, landValue, 255, 0);
            
            // TODO: Compute blur radius based on parameters
            map = util::ip::blur(map, 15);
            
            //
            //  Now apply vignette to prevent blobs from touching edges
            //
            
            {
                const float outerVignetteRadius = size * 0.5f;
                const float innerVignetteRadius = outerVignetteRadius * vignetteStart;
                const float innerVignetteRadius2 = innerVignetteRadius * innerVignetteRadius;
                const float vignetteThickness = outerVignetteRadius - innerVignetteRadius;
                
                Channel8u::Iter iter = map.getIter();
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
        
        void prune_floater(vector<terrain::ShapeRef> &shapes) {
            if (shapes.size() > 1) {
                // find biggest shape, keep it only
                sort(shapes.begin(), shapes.end(), [](const terrain::ShapeRef &a, const terrain::ShapeRef & b) -> bool {
                    return a->getSurfaceArea() > b->getSurfaceArea();
                });
                shapes = vector<terrain::ShapeRef> { shapes.front() };
            }
        }
        
    }

    void generate_terrain_map(const params &p, Channel8u &terrain) {
        generate_map(p, 0.9, terrain);
    }

    void generate_anchors_map(const params &p, Channel8u &anchors) {
        params p2 = p;
        p2.seed++;
        generate_map(p2, 0.5, anchors);
    }

    void generate_maps(const params &p, Channel8u &terrain, Channel8u &anchors) {
        generate_terrain_map(p, terrain);
        generate_anchors_map(p, anchors);
    }
    
    pair<ci::Channel8u, ci::Channel8u> generate(const params &p, vector <terrain::ShapeRef> &shapes, vector <terrain::AnchorRef> &anchors) {
        Channel8u terrainMap, anchorsMap;
        generate_maps(p, terrainMap, anchorsMap);

        double isoLevel = 0.5;
        shapes = terrain::Shape::fromContours(terrain::detail::march(terrainMap, isoLevel, p.transform, 0.01));
        anchors = terrain::Anchor::fromContours(terrain::detail::march(anchorsMap, isoLevel, p.transform, 0.01));
        
        if (p.pruneFloaters) {
            prune_floater(shapes);
        }
        
        return make_pair(terrainMap, anchorsMap);
    }
    
    ci::Channel8u generate(const params &p, vector <terrain::ShapeRef> &shapes) {
        Channel8u terrainMap;
        generate_terrain_map(p, terrainMap);
        
        double isoLevel = 0.5;
        double linearOptimizationThreshold = 0.01;
        vector<PolyLine2d> contours = terrain::detail::march(terrainMap, isoLevel, p.transform, linearOptimizationThreshold);
        shapes = terrain::Shape::fromContours(contours);
        
        if (p.pruneFloaters) {
            prune_floater(shapes);
        }
        
        return terrainMap;
    }
    
    pair<terrain::WorldRef, pair<ci::Channel8u, ci::Channel8u>> generate(const params &p, core::SpaceAccessRef space, terrain::material terrainMaterial, terrain::material anchorMaterial) {

        vector <terrain::ShapeRef> shapes;
        vector <terrain::AnchorRef> anchors;
        auto maps = generate(p, shapes, anchors);
        
        auto world = make_shared<terrain::World>(space, terrainMaterial, anchorMaterial);
        world->build(shapes, anchors);
        return make_pair(world, maps);
    }
    
    pair<terrain::WorldRef, ci::Channel8u> generate(const params &p, core::SpaceAccessRef space, terrain::material terrainMaterial) {
        auto world = make_shared<terrain::World>(space, terrainMaterial, terrain::material());

        vector <terrain::ShapeRef> shapes;
        auto map = generate(p, shapes);
        
        world->build(shapes);
        
        return make_pair(world, map);
    }

    
}} // end namespace precariously::planet_generation
