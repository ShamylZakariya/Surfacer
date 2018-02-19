//
//  TerrainWorld.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/1/17.
//
//

#ifndef TerrainWorld_hpp
#define TerrainWorld_hpp

#include <cinder/app/App.h>
#include <boost/functional/hash.hpp>
#include <unordered_set>

#include "Core.hpp"
#include "Signals.hpp"

namespace terrain {

    typedef boost::geometry::model::d2::point_xy<double> dpoint2;
    typedef boost::geometry::model::polygon<dpoint2> dpolygon2;

    SMART_PTR(World);

    SMART_PTR(GroupBase);

    SMART_PTR(StaticGroup);

    SMART_PTR(DynamicGroup);

    SMART_PTR(Drawable);

    SMART_PTR(Shape);

    SMART_PTR(Anchor);

    SMART_PTR(Element);
    
    SMART_PTR(Attachment);

    /**
     Edges whos vertices are within 1/POLY_EDGE_PRECISION distance of eachother are considered congruent, and thus snapped.
     */

#define POLY_EDGE_PRECISION 10.0

    /**
     poly_edge represents an edge in a PolyLine2d. It is not meant to represent the specific vertices in
     a coordinate system but rather it is meant to make it easy to determine if two PolyShapes share an edge.
     As such the coordinates a and b are integers, representing the world space coords of the PolyShape outer
     contour at a given stage of precision.
     */
    struct poly_edge {
        ivec2 a, b;

        poly_edge(dvec2 m, dvec2 n) {
            a.x = static_cast<int>(lround(m.x * POLY_EDGE_PRECISION));
            a.y = static_cast<int>(lround(m.y * POLY_EDGE_PRECISION));
            b.x = static_cast<int>(lround(n.x * POLY_EDGE_PRECISION));
            b.y = static_cast<int>(lround(n.y * POLY_EDGE_PRECISION));

            // we want consistent ordering
            if (a.x == b.x) {
                if (b.y < a.y) {
                    swap(a, b);
                }
            } else {
                if (b.x < a.x) {
                    swap(a, b);
                }
            }
        }

        friend bool operator==(const poly_edge &e0, const poly_edge &e1) {
            return e0.a == e1.a && e0.b == e1.b;
        }

        friend bool operator<(const poly_edge &e0, const poly_edge &e1) {
            if (e0.a.x != e1.a.x) {
                return e0.a.x < e1.a.x;
            } else if (e0.a.y != e1.a.y) {
                return e0.a.y < e1.a.y;
            } else if (e0.b.x != e1.b.x) {
                return e0.b.x < e1.b.x;
            }
            return e0.b.y < e1.b.y;
        }

        friend std::ostream &operator<<(std::ostream &lhs, const poly_edge &rhs) {
            return lhs << "(poly_edge " << dvec2(rhs.a.x / POLY_EDGE_PRECISION, rhs.a.y / POLY_EDGE_PRECISION)
                       << " : " << dvec2(rhs.b.x / POLY_EDGE_PRECISION, rhs.b.y / POLY_EDGE_PRECISION) << ")";
        }
    };

} // namespace terrain

#pragma mark -

namespace std {

    template<>
    struct hash<terrain::poly_edge> {

        // make island::poly_edge hashable
        size_t operator()(const terrain::poly_edge &e) const {
            size_t seed = 0;
            boost::hash_combine(seed, e.a.x);
            boost::hash_combine(seed, e.a.y);
            boost::hash_combine(seed, e.b.x);
            boost::hash_combine(seed, e.b.y);
            return seed;
        }
    };

}


namespace terrain {

#pragma mark - Material

    /**
     Describes basic physics material properties for a collision shape
     */
    struct material {
        cpFloat density;
        cpFloat friction;
        cpFloat collisionShapeRadius;
        cpShapeFilter filter;
        cpCollisionType collisionType;
        double minSurfaceArea;
        ci::Color color;

        material() :
                density(1),
                friction(1),
                collisionShapeRadius(0),
                filter({0, 0, 0}),
                collisionType(0),
                minSurfaceArea(0.1),
                color(0.2, 0.2, 0.2) {
        }

        material(cpFloat d, cpFloat f, cpFloat csr, cpShapeFilter flt, cpCollisionType ct, double msa, ci::Color c) :
                density(d),
                friction(f),
                collisionShapeRadius(csr),
                filter(flt),
                collisionType(ct),
                minSurfaceArea(msa),
                color(c) {
        }

