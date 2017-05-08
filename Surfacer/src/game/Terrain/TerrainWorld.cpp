//
//  TerrainWorld.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/1/17.
//
//

#include "TerrainWorld.hpp"

#include <cinder/Log.h>
#include <cinder/Triangulate.h>
#include <cinder/gl/gl.h>
#include <cinder/Rand.h>
#include <cinder/Xml.h>

#include <list>
#include <queue>
#include <functional>

#include "ContourSimplification.hpp"
#include "TerrainDetail.hpp"
#include "SvgParsing.hpp"

namespace core { namespace game { namespace terrain {

#pragma mark - DrawDispatcher

	namespace {

		cpHashValue getCPHashValue(const DrawableRef &d) {
			return reinterpret_cast<cpHashValue>(d->getId());
		}

		cpHashValue getCPHashValue(Drawable *d) {
			return reinterpret_cast<cpHashValue>(d->getId());
		}

		cpBB gameObjectBBFunc( void *obj )
		{
			return static_cast<Drawable*>(obj)->getBB();
		}

		cpCollisionID visibleObjectCollector(void *obj1, void *obj2, cpCollisionID id, void *data) {
			//DrawDispatcher *dispatcher = static_cast<DrawDispatcher*>(obj1);
			DrawableRef drawable = static_cast<Drawable*>(obj2)->shared_from_this<Drawable>();
			DrawDispatcher::collector *collector = static_cast<DrawDispatcher::collector*>(data);

			//
			//	We don't want dupes in sorted vector, so filter based on whether
			//	the shape made it into the set
			//

			if( collector->visible.insert(drawable).second )
			{
				collector->sorted.push_back(drawable);
			}

			return id;
		}

		inline bool visibleDrawableDisplaySorter( const DrawableRef &a, const DrawableRef &b ) {

			const size_t aLayer = a->getLayer();
			const size_t bLayer = b->getLayer();

			if (aLayer != bLayer) {
				return aLayer < bLayer;
			}
			return a->getDrawingBatchId() < b->getDrawingBatchId();

		}

	}

	/*
		cpSpatialIndex *_index;
		set<ShapeRef> _all;
		collector _collector;
		size_t _drawPasses;
	 */
	DrawDispatcher::DrawDispatcher():
	_index(cpBBTreeNew( gameObjectBBFunc, NULL )),
	_drawPasses(1)
	{}

	DrawDispatcher::~DrawDispatcher() {
		cpSpatialIndexFree( _index );
	}

	void DrawDispatcher::add( const DrawableRef &d ) {
		if (_all.insert(d).second) {
			//CI_LOG_D("adding " << d->getId());
			cpSpatialIndexInsert( _index, d.get(), getCPHashValue(d) );
		}
	}

	void DrawDispatcher::remove( const DrawableRef &d ) {
		if (_all.erase(d)) {
			//CI_LOG_D("removing " << d->getId());
			cpSpatialIndexRemove( _index, d.get(), getCPHashValue(d) );
			_collector.remove(d);
		}
	}

	void DrawDispatcher::moved( const DrawableRef &d ) {
		moved(d.get());
	}

	void DrawDispatcher::moved( Drawable *d ) {
		cpSpatialIndexReindexObject( _index, d, getCPHashValue(d));
	}

	void DrawDispatcher::cull( const render_state &state ) {

		//
		//	clear storage - note: std::vector doesn't free, it keeps space reserved.
		//	then let cpSpatialIndex do its magic
		//

		_collector.visible.clear();
		_collector.sorted.clear();

		cpSpatialIndexQuery( _index, this, state.viewport->getFrustum(), visibleObjectCollector, &_collector );

		//
		//	Sort them all
		//

		std::sort(_collector.sorted.begin(), _collector.sorted.end(), visibleDrawableDisplaySorter );
	}

	void DrawDispatcher::draw( const render_state &state, const ci::gl::GlslProgRef &shader ) {
		render_state renderState = state;
		const size_t drawPasses = _drawPasses;
		gl::ScopedGlslProg sglp(shader);

		for( vector<DrawableRef>::iterator it(_collector.sorted.begin()), end(_collector.sorted.end()); it != end; ++it ) {
			vector<DrawableRef>::iterator groupRunIt = it;
			for( renderState.pass = 0; renderState.pass < drawPasses; ++renderState.pass ) {
				groupRunIt = _drawGroupRun(it, end, renderState, shader );
			}

			it = groupRunIt;
		}
	}

	vector<DrawableRef>::iterator
	DrawDispatcher::_drawGroupRun(vector<DrawableRef>::iterator firstInRun, vector<DrawableRef>::iterator storageEnd, const render_state &state, const ci::gl::GlslProgRef &shader ) {

		vector<DrawableRef>::iterator it = firstInRun;
		DrawableRef drawable = *it;
		const size_t batchId = drawable->getDrawingBatchId();

		gl::ScopedModelMatrix smm;
		gl::multModelMatrix(drawable->getModelMatrix());

		for( ; it != storageEnd; ++it )
		{
			//
			//	If the delegate run has completed, clean up after our run
			//	and return the current iterator.
			//

			if ( (*it)->getDrawingBatchId() != batchId )
			{
				return it - 1;
			}

			drawable = *it;
			if (drawable->shouldDraw(state)) {
				shader->uniform("Color", ColorA(drawable->getColor(), 1));
				gl::draw(drawable->getVboMesh());
			}
		}

		return it - 1;
	}


	
#pragma mark - World

	size_t World::_idCounter = 0;

	void World::loadSvg(ci::DataSourceRef svgData, dmat4 transform, vector<ShapeRef> &shapes, vector<AnchorRef> &anchors, vector<ElementRef> &elements, bool flip) {
		shapes.clear();
		anchors.clear();
		elements.clear();

		XmlTree svgNode = XmlTree( svgData ).getChild( "svg" );

		if (flip) {
			Rectd documentFrame = util::svg::parseDocumentFrame(svgNode);
			transform = transform * translate(dvec3(0,documentFrame.getY2(),0)) * glm::scale(dvec3(1,-1,1));
		}

		detail::svg_load(svgNode, transform, shapes, anchors, elements);
	}

