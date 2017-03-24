//
//  SetMerge.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/22/17.
//
//

#ifndef SetMerge_h
#define SetMerge_h

#include <algorithm>
#include <vector>
#include <set>

namespace set_utils {

	template<typename T>
	bool intersects(const std::set<T> &a, const std::set<T> &b) {
		for (auto &i : b) {
			if (a.find(i) != a.end()) {
				return true;
			}
		}
		return false;
	}

	template<typename T>
	std::vector<set<T>> merge(std::vector<std::set<T>> buckets) {
		std::vector<set<T>> merged;

		while (buckets.size() > 1) {

			// grab first bucket and find any buckets with shared items
			auto first = buckets.begin();
			for (auto it(buckets.begin()+1), e(buckets.end()); it != e; ++it) {
				if (intersects(*first, *it)) {
					// copy elements over to first, and clear
					first->insert(it->begin(), it->end());
					it->clear();
				}
			}

			// copy over to merged and clear
			merged.push_back(*first);
			first->clear();

			// now remove any empty buckets
			auto new_end = std::remove_if(buckets.begin(), buckets.end(), [](const set<T> &bucket) {
				return bucket.empty();
			});

			buckets.erase(new_end, end(buckets));
		}

		// copy over any remainders
		for (auto &bucket : buckets) {
			if (!bucket.empty()) {
				merged.push_back(bucket);
			}
		}
		
		return merged;
	}

}




#endif /* SetMerge_h */
