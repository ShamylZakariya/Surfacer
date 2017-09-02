//
//  ChipmunkDebugDraw.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/11/17.
//
//

#include "ChipmunkDebugDraw.hpp"


namespace core { namespace util { namespace cdd {

	void DrawCapsule(dvec2 a, dvec2 b, double radius) {
		dvec2 center = (a + b) * 0.5;
		double len = distance(a, b);
		dvec2 dir = (b - a) / len;
		double angle = atan2(dir.y, dir.x);

		gl::ScopedModelMatrix smm;
		mat4 M = glm::translate(dvec3(center.x, center.y, 0)) * glm::rotate(angle, dvec3(0,0,1));
		gl::multModelMatrix(M);

		gl::drawSolidRoundedRect(Rectf(-len/2 - radius, -radius, +len/2 + radius, +radius), radius, 8);
	}



}}} // namespace core::util::cdd