        material(const material &c) :
                density(c.density),
                friction(c.friction),
                collisionShapeRadius(c.collisionShapeRadius),
                filter(c.filter),
                collisionType(c.collisionType),
                minSurfaceArea(c.minSurfaceArea),
                color(c.color) {
        }
    };

#pragma mark - DrawDispatcher

    class DrawDispatcher {
    public:

        struct collector {
            set <DrawableRef> visible;
            vector <DrawableRef> sorted;

            void remove(const DrawableRef &s) {
                visible.erase(s);
                sorted.erase(std::remove(sorted.begin(), sorted.end(), s), sorted.end());
            }
        };

        DrawDispatcher();

        virtual ~DrawDispatcher();

        void add(const DrawableRef &);

        void remove(const DrawableRef &);

        void moved(const DrawableRef &);

        void moved(Drawable *);

        void cull(const core::render_state &);

        void draw(const core::render_state &, const ci::gl::GlslProgRef &shader);

        void setDrawPasses(size_t passes) {
            _drawPasses = passes;
        }

        size_t getPasses() const {
            return _drawPasses;
        }

        size_t visibleCount() const {
            return _collector.sorted.size();
        }

        const set <DrawableRef> &getVisible() const {
            return _collector.visible;
        }

        const vector <DrawableRef> &getVisibleSorted() const {
            return _collector.sorted;
        }

    private:

        /**
            render a run of shapes belonging to a common group
            returns iterator to last shape drawn
         */
        vector<DrawableRef>::iterator _drawGroupRun(vector<DrawableRef>::iterator first, vector<DrawableRef>::iterator storageEnd, const core::render_state &state, const ci::gl::GlslProgRef &shader);

    private:

        cpSpatialIndex *_index;
        set <DrawableRef> _all;
        collector _collector;
        size_t _drawPasses;

    };


#pragma mark - World


    /**
     @class World
     World "owns" and manages Group instances, which in turn own and manage Shape instances.
     */
    class World : public enable_shared_from_this<World> {
    public:

        static void loadSvg(ci::DataSourceRef svgData, dmat4 transform,
                vector <ShapeRef> &shapes, vector <AnchorRef> &anchors, vector <ElementRef> &elements,
                bool flip = true);

        static void march(const ci::Channel8u isoSurface, double isoLevel, dmat4 transform, vector <ShapeRef> &shapes);

        static void march(const ci::Channel8u isoSurface, const ci::Channel8u anchorIsoSurface, double isoLevel, dmat4 transform, vector <ShapeRef> &shapes, vector <AnchorRef> &anchors);

        /**
         Partitions shapes in `shapes to a grid with origin at partitionOrigin, with chunks of size paritionSize.
         The purpose of this is simple: You might have a large stage. You want to divide the geometry of the stage into manageable
         chunks for visibility culling and collision/physics performance.
         */
        static vector <ShapeRef> partition(const vector <ShapeRef> &shapes, dvec2 partitionOrigin, double partitionSize);

        static size_t nextId() {
            return _idCounter++;
        }

    public:

        World(core::SpaceAccessRef space, material worldMaterial, material anchorMaterial);

        virtual ~World();

        /**
         Build a world of dynamic and static shapes. Any shape overlapping an Anchor will be static.
         Pieces cut off a static shape will become dynamic provided they don't overlap another anchor.
         */

        void build(const vector <ShapeRef> &shapes,
                const vector <AnchorRef> &anchors = vector<AnchorRef>(),
                const vector <ElementRef> &elements = vector<ElementRef>());

        /**
         Perform a cut in world space from a to b, with half-thickness of radius.
         Discard any resultant geometry with less than minSurfaceArea
         */
        void cut(dvec2 a, dvec2 b, double radius, double minSurfaceArea = 1);

        /**
         Perform a cut in world space removing any geometry which overlaps the enclosed space of `polygonShape`
         Discard any resultant geometry with less than minSurfaceArea
         */
        void cut(const dpolygon2 &polygonShape, cpBB polygonShapeWorldBounds = cpBBInvalid, double minSurfaceArea = 0);


        void draw(const core::render_state &renderState);

        void step(const core::time_state &timeState);

