//
//  SvgParsing.cpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/11/17.
//
//

#include "SvgParsing.hpp"

#include <cinder/Utilities.h>
#include <cinder/Xml.h>

#include "Strings.hpp"
#include "MathHelpers.hpp"

using namespace core::strings;

namespace core { namespace util { namespace svg {

	namespace {

#pragma mark Matrix Parsing

		const float EPSILON = 1e-3;
		const float ALPHA_EPSILON = 1.f / 255.f;

		const std::string zero("0");

		float getNumericAttributeValue( const XmlTree &node, const std::string &attrName, float defaultValue )
		{
			if ( node.hasAttribute( attrName ))
			{
				return std::strtod( node.getAttribute( attrName ).getValue().c_str(), NULL );
			}

			return defaultValue;
		}

		void getTransformValues( const std::string &transform, std::vector< float > &values )
		{
			values.clear();

			std::string::size_type
			open = transform.find( '(' ) + 1,
			close = transform.find( ')' );

			for( const std::string &token : strings::split( transform.substr( open, close - open ), ',' ))
			{
				values.push_back( parseNumericAttribute(token));
			}
		}

		mat4 parseMatrixTransform( const std::string svgMatrixTransform )
		{
			/*
				matrix(<a> <b> <c> <d> <e> <f>), which specifies a transformation in the form of a transformation matrix of six values. matrix(a,b,c,d,e,f) is equivalent to applying the transformation matrix [a b c d e f].

				[a,b,c,d,e,f] ->

				[a,c,e]
				[b,d,f]
				[0,0,1]

				[a,c,0,e]
				[b,d,0,f]
				[0,0,1,0]
				[0,0,0,1]
			 */

			std::vector<float> values;
			getTransformValues( svgMatrixTransform, values );

			return mat4(
						vec4( values[0], values[1], 0, 0 ),
						vec4( values[2], values[3], 0, 0 ),
						vec4(        0 ,        0 , 1, 0 ),
						vec4( values[4], values[5], 0, 1 ));
		}

		mat4 parseTranslateTransform( const std::string svgTranslateTransform )
		{
			/*
			 translate(<tx> [<ty>]), which specifies a translation by tx and ty. If <ty> is not provided, it is assumed to be zero.
			 */

			std::vector<float> values;
			getTransformValues( svgTranslateTransform, values );

			float tx = values[0],
			ty = values.size() > 1 ? values[1] : 0;

			return translate(vec3(tx, ty, 0));
		}

		mat4 parseScaleTransform( const std::string svgScaleTransform )
		{
			/*
				scale(<sx> [<sy>]), which specifies a scale operation by sx and sy. If <sy> is not provided, it is assumed to be equal to <sx>.
			 */

			std::vector<float> values;
			getTransformValues( svgScaleTransform, values );

			float sx = values[0],
			sy = values.size() > 1 ? values[1] : sx;

			return scale(vec3(sx, sy, 1));

		}

		mat4 parseRotateTransform( const std::string svgRotateTransform )
		{
			/*
				rotate(<rotate-angle> [<cx> <cy>]), which specifies a rotation by <rotate-angle> degrees about a given point.
			 If optional parameters <cx> and <cy> are not supplied, the rotate is about the origin of the current user coordinate system. The operation corresponds to the matrix [cos(a) sin(a) -sin(a) cos(a) 0 0].
			 If optional parameters <cx> and <cy> are supplied, the rotate is about the point (cx, cy). The operation represents the equivalent of the following specification: translate(<cx>, <cy>) rotate(<rotate-angle>) translate(-<cx>, -<cy>).
			 */

			std::vector<float> values;
			getTransformValues( svgRotateTransform, values );

			mat4 rot = rotate(values[0], vec3(0,0,1));

			if ( values.size() == 1 )
			{
				return rot;
			}
			else if ( values.size() == 3 )
			{
				float tx = values[1],
				ty = values[2];

				mat4
				t0 = translate( vec3( tx, ty, 0 )),
				t1 = translate( vec3( -tx, -ty, 0 ));

				return t0 * rot * t1;
			}

			// hopefully this doesn't happen
			return mat4();
		}

		mat4 parseSkewXTransform( const std::string svgSkewXTransform )
		{
			/*
				skewX(<skew-angle>), which specifies a skew transformation along the x-axis.
				[1, tan(a), 0]
				[0,     1,  0]
				[0      0   1]
			 */

			std::vector<float> values;
			getTransformValues( svgSkewXTransform, values );

			float sx = std::tan( values[0] );

			return mat4(
						vec4(1,  0, 0, 0 ),
						vec4(sx, 1, 0, 0 ),
						vec4(0,  0, 1, 0 ),
						vec4(0,  0, 0, 1 ));
		}