	vector<ShapeRef> World::partition(const vector<ShapeRef> &shapes, dvec2 partitionOrigin, double partitionSize) {

		// first compute the march area
		cpBB bounds = cpBBInvalid;
		for (auto shape : shapes) {
			cpBBExpand(bounds, shape->getWorldSpaceContourEdgesBB());
		}

		const int marchLeft = static_cast<int>(floor((bounds.l - partitionOrigin.x) / partitionSize));
		const int marchRight = static_cast<int>(ceil((bounds.r - partitionOrigin.x) / partitionSize));
		const int marchBottom = static_cast<int>(floor((bounds.b - partitionOrigin.y) / partitionSize));
		const int marchTop = static_cast<int>(ceil((bounds.t - partitionOrigin.y) / partitionSize));

		const dmat4 identity(1);
		cpBB quadBB = cpBBInvalid;

		PolyLine2d quad;
		quad.getPoints().resize(4);

		vector<ShapeRef> result;

		for (auto shape : shapes) {
			detail::polygon testPolygon = detail::convertTerrainShapeToBoostGeometry( shape );

			for (int marchY = marchBottom; marchY <= marchTop; marchY++) {

				quadBB.b = marchY * partitionSize;
				quadBB.t = quadBB.b + partitionSize;

				for (int marchX = marchLeft; marchX <= marchRight; marchX++) {
					quadBB.l = marchX * partitionSize;
					quadBB.r = quadBB.l + partitionSize;

					if (cpBBIntersects(quadBB, shape->getWorldSpaceContourEdgesBB())) {
						// generate the test quad
						quad.getPoints()[0] = dvec2(quadBB.l, quadBB.b);
						quad.getPoints()[1] = dvec2(quadBB.l, quadBB.t);
						quad.getPoints()[2] = dvec2(quadBB.r, quadBB.t);
						quad.getPoints()[3] = dvec2(quadBB.r, quadBB.b);

						auto polygonToIntersect = detail::convertPolyLineToBoostGeometry( quad );

						std::vector<detail::polygon> output;
						boost::geometry::intersection( testPolygon, polygonToIntersect, output );

						auto newShapes = detail::convertBoostGeometryToTerrainShapes( output, identity );
						result.insert(result.end(), newShapes.begin(), newShapes.end());
					}
				}
			}
		}

		return result;
	}

	/*
		material _worldMaterial, _anchorMaterial;
		SpaceAccessRef _space;
		StaticGroupRef _staticGroup;
		set<DynamicGroupRef> _dynamicGroups;
		vector<AnchorRef> _anchors;

		DrawDispatcher _drawDispatcher;
		ci::gl::GlslProgRef _shader;
	 */
	
	World::World(SpaceAccessRef space, material worldMaterial, material anchorMaterial):
	_worldMaterial(worldMaterial),
	_anchorMaterial(anchorMaterial),
	_space(space)
	{
		auto vsh = CI_GLSL(150,
						   uniform mat4 ciModelViewProjection;
						   in vec4 ciPosition;

						   void main(void){
							   gl_Position = ciModelViewProjection * ciPosition;
						   }
						   );

		auto fsh = CI_GLSL(150,
						   uniform vec4 Color;
						   out vec4 oColor;

						   void main(void) {
							   oColor = Color;
						   }
						   );


		_shader = gl::GlslProg::create(gl::GlslProg::Format().vertex(vsh).fragment(fsh));
	}

	World::~World() {
		//
		// we want these destructors run before the drawDispatcher is destroyed
		//

		_staticGroup.reset();
		_dynamicGroups.clear();
		_anchors.clear();
	}

	void World::build(const vector<ShapeRef> &shapes, const vector<AnchorRef> &anchors, const vector<ElementRef> &elements) {

		// build the anchors, adding all that triangulated and made physics representations
		const auto self = shared_from_this();
		for (auto anchor : anchors) {
			if (anchor->build(_space, _anchorMaterial)) {
				anchor->setWorld(self);
				_anchors.push_back(anchor);
				_drawDispatcher.add(anchor);
			}
		}

		// add all the elements
		_elements.insert(_elements.end(), elements.begin(), elements.end());
		for (auto element : elements) {
			CI_LOG_D("element: " + element->getId() << " pos: " << (element->getModelMatrix() * element->getModelCentroid()));
			_elementsById[element->getId()] = element;
			_drawDispatcher.add(element);
		}

		// now build
		build(shapes, map<ShapeRef,GroupBaseRef>());
	}

