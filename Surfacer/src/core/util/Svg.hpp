//
//  Svg.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/4/17.
//
//

#ifndef Svg_hpp
#define Svg_hpp

#include <cinder/Triangulate.h>
#include <cinder/Xml.h>
#include <cinder/gl/VboMesh.h>

#include "Common.hpp"
#include "Object.hpp"
#include "BlendMode.hpp"
#include "Exception.hpp"
#include "MathHelpers.hpp"
#include "RenderState.hpp"
#include "SvgParsing.hpp"

namespace core {
    namespace util {
        namespace svg {

            SMART_PTR(Appearance);

            SMART_PTR(Shape);

            SMART_PTR(Group);

            class Appearance {
            public:

                Appearance();

                virtual ~Appearance();

                void parse(const ci::XmlTree &shapeNode);

                void setParentAppearance(AppearanceRef parentAppearance) {
                    _parentAppearance = parentAppearance;
                }

                AppearanceRef getParentAppearance() const {
                    return _parentAppearance.lock();
                }

                void setFillColor(const ci::ColorA &color);

                ci::ColorA getFillColor() const;

                void setFillAlpha(double a);

                bool getFillAlpha() const;

                void setStrokeColor(const ci::ColorA &color);

                ci::ColorA getStrokeColor() const;

                void setStrokeWidth(double w);

                double getStrokeWidth() const;

                bool isStroked() const;

                bool isFilled() const;

                void setBlendMode(const BlendMode &bm);

                const BlendMode &getBlendMode() const;

                void setFillRule(ci::Triangulator::Winding fr);

                ci::Triangulator::Winding getFillRule() const;

                string getAttribute(string name) const;

            private:

                ci::Color _getFillColor() const;

                double _getFillOpacity() const;

                ci::Color _getStrokeColor() const;

                double _getStrokeOpacity() const;

                double _getStrokeWidth() const;

                ci::Triangulator::Winding _getFillRule() const;


            private:

                AppearanceWeakRef _parentAppearance;
                svg_style _style;
                BlendMode _blendMode;
                map <string, string> _attributes;

            };

            /**
             Shape
             Represents an Svg node which draws something, e.g, rect/circle/polyline, etc. Everything supported by core::util::svg::canParseShape
             */
            class Shape {
            public:

                struct stroke {
                    vector <vec2> vertices;
                    bool closed;
                    ci::gl::VboMeshRef vboMesh;

                    stroke() :
                            closed(false) {
                    }
                };

            public:

                Shape();

                ~Shape();

                void parse(const ci::XmlTree &shapeNode);

                void draw(const render_state &, const GroupRef &owner, double opacity, const ci::gl::GlslProgRef &shader);

                /**
                 Get appearance info (color, stroke, etc)
                 */
                const AppearanceRef &getAppearance() const {
                    return _appearance;
                }

                /**
                 Get bounding box of this shape's geometry in its local coordinate space
                 */
                cpBB getLocalBB() const {
                    return _localBB;
                }

                /**
                 return true if this shape represents the origin shape for the parent Group
                 origin shapes aren't drawn, they're used to position the origin for the parent object's shapes and children

                 to mark a shape as origin, do any of the following
                 - set the node's id to anything starting with "__origin__", e.g., "__origin__1", "___origin__2", etc
                 - set attriubute __origin__="true" to the shape node
                 - add class "__origin__" to the shape node's classes

                 */
                bool isOrigin() const {
                    return _origin;
                }

                // returns 'path', 'rect', etc
                string getType() const {
                    return _type;
                }

                // returns the shape's id
                string getId() const {
                    return _id;
                }

                // get all the attribute pairs on this shape
                const map <string, string> &getAttributes() const {
                    return _attributes;
                }

            private:

                friend class Group;

                void _build(const ci::Shape2d &shape);

                ci::Rectd _projectToWorld(const dvec2 &documentSize, double documentScale, const dmat4 &worldTransform);

                void _makeLocal(const dvec2 &originWorld);