		mat4 parseSkewYTransform( const std::string svgSkewYTransform )
		{
			/*
			 skewY(<skew-angle>), which specifies a skew transformation along the y-axis.

				[1      , 0, 0]
				[tan(a),  1, 0]
				[0        0, 1]
			 */

			std::vector<float> values;
			getTransformValues( svgSkewYTransform, values );

			float sy = std::tan( values[0] );

			return mat4(
						vec4(1, sy, 0, 0 ),
						vec4(0,  1, 0, 0 ),
						vec4(0,  0, 1, 0 ),
						vec4(0,  0, 0, 1 ));
		}

#pragma mark - Polyline Parsing

		void parsePolyline( const std::string &polyline, std::vector< vec2 > &points )
		{
			const char *current = polyline.c_str();
			while( *current != '\0' )
			{
				int c = *current;

				//
				//	gobble spaces
				//

				if ( std::isspace( c ))
				{
					current++;
					continue;
				}

				if ( std::isdigit( c ) || c == '.' || c == '+' || c == '-' )
				{
					float v[2];
					for ( int i = 0; i < 2; i++ )
					{
						char *end = NULL;
						v[i] = std::strtod( current, &end );
						current = end;

						// handle optional coordinate comma separator
						if ( *current == ',' ) current++;
					}

					points.push_back( ci::vec2( v[0], v[1] ));
					continue;
				}

				//
				//	If we're here, we have an unrecognized token!
				//

				throw ParserException( "Unrecognized polyline token: \"" + str(char(c)) + "\"" );
			}
		}

#pragma mark - PathParser

		class PathParser
		{
		public:

			PathParser();
			~PathParser();

			/**
				Parse an SVG path string.
				Implements, as best as my skill allows: http://www.w3.org/TR/SVG/paths.html

				Since SVG paths support multiple closings and re-openings, the generated Shape2d
				may have multiple contours.

				SVG arc handling is cribbed from http://librsvg.sourceforge.net/

				Will throw ParserException if something's wonky.
			 */
			void parse( const std::string &pathStr, Shape2d &shape );

		protected:

			void moveTo( vec2 p, bool relative );
			void lineTo( vec2 p, bool relative );
			void horizontalLineTo( float x, bool relative );
			void verticalLineTo( float y, bool relative );
			void curveTo( vec2 c0, vec2 c1, vec2 p, bool relative );
			void shorthandCurveTo( vec2 c1, vec2 p, bool relative );
			void quadraticCurveTo( vec2 c, vec2 p, bool relative );
			void shorthandQuadraticCurveTo( vec2 p, bool relative );
			void arcTo( float rx, float ry, float xAxisRotation, bool largeArcFlag, bool sweepFlag, vec2 end, bool relative );
			void close();

		private:

			void _pathArcSegment(
								 float xc, float yc,
								 float th0, float th1, float rx, float ry,
								 float x_axis_rotation);

			void _pathArc(
						  float rx,
						  float ry,
						  float x_axis_rotation,
						  int large_arc_flag,
						  int sweep_flag,
						  float x,
						  float y);

		private:

			bool _pathOpen;
			vec2 _lastPoint, _lastControl;
			Shape2d *_shape;

		};

		PathParser::PathParser() :
		_pathOpen(false),
		_shape(NULL)
		{}

		PathParser::~PathParser(){}