	void World::cut(dvec2 a, dvec2 b, double radius, cpShapeFilter filter) {
		const double MinLength = 1e-2;
		const double MinRadius = 1e-2;

		dvec2 dir = b - a;
		double l = length(dir);

		if (l > MinLength && radius > MinRadius) {
			dir /= l;

			dvec2 right = radius * rotateCW(dir);
			dvec2 left = -right;

			dvec2 ca = a + right;
			dvec2 cb = a + left;
			dvec2 cc = b + left;
			dvec2 cd = b + right;

			PolyLine2d contour;
			contour.push_back(ca);
			contour.push_back(cb);
			contour.push_back(cc);
			contour.push_back(cd);
			contour.setClosed();

			CI_LOG_D("Performing cut from " << a << " to " << b << " radius: " << radius);

			struct cut_collector {
				cpShapeFilter filter;
				set<ShapeRef> shapes;
				set<GroupBaseRef> groups;

				cut_collector(cpShapeFilter f):filter(f){}

				void clear() {
					shapes.clear();
					groups.clear();
				}
			};

			cut_collector collector(filter);

			//
			// while intuition would have us use a segment query, the cpSegmentQuery call seemed to miss
			// literal "corner cases" where a TINY corner of a shape, even if it was WELL INSIDE the segment,
			// would be missed. My guess is segment queries use some sort of stepping algorithm and miss
			// tiny overhangs. So we're building a cpShape explicitly.
			//

			cpVect verts[] = { cpv(cd), cpv(cc), cpv(cb), cpv(ca) };
			cpShape *cuttingShape = cpPolyShapeNew(cpSpaceGetStaticBody(_space->getSpace()), 4, verts, cpTransformIdentity, radius * 2);
			cpSpaceShapeQuery(_space->getSpace(), cuttingShape, [](cpShape *collisionShape, cpContactPointSet *points, void *data){
				cut_collector *collector = static_cast<cut_collector*>(data);
				if (!cpShapeFilterReject(collector->filter, cpShapeGetFilter(collisionShape))) {
					Shape *terrainShapePtr = static_cast<Shape*>(cpShapeGetUserData(collisionShape));
					ShapeRef terrainShape = terrainShapePtr->shared_from_this<Shape>();
					collector->shapes.insert(terrainShape);
					collector->groups.insert(terrainShape->getGroup());
				}
			}, &collector);

			cpShapeFree(cuttingShape);

			CI_LOG_D("Collected " << collector.shapes.size() << " shapes (" << collector.groups.size() << " groups) to cut" );

			//
			// Collect all shapes which are in groups affected by the cut
			//

			vector<ShapeRef> affectedShapes;
			for (auto &group : collector.groups) {
				for (auto &shape : group->getShapes()) {
					if (collector.shapes.find(shape) == collector.shapes.end()) {
						affectedShapes.push_back(shape);
					}
				}
			}

			//
			//	Perform cut, adding results to affectedShapes and assigning parentage
			//	so we can apply lin/ang vel to new bodies
			//

			map<ShapeRef,GroupBaseRef> parentage;
			for (auto &shapeToCut : collector.shapes) {
				GroupBaseRef parentGroup = shapeToCut->getGroup();

				auto result = shapeToCut->subtract(contour);
				affectedShapes.insert(end(affectedShapes), begin(result), end(result));

				//
				//	Update parentage map for use in build() - maps a shape to its previous parent group
				//

				for (auto &newShape : result) {
					parentage[newShape] = parentGroup;
				}

				//
				//	Release the parent group's shapes. we do this because the group's
				//	destructors would remove the shapes from the draw dispatcher, and that
				//	would run right after build() completes, which would result in active shapes
				//	not being in the draw dispatcher!
				//

				parentGroup->releaseShapes();

				//
				// if the shape belonged to the static group, just remove it
				// otherwise, remove the shape's dynamic parent group because
				// we'll be rebuilding it completely in build()
				//

				if (parentGroup == _staticGroup) {
					_staticGroup->removeShape(shapeToCut);
				} else {
					_dynamicGroups.erase(dynamic_pointer_cast<DynamicGroup>(parentGroup));
				}
			}

			// let go of strong references
			collector.clear();

			build(affectedShapes, parentage);

		} else {
			CI_LOG_E("Either length and/or radius were below minimum thresholds");
		}
	}
	
	void World::draw(const render_state &renderState) {

		_drawDispatcher.cull(renderState);
		_drawDispatcher.draw(renderState, _shader);

		//
		//	In dev mode draw BBs and drawable IDs
		//

		if (renderState.mode == RenderMode::DEVELOPMENT) {
			const double rScale = renderState.viewport->getReciprocalScale();
			const dmat4 rScaleMat = glm::scale(dvec3(rScale, -rScale, 1));
			const ColorA bbColor(1,0.2,1,0.5);

			gl::lineWidth(1);
			for (const auto &drawable : _drawDispatcher.getVisibleSorted()) {

				// draw the bounds
				const auto bb = drawable->getBB();
				gl::color(bbColor);
				gl::drawStrokedRect(Rectf(bb.l, bb.b, bb.r, bb.t));

				// draw the shape id
				const auto R = drawable->getModelMatrix();
				const double angle = drawable->getAngle();
				const dvec3 modelCentroid = dvec3(drawable->getModelCentroid(), 0);

				gl::ScopedModelMatrix smm2;
				gl::multModelMatrix(R * glm::translate(modelCentroid) * glm::rotate(-angle, dvec3(0,0,1)) * rScaleMat);
				gl::drawString(str(drawable->getId()), dvec2(0,0), Color(1,1,1));
			}
		}
	}

	void World::step(const time_state &timeState) {

		if (_staticGroup) {
			_staticGroup->step(timeState);
		}

		for (auto &body : _dynamicGroups) {
			body->step(timeState);
		}
	}

	void World::update(const time_state &timeState) {

		if (_staticGroup) {
			_staticGroup->update(timeState);
		}

		for (auto &body : _dynamicGroups) {
			body->update(timeState);
		}
	}

	ElementRef World::getElementById(string id) const {
		auto pos = _elementsById.find(id);
		if (pos != _elementsById.end()) {
			return pos->second;
		}
		return nullptr;
	}

	void World::setGameObject(GameObjectRef gameObject) {
		_gameObject = gameObject;
	}

	GameObjectRef World::getGameObject() const {
		return _gameObject.lock();
	}

	void World::build(const vector<ShapeRef> &affectedShapes, const map<ShapeRef,GroupBaseRef> &parentage) {

		const auto self = shared_from_this();

		//
		//	Build the static group if we haven't already
		//

		if (!_staticGroup) {
			_staticGroup = make_shared<StaticGroup>(self, _worldMaterial, _drawDispatcher);
		}

		//
		//	Assign self for IChipmunkUserData lookup
		//

		for (const auto &shape : affectedShapes) {
			shape->setWorld(self);
		}

		//
		// given the new shapes, their parentage, and existing shapes, find the groups they make up
		//

		auto shapeGroups = findShapeGroups(affectedShapes, parentage);

		for (const auto &shapeGroup : shapeGroups) {

			// find parent
			GroupBaseRef parentGroup;
			if (!parentage.empty()) {
				for (const auto &shape : shapeGroup) {
					auto pos = parentage.find(shape);
					if (pos != parentage.end()) {
						parentGroup = pos->second;
						break;
					}
				}
			}

			if (isShapeGroupStatic(shapeGroup, parentGroup)) {

				//
				//	 Add these shapes to the singleton static group
				//

				for (const auto &shape : shapeGroup) {
					_staticGroup->addShape(shape);
				}
			} else {

				//
				//	This cut promoted static shapes to dynamic - got to remove them from _staticGroup
				//

				if (parentGroup == _staticGroup) {
					for (const auto &shape : shapeGroup) {
						_staticGroup->removeShape(shape);
					}
				}

				//
				//	Build a dynamic group
				//


				DynamicGroupRef group = make_shared<DynamicGroup>(shared_from_this(), _worldMaterial, _drawDispatcher);
				if (group->build(shapeGroup, parentGroup)) {
					_dynamicGroups.insert(group);
				}
			}
		}
	}