        void update(const core::time_state &timeState);

        const set <DynamicGroupRef> &getDynamicGroups() const {
            return _dynamicGroups;
        }

        StaticGroupRef getStaticGroup() const {
            return _staticGroup;
        }

        const vector <AnchorRef> &getAnchors() const {
            return _anchors;
        }

        const vector <ElementRef> &getElements() const {
            return _elements;
        }

        ElementRef getElementById(string id) const;
        
        const set <AttachmentRef> &getOrphanedAttachments() const {
            return _orphanedAttachments;
        }

        core::SpaceAccessRef getSpace() const {
            return _space;
        }

        material getWorldMaterial() const {
            return _worldMaterial;
        }

        material getAnchorMaterial() const {
            return _anchorMaterial;
        }

        DrawDispatcher &getDrawDispatcher() {
            return _drawDispatcher;
        }

        const DrawDispatcher &getDrawDispatcher() const {
            return _drawDispatcher;
        }


        typedef function<bool(const DynamicGroupRef &)> DynamicGroupVisitor;

        /**
         Perform a culling of dynamic groups with less than a given surface area.
         All geometry of less than `minSurfaceArea will be collected and sorted,
         and `portion amount of that geometry (0 (none) to 1 (all)) will be discarded.
         Optional test argument is a functor called for each group smaller than minSurfaceArea to decide if it should be in the cull set.
         Returns number of groups culled.
         */
        size_t cullDynamicGroups(double minSurfaceArea, double portion = 1, const DynamicGroupVisitor &test = DynamicGroupVisitor());

        /**
         Gather DynamicGroups that have been sleeping for at least `minSleepTime (and if `test is provided, winnow to those who
         pass the test), then take the longest sleeping portion of that set and make them part of the static group.
         Think of this as a way of saying, "I want the rubble which has been still for a long time to become static environment"
         */
        size_t makeSleepingDynamicGroupsStatic(core::seconds_t minSleepTime, double portion = 1, const DynamicGroupVisitor &test = DynamicGroupVisitor());

        /**
         Get the Object which wraps this World for use in a Stage
         */
        core::ObjectRef getObject() const;
        
        /**
         Attempt to place an attachment in the World. Returns true if the position corresponded to solid geometry
         and the attachment was able to be parented to it.
         */
        bool addAttachment(const AttachmentRef &attachment, dvec2 worldPosition, double angle = 0);

    protected:

        friend class TerrainObject;

        friend class Shape;

        friend class DynamicGroup;
        
        bool addAttachment(const AttachmentRef &attachment, dvec2 worldPosition, dvec2 rotation);
        
        void handleOrphanedAttachment(const AttachmentRef &attachment);

        void notifyCollisionShapesWillBeDestoyed(vector<cpShape *> shapes);

        void notifyBodyWillBeDestoyed(cpBody *body);

        void setObject(core::ObjectRef object);

        void build(const vector <ShapeRef> &shapes, const map <ShapeRef, GroupBaseRef> &parentage);

        vector <set<ShapeRef>> findShapeGroups(const vector <ShapeRef> &shapes, const map <ShapeRef, GroupBaseRef> &parentage);

        bool isShapeGroupStatic(const set <ShapeRef> shapeGroup, const GroupBaseRef &parentGroup);

    private:

        static size_t _idCounter;

        material _worldMaterial, _anchorMaterial;
        core::SpaceAccessRef _space;
        StaticGroupRef _staticGroup;
        set <DynamicGroupRef> _dynamicGroups;
        vector <AnchorRef> _anchors;
        vector <ElementRef> _elements;
        set <AttachmentRef> _orphanedAttachments;
        map <string, ElementRef> _elementsById;

        DrawDispatcher _drawDispatcher;
        ci::gl::GlslProgRef _shader;

        core::ObjectWeakRef _object;
    };
    
#pragma mark - Attachment
    
    class Attachment {
    public:

        // signal fired when this attachment is orphaned from a group and moved from Group ownership to the World's orphan set
        core::signals::signal<void()> onOrphaned;
        
    public:

        // create an unparented Attachment. Position is assigned when calling World::addAttachment
        Attachment();
        virtual ~Attachment();
        
        size_t getId() const { return _id; }
        
        // get the Group this Attachment is attached to
        GroupBaseRef getGroup() const { return _group.lock(); }
        
