//
//  TerrainDetail_MarchingSquares.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 1/30/18.
//

#ifndef TerrainDetail_MarchingSquares_hpp
#define TerrainDetail_MarchingSquares_hpp

#include "Core.hpp"
#include "MarchingSquares.hpp"

namespace terrain {
    namespace detail {

        /**
         implements marching_cubes::march voxel store object
         */
        class Channel8uVoxelStoreAdapter {
        public:

            Channel8uVoxelStoreAdapter(const ci::Channel8u &s) :
                    _store(s),
                    _min(0, 0),
                    _max(s.getWidth() - 1, s.getHeight() - 1) {
                _data = _store.getData();
                _rowBytes = _store.getRowBytes();
                _pixelIncrement = _store.getIncrement();
            }

            ivec2 min() const {
                return _min;
            }

            ivec2 max() const {
                return _max;
            }

            double valueAt(int x, int y) const {
                if (x >= _min.x && x <= _max.x && y >= _min.y && y <= _max.y) {
                    uint8_t pv = _data[y * _rowBytes + x * _pixelIncrement];
                    return static_cast<double>(pv) / 255.0;
                }
                return 0.0;
            }

        private:

            ci::Channel8u _store;
            ivec2 _min, _max;
            uint8_t *_data;
            int32_t _rowBytes, _pixelIncrement;
        };

        /**
         PerimeterGenerator implements marching_cubes::march segment callback - stitching a soup of short segments into line loops.
         NOTE: PG requires that the line segment soup be sane:
             1) Closed non-intersecting loops
             2) Edges have consistent windings. E.g., output from marching squares guarantees that each loop has one winding
        */
        struct PerimeterGenerator {
        public:

            enum winding {
                CLOCKWISE,
                COUNTER_CLOCKWISE
            };

        private:

            typedef std::pair<ivec2, ivec2> Edge;

            std::map<ivec2, Edge, ivec2Comparator> _edgesByFirstVertex;
            winding _winding;

        public:

            PerimeterGenerator(winding w) :
                    _winding(w) {
            }

            void operator()(int x, int y, const marching_squares::segment &seg) {
                switch (_winding) {
                    case CLOCKWISE: {
                        Edge e(scaleUp(seg.a), scaleUp(seg.b));

                        if (e.first != e.second) {
                            _edgesByFirstVertex[e.first] = e;
                        }
                        break;
                    }

                    case COUNTER_CLOCKWISE: {
                        Edge e(scaleUp(seg.b),
                                scaleUp(seg.a));

                        if (e.first != e.second) {
                            _edgesByFirstVertex[e.first] = e;
                        }
                        break;
                    }
                }
            }

            /*
             Populate a vector of PolyLine2d with every perimeter computed for the isosurface
             Exterior perimeters will be in the current winding direction, interior perimeters
             will be in the opposite winding.
             */

            size_t generate(std::vector<ci::PolyLine2d> &perimeters, dmat4 transform) {
                perimeters.clear();

                while (!_edgesByFirstVertex.empty()) {
                    std::map<ivec2, Edge, ivec2Comparator>::iterator
                            begin = _edgesByFirstVertex.begin(),
                            end = _edgesByFirstVertex.end(),
                            it = begin;

                    size_t count = _edgesByFirstVertex.size();
                    perimeters.push_back(PolyLine2d());

                    do {
                        const Edge &e = it->second;
                        perimeters.back().push_back(transform * scaleDown(e.first));
                        _edgesByFirstVertex.erase(it);

                        it = _edgesByFirstVertex.find(e.second);
                        count--;

                        // final segment of loop - close it
                        if (it == end) {
                            perimeters.back().push_back(transform * scaleDown(e.second));
                        }

                    } while (it != begin && it != end && count > 0);
                }

                return perimeters.size();
            }

        private:

            const double V_SCALE = 1024;

            ivec2 scaleUp(const dvec2 &v) const {
                return ivec2(lrint(V_SCALE * v.x), lrint(V_SCALE * v.y));
            }

            dvec2 scaleDown(const ivec2 &v) const {
                return dvec2(static_cast<double>(v.x) / V_SCALE, static_cast<double>(v.y) / V_SCALE);
            }
        };

        inline bool march(const Channel8u &store, double isoLevel, dmat4 transform, double simplificationThreshold, std::vector<PolyLine2d> &resultPerimeters) {
            Channel8uVoxelStoreAdapter adapter(store);
            PerimeterGenerator pgen(PerimeterGenerator::CLOCKWISE);
            marching_squares::march(adapter, pgen, isoLevel);

            if (simplificationThreshold > 0) {
                std::vector<PolyLine2d> perimeters;
                if (pgen.generate(perimeters, transform)) {
                    for (auto &perimeter : perimeters) {
                        perimeter.setClosed(true);
                        if (perimeter.size() > 0) {
                            resultPerimeters.push_back(core::util::simplify(perimeter, simplificationThreshold));
                        }
                    }
                }
            } else {
                pgen.generate(resultPerimeters, transform);
            }

            //
            //	return whether we got any usable perimeters
            //

            return resultPerimeters.empty() && resultPerimeters.front().size() > 0;
        }

        inline std::vector<PolyLine2d> march(const Channel8u &store, double isoLevel, dmat4 transform, double simplificationThreshold) {
            std::vector<PolyLine2d> soup;
            march(store, isoLevel, transform, simplificationThreshold, soup);

            // filter out degenerate polylines
            soup.erase(std::remove_if(soup.begin(), soup.end(), [](const PolyLine2d &contour) {
                return contour.size() < 3;
            }), soup.end());

            return soup;
        }

    }
} // end namespace terrain::detail


#endif /* TerrainDetail_MarchingSquares_hpp */