		void PathParser::parse( const std::string &pathStr, Shape2d &shape )
		{
			_shape = &shape;
			_pathOpen = false;

			const char *current = pathStr.c_str();
			int command = 0;
			bool relative = false, firstCommand = true;

			std::map< char, unsigned int > coordinatesToRead;
			coordinatesToRead['z'] = 0;
			coordinatesToRead['m'] = 2;
			coordinatesToRead['l'] = 2;
			coordinatesToRead['h'] = 1;
			coordinatesToRead['v'] = 1;
			coordinatesToRead['c'] = 6;
			coordinatesToRead['s'] = 4;
			coordinatesToRead['q'] = 4;
			coordinatesToRead['t'] = 2;
			coordinatesToRead['a'] = 7;

			while( *current != '\0' )
			{
				int c = *current;

				//
				//	gobble spaces
				//

				if ( std::isspace( c ))
				{
					current++;
					continue;
				}

				//
				//	if this is a letter get the command
				//
				if ( std::isalpha( c ) && c != '.' )
				{
					relative = std::islower( c );
					command = c;

					//
					//	the first command can ONLY be moveto, and if it's relative, make it absolute
					//

					if ( firstCommand )
					{
						firstCommand = false;
						relative = false;

						if ( command != 'm' && command != 'M' )
						{
							throw ParserException( "First command in path MUST be a moveto M/m" );
						}
					}

					if ( coordinatesToRead.find( std::tolower(command)) == coordinatesToRead.end())
					{
						throw ParserException( "Unrecognized command token: \"" + str(char(command)) + "\"" );
					}
					else if ( command == 'm' || command == 'M' )
					{
						if ( !_pathOpen )
						{
							_pathOpen = true;
						}
						else
						{
							throw ParserException( "Cannot call 'move' on an already open path" );
						}
					}
					else if ( command == 'z' || command == 'Z' )
					{
						if ( _pathOpen )
						{
							this->close();
							_pathOpen = false;
						}
						else
						{
							throw ParserException( "Cannot call 'close' on an already closed path" );
						}
					}

					current++;
					continue;
				}

				//
				//	if we're here it must be a coordinate
				//	read the number of coordinate appropriate for the current command. If the path's not open, trouble.
				//

				if ( !_pathOpen )
				{
					throw ParserException( "Cannot parse coordinates for a non-open path" );
				}


				if ( std::isdigit( c ) || c == '.' || c == '+' || c == '-' )
				{
					float v[7];
					unsigned int toRead = coordinatesToRead[std::tolower(command)];

					for ( unsigned int i = 0; i < toRead; i++ )
					{
						char *end = NULL;
						v[i] = std::strtod( current, &end );
						current = end;

						// handle optional coordinate comma separator
						if ( *current == ',' ) current++;
					}

					// note z is not handled here since it has no coordinates
					switch( std::tolower(command))
					{
						case 'm':
							moveTo( vec2( v[0],v[1] ), relative );

							//
							//	after move implicit next command becomes lineTo,
							//	but we may need to restore the relativity of the command
							//	since the first 'm' is treated as absolute if it's the first entry in the path
							//

							relative = std::islower(command);
							command = 'l';

							break;

						case 'l':
							lineTo( vec2( v[0],v[1] ), relative );
							break;

						case 'h':
							horizontalLineTo( v[0], relative );
							break;

						case 'v':
							verticalLineTo( v[0], relative );
							break;

						case 'c':
							curveTo( vec2(v[0],v[1]), vec2(v[2],v[3]), vec2(v[4],v[5]), relative );
							break;

						case 's':
							shorthandCurveTo(vec2(v[0],v[1]), vec2(v[2],v[3]), relative );
							break;

						case 'q':
							quadraticCurveTo(vec2(v[0],v[1]), vec2(v[2],v[3]), relative );
							break;

						case 't':
							shorthandQuadraticCurveTo(vec2(v[0],v[1]), relative );
							break;

						case 'a':
							arcTo(v[0],v[1],v[2], v[3] > 0, v[4] > 0, vec2(v[5],v[6]), relative );
							break;

						case 'z': break; // this should never come up

						default:
							throw ParserException( "Unrecognized command token: \"" + str(command) + "\"" );

					}

					continue;
				}

				//
				//	If we're here, we have an unrecognized token!
				//

				throw ParserException( "Unrecognized command token: \"" + str(char(c)) + "\"" );

			}
		}

		void PathParser::moveTo( vec2 p, bool relative )
		{
			if ( relative ) p += _lastPoint;
			_lastPoint = p;

			_shape->moveTo( p );
		}

		void PathParser::lineTo( vec2 p, bool relative )
		{
			if ( relative ) p += _lastPoint;
			_lastPoint = p;

			_shape->lineTo( p );
		}

		void PathParser::horizontalLineTo( float x, bool relative )
		{
			if( relative ) x += _lastPoint.x;
			lineTo( vec2( x, _lastPoint.y ), false );
		}

		void PathParser::verticalLineTo( float y, bool relative )
		{
			if( relative ) y += _lastPoint.y;
			lineTo( vec2( _lastPoint.x, y ), false );
		}

		void PathParser::curveTo( vec2 c0, vec2 c1, vec2 p, bool relative )
		{
			if ( relative )
			{
				c0 += _lastPoint;
				c1 += _lastPoint;
				p += _lastPoint;
			}

			_lastPoint = p;
			_lastControl = c1;

			_shape->curveTo( c0, c1, p );
		}