        // return true if this Attachment has become detached from a Group and has become orphaned
        bool isOrphaned() const { return _group.expired(); }

        // get transform (position/rotation) of this Attachment relative to its parent group
        dmat4 getLocalTransform() const { return _localTransform; }
        
        // get transform (position/rotation) of this attachment relative to world
        dmat4 getWorldTransform() const { return _worldTransform; }
        
        // get position of this attachment in coordinate space of its group
        dvec2 getLocalPosition() const;
        
        // get rotation of this attachment in coordinate space of its group
        dvec2 getLocalRotation() const;
        
        // get position of this attachment in world
        dvec2 getWorldPosition() const;
        
        // get rotation of this attachment in world
        dvec2 getWorldRotation() const;
        
        // To "kill" an attachment, call setFinished(true); it will be detached from its group
        // or if it's an orphan, will be detached from the world's orphan set. It will no longer
        // be updated and if no strong references are held, it will be destroyed.
        void setFinished(bool finished=true);

        // return true if this attachment is scheduled to be let go next timestep
        bool isFinished() const { return _finished; }
        
    private:
        
        friend class World;
        friend class GroupBase;
        friend class StaticGroup;
        friend class DynamicGroup;
        
        // called by World::addAttachment
        void configure(const GroupBaseRef &group, dvec2 position, dvec2 rotation);
        
        size_t _id;
        dmat4 _localTransform;
        dmat4 _worldTransform;
        GroupBaseWeakRef _group;
        bool _finished;
        
    };


#pragma mark - GroupBase


    class GroupBase : public core::IChipmunkUserData {
    public:

        GroupBase(WorldRef world, material m, DrawDispatcher &dispatcher);

        virtual ~GroupBase();

        DrawDispatcher &getDrawDispatcher() const {
            return _drawDispatcher;
        }

        const core::SpaceAccessRef &getSpace() const {
            return _space;
        }

        const material &getMaterial() const {
            return _material;
        }

        virtual string getName() const {
            return _name;
        }

        virtual Color getColor(const core::render_state &state) const {
            return _material.color;
        }

        virtual cpHashValue getHash() const {
            return _hash;
        }
        
        virtual bool isStatic() const = 0;
        
        virtual bool isDynamic() const { return !isStatic(); }

        virtual cpBody *getBody() const = 0;

        virtual cpBB getBB() const = 0;

        virtual dmat4 getModelMatrix() const = 0;

        virtual dmat4 getInverseModelMatrix() const = 0;

        virtual dvec2 getPosition() const = 0;

        virtual double getAngle() const = 0;

        virtual double getSurfaceArea() const = 0;

        virtual set <ShapeRef> getShapes() const = 0;

        virtual void releaseShapes() = 0;

        virtual size_t getDrawingBatchId() const {
            return _drawingBatchId;
        }

        virtual void step(const core::time_state &timeState);

        virtual void update(const core::time_state &timeState);

        cpTransform getModelTransform() const {
            dmat4 mm = getModelMatrix();
            return cpTransform {mm[0].x, mm[0].y, mm[1].x, mm[1].y, mm[3].x, mm[3].y};
        }

        cpTransform getInverseModelTransform() const {
            dmat4 mvi = getInverseModelMatrix();
            return cpTransform {mvi[0].x, mvi[0].y, mvi[1].x, mvi[1].y, mvi[3].x, mvi[3].y};
        }

        WorldRef getWorld() const;
        
        // return true iff lp (in local coordinate space) is inside one of this Group's shapes
        virtual bool isLocalPointInsideShapes(const dvec2 lp) const = 0;

        // return true iff wp (in world coordinate space) is inside one of this Group's shapes
        virtual bool isWorldPointInsideShapes(const dvec2 wp) const { return isLocalPointInsideShapes(getInverseModelMatrix() * wp); };
        
        const set <AttachmentRef> &getAttachments() const { return _attachments; }
        
        void clearAttachments() { _attachments.clear(); }
        
        // add an attachment to this Group's attachment set
        void addAttachment(const AttachmentRef &a) {
            _attachments.insert(a);
        }
        
        // remove an attachment from this Group's attachment set, returning true if it was removed
        virtual bool removeAttachment(const AttachmentRef &a) {
            if (_attachments.erase(a) > 0) {
                a->_group.reset();
                return true;
            }
            return false;
        }

