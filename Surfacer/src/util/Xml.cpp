//
//  Xml.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/17/17.
//
//

#include "Xml.hpp"

#include "Strings.hpp"

namespace core { namespace util { namespace xml {

	pair<bool, ci::XmlTree> findElement(const ci::XmlTree &node, string tag) {
		if (node.isElement() && node.getTag() == tag) {
			return make_pair(true, node);
		} else {
			for ( auto childNode : node ) {
				auto result = findElement(childNode, tag);
				if (result.first) {
					return result;
				}
			}
		}
		return make_pair(false, XmlTree());
	}

	pair<bool, ci::XmlTree> findNodeWithId(const ci::XmlTree &node, string id ) {
		if (node.isElement() && node.hasAttribute("id") && node.getAttribute("id").getValue() == id) {
			return make_pair(true, node);
		} else {
			for ( auto childNode : node ) {
				auto result = findNodeWithId(childNode, id);
				if (result.first) {
					return result;
				}
			}
		}
		return make_pair(false, XmlTree());
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

	double readNumericAttribute(ci::XmlTree &node, string attributeName, double defaultValue) {
		if (node.hasAttribute(attributeName)) {
			return strtod(node.getAttribute(attributeName).getValue().c_str(), nullptr);
		}
		return defaultValue;
	}

	vector<double> readNumericSequenceAttribute(ci::XmlTree &node, string attributeName, vector<double> defaultValue) {
		if (node.hasAttribute(attributeName)) {
			return readNumericSequence(node.getAttribute(attributeName));
		}
		return defaultValue;
	}

	dvec2 readPointAttribute(ci::XmlTree &node, string attributeName, dvec2 defaultValue) {
		if (node.hasAttribute(attributeName)) {
			auto r = readNumericSequence(node.getAttribute(attributeName));
			CI_ASSERT_MSG(r.size() == 2, ("dvec2 readPointAttribute expects 2 components in seequence, got: " + str(r.size())).c_str());
			return dvec2(r[0], r[1]);
		}
		return defaultValue;
	}

	dvec3 readPointAttribute(ci::XmlTree &node, string attributeName, dvec3 defaultValue) {
		if (node.hasAttribute(attributeName)) {
			auto r = readNumericSequence(node.getAttribute(attributeName));
			CI_ASSERT_MSG(r.size() == 3, ("dvec3 readPointAttribute expects 3 components in seequence, got: " + str(r.size())).c_str());
			return dvec3(r[0], r[1], r[2]);
		}
		return defaultValue;
	}


}}}