	vector<set<ShapeRef>> World::findShapeGroups(const vector<ShapeRef> &affectedShapes, const map<ShapeRef,GroupBaseRef> &parentage) {

		//
		// for each shape in affectedShapes, use floodfill to find connected shapes.
		// these neighbors all go into a new group, and get removed from the affectedShapes set
		// note: A singleton shape (no neighbors) still becomes a group
		//

		// TODO: Speed up findGroup with a spatial index when the search space > ~400

		set<ShapeRef> affectedShapesSet(affectedShapes.begin(), affectedShapes.end());
		vector<set<ShapeRef>> shapeGroups;

		while(!affectedShapesSet.empty()) {
			ShapeRef shape = *affectedShapesSet.begin(); // grab one

			set<ShapeRef> group = detail::findGroup(shape, affectedShapesSet, parentage);
			for (auto groupedShape : group) {
				affectedShapesSet.erase(groupedShape);
			}

			shapeGroups.push_back(group);
		}

		return shapeGroups;
	}

	bool World::isShapeGroupStatic(const set<ShapeRef> shapeGroup, const GroupBaseRef &parentGroup) {

		// the static-dynamic rule is this: a body may check to see if its static
		// if has NOT been dynamic in previous parentage

		if (!parentGroup || parentGroup == _staticGroup) {
			for (auto &shape : shapeGroup) {
				for (auto &anchor : _anchors) {
					if (cpBBIntersects(anchor->getBB(), shape->getWorldSpaceContourEdgesBB())) {

						const PolyLine2d &shapeContour = shape->getOuterContour().world;
						const PolyLine2d &anchorContour = anchor->getContour();

						//
						// check if the anchor overlaps the shape's outer contour
						//

						for (auto p : anchorContour.getPoints()) {
							if (shapeContour.contains(p)) {
								return true;
							}
						}

						for (auto p : shapeContour.getPoints()) {
							if (anchorContour.contains(p)) {
								return true;
							}
						}
					}
				}
			}
		}

		return false;
	}

#pragma mark - GroupBase

	/*
		DrawDispatcher &_drawDispatcher;
		size_t _drawingBatchId;
		WorldWeakRef _world;
		SpaceAccessRef _space;
		material _material;
		string _name;
		Color _color;
		cpHashValue _hash;
	 */

	GroupBase::GroupBase(WorldRef world, material m, DrawDispatcher &dispatcher):
	_drawDispatcher(dispatcher),
	_drawingBatchId(World::nextId()),
	_world(world),
	_space(world->getSpace()),
	_material(m)
	{}

	GroupBase::~GroupBase(){}

	WorldRef GroupBase::getWorld() const {
		return _world.lock();
	}


#pragma mark - StaticGroup

	/*
		cpBody *_body;
		set<ShapeRef> _shapes;
		mutable cpBB _worldBB;
	 */

	StaticGroup::StaticGroup(WorldRef world, material m, DrawDispatcher &dispatcher):
	terrain::GroupBase(world, m, dispatcher),
	_body(nullptr),
	_worldBB(cpBBInvalid) {
		_name = "StaticGroup";
		_color = Color(0.15,0.15,0.15);

		_body = cpBodyNewStatic();
		cpBodySetUserData(_body, this);
		_space->addBody(_body);
	}
	
	StaticGroup::~StaticGroup() {
		releaseShapes();
		cpCleanupAndFree(_body);
	}

	cpBB StaticGroup::getBB() const {
		if (!cpBBIsValid(_worldBB)) {
			_worldBB = cpBBInvalid;
			for (auto shape : _shapes) {
				cpBBExpand(_worldBB, shape->getWorldSpaceContourEdgesBB());
			}
		}

		return _worldBB;
	}

	void StaticGroup::releaseShapes() {
		// remove shapes from draw dispatcher
		for (auto &shape : _shapes) {
			_drawDispatcher.remove(shape);
		}
		_shapes.clear();
	}

	void StaticGroup::addShape(ShapeRef shape) {

		if (!shape->hasValidTriMesh()) {
			if (!shape->triangulate()) {
				return;
			}
		}

		if (shape->hasValidTriMesh()) {

			shape->_modelCentroid = shape->_outerContour.model.calcCentroid();

			double area = shape->computeArea();
			if (area >= detail::MIN_SHAPE_AREA) {

				shape->setGroup(shared_from_this());
				_shapes.insert(shape);

				//
				//	Create new collision shapes if needed. Note, since static shapes can only stay static or become dynamic
				// but never can go from dynamic to static, we can trust that the shapes are in world space here. As such,
				// we can re-use existing collision shapes, etc.
				//

				cpBB modelBB = cpBBInvalid;
				vector<cpShape*> collisionShapes = shape->getShapes(modelBB);
				bool didCreateNewShapes = false;
				if (collisionShapes.empty()) {
					collisionShapes = shape->createCollisionShapes(_body, modelBB);
					didCreateNewShapes = true;
				}

				//
				//	Register this shape with the world's draw dispatcher. Note, we do this AFTER
				//	calling getShapes/createCollisionShapes because adding a shape requires the BB
				//	to be valid, and it's not computed until the collision geometry is created.
				//

				getDrawDispatcher().add(shape);


				if (!collisionShapes.empty() && cpBBIsValid(modelBB)) {
					cpBBExpand(_worldBB, modelBB);

					if (didCreateNewShapes) {
						for (cpShape *collisionShape : collisionShapes) {
							cpShapeSetFilter(collisionShape, _material.filter);
							cpShapeSetCollisionType(collisionShape, _material.collisionType);
							cpShapeSetFriction(collisionShape, _material.friction);
							_space->addShape(collisionShape);
						}
					}

				} else {
					cpCleanupAndFree(collisionShapes);
					_shapes.erase(shape);
				}
			}
		}
	}