		void PathParser::shorthandCurveTo( vec2 c1, vec2 p, bool relative )
		{
			if ( relative )
			{
				c1 += _lastPoint;
				p += _lastPoint;
			}

			vec2 c0 = _lastPoint - ( _lastControl - _lastPoint );
			curveTo( c0, c1, p, false );
		}

		void PathParser::quadraticCurveTo( vec2 c, vec2 p, bool relative )
		{
			if ( relative )
			{
				c += _lastPoint;
				p += _lastPoint;
			}

			curveTo( c, c, p, false );
		}

		void PathParser::shorthandQuadraticCurveTo( vec2 p, bool relative )
		{
			if ( relative ) p += _lastPoint;
			vec2 c = _lastPoint - ( _lastControl - _lastPoint );
			quadraticCurveTo( c, p, false );
		}

		void PathParser::arcTo( float rx, float ry, float xAxisRotation, bool largeArcFlag, bool sweepFlag, vec2 end, bool relative )
		{
			if ( relative )
			{
				end += _lastPoint;
			}

			_pathArc( rx, ry, xAxisRotation, largeArcFlag ? 1 : 0, sweepFlag ? 1 : 0, end.x, end.y );
		}

		void PathParser::close()
		{
			_shape->close();
		}


		//
		//	the following two methods are re-implemented from rsvg-path.c
		//	http://librsvg.sourceforge.net/
		//

		void PathParser::_pathArcSegment(
										 float xc, float yc,
										 float th0, float th1, float rx, float ry,
										 float x_axis_rotation)
		{
			using std::sin;
			using std::cos;

			float x1, y1, x2, y2, x3, y3;
			float t;
			float th_half;
			float f, sinf, cosf;

			f = x_axis_rotation * M_PI / 180.0;
			sinf = sin(f);
			cosf = cos(f);

			th_half = 0.5 * (th1 - th0);
			t = (8.0 / 3.0) * sin (th_half * 0.5) * sin (th_half * 0.5) / sin (th_half);
			x1 = rx*(cos (th0) - t * sin (th0));
			y1 = ry*(sin (th0) + t * cos (th0));
			x3 = rx*cos (th1);
			y3 = ry*sin (th1);
			x2 = x3 + rx*(t * sin (th1));
			y2 = y3 + ry*(-t * cos (th1));

			curveTo(
					vec2(xc + cosf*x1 - sinf*y1, yc + sinf*x1 + cosf*y1 ),
					vec2(xc + cosf*x2 - sinf*y2, yc + sinf*x2 + cosf*y2 ),
					vec2(xc + cosf*x3 - sinf*y3, yc + sinf*x3 + cosf*y3 ),
					false);
		}

