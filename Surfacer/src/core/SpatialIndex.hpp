//
//  SpatialIndex.hpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 3/14/17.
//
//

#ifndef SpatialIndex_h
#define SpatialIndex_h

#include <unordered_set>

#include <cinder/CinderMath.h>
#include <cinder/Area.h>
#include <chipmunk/chipmunk.h>
#include <cinder/gl/scoped.h>

#include "ChipmunkHelpers.hpp"
#include "MathHelpers.hpp"

using namespace ci;
using namespace std;

namespace core {

	template<class T>
	class SpatialIndex {
	public:

		typedef pair<cpBB,T> item_t;
		typedef vector<item_t> bucket_t;

		typedef unordered_set<T> set_t;
		typedef unordered_map<size_t,bucket_t> map_t;

		/**
		 Create a spatial index with a given cell size. 
		 Be certain to pick a good cell size for your expected scenario. 
		 - If your items tend to be, say, 100x100, picking at least 100 for your cellsize is reasonable.
		 - Picking a huge value might be problematic if your items tend to be close together - many items would live in a single cell.
		 - Picking 10 would be bad because each item inserted would have to be added to 100 cells.
		 */
		SpatialIndex(float cellSize):
		_cellSize(cellSize)
		{}

		/**
		 Clear all items
		 */
		void clear() { _spatialHash.clear(); }

		/**
		 Insert an item and associated payload
		 */
		void insert(cpBB bb, T data) {
			if (cpBBIsValid(bb)) {
				const int left = static_cast<int>(round(bb.l/_cellSize));
				const int right = static_cast<int>(round(bb.r/_cellSize));
				const int bottom = static_cast<int>(round(bb.b/_cellSize));
				const int top = static_cast<int>(round(bb.t/_cellSize));

				for (int y = bottom; y <= top; y++) {
					for (int x = left; x <= right; x++) {
						const size_t h = _hash(x,y);
						auto pos = _spatialHash.find(h);
						if (pos == _spatialHash.end()) {
							bucket_t b = { make_pair(bb, data) };
							_spatialHash[h] = b;
						} else {
							pos->second.push_back(make_pair(bb, data));
						}
					}
				}
			}
		}

		/**
		 Return a set of all items whose AABBs intersect `test
		 */
		set_t sweep(cpBB test) {
			set_t found;

			if (cpBBIsValid(test)) {
				const int left = static_cast<int>(round(test.l/_cellSize));
				const int right = static_cast<int>(round(test.r/_cellSize));
				const int bottom = static_cast<int>(round(test.b/_cellSize));
				const int top = static_cast<int>(round(test.t/_cellSize));

				for (int y = bottom; y <= top; y++) {
					for (int x = left; x <= right; x++) {
						const size_t h = _hash(x,y);
						auto pos = _spatialHash.find(h);
						if (pos != _spatialHash.end()) {
							for (auto i : pos->second) {
								if (cpBBIntersects(test, i.first)) {
									found.insert(i.second);
								}
							}
						}
					}
				}
			}

			return found;
		}

		/**
		 Find all AABB intersections with `test, calling `visitor on each.
		 Note: visitor may be called more than once with the same payload.
		 */
		void sweep(cpBB test, const function<void(cpBB,T)> &visitor) {
			if (cpBBIsValid(test)) {
				const int left = static_cast<int>(round(test.l/_cellSize));
				const int right = static_cast<int>(round(test.r/_cellSize));
				const int bottom = static_cast<int>(round(test.b/_cellSize));
				const int top = static_cast<int>(round(test.t/_cellSize));

				for (int y = bottom; y <= top; y++) {
					for (int x = left; x <= right; x++) {
						const size_t h = _hash(x,y);
						auto pos = _spatialHash.find(h);
						if (pos != _spatialHash.end()) {
							for (auto i : pos->second) {
								if (cpBBIntersects(test, i.first)) {
									visitor(i.first, i.second);
								}
							}
						}
					}
				}
			}
		}

	private:

		size_t _hash(int x, int y) const {
			const auto hasher = hash<size_t>{};
			const auto hx = hasher(x);
			const auto hy = hasher(y);
			return hx ^ (hy << 1);
		}


		float _cellSize;
		map_t _spatialHash;
		
	};

}


#endif /* SpatialIndex_h */