	void StaticGroup::removeShape(ShapeRef shape) {
		if (_shapes.erase(shape)) {
			shape->setGroup(nullptr);
			_worldBB = cpBBInvalid;

			//
			//	Unregister this shape with the world's draw dispatcher
			//

			getDrawDispatcher().remove(shape);
		}
	}


#pragma mark - DynamicGroup

	/*
		static size_t _count;

		bool _dynamic;
		material _material;
		SpaceAccessRef _space;
		cpBody *_body;
		cpVect _position;
		cpFloat _angle;
		cpBB _worldBB, _modelBB;
		dmat4 _modelMatrix, _inverseModelMatrix;

		set<ShapeRef> _shapes;

		string _name;
		cpHashValue _hash;
		Color _color;
	 */

	DynamicGroup::DynamicGroup(WorldRef world, material m, DrawDispatcher &dispatcher):
	GroupBase(world, m, dispatcher),
	_body(nullptr),
	_position(cpv(0,0)),
	_angle(0),
	_worldBB(cpBBInvalid),
	_modelBB(cpBBInvalid),
	_modelMatrix(1),
	_inverseModelMatrix(1) {
		_name = str(World::nextId());
		_hash = hash<string>{}(_name);
		_color = detail::next_random_color();
	}

	DynamicGroup::~DynamicGroup() {
		releaseShapes();
		cpCleanupAndFree(_body);
	}

	string DynamicGroup::getName() const {
		stringstream ss;
		ss << "Body[" << _name << "](";
		for (auto &shape : _shapes) {
			ss << shape->getId() << ":";
		}
		ss << ")";
		return ss.str();
	}

	void DynamicGroup::releaseShapes() {
		// remove shapes from draw dispatcher
		for (auto &shape : getShapes()) {
			_drawDispatcher.remove(shape);
		}
		_shapes.clear();
	}

	void DynamicGroup::draw(const render_state &renderState) {
		if (renderState.mode == RenderMode::DEVELOPMENT) {
			const double rScale = renderState.viewport->getReciprocalScale();

			gl::ScopedModelMatrix smm2;
			gl::multModelMatrix(translate(dvec3(getPosition() + dvec2(0,20),0)) * scale(dvec3(rScale, -rScale, 1)));

			gl::drawString(getName(), dvec2(0,0), Color(1,1,1));
		}
	}

	void DynamicGroup::step(const time_state &timeState) {
		syncToCpBody();
	}

	void DynamicGroup::update(const time_state &timeState) {
	}

	bool DynamicGroup::build(set<ShapeRef> shapes, const GroupBaseRef &parentGroup) {

		set<ShapeRef> garbage;
		auto emptyGarbage = [&garbage, &shapes]() {
			for (auto &s : garbage) {
				s->setGroup(nullptr);
				shapes.erase(s);
			}
			garbage.clear();
		};

		_modelBB = cpBBInvalid;

		// PRECONDITIONS: It is assumed that each shape's contour model forms are in world space.
		// This is already true for any new shape, but any shape re-attached or reparented needs
		// to have its world contours updated from the model and transform, and then reassigned to model
		for (auto &shape : shapes) {
			// check if this object has a parent (and that the parent isn't us)
			GroupBaseRef body = shape->getGroup();
			if (body && body.get() != this) {

				const dmat4 T = shape->getModelMatrix();
				detail::transform(shape->_outerContour.model, shape->_outerContour.world, T);
				shape->_outerContour.model = shape->_outerContour.world;

				for (auto &holeContour : shape->_holeContours) {
					detail::transform(holeContour.model, holeContour.world, T);
					holeContour.model = holeContour.world;
				}
			}

			// assign parentage AFTER accessing shape's modelview
			shape->setGroup(shared_from_this());
		}

		//
		// 1) first pass, gather up all shapes for attached bodies and compute common centroid, then apply a model space transform
		// 2) compute group total for areas, masses, moments, etc
		// 3) create the body
		// 4) then transform each shape's contours to local space, attach colliders, etc
		//


		//
		// first compute the averaged centroid
		//

		dvec2 averageModelSpaceCentroid(0,0);
		const double averagingScale = 1.0 / shapes.size();
		for (auto &shape : shapes) {
			shape->_modelCentroid = shape->_outerContour.model.calcCentroid();
			averageModelSpaceCentroid += dvec2(shape->_modelCentroid) * averagingScale;
		}

		//
		// move contours from their own private model space to shared model space
		//

		for (auto &shape : shapes) {
			shape->_modelCentroid -= averageModelSpaceCentroid;
			shape->_outerContour.model.offset(-averageModelSpaceCentroid);
			for (auto &hole : shape->_holeContours) {
				hole.model.offset(-averageModelSpaceCentroid);
			}
		}

		_modelMatrix = _modelMatrix * translate(dvec3(averageModelSpaceCentroid.x, averageModelSpaceCentroid.y, 0));
		_inverseModelMatrix = inverse(_modelMatrix);

		//
		//	Triangulate - any which fail should be collected to garbage
		//

		for (auto &shape : shapes) {
			if (!shape->triangulate()) {
				garbage.insert(shape);
			}
		}
		
		emptyGarbage();

		//
		//	Compute collective mass and moment
		//

		double totalArea = 0, totalMass = 0, totalMoment = 0;
		for (auto &shape : shapes) {
			double area = 0, mass = 0, moment = 0;
			shape->computeMassAndMoment(_material.density, mass, moment, area);
			totalArea += area;
			totalMass += mass;
			totalMoment += moment;
		}


		if (totalArea > detail::MIN_SHAPE_AREA) {

			_body = cpBodyNew(totalMass, totalMoment);
			cpBodySetUserData(_body, this);

			// glm::dmat4 is column major
			dvec2 position;
			position.x = _modelMatrix[3][0];
			position.y = _modelMatrix[3][1];
			cpBodySetPosition(_body, cpv(position));

			for (auto &shape : shapes) {
				cpBB modelBB = cpBBInvalid;

				// destroy any lingering collision shapes from previous tessellations and build new shapes

				shape->destroyCollisionShapes();
				vector<cpShape*> collisionShapes = shape->createCollisionShapes(_body, modelBB);

				if (!collisionShapes.empty() && cpBBIsValid(modelBB)) {
					cpBBExpand(_modelBB, modelBB);

					for (cpShape *collisionShape : collisionShapes) {
						cpShapeSetFilter(collisionShape, _material.filter);
						cpShapeSetCollisionType(collisionShape, _material.collisionType);
						cpShapeSetFriction(collisionShape, _material.friction);
						_space->addShape(collisionShape);
					}
				} else {
					garbage.insert(shape);
				}
			}

			emptyGarbage();

			if (!shapes.empty()) {

				//
				//	We're good to go
				//

				_shapes = shapes;

				//
				//	Add the shapes to the draw dispatcher
				//

				DrawDispatcher &drawDispatcher = getDrawDispatcher();
				for (const auto &shape : _shapes) {
					drawDispatcher.add(shape);
				}

				_space->addBody(_body);

				syncToCpBody();

				if (parentGroup && cpBodyGetType(parentGroup->getBody()) == CP_BODY_TYPE_DYNAMIC) {

					//
					//	Compute world linearvel and angularvel for parent and apply to self
					//

					const double angularVel = cpBodyGetAngularVelocity(parentGroup->getBody());
					const cpVect linearVel = cpBodyGetVelocityAtWorldPoint(parentGroup->getBody(), cpv(getPosition()));
					cpBodySetVelocity(_body, linearVel);
					cpBodySetAngularVelocity(_body, angularVel);
				}

				return true;
			}
		}

		//
		//	We didn't have enough total area, or none of our shapes created collision shapes
		//

		return false;
	}