		void PathParser::_pathArc(
								  float rx,
								  float ry,
								  float x_axis_rotation,
								  int large_arc_flag,
								  int sweep_flag,
								  float x,
								  float y)
		{
			using std::sin;
			using std::cos;
			using std::acos;
			using std::sqrt;
			using std::abs;
			using std::ceil;

			/* See Appendix F.6 Elliptical arc implementation notes
			 http://www.w3.org/TR/SVG/implnote.html#ArcImplementationNotes */

			float f, sinf, cosf;
			float x1, y1, x2, y2;
			float x1_, y1_;
			float cx_, cy_, cx, cy;
			float gamma;
			float theta1, delta_theta;
			float k1, k2, k3, k4, k5;

			int i, n_segs;

			/* Start and end of path segment */
			x1 = _lastPoint.x;
			y1 = _lastPoint.y;

			x2 = x;
			y2 = y;

			if ( distance( x1,x2 ) < EPSILON && distance( y1,y2 ) < EPSILON )
			{
				return;
			}

			/* X-axis */
			f = x_axis_rotation * M_PI / 180.0;
			sinf = sin(f);
			cosf = cos(f);

			/* Check the radius against floading point underflow.
			 See http://bugs.debian.org/508443 */
			if ((abs(rx) < EPSILON) || (abs(ry) < EPSILON))
			{
				lineTo( vec2( x,y ), false );
				return;
			}

			if(rx < 0)rx = -rx;
			if(ry < 0)ry = -ry;

			k1 = (x1 - x2)/2;
			k2 = (y1 - y2)/2;

			x1_ = cosf * k1 + sinf * k2;
			y1_ = -sinf * k1 + cosf * k2;

			gamma = (x1_*x1_)/(rx*rx) + (y1_*y1_)/(ry*ry);
			if (gamma > 1) {
				rx *= sqrt(gamma);
				ry *= sqrt(gamma);
			}

			/* Compute the center */

			k1 = rx*rx*y1_*y1_ + ry*ry*x1_*x1_;
			if( k1 < EPSILON )
				return;

			k1 = sqrt(abs((rx*rx*ry*ry)/k1 - 1));
			if(sweep_flag == large_arc_flag)
				k1 = -k1;

			cx_ = k1*rx*y1_/ry;
			cy_ = -k1*ry*x1_/rx;

			cx = cosf*cx_ - sinf*cy_ + (x1+x2)/2;
			cy = sinf*cx_ + cosf*cy_ + (y1+y2)/2;

			/* Compute start angle */

			k1 = (x1_ - cx_)/rx;
			k2 = (y1_ - cy_)/ry;
			k3 = (-x1_ - cx_)/rx;
			k4 = (-y1_ - cy_)/ry;

			k5 = sqrt(abs(k1*k1 + k2*k2));
			if (k5 < EPSILON) return;

			k5 = k1/k5;
			if (k5 < -1)k5 = -1;
			else if (k5 > 1)k5 = 1;
			theta1 = acos(k5);
			if (k2 < 0) theta1 = -theta1;

			/* Compute delta_theta */

			k5 = sqrt(abs((k1*k1 + k2*k2)*(k3*k3 + k4*k4)));
			if (k5 < EPSILON) return;

			k5 = (k1*k3 + k2*k4)/k5;
			if(k5 < -1)k5 = -1;
			else if(k5 > 1)k5 = 1;
			delta_theta = acos(k5);
			if (k1*k4 - k3*k2 < 0) delta_theta = -delta_theta;

			if(sweep_flag && delta_theta < 0)
				delta_theta += M_PI*2;
			else if(!sweep_flag && delta_theta > 0)
				delta_theta -= M_PI*2;

			/* Now draw the arc */

			n_segs = ceil( abs(delta_theta / (M_PI * 0.5 + 0.001)));

			for ( i = 0; i < n_segs; i++ )
			{
				_pathArcSegment(
								cx,
								cy,
								theta1 + i * delta_theta / n_segs,
								theta1 + (i + 1) * delta_theta / n_segs,
								rx,
								ry,
								x_axis_rotation );
			}

			_lastPoint.x = x;
			_lastPoint.y = y;
		}

#pragma mark - Shape Loading

		void parsePathShape( const XmlTree &shapeNode, Shape2d &shape )
		{
			parsePath( shapeNode.getAttribute("d").getValue(), shape );
		}

		void parseRectShape( const XmlTree &shapeNode, Shape2d &shape )
		{
			float
			x = getNumericAttributeValue( shapeNode, "x", 0 ),
			y = getNumericAttributeValue( shapeNode, "y", 0 ),
			width = getNumericAttributeValue( shapeNode, "width", 0 ),
			height = getNumericAttributeValue( shapeNode, "height", 0 ),
			rx = getNumericAttributeValue( shapeNode, "rx", 0 ),
			ry = getNumericAttributeValue( shapeNode, "ry", rx );

			if ( !shapeNode.hasAttribute( "rx" )) rx = ry;

			if ( width < EPSILON || height < EPSILON ) return;

			if ( rx > 0 && ry > 0 )
			{
				rx = std::min( rx, width/2 );
				ry = std::min( ry, height/2 );

				/*
				 from: http://www.w3.org/TR/SVG/shapes.html#RectElement

				 1 perform an absolute moveto operation to location (x+rx,y), where x is the value of the ‘rect’ element's ‘x’ attribute converted to user space, rx is the effective value of the ‘rx’ attribute converted to user space and y is the value of the ‘y’ attribute converted to user space

				 2 perform an absolute horizontal lineto operation to location (x+width-rx,y), where width is the ‘rect’ element's ‘width’ attribute converted to user space

				 3 perform an absolute elliptical arc operation to coordinate (x+width,y+ry), where the effective values for the ‘rx’ and ‘ry’ attributes on the ‘rect’ element converted to user space are used as the rx and ry attributes on the elliptical arc command, respectively, the x-axis-rotation is set to zero, the large-arc-flag is set to zero, and the sweep-flag is set to one

				 4 perform a absolute vertical lineto to location (x+width,y+height-ry), where height is the ‘rect’ element's ‘height’ attribute converted to user space

				 5 perform an absolute elliptical arc operation to coordinate (x+width-rx,y+height)

				 6 perform an absolute horizontal lineto to location (x+rx,y+height)

				 7 perform an absolute elliptical arc operation to coordinate (x,y+height-ry)

				 8 perform an absolute absolute vertical lineto to location (x,y+ry)

				 9 perform an absolute elliptical arc operation to coordinate (x+rx,y)
				 */

				std::stringstream command;

				//1
				command << " M " << (x + rx) << "," << y;

				//2
				command << " L " << (x + width - rx) << "," << y;

				//3
				command << " A " << rx << " " << ry << " " << 0 << " " << 0 << " " << 1 << " " << (x+width) << "," << (y+ry);

				//4
				command << " L " << (x + width) << "," << (y+height-ry);

				//5
				command << " A " << rx << " " << ry << " " << 0 << " " << 0 << " " << 1 << " " << (x+width-rx) << "," << (y+height);

				//6
				command << " L " << (x+rx) << ", " << (y+height);

				//7
				command << " A " << rx << " " << ry << " " << 0 << " " << 0 << " " << 1 << " " << (x) << "," << (y+height-ry);

				//8
				command << " L " << (x) << "," << (y+ry);

				//9
				command << " A " << rx << " " << ry << " " << 0 << " " << 0 << " " << 1 << " " << (x+rx) << "," << (y);

				command << " Z";

				parsePath( command.str(), shape );
			}
			else
			{
				shape.moveTo( vec2(x,y ));
				shape.lineTo( vec2(x + width, y ));
				shape.lineTo( vec2(x + width, y + height ));
				shape.lineTo( vec2(x, y + height ));
				shape.close();
			}
		}

