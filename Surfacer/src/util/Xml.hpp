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
	pair<ci::XmlTree,bool> findElement(const ci::XmlTree &node, string tag);

	// find first element with a given id
	pair<ci::XmlTree,bool> findNodeWithId(const ci::XmlTree &node, string id );

	// given a sequence of numbers separated by spaces, commas, or comma-spaces, return their values
	vector<double> readNumericSequence(const string &sequence);

}}}

#endif /* Xml_hpp */