	void DynamicGroup::syncToCpBody() {
		// extract position and rotation from body and apply to _modelMatrix
		cpVect position = cpBodyGetPosition(_body);
		cpVect rotation = cpBodyGetRotation(_body);
		cpFloat angle = cpBodyGetAngle(_body);

		// determine if we moved/rotated since last step
		const cpFloat Epsilon = 1e-3;
		bool moved = (cpvlengthsq(cpvsub(position, _position)) > Epsilon || abs(angle - _angle) > Epsilon);
		_position = position;
		_angle = angle;

		// dmat4 - column major, each column is a vec4
		_modelMatrix = dmat4(
			vec4(rotation.x, rotation.y, 0, 0),
			vec4(-rotation.y, rotation.x, 0, 0),
			vec4(0,0,1,0),
			vec4(position.x, position.y, 0, 1));

		_inverseModelMatrix = inverse(_modelMatrix);

		// update our world BB
		_worldBB = cpTransformbBB(getModelTransform(), _modelBB);

		//
		//	If this shape moved we need to mark our edges as dirty (so world space edges and bounds
		//	can be correctly recomputed, and we need to notify the draw dispatcher
		//
		if (moved) {
			DrawDispatcher &drawDispatcher = getDrawDispatcher();
			for (auto &shape : _shapes) {
				shape->_worldSpaceShapeContourEdgesDirty = true;
				drawDispatcher.moved(shape);
			}
		}
	}

#pragma mark - Drawable

	/*
		size_t _id;
		WorldWeakRef _world;
	 */

	Drawable::Drawable():
	_id(World::nextId())
	{}

	Drawable::~Drawable(){}

	void Drawable::setWorld(WorldRef w) {
		_world = w;
	}

	WorldRef Drawable::getWorld() const {
		return _world.lock();
	}

	// IChipmunkUserData
	GameObjectRef Drawable::getGameObject() const {
		return _world.lock()->getGameObject();
	}


#pragma mark - Element

	/*
		cpBB _bb;
		string _id;
		TriMeshRef _trimesh;
		ci::gl::VboMeshRef _vboMesh;
	 */
	Element::Element(string id, const PolyLine2d &contour):
	_bb(cpBBInvalid),
	_id(id) {

		for (auto v : contour) {
			cpBBExpand(_bb, v);
		}

		Triangulator triangulator;
		triangulator.addPolyLine(detail::polyline2d_to_2f(contour));
		_trimesh = triangulator.createMesh();
		if (_trimesh->getNumTriangles() > 0) {
			_vboMesh = gl::VboMesh::create(*_trimesh);
		}
	}
	Element::~Element() {}

	dvec2 Element::getPosition() const {
		return getModelCentroid();
	}

	dvec2 Element::getModelCentroid() const {
		return dvec2((_bb.l + _bb.r) * 0.5, (_bb.b + _bb.t) * 0.5);
	}

	bool Element::shouldDraw(const render_state &state) const {
		return state.mode == RenderMode::DEVELOPMENT;
	}



#pragma mark - Anchor

	AnchorRef Anchor::fromShape(const Shape2d &shape) {
		PolyLine2d contour;
		for (const dvec2 v : shape.getContour(0).subdivide()) {
			contour.push_back(v);
		}

		return fromContour(contour);
	}

	/*
		cpBody *_staticBody;
		vector<cpShape*> _shapes;

		cpBB _bb;
		material _material;
		PolyLine2d _contour;

		TriMeshRef _trimesh;
		ci::gl::VboMeshRef _vboMesh;
	 */

	Anchor::Anchor(const PolyLine2d &contour):
	_staticBody(nullptr),
	_bb(cpBBInvalid),
	_contour(contour) {

		for (auto &p : _contour.getPoints()) {
			cpBBExpand(_bb, p);
		}

		Triangulator triangulator;
		triangulator.addPolyLine(detail::polyline2d_to_2f(_contour));
		_trimesh = triangulator.createMesh();
		if (_trimesh->getNumTriangles() > 0) {
			_vboMesh = gl::VboMesh::create(*_trimesh);
		}
	}

