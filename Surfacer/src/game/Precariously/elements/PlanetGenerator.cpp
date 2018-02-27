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
        
        /**
         Prune any unconnected floating islands that may have been generated.
         Return number of pruned islands.
         */
        size_t prune_floater(vector<terrain::ShapeRef> &shapes) {
            if (shapes.size() > 1) {
                
                //
                // sort such that biggest shape is front in list, and then keep just that one
                //

                sort(shapes.begin(), shapes.end(), [](const terrain::ShapeRef &a, const terrain::ShapeRef & b) -> bool {
                    return a->getSurfaceArea() > b->getSurfaceArea();
                });
                size_t originalSize = shapes.size();
                shapes = vector<terrain::ShapeRef> { shapes.front() };
                size_t newSize = shapes.size();

                return originalSize - newSize;
            }
            return 0;
        }
        
    }
    
    Channel8u generate_map(const params &p) {
        
        StopWatch timer("generate_map");
        Channel8u map = Channel8u(p.size, p.size);
        
        //
        //    Initilialize our terrain with perlin noise, fading out to black with radial vignette
        //
        
        const float size = map.getWidth();
        const vec2 center(size/2, size/2);
        
        {
            ci::Perlin pn(p.noiseOctaves, p.seed);
            const float frequency = p.noiseFrequencyScale / 32.f;
            core::util::ip::in_place::perlin_abs_thresh(map, pn, frequency, 12);
        }
        
        //
        // now perform radial samples from inside out, floodfilling
        // white blobs to grey. grey will be our marker for "solid land". we
        // sample less frequently as we move out so as to allow for "swiss cheese" surface
        //
        
        const double surfaceSolidity = saturate<double>(p.surfaceSolidity);
        const uint8_t landValue = 128;
        
        {
            uint8_t *data = map.getData();
            int32_t rowBytes = map.getRowBytes();
            int32_t increment = map.getIncrement();
            
            auto sample = [&](const ivec2 &p) -> uint8_t {
                return data[p.y * rowBytes + p.x * increment];
            };
            
            const double radiusStep = p.size / 64.0;
            const double endRadius = p.size * 0.5 * p.vignetteEnd;
            
            // we want to sample the ring less and less as we step out towards end radius. so we have a
            // scale factor which increases the sample distance, and the pow factor which delinearizes the change
            const double surfaceSolidityRadIncrementPow = lrp<double>(surfaceSolidity, 0.5, 16);
            const double surfaceSolidityRadIncrmementScale = lrp<double>(surfaceSolidity, p.size * 0.5, p.size * 0.01);
            
            // walk a ring of positions from center out to endRadius. skip the origin pixel to avoid div by zero
            for (double radius = 1; radius <= endRadius; radius += radiusStep) {
                
                const double pixelArcWidth = abs(sin(1/radius)); // approx arc-width of 1 pixel at this radius
                const double progress = radius / endRadius;
                const double radsIncrement = max<double>(surfaceSolidityRadIncrmementScale * pow(progress, surfaceSolidityRadIncrementPow) * pixelArcWidth, 2 * pixelArcWidth);
                
                for (double radians = 0; radians < 2 * M_PI; radians += radsIncrement) {
                    double px = center.x + radius * cos(radians);
                    double py = center.y + radius * sin(radians);
                    ivec2 plot(static_cast<int>(round(px)), static_cast<int>(round(py)));
                    if (sample(plot) == 255) {
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
        
        //
        // blur to join up the island masses so marching squares isolevel will treat them as a single solid
        // use more blur for more solid surfaces since it will join more
        //
        
        int blurRadius = static_cast<int>(lrp<double>(surfaceSolidity, 5, 21));
        map = util::ip::blur(map, blurRadius);
        
        
        //
        // Now apply vignette to prevent blobs from touching edges and, generally,
        // circularize the median geometry
        //
        
        util::ip::in_place::vignette(map, p.vignetteStart, p.vignetteEnd, 0);
        
        return map;
    }

    pair<ci::Channel8u, ci::Channel8u> generate_maps(const params &p) {
        params p2 = p;
        p2.seed++;

        return make_pair(generate_map(p), generate_map(p2));
    }
    
    pair<ci::Channel8u, ci::Channel8u> generate(const params &p, vector <terrain::ShapeRef> &shapes, vector <terrain::AnchorRef> &anchors) {

        params p2 = p;
        p2.seed++;
        p2.vignetteEnd *= 0.5;
        p2.vignetteStart += 0.5;

        auto terrainMap = generate_map(p);
        auto anchorsMap = generate_map(p2);

        const double isoLevel = 0.5;
        const double linearOptimizationThreshold = 0; // zero, because terrain::Shape::fromContours performs its own optimization
        shapes = terrain::Shape::fromContours(terrain::detail::march(terrainMap, isoLevel, p.transform, linearOptimizationThreshold));
        anchors = terrain::Anchor::fromContours(terrain::detail::march(anchorsMap, isoLevel, p.transform, linearOptimizationThreshold));
        
        if (p.pruneFloaters) {
            size_t culled = prune_floater(shapes);
            if (culled > 0) {
                CI_LOG_D("Culled " << culled << " islands from generated shapes");
            }
        }
        
        return make_pair(terrainMap, anchorsMap);
    }
    
    ci::Channel8u generate(const params &p, vector <terrain::ShapeRef> &shapes) {
        Channel8u terrainMap = generate_map(p);
        
        const double isoLevel = 0.5;
        const double linearOptimizationThreshold = 0; // zero, because terrain::Shape::fromContours performs its own optimization
        vector<PolyLine2d> contours = terrain::detail::march(terrainMap, isoLevel, p.transform, linearOptimizationThreshold);
        shapes = terrain::Shape::fromContours(contours);
        
        if (p.pruneFloaters) {
            size_t culled = prune_floater(shapes);
            if (culled > 0) {
                CI_LOG_D("Culled " << culled << " islands from generated shapes");
            }
        }
        
        return terrainMap;
    }
    
    ci::Channel8u generate(const params &p, vector <terrain::AnchorRef> &anchors) {
        Channel8u anchorMap = generate_map(p);
        
        const double isoLevel = 0.5;
        const double linearOptimizationThreshold = 0; // zero, because terrain::Shape::fromContours performs its own optimization
        vector<PolyLine2d> contours = terrain::detail::march(anchorMap, isoLevel, p.transform, linearOptimizationThreshold);
        anchors = terrain::Anchor::fromContours(contours);
        
        return anchorMap;
    }
    
    pair<terrain::WorldRef, pair<ci::Channel8u, ci::Channel8u>> generate(const params &p, core::SpaceAccessRef space, terrain::material terrainMaterial, terrain::material anchorMaterial) {
        
        StopWatch timer("generate");

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