                void _drawDebug(const render_state &state, const GroupRef &owner, double opacity, const ci::TriMeshRef &mesh, vector <stroke> &strokes, const ci::gl::GlslProgRef &shader);

                void _drawGame(const render_state &state, const GroupRef &owner, double opacity, const ci::TriMeshRef &mesh, vector <stroke> &strokes, const ci::gl::GlslProgRef &shader);

                void _drawStrokes(const render_state &state, const GroupRef &owner, double opacity, vector <stroke> &strokes, const ci::gl::GlslProgRef &shader);

                void _drawFills(const render_state &state, const GroupRef &owner, double opacity, const ci::TriMeshRef &mesh, const ci::gl::GlslProgRef &shader);

            private:

                AppearanceRef _appearance;
                bool _origin;
                dmat4 _svgTransform;
                map <string, string> _attributes;
                string _type, _id;
                cpBB _localBB;

                ci::TriMeshRef _svgMesh, _worldMesh, _localMesh;
                ci::gl::VboMeshRef _localVboMesh;

                vector <stroke> _svgStrokes, _worldStrokes, _localStrokes;
                ci::Rectd _worldBounds, _localBounds;

            };


            /**
             @class Group
             Group corresponds to an SVG group <g> element. Each group gets a transform, name, and has child SVGObjects and child SVGShapes.
             */
            class Group : public enable_shared_from_this<Group> {
            public:

                static GroupRef loadSvgDocument(ci::DataSourceRef svgData, double documentScale = 1) {
                    GroupRef g = make_shared<Group>();
                    g->load(svgData, documentScale);
                    return g;
                }

            public:

                Group();

                ~Group();

                /**
                 Load an SVG file.
                 @param svgData SVG byte stream
                 @param documentScale A scaling factor to apply to the SVG geometry.
                 */
                void load(ci::DataSourceRef svgData, double documentScale = 1);

                void clear();

                void parse(const ci::XmlTree &svgGroupNode);

                string getId() const {
                    return _id;
                }

                const map <string, string> &getAttributes() const {
                    return _attributes;
                }

                const vector <ShapeRef> getShapes() const {
                    return _shapes;
                }

                const vector <GroupRef> getGroups() const {
                    return _groups;
                }

                /**
                 Get this group's appearance. Generall the style attributes in the appearance of a Group will not be set. It's when
                 the group has a chile <use .../> declaration which will specify fill, stroke etc that the Appearance will have
                 meaningful values.
                 */
                AppearanceRef getAppearance() const {
                    return _appearance;
                }

                /**
                 get this Group's parent. May be empty.
                 */
                GroupRef getParent() const {
                    return _parent.lock();
                }

                /**
                 get root group of the tree
                 */
                GroupRef getRoot() const;

                /**
                 return true iff the root is this Group
                 */
                bool isRoot() const;

                /**
                 Find the child Group of this Group with id=name
                 */
                GroupRef getGroupNamed(const string &name) const;

                /**
                 Find the child SvgShape of this Group with id=name
                 */
                ShapeRef getShapeNamed(const string &name) const;

                /**
                 Get this group's origin shape if it has one.
                 The origin shape is a shape defining the (0,0) for the shape's coordinate system. The origin shape does not draw.
                 */
                ShapeRef getOriginShape() const {
                    return _originShape;
                }

                /**
                 Find a child Group given a path, in form of path/to/child.
                 If @pathToChild starts with the separator, it is considered an absolute path and is evaluated from the root Group.
                 find() uses the name() and label() for name comparison, trying name() first, and then label(). This is an artifact
                 of Inkscape making it really hard to assign ids, but easy to assign labels to groups.
                 */
                GroupRef find(const string &pathToChild, char separator = '/') const;

                void draw(const render_state &state);

                /**
                 Compute world-space BB of this group and its contents, recursively
                 */
                cpBB getBB();

                /**
                 Dumps human-readable tree to stdout
                 */
                void trace(int depth = 0) const;

                dvec2 getDocumentSize() const;

                void setPosition(const dvec2 &position);