	Anchor::~Anchor() {
		for (auto s : _shapes) {
			cpCleanupAndFree(s);
		}
		_shapes.clear();
		cpCleanupAndFree(_staticBody);
	}

	dvec2 Anchor::getModelCentroid() const {
		return dvec2((_bb.l + _bb.r) * 0.5, (_bb.b + _bb.t) * 0.5);
	}

	bool Anchor::build(SpaceAccessRef space, material m) {
		assert(_shapes.empty());

		_material = m;
		_staticBody = cpBodyNewStatic();

		cpVect triangle[3];
		for (size_t i = 0, N = _trimesh->getNumTriangles(); i < N; i++) {
			vec2 a,b,c;
			_trimesh->getTriangleVertices(i, &a, &b, &c);
			triangle[0] = cpv(a);
			triangle[1] = cpv(b);
			triangle[2] = cpv(c);

			double area = cpAreaForPoly(3, triangle, 0);
			if (area < 0) {
				triangle[0] = cpv(c);
				triangle[1] = cpv(b);
				triangle[2] = cpv(a);
				area = cpAreaForPoly(3, triangle, 0);
			}

			if (area >= detail::MIN_TRIANGLE_AREA) {
				_shapes.push_back(cpPolyShapeNew(_staticBody, 3, triangle, cpTransformIdentity, 0));
			}
		}

		if (!_shapes.empty()) {

			//
			// looks like we got some valid collision hulls, set them up
			//

			space->addBody(_staticBody);
			for (auto shape : _shapes) {
				space->addShape(shape);
				cpShapeSetUserData(shape, this);
				cpShapeSetFilter(shape, _material.filter);
				cpShapeSetCollisionType(shape, _material.collisionType);
				cpShapeSetFriction(shape, _material.friction);
			}

			return true;
		}

		return false;
	}

	

#pragma mark - Shape

	ShapeRef Shape::fromContour(const PolyLine2d &outerContour) {
		return make_shared<Shape>(detail::optimize(outerContour));
	}

	vector<ShapeRef> Shape::fromContours(const vector<PolyLine2d> &contourSoup) {

		const auto optimizedContourSoup = detail::optimize(contourSoup);
		vector<detail::contour_tree_node_ref> rootNodes = detail::build_contour_tree(optimizedContourSoup);

		vector<ShapeRef> shapes;
		for (auto &rn : rootNodes) {
			vector<ShapeRef> built = build_from_contour_tree(rn, 0);
			shapes.insert(end(shapes), begin(built), end(built));
		}

		return shapes;
	}

	vector<ShapeRef> Shape::fromShapes(const vector<Shape2d> &shapeSoup) {
		vector<PolyLine2d> contours;
		for (auto &shape : shapeSoup) {
			for (auto &path : shape.getContours()) {

				// path.subdivide returns a vector<dvec2> which isn't convertible to vector<dvec2>
				PolyLine2d contour;
				for (const dvec2 v : path.subdivide()) {
					contour.push_back(v);
				}

				contours.push_back(contour);
			}
		}

		return fromContours(contours);
	}

	/*
		bool _worldSpaceShapeContourEdgesDirty;
		contour_pair _outerContour;
		vector<contour_pair> _holeContours;
		dvec2 _modelCentroid;

		cpBB _shapesModelBB;
		vector<cpShape*> _shapes;
		GroupBaseWeakRef _group;
		size_t _groupDrawingBatchId;
		cpHashValue _groupHash;

		unordered_set<poly_edge> _worldSpaceContourEdges;
		cpBB _worldSpaceContourEdgesBB;

		TriMeshRef _trimesh;
		ci::gl::VboMeshRef _vboMesh;	 */


	Shape::Shape(const PolyLine2d &sc):
	_worldSpaceShapeContourEdgesDirty(true),
	_outerContour(sc),
	_modelCentroid(0,0),
	_groupDrawingBatchId(0),
	_groupHash(0),
	_worldSpaceContourEdgesBB(cpBBInvalid) {
		detail::wind_clockwise(_outerContour.world);
	}

	Shape::Shape(const PolyLine2d &sc, const std::vector<PolyLine2d> &hcs):
	_worldSpaceShapeContourEdgesDirty(true),
	_outerContour(sc),
	_modelCentroid(0,0),
	_groupDrawingBatchId(0),
	_groupHash(0),
	_worldSpaceContourEdgesBB(cpBBInvalid) {

		for (const auto hc : hcs) {
			_holeContours.emplace_back(hc);
		}

		detail::wind_clockwise(_outerContour.world);
		for (auto &hc : _holeContours) {
			detail::wind_counter_clockwise(hc.world);
		}
	}

	Shape::~Shape() {
		destroyCollisionShapes();
		_group.reset();
	}

	const unordered_set<poly_edge> &Shape::getWorldSpaceContourEdges() {
		updateWorldSpaceContourAndBB();
		return _worldSpaceContourEdges;
	}

	cpBB Shape::getBB() const {
		return cpBBTransform(_shapesModelBB, getModelMatrix());
	}

	cpBB Shape::getWorldSpaceContourEdgesBB() {
		updateWorldSpaceContourAndBB();
		return _worldSpaceContourEdgesBB;
	}

	vector<ShapeRef> Shape::subtract(const PolyLine2d &contourToSubtract) const {
		if (!contourToSubtract.getPoints().empty()) {

			//
			// move the shape to subtract from world space to the our model space, and convert to a boost poly
			//

			PolyLine2d transformedShapeToSubtract = detail::transformed(contourToSubtract, getInverseModelMatrix());
			detail::polygon polyToSubtract = detail::convertPolyLineToBoostGeometry( transformedShapeToSubtract );

			//
			// convert self (outerContour & holeContours) to a boost poly in model space
			//

			detail::polygon thisPoly = detail::convertTerrainShapeToBoostGeometry( const_cast<Shape*>(this)->shared_from_this<Shape>() );

			//
			// now subtract - results are in model space
			//

			std::vector<detail::polygon> output;
			boost::geometry::difference( thisPoly, polyToSubtract, output );

			//
			// convert output to Shapes - they need to have our modelview applied to them to move them back to world space
			//

			vector<ShapeRef> newShapes = detail::convertBoostGeometryToTerrainShapes( output, getModelMatrix() );

			if (!newShapes.empty()) {
				return newShapes;
			}
		}

		// handle failure case
		vector<ShapeRef> result = { const_cast<Shape*>(this)->shared_from_this<Shape>() };
		return result;
	}