		void parseCircleShape( const XmlTree &shapeNode, Shape2d &shape )
		{
			float
			cx = getNumericAttributeValue( shapeNode, "cx", 0 ),
			cy = getNumericAttributeValue( shapeNode, "cy", 0 ),
			r = getNumericAttributeValue( shapeNode, "r", 0 );

			if ( r > 0 )
			{
				shape.moveTo( vec2( cx + r, cy ));
				shape.arc( vec2( cx, cy ), r, 0, 2 * M_PI );
				shape.close();
			}
		}

		void parseEllipseShape( const XmlTree &shapeNode, Shape2d &shape )
		{
			float
			cx = getNumericAttributeValue( shapeNode, "cx", 0 ),
			cy = getNumericAttributeValue( shapeNode, "cy", 0 ),
			rx = getNumericAttributeValue( shapeNode, "rx", 0 ),
			ry = getNumericAttributeValue( shapeNode, "ry", 0 );

			if ( rx > 0 && ry > 0 )
			{
				std::stringstream command;

				command << "M " << (cx + rx) << "," << (cy);
				command << " A " << rx << "," << ry << " " << 0 << " " << 0 << " " << 1 << " " << (cx) << "," << (cy+ry);
				command << " A " << rx << "," << ry << " " << 0 << " " << 0 << " " << 1 << " " << (cx-rx) << "," << (cy);
				command << " A " << rx << "," << ry << " " << 0 << " " << 0 << " " << 1 << " " << (cx) << "," << (cy-ry);
				command << " A " << rx << "," << ry << " " << 0 << " " << 0 << " " << 1 << " " << (cx+rx) << "," << (cy);
				command << " Z";

				parsePath( command.str(), shape );
			}
		}

		void parseLineShape( const XmlTree &shapeNode, Shape2d &shape )
		{
			vec2 a,b;

			a.x = getNumericAttributeValue( shapeNode, "x1", 0 );
			a.y = getNumericAttributeValue( shapeNode, "y1", 0 );
			b.x = getNumericAttributeValue( shapeNode, "x2", 0 );
			b.y = getNumericAttributeValue( shapeNode, "y2", 0 );

			if ( length2(a-b) > EPSILON )
			{
				shape.moveTo( a );
				shape.lineTo( b );
			}
		}

		void parsePolylineShape( const XmlTree &shapeNode, Shape2d &shape )
		{
			if ( shapeNode.hasAttribute( "points" ))
			{
				std::vector< ci::vec2 > points;
				parsePolyline( shapeNode.getAttribute( "points" ).getValue(), points );

				if ( !points.empty() )
				{
					// we don't close polyline shapes
					shape.moveTo( points.front() );
					for( std::vector< ci::vec2 >::const_iterator p(points.begin() + 1),end(points.end()); p != end; ++p )
					{
						shape.lineTo( *p );
					}
				}
			}
		}

