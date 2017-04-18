//
//  Xml.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/17/17.
//
//

#include "Xml.hpp"

namespace core { namespace util { namespace xml {

	pair<ci::XmlTree,bool> findElement(const ci::XmlTree &node, string tag) {
		if (node.isElement() && node.getTag() == tag) {
			return make_pair(node,true);
		} else {
			for ( auto childNode : node ) {
				auto result = findElement(childNode, tag);
				if (result.second) {
					return result;
				}
			}
		}
		return make_pair(XmlTree(), false);
	}

	pair<ci::XmlTree,bool> findNodeWithId(const ci::XmlTree &node, string id ) {
		if (node.isElement() && node.hasAttribute("id") && node.getAttribute("id").getValue() == id) {
			return make_pair(node,true);
		} else {
			for ( auto childNode : node ) {
				auto result = findNodeWithId(childNode, id);
				if (result.second) {
					return result;
				}
			}
		}
		return make_pair(XmlTree(),false);
	}

	vector<double> readNumericSequence(const string &sequence) {
		vector<double> values;
		string token;
		double value;
		for (auto c : sequence) {
			if (c == ',' || c == ' ') {
				if (!token.empty()) {
					value = strtod(token.c_str(),nullptr);
					token.clear();
					values.push_back(value);
				}
				continue;
			} else {
				token = token + c;
			}
		}
		if (!token.empty()) {
			value = strtod(token.c_str(),nullptr);
			values.push_back(value);
		}
		return values;
	}

}}}
