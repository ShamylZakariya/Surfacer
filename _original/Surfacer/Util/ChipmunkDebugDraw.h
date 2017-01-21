#pragma once

//
//  ChipmunkDebugDraw.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 1/11/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Common.h"

namespace core { namespace util { namespace cpdd {

void DrawCircle( cpVect center, cpFloat angle, cpFloat radius, const ci::ColorA &fillColor, const ci::ColorA &lineColor);
void DrawSegment( cpVect a, cpVect b, const ci::ColorA & fillColor );
void DrawFatSegment( cpVect a, cpVect b, cpFloat radius, const ci::ColorA & fillColor, const ci::ColorA & lineColor);
void DrawPolygon( int count, cpVect *verts, const ci::ColorA & fillColor, const ci::ColorA & lineColor);
void DrawPoints( cpFloat size, int count, cpVect *verts, bool filled, const ci::ColorA & color);
void DrawBB( cpBB bb, const ci::ColorA & color);

void DrawConstraint( cpConstraint *constraint, const ci::ColorA &color, cpFloat scale );
void DrawShape( cpShape *shape, ci::ColorA fillColor, ci::ColorA lineColor );
void DrawBody( cpBody *body, cpFloat scale );

void DrawShapes( const cpShapeSet &shapes, const ci::ColorA &fillColor, const ci::ColorA &lineColor = ci::ColorA(0,0,0,0) );
void DrawConstraints( const cpConstraintSet &constraints, const ci::ColorA &color, cpFloat scale );
void DrawBodies( const cpBodySet &bodies, cpFloat scale );
void DrawCollisionPoints( cpSpace *space, const ci::ColorA &color );


}}} // end namespace core::util::cpdd