		void parsePolygonShape( const XmlTree &shapeNode, Shape2d &shape )
		{
			parsePolylineShape( shapeNode, shape );

			// polygon shapes are just polylines that are closed
			shape.close();
		}

	}

	float parseNumericAttribute( const std::string &numericAttributeValue )
	{
		return std::strtod( numericAttributeValue.c_str(), NULL );
	}

	namespace {

		void applyStyle( const std::string &name, const std::string & value, svg_style &style )
		{
			if ( name == "opacity" )
			{
				style.opacity *= parseNumericAttribute(value);
			}
			else if ( name == "fill-opacity" )
			{
				style.fillOpacity *= parseNumericAttribute(value);
			}
			else if ( name == "fill" )
			{
				style.filled = parseColor( value, style.fillColor );
			}
			else if ( name == "stroke" )
			{
				style.stroked = parseColor( value, style.strokeColor );
			}
			else if ( name == "stroke-opacity" )
			{
				style.strokeOpacity *= parseNumericAttribute( value );
			}
			else if ( name == "stroke-width" )
			{
				style.strokeWidth = parseNumericAttribute( value );
				if (style.strokeWidth <= EPSILON ) style.stroked = false;
			}
			else if ( name == "fill-rule" )
			{
				if ( value == "evenodd" ) style.fillRule = Triangulator::WINDING_ODD;
				else if ( value == "nonzero" ) style.fillRule = Triangulator::WINDING_NONZERO;
			}
		}
	}

	svg_style parseStyle( const XmlTree &node )
	{
		svg_style style;

		// individual fill styles
		for( const XmlTree::Attr &attr : node.getAttributes() )
		{
			applyStyle( attr.getName(), attr.getValue(), style );
		}

		// style-list overrides attrs?
		if ( node.hasAttribute( "style" ))
		{
			for( const std::string &styleToken : strings::split( node.getAttribute("style").getValue(), ';' ) )
			{
				strings::stringvec params = strings::split( styleToken, ':' );
				applyStyle( params[0], params[1], style );
			}
		}

		//
		//	Unset filled/stroked flags if the opacity or stroke width make it invisible
		//

		if ( style.fillOpacity < ALPHA_EPSILON )
		{
			style.filled = false;
		}

		if ( style.strokeOpacity < ALPHA_EPSILON || style.strokeWidth <= EPSILON )
		{
			style.stroked = false;
		}

		return style;
	}


	/**
	 transforms are in form of: http://www.w3.org/TR/SVG/coords.html#TransformAttribute

	 matrix(<a> <b> <c> <d> <e> <f>), which specifies a transformation in the form of a transformation matrix of six values. matrix(a,b,c,d,e,f) is equivalent to applying the transformation matrix [a b c d e f].

	 translate(<tx> [<ty>]), which specifies a translation by tx and ty. If <ty> is not provided, it is assumed to be zero.

	 scale(<sx> [<sy>]), which specifies a scale operation by sx and sy. If <sy> is not provided, it is assumed to be equal to <sx>.

	 rotate(<rotate-angle> [<cx> <cy>]), which specifies a rotation by <rotate-angle> degrees about a given point.
	 If optional parameters <cx> and <cy> are not supplied, the rotate is about the origin of the current user coordinate system. The operation corresponds to the matrix [cos(a) sin(a) -sin(a) cos(a) 0 0].
	 If optional parameters <cx> and <cy> are supplied, the rotate is about the point (cx, cy). The operation represents the equivalent of the following specification: translate(<cx>, <cy>) rotate(<rotate-angle>) translate(-<cx>, -<cy>).

	 skewX(<skew-angle>), which specifies a skew transformation along the x-axis.

	 skewY(<skew-angle>), which specifies a skew transformation along the y-axis.


	 They can be concatenated:
		"translate(-10,-20) scale(2) rotate(45) translate(5,10)"
	 */

	mat4 parseTransform( const std::string &svgTransform )
	{
		mat4 transform;

		//	split - only supporting multiple transforms separated by spaces
		//	note: we're concatenating each transform into one

		for( const std::string &transformToken : strings::split( svgTransform, ' ' ))
		{
			mat4 m;
			if ( strings::startsWith( transformToken, "matrix" ))			m = parseMatrixTransform( transformToken );
			else if ( strings::startsWith( transformToken, "translate" )) m = parseTranslateTransform( transformToken );
			else if ( strings::startsWith( transformToken, "scale" ))		m = parseScaleTransform( transformToken );
			else if ( strings::startsWith( transformToken, "rotate" ))	m = parseRotateTransform( transformToken );
			else if ( strings::startsWith( transformToken, "skewX" ))		m = parseSkewXTransform( transformToken );
			else if ( strings::startsWith( transformToken, "skewY" ))		m = parseSkewYTransform( transformToken );
			else continue;

			transform *= m;
		}

		return transform;
	}