        // IChipmunkUserData
        core::ObjectRef getObject() const override;
        
    protected:


        DrawDispatcher &_drawDispatcher;
        size_t _drawingBatchId;
        WorldWeakRef _world;
        core::SpaceAccessRef _space;
        set<AttachmentRef> _attachments;
        material _material;
        string _name;
        cpHashValue _hash;

    };

#pragma mark - StaticGroup

    class StaticGroup : public GroupBase, public enable_shared_from_this<StaticGroup> {
    public:
        StaticGroup(WorldRef world, material m, DrawDispatcher &dispatcher);

        ~StaticGroup();
        
        virtual bool isStatic() const override { return true; }

        cpBody *getBody() const override {
            return _body;
        }

        cpBB getBB() const override;

        dmat4 getModelMatrix() const override {
            return dmat4(1);
        }

        dmat4 getInverseModelMatrix() const override {
            return dmat4(1);
        }

        dvec2 getPosition() const override {
            return dvec2(0);
        }

        double getAngle() const override {
            return 0;
        }

        double getSurfaceArea() const override {
            return _surfaceArea;
        }

        set <ShapeRef> getShapes() const override {
            return _shapes;
        }

        void releaseShapes() override;
        
        bool isLocalPointInsideShapes(const dvec2 lp) const override;

        void addShape(ShapeRef shape, double minShapeArea);

        void removeShape(ShapeRef shape);

    protected:

        void updateWorldBB();

    protected:
        cpBody *_body;
        set <ShapeRef> _shapes;
        mutable cpBB _worldBB;
        double _surfaceArea;
    };


#pragma mark - DynamicGroup


    class DynamicGroup : public GroupBase, public enable_shared_from_this<DynamicGroup> {
    public:
        DynamicGroup(WorldRef world, material m, DrawDispatcher &dispatcher);

        ~DynamicGroup();

        // GroupBase
        string getName() const override;

        Color getColor(const core::render_state &state) const override;
        
        virtual bool isStatic() const override { return false; }

        cpBody *getBody() const override {
            return _body;
        }

        cpBB getBB() const override {
            return _worldBB;
        }

        dmat4 getModelMatrix() const override {
            return _modelMatrix;
        }

        dmat4 getInverseModelMatrix() const override {
            return _inverseModelMatrix;
        }

        dvec2 getPosition() const override {
            return v2(_position);
        }

        double getAngle() const override {
            return static_cast<double>(_angle);
        }

        double getSurfaceArea() const override {
            return _surfaceArea;
        }

        set <ShapeRef> getShapes() const override {
            return _shapes;
        }

        void releaseShapes() override;

        void step(const core::time_state &timeState) override;
        void update(const core::time_state &timeState) override;

        bool isLocalPointInsideShapes(const dvec2 lp) const override;

        // DynamicGroup
        // return true iff the physics body is sleeping
        bool isSleeping() const;

        // return the number of seconds that the body has been sleeping, or < 0 if it is awake
        core::seconds_t getSleepDuration() const {
            return _sleepDuration;
        }

    protected:
        friend class World;

        bool build(set <ShapeRef> shapes, const GroupBaseRef &parentGroup, double minShapeArea);

        void syncToCpBody();

    private:

        cpBody *_body;
        cpVect _position;
        cpFloat _angle;
        double _surfaceArea;
        cpBB _worldBB, _modelBB;
        dmat4 _modelMatrix, _inverseModelMatrix;
        core::seconds_t _sleepDuration;

        set <ShapeRef> _shapes;
    };

#pragma mark - Drawable

    class Drawable : public core::IChipmunkUserData, public enable_shared_from_this<Drawable> {
    public:
        Drawable();

        virtual ~Drawable();

        size_t getId() const {
            return _id;
        }

        virtual cpBB getBB() const = 0;

        virtual size_t getDrawingBatchId() const = 0;

        virtual size_t getLayer() const = 0;

        virtual dmat4 getModelMatrix() const = 0;

        virtual double getAngle() const = 0;

        virtual dvec2 getModelCentroid() const = 0;

        virtual const TriMeshRef &getTriMesh() const = 0;

        virtual const gl::VboMeshRef &getVboMesh() const = 0;

        virtual Color getColor(const core::render_state &state) const = 0;

        virtual bool shouldDraw(const core::render_state &state) const = 0;

