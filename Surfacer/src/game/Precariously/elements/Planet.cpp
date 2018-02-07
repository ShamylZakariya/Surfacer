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
#include "PlanetGenerator.hpp"


using namespace core;
using namespace terrain;


namespace precariously {

    namespace {
        
        const int MAP_RESOLUTION = 512;
    
        precariously::planet_generation::params create_planet_generation_params(const Planet::config &config, dvec2 worldOrigin, double worldRadius) {
            precariously::planet_generation::params params;
            params.size = MAP_RESOLUTION;
            params.seed = config.seed;
            params.surfaceSolidity = config.surfaceSolidity;
            
            //
            // compute a transform to put this at origin with requested radius
            //

            params.transform = glm::scale( dvec3(worldRadius / params.size, worldRadius / params.size, 1)) * glm::translate(dvec3(worldOrigin.x - params.size/2, worldOrigin.y - params.size/2, 0));
            
            return params;
        }
        
    }

    Planet::config Planet::config::parse(core::util::xml::XmlMultiTree configNode) {
        config c;
        c.seed = util::xml::readNumericAttribute<int>(configNode, "seed", c.seed);
        c.radius = util::xml::readNumericAttribute<double>(configNode, "radius", c.radius);
        c.surfaceSolidity = util::xml::readNumericAttribute<double>(configNode, "surfaceSolidity", c.surfaceSolidity);

        return c;
    }

    PlanetRef Planet::create(string name, terrain::WorldRef world, core::util::xml::XmlMultiTree planetNode, int drawLayer) {
        dvec2 origin = util::xml::readPointAttribute(planetNode, "origin", dvec2(0, 0));
        double partitionSize = util::xml::readNumericAttribute<double>(planetNode, "partitionSize", 250);
        config surfaceConfig = config::parse(planetNode.getChild("surface"));
        config coreConfig = config::parse(planetNode.getChild("core"));
        return create(name, world, surfaceConfig, coreConfig, origin, partitionSize, drawLayer);
    }

    PlanetRef Planet::create(string name, terrain::WorldRef world, const config &surfaceConfig, const config &coreConfig, dvec2 origin, double partitionSize, int drawLayer) {
        
        planet_generation::params surfaceParams = create_planet_generation_params(surfaceConfig, origin, surfaceConfig.radius);
        planet_generation::params coreParams = create_planet_generation_params(coreConfig, origin, coreConfig.radius);

        vector<ShapeRef> shapes;
        vector<AnchorRef> anchors;
        planet_generation::generate(surfaceParams, shapes);
        planet_generation::generate(coreParams, anchors);

        if (partitionSize > 0) {
            shapes = terrain::World::partition(shapes, origin, partitionSize);
        }

        world->build(shapes, anchors);

        // finally create the Planet
        return make_shared<Planet>(name, world, drawLayer);
    }

    Planet::Planet(string name, WorldRef world, int drawLayer) :
            TerrainObject(name, world, drawLayer) {
    }

    Planet::~Planet() {
    }

    void Planet::onReady(core::StageRef stage) {
        TerrainObject::onReady(stage);
    }

#pragma mark - CrackGeometry

    dpolygon2 CrackGeometry::lineSegmentToPolygon(dvec2 a, dvec2 b, double width) {
        typedef boost::geometry::model::d2::point_xy<double> point;
        using boost::geometry::make;

        double halfWidth = width * 0.5;
        dpolygon2 result;
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

            result.outer().push_back(make<point>(aRight.x, aRight.y));
            result.outer().push_back(make<point>(aBack.x, aBack.y));
            result.outer().push_back(make<point>(aLeft.x, aLeft.y));
            result.outer().push_back(make<point>(bLeft.x, bLeft.y));
            result.outer().push_back(make<point>(bForward.x, bForward.y));
            result.outer().push_back(make<point>(bRight.x, bRight.y));

            boost::geometry::correct(result);
        }
        return result;
    }