                dvec2 getPosition() const {
                    return _localTransformPosition;
                }
                
                void setRotation(const dvec2 &rotation);
                
                dvec2 getRotation() const;

                void setAngle(double a);

                double getAngle() const { return _localTransformAngle; }

                void setScale(double s) {
                    setScale(s, s);
                }

                void setScale(double xScale, double yScale) {
                    setScale(dvec2(xScale, yScale));
                }

                void setScale(const dvec2 &scale);

                dvec2 getScale() const {
                    return _localTransformScale;
                }

                /**
                 Set the opacity for this SVGObject.
                 When drawn, each child SVGShape will draw with its fill opacity * parent SVGObject.alpha. The alpha
                 is inherited down the line by child SVGObjects.
                 */
                void setOpacity(double a) {
                    _opacity = saturate(a);
                }

                double getOpacity(void) const {
                    return _opacity;
                }

                /**
                 Set blend mode for this layer. This blend mode affects all
                 children of the layer which don't specify their own blend modes.

                 Note: default BlendMode is GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA
                 */
                void setBlendMode(const BlendMode &bm);

                const BlendMode &getBlendMode() const {
                    return _blendMode;
                }

                /**
                 Project a point in this Group's local space to world
                 */
                dvec2 localToGlobal(const dvec2 &p);

                /**
                 Project a point in world space to this Group's local space
                 */
                dvec2 globalToLocal(const dvec2 &p);

            private:

                friend class Shape;

                void _draw(const render_state &state, double opacity, const ci::gl::GlslProgRef &shader);

                void _parseGroupAttributes(const ci::XmlTree &groupNode);

                void _loadGroupsAndShapes(const ci::XmlTree &fromNode);

                void _loadAppearances(const ci::XmlTree &fromNode);

                void _updateTransform();

                cpBB _getBB(dmat4 toWorld);

                /**
                 walk down tree, moving vertices of child shapes to world space, then walk back up tree transforming them to space local to their transform.
                 */
                void _normalize(const dvec2 &documentSize, double documentScale, const dmat4 &worldTransform, const dvec2 &parentWorldOrigin);

                /**
                 get the world transform of this Group
                 The world transform is the concatenation of the matrices from root down to this object
                 */
                dmat4 _worldTransform(dmat4 m);

            private:

                typedef pair <GroupRef, ShapeRef> drawable;

                GroupWeakRef _parent;
                ShapeRef _originShape;
                AppearanceRef _appearance;

                string _id;
                double _opacity;
                bool _transformDirty;
                double _localTransformAngle;
                dvec2 _localTransformPosition, _localTransformScale, _documentSize;
                dmat4 _groupTransform, _transform;
                BlendMode _blendMode;

                vector <ShapeRef> _shapes;
                map <string, ShapeRef> _shapesByName;
                vector <GroupRef> _groups;
                map <string, GroupRef> _groupsByName;
                map <string, string> _attributes;
                vector <drawable> _drawables;

                ci::gl::GlslProgRef _shader;
            };

            class LoadException : public core::Exception {
            public:

                LoadException(const std::string &w) : Exception(w) {
                }

                virtual ~LoadException() throw() {
                }

            };

#pragma mark - SvgDrawComponent

            class SvgDrawComponent : public DrawComponent {
            public:
                SvgDrawComponent(util::svg::GroupRef doc, int layer = 0) :
                        _docRoot(doc),
                        _layer(layer) {
                }

                cpBB getBB() const override {
                    return _docRoot->getBB();
                }

                void draw(const core::render_state &state) override {
                    _docRoot->draw(state);
                }

                VisibilityDetermination::style getVisibilityDetermination() const override {
                    return VisibilityDetermination::FRUSTUM_CULLING;
                }

                int getLayer() const override {
                    return _layer;
                }

                void setLayer(int layer) {
                    _layer = layer;
                }

                const GroupRef &getSvg() const {
                    return _docRoot;
                }

            private:
                GroupRef _docRoot;
                int _layer;
            };


        }
    }
} // core::util::svg

#endif /* Svg_hpp */