        // get typed shared_from_this, e.g., shared_ptr<Shape> = shared_from_this_as<Shape>();
        template<typename T>
        shared_ptr<T const> shared_from_this_as() const {
            return static_pointer_cast<T const>(shared_from_this());
        }

        // get typed shared_from_this, e.g., shared_ptr<Shape> = shared_from_this_as<Shape>();
        template<typename T>
        shared_ptr <T> shared_from_this_as() {
            return static_pointer_cast<T>(shared_from_this());
        }

        void setWorld(WorldRef w);

        WorldRef getWorld() const;

        // IChipmunkUserData
        core::ObjectRef getObject() const override;

    private:

        size_t _id;
        WorldWeakRef _world;

    };

#pragma mark - Element

    /**
     @class Element
     An element is a shape specified in the <g id="elements">...</g> section of a stage SVG file.
     Each element represents simply a shape in world space with an associated ID. Elements can be used
     as location markers for enemy spawn points, or possibly as trigger geometry.
     */
    class Element : public Drawable {
    public:

        static const size_t DRAWING_BATCH_ID = 0;
        static const size_t LAYER = 2;

        static ElementRef fromContour(string id, const PolyLine2d &contour) {
            return make_shared<Element>(id, contour);
        }

    public:

        Element(string id, const PolyLine2d &contour);

        virtual ~Element();

        // Element
        string getId() const {
            return _id;
        }

        dvec2 getPosition() const;

        // Drawable
        cpBB getBB() const override {
            return _bb;
        }

        size_t getDrawingBatchId() const override {
            return DRAWING_BATCH_ID;
        }

        size_t getLayer() const override {
            return LAYER;
        }

        dmat4 getModelMatrix() const override {
            return dmat4(1);
        } // identity
        
        double getAngle() const override {
            return 0;
        };

        dvec2 getModelCentroid() const override;

        const TriMeshRef &getTriMesh() const override {
            return _trimesh;
        }

        const gl::VboMeshRef &getVboMesh() const override {
            return _vboMesh;
        }

        Color getColor(const core::render_state &state) const override {
            return Color(1, 0, 1);
        }

        bool shouldDraw(const core::render_state &state) const override;

    private:

        cpBB _bb;
        string _id;
        TriMeshRef _trimesh;
        ci::gl::VboMeshRef _vboMesh;

    };

#pragma mark - Anchor

    /**
     @class Anchor
     An Anchor is a single contour in world space which "anchors" shapes to be static and not dynamic bodies.
     A body is static if one of its shapes overlaps an anchor, and if the body's parentage has never been dynamic.
     This is to say, once a shape is severed from a static body and becomes dynamic, it can never be static again.
     */
    class Anchor : public Drawable {
    public:

        static const size_t DRAWING_BATCH_ID = 0;
        static const size_t LAYER = 1;

        static AnchorRef fromContour(const PolyLine2d &contour) {
            return make_shared<Anchor>(contour);
        }

        static vector <AnchorRef> fromContours(const vector <PolyLine2d> &contours) {
            vector<AnchorRef> anchors;
            anchors.reserve(contours.size());
            for (auto c : contours) {
                anchors.push_back(make_shared<Anchor>(c));
            }
            return anchors;
        }

        static AnchorRef fromShape(const Shape2d &shape);

    public:

        Anchor(const PolyLine2d &contour);

        virtual ~Anchor();

        const PolyLine2d &getContour() const {
            return _contour;
        }

        // Drawable
        cpBB getBB() const override {
            return _bb;
        }

        size_t getDrawingBatchId() const override {
            return DRAWING_BATCH_ID;
        }

        size_t getLayer() const override {
            return LAYER;
        }

        // anchors are static, so always returns identity
        dmat4 getModelMatrix() const override {
            return dmat4(1);
        }
        
        // anchors are static, no rotation
        double getAngle() const override {
            return 0;
        };

        dvec2 getModelCentroid() const override;

        const TriMeshRef &getTriMesh() const override {
            return _trimesh;
        }

        const gl::VboMeshRef &getVboMesh() const override {
            return _vboMesh;
        }

        Color getColor(const core::render_state &state) const override {
            return _material.color;
        }

        bool shouldDraw(const core::render_state &state) const override {
            return true;
        }

    protected:

        friend class World;