#pragma mark - radial_crack_data

    /*
     double _thickness;
     dvec2 _origin;
     vector<dvec2> _vertices;
     vector<vector<pair<size_t, size_t>>> _spokes, _rings;
     polygon _polygon;
     cpBB _bb;
     */
    RadialCrackGeometry::RadialCrackGeometry(dvec2 origin, int numSpokes, int numRings, double radius, double thickness, double variance)
            :
            _thickness(thickness),
            _origin(origin),
            _bb(cpBBInvalid) {
        // compute max variance that won't result in degenerate geometry
        variance = min(variance, (radius / numSpokes) - 2 * thickness);

        auto plotRingVertex = [origin](double ringRadius, double theta) -> dvec2 {
            return origin + ringRadius * dvec2(cos(theta), sin(theta));
        };


        _vertices.push_back(origin);
        for (int i = 1; i <= numRings + 1; i++) {
            double ringRadius = (radius * i) / numRings;
            double arcWidth = distance(plotRingVertex(ringRadius, 0), plotRingVertex(ringRadius, 2 * M_PI / numSpokes));
            double ringVariance = min(variance, arcWidth);
            for (int j = 0; j < numSpokes; j++) {
                double angleRads = (2.0 * M_PI * j) / numSpokes;
                dvec2 v = plotRingVertex(ringRadius, angleRads);
                if (ringVariance > 0) {
                    v += ringVariance * dvec2(Rand::randVec2());
                }
                _vertices.push_back(v);
                _bb = cpBBExpand(_bb, v);
            }
        }

        // build spokes
        for (size_t spoke = 0; spoke < numSpokes; spoke++) {
            vector<pair<size_t, size_t>> spokeSegments;
            size_t a = 0;
            for (size_t ring = 1; ring <= numRings + 1; ring++) {
                size_t b = (ring - 1) * numSpokes + 1 + spoke;
                spokeSegments.push_back(make_pair(a, b));
                a = b;
            }
            _spokes.push_back(spokeSegments);
        }

        // build rings
        for (int ring = 1; ring <= numRings; ring++) {
            vector<pair<size_t, size_t>> ringSegments;
            for (int spoke = 0; spoke < numSpokes; spoke++) {
                size_t a = (ring - 1) * numSpokes + 1 + spoke;
                size_t b = spoke < numSpokes - 1 ? a + 1 : ((ring - 1) * numSpokes + 1);
                ringSegments.push_back(make_pair(a, b));
            }
            _rings.push_back(ringSegments);
        }

        _polygon = createPolygon();
    }

    void RadialCrackGeometry::debugDrawSkeleton() const {
        for (const auto &spoke : _spokes) {
            for (const auto &seg : spoke) {
                gl::drawLine(_vertices[seg.first], _vertices[seg.second]);
            }
        }
        for (const auto &ring : _rings) {
            for (const auto &seg : ring) {
                gl::drawLine(_vertices[seg.first], _vertices[seg.second]);
            }
        }
    }

    dpolygon2 RadialCrackGeometry::createPolygon() const {
        vector<dpolygon2> polygons;
        vector<dpolygon2> collector;

        // boost::geometry::union only takes two polygons at a time, and returns a vector of polygons.
        // this means, if two polygons dont overlap, the result is two polygons. If they do overlap,
        // the result is one. Note, a polygon has outer contour, and a vector of "holes" so if we
        // carefully order the construction of this polygon, we can always have a single polygon
        // contour with N holes.
        // so we're building the spokes from center out, always maintaining a single polygon. Then
        // we build the rings as single polygons, then merge those rings with the spoke structure.

        // connect spoke segments into array of polygons where each is a single spoke
        for (const auto &segments : _spokes) {
            dpolygon2 spoke = createPolygon(segments[0]);
            for (size_t i = 1, N = segments.size(); i < N; i++) {
                boost::geometry::union_(spoke, createPolygon(segments[i]), collector);
                CI_ASSERT_MSG(collector.size() == 1, "When joining spoke segment polygons expect result of length 1");
                spoke = collector[0];
                collector.clear();
            }
            polygons.push_back(spoke);
        }

        // join spokes into single polygon
        dpolygon2 joinedSpokes = polygons[0];
        for (size_t i = 1, N = polygons.size(); i < N; i++) {
            boost::geometry::union_(joinedSpokes, polygons[i], collector);
            CI_ASSERT_MSG(collector.size() == 1, "When joining spoke polygons expect result of length 1");
            joinedSpokes = collector[0];
            collector.clear();
        }

        // join ring segments into array of polygons where each is a single ring
        polygons.clear();
        collector.clear();
        for (const auto &segments : _rings) {
            dpolygon2 ring = createPolygon(segments[0]);
            for (size_t i = 1, N = segments.size(); i < N; i++) {
                boost::geometry::union_(ring, createPolygon(segments[i]), collector);
                CI_ASSERT_MSG(collector.size() == 1, "When joining ring segment polygons expect result of length 1");
                ring = collector[0];
                collector.clear();
            }
            polygons.push_back(ring);
        }

        // merge rings with the joinedSpokes
        dpolygon2 result = joinedSpokes;
        for (const auto &ring : polygons) {
            boost::geometry::union_(result, ring, collector);
            CI_ASSERT_MSG(collector.size() == 1, "When joining ring polygons with spokes, expect result of length 1");
            result = collector[0];
            collector.clear();
        }

        return result;
    }

    dpolygon2 RadialCrackGeometry::createPolygon(const pair<std::size_t, std::size_t> &segment) const {
        return lineSegmentToPolygon(_vertices[segment.first], _vertices[segment.second], _thickness);
    }


#pragma mark - CrackGeometryDrawComponent

    /*
     CrackGeometryRef _crackGeometry;
     TriMeshRef _trimesh;
     */

    CrackGeometryDrawComponent::CrackGeometryDrawComponent(const CrackGeometryRef &crackGeometry) :
            _crackGeometry(crackGeometry) {
        // convert polygon to something we can tesselate
        vector<dpolygon2> crackPolys = {_crackGeometry->getPolygon()};
        vector<terrain::detail::polyline_with_holes> result = terrain::detail::dpolygon2_to_polyline_with_holes(crackPolys, dmat4());

        Triangulator triangulator;
        for (const auto &thing : result) {
            triangulator.addPolyLine(terrain::detail::polyline2d_to_2f(thing.contour));
            for (const auto &hole : thing.holes) {
                triangulator.addPolyLine(terrain::detail::polyline2d_to_2f(hole));
            }
        }

        _trimesh = triangulator.createMesh();
    }

    cpBB CrackGeometryDrawComponent::getBB() const {
        return _crackGeometry->getBB();
    }

    void CrackGeometryDrawComponent::draw(const render_state &renderState) {
        gl::color(1, 1, 1);
        gl::draw(*_trimesh);

        gl::color(0, 0, 0);
        _crackGeometry->debugDrawSkeleton();
    }

    VisibilityDetermination::style CrackGeometryDrawComponent::getVisibilityDetermination() const {
        return VisibilityDetermination::FRUSTUM_CULLING;
    }

    int CrackGeometryDrawComponent::getLayer() const {
        return DrawLayers::EFFECTS;
    }


}
