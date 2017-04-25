//
//  Xml.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/17/17.
//
//

#ifndef Xml_hpp
#define Xml_hpp

#include "Core.hpp"
#include <cinder/Xml.h>

namespace core { namespace util { namespace xml {

	// find first element with a given tag
	pair<bool, ci::XmlTree> findElement(const ci::XmlTree &node, string tag);

	// find first element with a given id
	pair<bool, ci::XmlTree> findNodeWithId(const ci::XmlTree &node, string id );

	// given a sequence of numbers separated by spaces, commas, or comma-spaces, return their values
	vector<double> readNumericSequence(const string &sequence);

	// read a numeric attribute from an XmlNode
	double readNumericAttribute(ci::XmlTree &node, string attributeName, double defaultValue);

	// read a sequence of numeric attributes from an xml node (where a sequence is a list of numbers separated by comma, space, or comma-space)
	vector<double> readNumericSequenceAttribute(ci::XmlTree &node, string attributeName, vector<double> defaultValue);

}}}

#endif /* Xml_hpp */