        bool build(core::SpaceAccessRef space, material m);

    private:

        cpBody *_staticBody;
        vector<cpShape *> _shapes;

        cpBB _bb;
        material _material;
        PolyLine2d _contour;

        TriMeshRef _trimesh;
        ci::gl::VboMeshRef _vboMesh;

    };

#pragma mark - Shape

    class Shape : public Drawable {
    public:

        static const size_t LAYER = 0;

        struct contour_pair {
            PolyLine2d world, model;

            // when constructed model and world are both equal
            contour_pair(const PolyLine2d &w) :
                    world(w),
                    model(w) {
            }
        };

        static ShapeRef fromContour(const PolyLine2d &outerContour);

        static vector <ShapeRef> fromContours(const vector <PolyLine2d> &contourSoup);

        static vector <ShapeRef> fromShapes(const vector <Shape2d> &shapeSoup);

    public:

        Shape(const PolyLine2d &shapeContour);

        Shape(const PolyLine2d &shapeContour, const std::vector<PolyLine2d> &holeContours);

        virtual ~Shape();

        const contour_pair &getOuterContour() const {
            return _outerContour;
        }

        const vector <contour_pair> &getHoleContours() const {
            return _holeContours;
        }

        GroupBaseRef getGroup() const {
            return _group.lock();
        }

        cpHashValue getGroupHash() const {
            return _groupHash;
        }

        dmat4 getModelMatrix() const override {
            if (GroupBaseRef group = _group.lock()) {
                return group->getModelMatrix();
            } else {
                return dmat4();
            }
        }

        dmat4 getInverseModelMatrix() const {
            if (GroupBaseRef group = _group.lock()) {
                return group->getInverseModelMatrix();
            } else {
                return dmat4();
            }
        }

        const vector<cpShape *> &getShapes(cpBB &shapesModelBB) const {
            shapesModelBB = _shapesModelBB;
            return _shapes;
        }

        bool hasValidTriMesh() const {
            return _trimesh && _trimesh->getNumTriangles() > 0;
        }

        cpBB getBB() const override;

        size_t getDrawingBatchId() const override {
            return _groupDrawingBatchId;
        }

        size_t getLayer() const override {
            return LAYER;
        }

        double getAngle() const override {
            return getGroup()->getAngle();
        }

        dvec2 getModelCentroid() const override {
            return _modelCentroid;
        }

        const TriMeshRef &getTriMesh() const override {
            return _trimesh;
        }

        const gl::VboMeshRef &getVboMesh() const override {
            return _vboMesh;
        }

        Color getColor(const core::render_state &state) const override {
            return getGroup()->getColor(state);
        }

        bool shouldDraw(const core::render_state &state) const override {
            return true;
        }
        
        double getSurfaceArea();

        const unordered_set<poly_edge> &getWorldSpaceContourEdges();

        cpBB getWorldSpaceContourEdgesBB();

        vector <ShapeRef> subtract(const dpolygon2 &polygonToSubtract) const;
        
        // return true iff the point localPoint (in this shape's group's coordinate space) is inside this shape
        bool isLocalPointInside(dvec2 localPoint);

    protected:

        friend class StaticGroup;

        friend class DynamicGroup;

        friend class World;

        void updateWorldSpaceContourAndBB();

        void setGroup(GroupBaseRef group);

        // build the trimesh, returning true iff we got > 0 triangles
        bool triangulate();

        void computeMassAndMoment(double density, double &mass, double &moment, double &area);

        void destroyCollisionShapes();

        const vector<cpShape *> &createCollisionShapes(cpBody *body, cpBB &shapesModelBB, double collisionShapeRadius);

    private:

        bool _worldSpaceShapeContourEdgesDirty;
        contour_pair _outerContour;
        vector <contour_pair> _holeContours;
        dvec2 _modelCentroid;

        cpBB _shapesModelBB;
        vector<cpShape *> _shapes;
        GroupBaseWeakRef _group;
        size_t _groupDrawingBatchId;
        cpHashValue _groupHash;

        unordered_set<poly_edge> _worldSpaceContourEdges;
        cpBB _worldSpaceContourEdgesBB;

        TriMeshRef _trimesh;
        ci::gl::VboMeshRef _vboMesh;
    };
    
} // namespace terrain

#endif /* TerrainWorld_hpp */
