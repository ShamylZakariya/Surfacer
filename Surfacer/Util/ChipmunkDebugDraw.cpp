//
//  ChipmunkDebugDraw.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 1/11/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "ChipmunkDebugDraw.h"

#include <cinder/gl/gl.h>
//#include <chipmunk/chipmunk_private.h>

using namespace ci;
namespace core { namespace util { namespace cpdd {

namespace {

	const GLfloat circleVAR[] = {
		 0.0000f,  1.0000f,
		 0.2588f,  0.9659f,
		 0.5000f,  0.8660f,
		 0.7071f,  0.7071f,
		 0.8660f,  0.5000f,
		 0.9659f,  0.2588f,
		 1.0000f,  0.0000f,
		 0.9659f, -0.2588f,
		 0.8660f, -0.5000f,
		 0.7071f, -0.7071f,
		 0.5000f, -0.8660f,
		 0.2588f, -0.9659f,
		 0.0000f, -1.0000f,
		-0.2588f, -0.9659f,
		-0.5000f, -0.8660f,
		-0.7071f, -0.7071f,
		-0.8660f, -0.5000f,
		-0.9659f, -0.2588f,
		-1.0000f, -0.0000f,
		-0.9659f,  0.2588f,
		-0.8660f,  0.5000f,
		-0.7071f,  0.7071f,
		-0.5000f,  0.8660f,
		-0.2588f,  0.9659f,
		 0.0000f,  1.0000f,
		 0.0f, 0.0f, // For an extra line to see the rotation.
	};
	const int circleVAR_count = sizeof(circleVAR)/sizeof(GLfloat)/2;

	const GLfloat pillVAR[] = {
		 0.0000f,  1.0000f, 1.0f,
		 0.2588f,  0.9659f, 1.0f,
		 0.5000f,  0.8660f, 1.0f,
		 0.7071f,  0.7071f, 1.0f,
		 0.8660f,  0.5000f, 1.0f,
		 0.9659f,  0.2588f, 1.0f,
		 1.0000f,  0.0000f, 1.0f,
		 0.9659f, -0.2588f, 1.0f,
		 0.8660f, -0.5000f, 1.0f,
		 0.7071f, -0.7071f, 1.0f,
		 0.5000f, -0.8660f, 1.0f,
		 0.2588f, -0.9659f, 1.0f,
		 0.0000f, -1.0000f, 1.0f,

		 0.0000f, -1.0000f, 0.0f,
		-0.2588f, -0.9659f, 0.0f,
		-0.5000f, -0.8660f, 0.0f,
		-0.7071f, -0.7071f, 0.0f,
		-0.8660f, -0.5000f, 0.0f,
		-0.9659f, -0.2588f, 0.0f,
		-1.0000f, -0.0000f, 0.0f,
		-0.9659f,  0.2588f, 0.0f,
		-0.8660f,  0.5000f, 0.0f,
		-0.7071f,  0.7071f, 0.0f,
		-0.5000f,  0.8660f, 0.0f,
		-0.2588f,  0.9659f, 0.0f,
		 0.0000f,  1.0000f, 0.0f,
	};
	
	const int pillVAR_count = sizeof(pillVAR)/sizeof(GLfloat)/3;

	const GLfloat springScale = 0.01;
	const GLfloat springVAR[] = {
		0.00f, 0.0f * springScale,
		0.20f, 0.0f * springScale,
		0.25f, 3.0f * springScale,
		0.30f,-6.0f * springScale,
		0.35f, 6.0f * springScale,
		0.40f,-6.0f * springScale,
		0.45f, 6.0f * springScale,
		0.50f,-6.0f * springScale,
		0.55f, 6.0f * springScale,
		0.60f,-6.0f * springScale,
		0.65f, 6.0f * springScale,
		0.70f,-3.0f * springScale,
		0.75f, 6.0f * springScale,
		0.80f, 0.0f * springScale,
		1.00f, 0.0f * springScale
	};
	