	bool parseColor( const std::string &colorValue, Color &color )
	{
		ColorA temp;
		if ( parseColor( colorValue, temp ) )
		{
			color = temp;
			return true;
		}

		return false;
	}

	bool parseColor( const std::string &colorValue, ColorA &color )
	{
		color = ColorA::black();

		if ( colorValue.find( "#" ) == 0 )
		{
			if ( colorValue.size() == 7 )
			{
				std::string
				red = colorValue.substr( 1,2 ),
				green = colorValue.substr( 3,2 ),
				blue = colorValue.substr( 5,2 );

				color.r = saturate(float(std::strtol( red.c_str(), NULL, 16 )) / 255);
				color.g = saturate(float(std::strtol( green.c_str(), NULL, 16 )) / 255);
				color.b = saturate(float(std::strtol( blue.c_str(), NULL, 16 )) / 255);
				color.a = 1;

				return true;
			}
			else if ( colorValue.size() == 9 )
			{
				std::string
				alpha = colorValue.substr( 1,2 ),
				red = colorValue.substr( 3,2 ),
				green = colorValue.substr( 5,2 ),
				blue = colorValue.substr( 7,2 );

				color.r = saturate(float(std::strtol( red.c_str(), NULL, 16 )) / 255);
				color.g = saturate(float(std::strtol( green.c_str(), NULL, 16 )) / 255);
				color.b = saturate(float(std::strtol( blue.c_str(), NULL, 16 )) / 255);
				color.a = saturate(float(std::strtol( alpha.c_str(), NULL, 16 )) / 255);

				return true;
			}
		}
		else if ( colorValue.find( "rgb" ) == 0 )
		{
			const std::string::size_type
			OpenParen = colorValue.find_first_of( '(' ),
			CloseParen = colorValue.find_last_of( ')' );

			if ( OpenParen == std::string::npos || CloseParen == std::string::npos )
			{
				return false;
			}

			const std::string ValueString = colorValue.substr( OpenParen+1, (CloseParen-OpenParen-1));
			std::vector<float> components;

			for( const std::string &token : strings::split( ValueString, "," ))
			{
				if ( token.find("%") != std::string::npos )
				{
					float value = 255 * saturate(std::strtod( token.c_str(), NULL ) / 100);
					components.push_back( value );
				}
				else
				{
					float value = std::strtod( token.c_str(), NULL );
					components.push_back( saturate( value / 255 ));
				}
			}

			if ( components.size() >= 3 )
			{
				color.r = components[0];
				color.g = components[1];
				color.b = components[2];
				color.a = components.size() >=4 ? saturate( components[3] ) : 1;
				return true;
			}
		}

		return false;
	}


	void parsePath( const std::string &pathString, Shape2d &shape )
	{
		PathParser pp;
		pp.parse( pathString, shape );
	}

	bool canParseShape( std::string shapeName )
	{
		return
		shapeName == "path" ||
		shapeName == "rect" ||
		shapeName == "circle" ||
		shapeName == "ellipse" ||
		shapeName == "line" ||
		shapeName == "polyline" ||
		shapeName == "polygon";
	}
	
	void parseShape( const XmlTree &shapeNode, ci::Shape2d &shape )
	{
		std::string name = shapeNode.getTag();
		
		if ( name == "path" )		{ parsePathShape( shapeNode, shape ); return; }
		if ( name == "rect" )		{ parseRectShape( shapeNode, shape ); return; }
		if ( name == "circle" )		{ parseCircleShape( shapeNode, shape ); return; }
		if ( name == "ellipse" )	{ parseEllipseShape( shapeNode, shape ); return; }
		if ( name == "line" )		{ parseLineShape( shapeNode, shape ); return; }
		if ( name == "polyline" )	{ parsePolylineShape( shapeNode, shape ); return; }
		if ( name == "polygon" )	{ parsePolygonShape( shapeNode, shape ); return; }
		
		throw ParserException( "Unsupported SVG shape type: \"" + name + "\"" );
	}
	
}}} // end namespace core::util::svg