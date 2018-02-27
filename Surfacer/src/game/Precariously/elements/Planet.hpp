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
            double surfaceSolidity;

            config() :
                    seed(12345),
                    radius(500),
                    surfaceSolidity(0.5)
            {
            }

            static config parse(core::util::xml::XmlMultiTree node);

        };

        static PlanetRef create(string name, terrain::WorldRef world, core::util::xml::XmlMultiTree planetNode, int drawLayer);

        static PlanetRef create(string name, terrain::WorldRef world, const config &surfaceConfig, const config &coreConfig, dvec2 origin, double partitionSize, int drawLayer);

    public:

        Planet(string name, terrain::WorldRef world, int drawLayer);

        virtual ~Planet();

        // Object
        void onReady(core::StageRef stage) override;

        // Planet
        dvec2 getOrigin() const {
            return _origin;
        }

    private:

        dvec2 _origin;

    };


    SMART_PTR(CrackGeometry);

    class CrackGeometry {
    public:
        virtual ~CrackGeometry() {
        }

        virtual cpBB getBB() const = 0;

        virtual terrain::dpolygon2 getPolygon() const = 0;

        virtual void debugDrawSkeleton() const {
        }

    protected:

        static terrain::dpolygon2 lineSegmentToPolygon(dvec2 a, dvec2 b, double width);

    };

    class RadialCrackGeometry : public CrackGeometry {
    public:

        RadialCrackGeometry(dvec2 origin, int numSpokes, int numRings, double radius, double thickness, double variance);

        cpBB getBB() const override {
            return _bb;
        }

        terrain::dpolygon2 getPolygon() const override {
            return _polygon;
        }

        void debugDrawSkeleton() const override;

    private:

        double _thickness;
        dvec2 _origin;
        vector<dvec2> _vertices;
        vector<vector<pair<size_t, size_t>>> _spokes, _rings;
        terrain::dpolygon2 _polygon;
        cpBB _bb;

        terrain::dpolygon2 createPolygon() const;

        terrain::dpolygon2 createPolygon(const pair<std::size_t, std::size_t> &segment) const;
    };

    class CrackGeometryDrawComponent : public core::DrawComponent {
    public:

        CrackGeometryDrawComponent(const CrackGeometryRef &crackGeometry);

        ~CrackGeometryDrawComponent() {
        }

        cpBB getBB() const override;

        void draw(const core::render_state &renderState) override;

        core::VisibilityDetermination::style getVisibilityDetermination() const override;

        int getLayer() const override;

    private:
        CrackGeometryRef _crackGeometry;
        TriMeshRef _trimesh;
    };


}

#endif /* Planet_hpp */