	const int springVAR_count = sizeof(springVAR)/sizeof(GLfloat)/2;

	void drawSpring(cpDampedSpring *spring, cpBody *body_a, cpBody *body_b, const ci::ColorA &color, cpFloat scale )
	{
		cpVect a = cpvadd(body_a->p, cpvrotate(spring->anchr1, body_a->rot));
		cpVect b = cpvadd(body_b->p, cpvrotate(spring->anchr2, body_b->rot));
		
		cpVect points[] = {a, b};
		DrawPoints(6 * scale, 2, points, true, color );

		cpVect delta = cpvsub(b, a);

		glColor4f( color );

		glEnable( GL_BLEND );
		glEnableClientState( GL_VERTEX_ARRAY );
		glVertexPointer(2, GL_FLOAT, 0, springVAR);

		glPushMatrix(); {
			GLfloat x = a.x;
			GLfloat y = a.y;
			GLfloat cos = delta.x;
			GLfloat sin = delta.y;
			GLfloat s = 1.0f/cpvlength(delta);

			const GLfloat matrix[] = {
				cos,    sin,   0.0f, 0.0f,
				-sin*s, cos*s, 0.0f, 0.0f,
				0.0f,   0.0f,  1.0f, 0.0f,
				x,      y,     0.0f, 1.0f,
			};
			
			glMultMatrixf(matrix);
			glDrawArrays(GL_LINE_STRIP, 0, springVAR_count);
		} glPopMatrix();
		
		glDisableClientState( GL_VERTEX_ARRAY );
	}

}

void DrawCircle( cpVect center, cpFloat angle, cpFloat radius, const ci::ColorA &fillColor, const ci::ColorA &lineColor)
{
	glEnable( GL_BLEND );
	glEnableClientState( GL_VERTEX_ARRAY );
	glVertexPointer(2, GL_FLOAT, 0, circleVAR);

	glPushMatrix(); {
		glTranslatef(center.x, center.y, 0.0f);
		glRotatef(angle*180.0f/M_PI, 0.0f, 0.0f, 1.0f);
		glScalef(radius, radius, 1.0f);
		
		if(fillColor.a > 0){
			glColor4f(fillColor);
			glDrawArrays(GL_TRIANGLE_FAN, 0, circleVAR_count - 1);
		}
		
		if(lineColor.a > 0){
			glColor4f(lineColor);
			glDrawArrays(GL_LINE_STRIP, 0, circleVAR_count);
		}
	} glPopMatrix();
	glDisableClientState( GL_VERTEX_ARRAY );
}

void DrawSegment( cpVect a, cpVect b, const ci::ColorA & color)
{
	GLfloat verts[] = {
		a.x, a.y,
		b.x, b.y,
	};
	
	glEnable( GL_BLEND );
	glEnableClientState( GL_VERTEX_ARRAY );
	glVertexPointer(2, GL_FLOAT, 0, verts);
	glColor4f( color );
	glDrawArrays(GL_LINES, 0, 2);
	glDisableClientState( GL_VERTEX_ARRAY );
}

void DrawFatSegment( cpVect a, cpVect b, cpFloat radius, const ci::ColorA & fillColor, const ci::ColorA & lineColor)
{
	if(radius > 0 )
	{
		glEnable( GL_BLEND );

		glEnableClientState( GL_VERTEX_ARRAY );
		glVertexPointer(3, GL_FLOAT, 0, pillVAR);

		glPushMatrix(); {
			cpVect d = cpvsub(b, a);
			cpVect r = cpvmult(d, radius/cpvlength(d));

			const GLfloat matrix[] = {
				 r.x, r.y, 0.0f, 0.0f,
				-r.y, r.x, 0.0f, 0.0f,
				 d.x, d.y, 0.0f, 0.0f,
				 a.x, a.y, 0.0f, 1.0f,
			};
			glMultMatrixf(matrix);
			
			if(fillColor.a > 0){
				glColor4f(fillColor);
				glDrawArrays(GL_TRIANGLE_FAN, 0, pillVAR_count);
			}
			
			if(lineColor.a > 0){
				glColor4f(lineColor);
				glDrawArrays(GL_LINE_LOOP, 0, pillVAR_count);
			}
		} glPopMatrix();
		
		glDisableClientState( GL_VERTEX_ARRAY );
		
	} 
	else 
	{
		DrawSegment(a, b, lineColor );
	}
}

void DrawPolygon( int count, cpVect *verts, const ci::ColorA & fillColor, const ci::ColorA & lineColor)
{
	glEnable( GL_BLEND );
	glEnableClientState( GL_VERTEX_ARRAY );

	#if CP_USE_DOUBLES
		glVertexPointer(2, GL_DOUBLE, 0, verts);
	#else
		glVertexPointer(2, GL_FLOAT, 0, verts);
	#endif
	
	if(fillColor.a > 0){
		glColor4f( fillColor );
		glDrawArrays(GL_TRIANGLE_FAN, 0, count);
	}
	
	if(lineColor.a > 0){
		glColor4f( lineColor );
		glDrawArrays(GL_LINE_LOOP, 0, count);
	}

	glDisableClientState( GL_VERTEX_ARRAY );	
}

void DrawPoints( cpFloat size, int count, cpVect *verts, bool filled, const ci::ColorA & color)
{
	for ( int i = 0; i < count; i++ )
	{
		DrawCircle( verts[i], 0, size, ci::ColorA(0,0,0,0), color );
	}	
}

void DrawBB( cpBB bb, const ci::ColorA & color)
{
	cpVect verts[] = {
		cpv(bb.l, bb.b),
		cpv(bb.l, bb.t),
		cpv(bb.r, bb.t),
		cpv(bb.r, bb.b),
	};

	DrawPolygon(4, verts, ci::ColorA(0,0,0,0), color );
}

#pragma mark -

void DrawConstraint( cpConstraint *constraint, const ci::ColorA &color, cpFloat scale )
{
	cpBody *body_a = constraint->a;
	cpBody *body_b = constraint->b;

	const cpConstraintClass *klass = constraint->CP_PRIVATE(klass);

	if(klass == cpPinJointGetClass())
	{
		cpPinJoint *joint = (cpPinJoint *)constraint;
	
		cpVect a = cpvadd(body_a->p, cpvrotate(joint->anchr1, body_a->rot));
		cpVect b = cpvadd(body_b->p, cpvrotate(joint->anchr2, body_b->rot));
		
		cpVect points[] = {a, b};
		DrawPoints(6*scale, 2, points, false, color);
		DrawSegment(a, b, color);
	} 
	else if (klass == cpSlideJointGetClass())
	{
		cpSlideJoint *joint = (cpSlideJoint *)constraint;
	
		cpVect a = cpvadd(body_a->p, cpvrotate(joint->anchr1, body_a->rot));
		cpVect b = cpvadd(body_b->p, cpvrotate(joint->anchr2, body_b->rot));
		
		cpVect points[] = {a, b};
		DrawPoints(6 * scale, 2, points, true, color);
		DrawSegment(a, b, color);
	} 
	else if(klass == cpPivotJointGetClass())
	{
		cpPivotJoint *joint = (cpPivotJoint *)constraint;
	
		cpVect a = cpvadd(body_a->p, cpvrotate(joint->anchr1, body_a->rot));
		cpVect b = cpvadd(body_b->p, cpvrotate(joint->anchr2, body_b->rot));

		cpVect points[] = {a, b};
		DrawPoints(6 * scale, 2, points, false, color);
	} 
	else if(klass == cpGrooveJointGetClass())
	{
		cpGrooveJoint *joint = (cpGrooveJoint *)constraint;
	
		cpVect a = cpvadd(body_a->p, cpvrotate(joint->grv_a, body_a->rot));
		cpVect b = cpvadd(body_a->p, cpvrotate(joint->grv_b, body_a->rot));
		cpVect c = cpvadd(body_b->p, cpvrotate(joint->anchr2, body_b->rot));
		
		DrawPoints(6 * scale, 1, &c, true, color);
		DrawSegment(a, b, color);
	} 
	else if(klass == cpDampedSpringGetClass())
	{
		drawSpring((cpDampedSpring *)constraint, body_a, body_b, color, scale );
	}
}

void DrawShape( cpShape *shape, ci::ColorA color, ci::ColorA lineColor )
{
	cpBody *body = cpShapeGetBody( shape );

	switch(shape->CP_PRIVATE(klass)->type)
	{
		case CP_CIRCLE_SHAPE: {
			cpCircleShape *circle = (cpCircleShape *)shape;
			DrawCircle(circle->tc, body->a, circle->r, color, lineColor );
			break;
		}
		case CP_SEGMENT_SHAPE: {
			cpSegmentShape *seg = (cpSegmentShape *)shape;
			DrawFatSegment(seg->ta, seg->tb, seg->r, color, lineColor );
			break;
		}
		case CP_POLY_SHAPE: {
			cpPolyShape *poly = (cpPolyShape *)shape;
			DrawPolygon(poly->numVerts, poly->tVerts, color, lineColor );
			break;
		}
		default: break;
	}
}

void DrawBody( cpBody *body, cpFloat scale )
{
	glEnableClientState( GL_VERTEX_ARRAY );

	cpVect xAxis = cpvmult( cpBodyGetRot( body ), 20 * scale ),
	       yAxis = cpvperp( xAxis ),
		   pos = cpBodyGetPos( body );

	cpVect verts[4] = {
		pos, cpvadd( pos, xAxis ),
		pos, cpvadd( pos, yAxis )
	};
	
	#if CP_USE_DOUBLES
		glVertexPointer(2, GL_DOUBLE, 0, verts);
	#else
		glVertexPointer(2, GL_FLOAT, 0, verts);
	#endif
	
	glColor4f( 1,0,0,1 );
	glDrawArrays(GL_LINES, 0, 2 );

	glColor4f( 0,1,0,1 );
	glDrawArrays(GL_LINES, 2, 2 );
	
	glDisableClientState( GL_VERTEX_ARRAY );	
}

#pragma mark -

void DrawShapes( const cpShapeSet &shapes, const ci::ColorA &fillColor, const ci::ColorA &lineColor )
{
	foreach( cpShape *shape, shapes )
	{
		DrawShape( shape, fillColor, lineColor );
	}
}

void DrawConstraints( const cpConstraintSet &constraints, const ci::ColorA &color, cpFloat scale )
{
	foreach( cpConstraint *c, constraints )
	{
		DrawConstraint( c, color, scale );
	}
}

void DrawBodies( const cpBodySet &bodies, cpFloat scale )
{
	foreach( cpBody *b, bodies )
	{
		DrawBody( b, scale );
	}
}


void DrawCollisionPoints( cpSpace *space, const ci::ColorA &color )
{
//	cpArray *arbiters = space->CP_PRIVATE(arbiters);
//	
//	glColor4f(color);
//	glPointSize(4.0f);
//	
//	glBegin(GL_POINTS); 
//	{
//		for(int i=0; i<arbiters->CP_PRIVATE(num); i++)
//		{
//			cpArbiter *arb = (cpArbiter*)arbiters->CP_PRIVATE(arr)[i];
//			
//			for(int i=0; i<arb->CP_PRIVATE(numContacts); i++)
//			{
//				cpVect v = arb->CP_PRIVATE(contacts)[i].p;
//				glVertex2f(v.x, v.y);
//			}
//		}
//	} glEnd();
}


}}} // end namespace core::util::cpdd