	void Shape::updateWorldSpaceContourAndBB() {
		if (_worldSpaceShapeContourEdgesDirty) {

			assert(_outerContour.model.size() > 1);

			const dmat4 modelMatrix = getModelMatrix();
			dvec2 worldPoint = modelMatrix * _outerContour.model.getPoints().front();

			_worldSpaceContourEdges.clear();
			cpBB bb = cpBBInvalid;

			// the shapeContour is in model coordinates - we need to move each vertex to world
			for (auto current = _outerContour.model.begin(), next = _outerContour.model.begin()+1, end = _outerContour.model.end(); current != end; ++current, ++next) {
				if (next == end) {
					next = _outerContour.model.begin();
				}
				const dvec2 worldNext = modelMatrix * (*next);
				_worldSpaceContourEdges.insert(poly_edge(worldPoint, worldNext));
				worldPoint = worldNext;
				cpBBExpand(bb, worldPoint);
			}

			_worldSpaceContourEdgesBB = bb;
			_worldSpaceShapeContourEdgesDirty = false;
		}
	}

	void Shape::setGroup(GroupBaseRef group) {
		_group = group;
		if (group) {
			_groupDrawingBatchId = group->getDrawingBatchId();
			_groupHash = group->getHash();
		} else {
			_groupDrawingBatchId = 0;
			_groupHash = 0;
		}
	}

	bool Shape::triangulate() {

		//
		// note that when a shape is static, its world and model contours are the same
		//

		Triangulator triangulator;
		triangulator.addPolyLine(detail::polyline2d_to_2f(_outerContour.model));

		for (auto holeContour : _holeContours) {
			triangulator.addPolyLine(detail::polyline2d_to_2f(holeContour.model));
		}

		_trimesh = triangulator.createMesh();
		const size_t numTriangles = _trimesh->getNumTriangles();

		if (numTriangles > 0) {
			_vboMesh = gl::VboMesh::create(*_trimesh);
			return true;
		}

		return false;
	}

	double Shape::computeArea() {
		double area = 0;
		cpVect triangle[3];

		for (size_t i = 0, N = _trimesh->getNumTriangles(); i < N; i++) {
			vec2 a,b,c;
			_trimesh->getTriangleVertices(i, &a, &b, &c);

			// find the winding with positive area
			triangle[0] = cpv(a);
			triangle[1] = cpv(b);
			triangle[2] = cpv(c);
			double triArea = length(cross(dvec3(b-a,0),dvec3(c-a,0))) * 0.5;

			if (triArea < 0) {

				//
				// sanity check. this doesn't seem to ever actually happen
				// but it's no skin off my back to be prepared
				//

				triangle[0] = cpv(c);
				triangle[1] = cpv(b);
				triangle[2] = cpv(a);
				triArea = -triArea;
			}

			area += triArea;
		}

		return area;
	}

	void Shape::computeMassAndMoment(double density, double &mass, double &moment, double &area) {
		mass = 0;
		moment = 0;
		area = 0;

		cpVect triangle[3];
		for (size_t i = 0, N = _trimesh->getNumTriangles(); i < N; i++) {
			vec2 a,b,c;
			_trimesh->getTriangleVertices(i, &a, &b, &c);

			// find the winding with positive area
			triangle[0] = cpv(a);
			triangle[1] = cpv(b);
			triangle[2] = cpv(c);
			double triArea = length(cross(dvec3(b-a,0),dvec3(c-a,0))) * 0.5;

			if (triArea < 0) {

				//
				// sanity check. this doesn't seem to ever actually happen
				// but it's no skin off my back to be prepared
				//

				triangle[0] = cpv(c);
				triangle[1] = cpv(b);
				triangle[2] = cpv(a);
				triArea = -triArea;
			}

			const double polyMass = abs(density * triArea);

			if (polyMass > 1e-3) {
				const double polyMoment = cpMomentForPoly(polyMass, 3, triangle, cpvzero, 0);
				if (!isnan(polyMoment)) {
					mass += polyMass;
					moment += abs(polyMoment);
					area += abs(triArea);
				}
			}
		}
	}

	void Shape::destroyCollisionShapes() {
		for (auto shape : _shapes) {
			cpCleanupAndFree(shape);
		}
		_shapes.clear();
	}

	const vector<cpShape*> &Shape::createCollisionShapes(cpBody *body, cpBB &modelBB) {
		assert(_shapes.empty());

		cpBB bb = cpBBInvalid;
		cpVect triangle[3];

		for (size_t i = 0, N = _trimesh->getNumTriangles(); i < N; i++) {
			vec2 a,b,c;
			_trimesh->getTriangleVertices(i, &a, &b, &c);
			triangle[0] = cpv(a);
			triangle[1] = cpv(b);
			triangle[2] = cpv(c);

			double area = cpAreaForPoly(3, triangle, 0);
			if (area < 0) {
				triangle[0] = cpv(c);
				triangle[1] = cpv(b);
				triangle[2] = cpv(a);
				area = cpAreaForPoly(3, triangle, 0);
			}

			if (area >= detail::MIN_TRIANGLE_AREA) {
				cpShape *polyShape = cpPolyShapeNew(body, 3, triangle, cpTransformIdentity, detail::COLLISION_SHAPE_RADIUS);
				cpShapeSetUserData(polyShape, this);
				_shapes.push_back(polyShape);

				// a,b, & c are in model space
				cpBBExpand(bb, a);
				cpBBExpand(bb, b);
				cpBBExpand(bb, c);
			}
		}

		modelBB = _shapesModelBB = bb;
		return _shapes;
	}
	

}}} // end namespace core::game::terrain
