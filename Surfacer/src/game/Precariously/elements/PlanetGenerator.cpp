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

#include "TerrainDetail.hpp"
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
        
        double circumference(const PolyLine2d &contour) {
            double dist = 0;
            for (size_t i = 0, j = 1, N = contour.size(); i < N; i++, j = (j+1) % N) {
                const dvec2 a = contour.getPoints()[i];
                const dvec2 b = contour.getPoints()[j];
                dist += length(b-a);
            }
            return dist;
        }
        
        void get_position_and_derivative(const PolyLine2d &contour, double t, dvec2 &position, dvec2 &derivative) {
            // this is taken from Cinder's position/derivative getters on contour, but merged so we only do the work once per call
            // TODO: Optimize by dropping this entirely, just get derivative for eahc segment and walk it.
            const auto &points = contour.getPoints();
            if( points.size() <= 1 ) return;
            if( t >= 1 ) {
                position = points.back();
                derivative = points.back() - points[points.size()-2];
                return;
            }
            if( t <= 0 ) {
                position = points[0];
                derivative = points[1] - points[0];
                return;
            }
            
            size_t numSpans = points.size() - 1;
            size_t span = static_cast<size_t>(math<double>::floor(t * numSpans));

            double lerpT = ( t - span / static_cast<double>(numSpans)) * numSpans;
            position = points[span] * ( 1 - lerpT ) + points[span+1] * lerpT;
            derivative = points[span+1] - points[span];
        }
        
        void get_position_and_normal(const PolyLine2d &contour, double t, dvec2 &position, dvec2 &normal) {
            dvec2 derivative;
            get_position_and_derivative(contour, t, position, derivative);
            normal = rotateCCW(normalize(derivative));
        }
        
        // walk perimeter(s) of contour calling visitor, returning false if walk was early terminated.
        // contour: contour to walk
        // step: distance to increment while walking about perimeter
        // visitor: the visitor invoked for each step, signature: bool visitor(dvec2 world, dvec2 contourNormal, bool isOuterContour)
        // if visitor returns false, walk is terminated
        bool walk_perimeter(const PolyLine2d &contour, double step, double wiggle, ci::Rand &rng, bool isOuterContour, const function<bool(dvec2,dvec2,bool)> &visitor) {

            // TODO: Add wiggle to step
            
            double len = circumference(contour);
            for (double d = 0; d <= len; d += step) {
                dvec2 position, normal;
                get_position_and_normal(contour, (d+rng.nextFloat(-wiggle, +wiggle))/len, position, normal);
                if (!visitor(position, normal, isOuterContour)) {
                    return false;
                }
            }
            
            return true;
        }
        
        // walk perimeter(s) of shape calling visitor with world space vertices.
        // shape: shape to walk
        // step: distance to increment while walking about perimeter
        // allContours: if true, walks inner ("hole") perimeters as well as primary outer perimeter
        // visitor: the visitor invoked for each step, signature: bool visitor(dvec2 world, dvec2 contourNormal, bool isOuterContour)
        // if visitor returns false, walk is terminated
        void walk_shape_perimeter(const terrain::ShapeRef &shape, double step, double wiggle, ci::Rand &rng, bool allContours, const function<bool(dvec2,dvec2,bool)> &visitor) {
            if (!walk_perimeter(shape->getOuterContour().world, step, wiggle, rng, true, visitor)) {
                return;
            }

            if (allContours) {
                for(const auto &hole : shape->getHoleContours()) {
                    if (!walk_perimeter(hole.world, step, wiggle, rng, false, visitor)) {
                        return;
                    }
                }
            }
        }
    }
    
    namespace detail {
        
        Channel8u generate_map(const params::generation_params &p, int size) {
            
            StopWatch timer("generate_map");
            Channel8u map = Channel8u(size, size);
            
            //
            //    Initilialize our terrain with perlin noise, fading out to black with radial vignette
            //
            
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
                
                const double radiusStep = size / 64.0;
                const double endRadius = size * 0.5 * p.vignetteEnd;
                
                // we want to sample the ring less and less as we step out towards end radius. so we have a
                // scale factor which increases the sample distance, and the pow factor which delinearizes the change
                const double surfaceSolidityRadIncrementPow = lrp<double>(surfaceSolidity, 0.5, 16);
                const double surfaceSolidityRadIncrmementScale = lrp<double>(surfaceSolidity, size * 0.5, size * 0.01);
                
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
        
        ci::Channel8u generate_shapes(const params &p, vector <terrain::ShapeRef> &shapes) {
            Channel8u terrainMap = generate_map(p.terrain, p.size);
            
            const double isoLevel = 0.5;
            const double linearOptimizationThreshold = 0; // zero, because terrain::Shape::fromContours performs its own optimization
            vector<PolyLine2d> contours = terrain::detail::march(terrainMap, isoLevel, p.transform, linearOptimizationThreshold);
            shapes = terrain::Shape::fromContours(contours);
            
            if (p.terrain.pruneFloaters) {
                size_t culled = prune_floater(shapes);
                if (culled > 0) {
                    CI_LOG_D("Culled " << culled << " islands from generated shapes");
                }
            }
            
            return terrainMap;
        }
        
        ci::Channel8u generate_anchors(const params &p, vector <terrain::AnchorRef> &anchors) {
            Channel8u anchorMap = generate_map(p.anchors, p.size);
            
            const double isoLevel = 0.5;
            const double linearOptimizationThreshold = 0; // zero, because terrain::Shape::fromContours performs its own optimization
            vector<PolyLine2d> contours = terrain::detail::march(anchorMap, isoLevel, p.transform, linearOptimizationThreshold);
            anchors = terrain::Anchor::fromContours(contours);
            
            return anchorMap;
        }

    }
    
    result generate(const params &params, terrain::WorldRef world) {
        StopWatch timer("generate");
        
        world->setWorldMaterial(params.terrain.material);
        world->setAnchorMaterial(params.anchors.material);
        
        vector <terrain::ShapeRef> nonPartitionedShapes, shapes;
        vector <terrain::AnchorRef> anchors;
        Channel8u terrainMap, anchorMap;
        
        if (params.terrain.enabled) {
            terrainMap = detail::generate_shapes(params, nonPartitionedShapes);
            if (params.terrain.partitionSize > 0) {
                shapes = terrain::World::partition(nonPartitionedShapes, params.terrain.partitionSize);
                CI_LOG_D("Partition size of " << params.terrain.partitionSize << " resulted in " << shapes.size() << " partitioned pieces" );
            } else {
                shapes = nonPartitionedShapes;
            }
        }
        
        if (params.anchors.enabled) {
            anchorMap = detail::generate_anchors(params, anchors);
        }
        
        world->build(shapes, anchors);
        
        result r;
        r.world = world;
        r.terrainMap = terrainMap;
        r.anchorMap = anchorMap;
        
        //
        // generate attachments
        //

        if (!params.attachments.empty()) {
            
            const dvec2 origin = params.origin();
            terrain::AttachmentRef attachment;

            ci::Rand rng;
            const double placementNudge = 1e-2;
            for(const auto &ap : params.attachments) {
                vector <terrain::AttachmentRef> attachments;
                const double step = 1/ ap.density;
                const double wiggle = step * 0.5;
                for(const terrain::ShapeRef &shape : nonPartitionedShapes) {
                    walk_shape_perimeter(shape, step, wiggle, rng, ap.includeHoleContours, [&](dvec2 position, dvec2 normal, bool isOuterContour)->bool {
                        
                        // early exit scenario
                        if (ap.maxCount > 0 && attachments.size() >= ap.maxCount) {
                            return false;
                        }

                        // dice roll, scaled by closeness to desired slope
                        double d = dot(normalize(position - origin), normal);
                        d = sqrt(1.0 - saturate<double>(abs(d - ap.normalToUpDotTarget) / ap.normalToUpDotRange));
                        if (rng.nextFloat() < ap.probability * d) {
                            dvec2 pos = position - normal * placementNudge;
                            
                            if (!attachment) {
                                attachment = make_shared<terrain::Attachment>();
                            }
                            
                            // now attempt insertion
                            if (world->addAttachment(attachment, pos, rotateCW(normal))) {
                                attachment->setTag(attachments.size());
                                attachments.push_back(attachment);
                                attachment.reset();
                            }
                        }
                        
                        return true;
                    });
                }
                CI_LOG_D("Generated " << attachments.size() << " attachments");
                r.attachmentsByBatchId[ap.batchId] = attachments;
            }
        }

        return r;
    }

    
    result generate(const params &params, core::SpaceAccessRef space) {
        auto world = make_shared<terrain::World>(space, params.terrain.material, params.anchors.material);
        return generate(params, world);
    }
    
    
}} // end namespace precariously::planet_generation
