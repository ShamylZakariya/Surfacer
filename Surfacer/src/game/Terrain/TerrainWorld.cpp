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

#include <list>
#include <queue>
#include <functional>

#include "ContourSimplification.hpp"
#include "TerrainDetail.hpp"

using namespace core;

namespace terrain {
	
#pragma mark - World

	vector<ShapeRef> World::partition(const vector<ShapeRef> &shapes, vec2 partitionOrigin, float partitionSize) {

		// first compute the march area
		cpBB bounds = cpBBInvalid;
		for (auto shape : shapes) {
			cpBBExpand(bounds, shape->getWorldSpaceContourEdgesBB());
		}

		const int marchLeft = static_cast<int>(floor((bounds.l - partitionOrigin.x) / partitionSize));
		const int marchRight = static_cast<int>(ceil((bounds.r - partitionOrigin.x) / partitionSize));
		const int marchBottom = static_cast<int>(floor((bounds.b - partitionOrigin.y) / partitionSize));
		const int marchTop = static_cast<int>(ceil((bounds.t - partitionOrigin.y) / partitionSize));

		const mat4 identity(1);
		cpBB quadBB = cpBBInvalid;

		PolyLine2f quad;
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
						quad.getPoints()[0] = vec2(quadBB.l, quadBB.b);
						quad.getPoints()[1] = vec2(quadBB.l, quadBB.t);
						quad.getPoints()[2] = vec2(quadBB.r, quadBB.t);
						quad.getPoints()[3] = vec2(quadBB.r, quadBB.b);

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
		material _material;
		cpSpace *_space;
		StaticGroupRef _staticGroup;
		set<DynamicGroupRef> _dynamicGroups;
		vector<AnchorRef> _anchors;
	 */
	
	World::World(cpSpace *space, material m):
	_material(m),
	_space(space),
	_staticGroup(make_shared<StaticGroup>(space,m))
	{}

	World::~World() {
	}

	void World::build(const vector<ShapeRef> &shapes, const vector<AnchorRef> &anchors) {

		// build the anchors, adding all that triangulated and made physics representations
		for (auto anchor : anchors) {
			if (anchor->build(_space)) {
				_anchors.push_back(anchor);
			}
		}

		// now build
		build(shapes, map<ShapeRef,GroupBaseRef>());
	}

	void World::cut(vec2 a, vec2 b, float radius, cpShapeFilter filter) {
		const float MinLength = 1e-2;
		const float MinRadius = 1e-2;

		vec2 dir = b - a;
		float l = length(dir);

		if (l > MinLength && radius > MinRadius) {
			dir /= l;

			vec2 right = radius * detail::rotate_cw(dir);
			vec2 left = -right;

			vec2 ca = a + right;
			vec2 cb = a + left;
			vec2 cc = b + left;
			vec2 cd = b + right;

			PolyLine2f contour;
			contour.push_back(ca);
			contour.push_back(cb);
			contour.push_back(cc);
			contour.push_back(cd);
			contour.setClosed();

			CI_LOG_D("Performing cut from " << a << " to " << b << " radius: " << radius);

			struct cut_collector {
				set<ShapeRef> shapes;
				set<GroupBaseRef> groups;

				void clear() {
					shapes.clear();
					groups.clear();
				}
			};

			cut_collector collector;

			//
			// while intuition would have us use a segment query, the cpSegmentQuery call seemed to miss
			// literal "corner cases" where a TINY corner of a shape, even if it was WELL INSIDE the segment,
			// would be missed. My guess is segment queries use some sort of stepping algorithm and miss
			// tiny overhangs. So we're building a cpShape explicitly.
			//

			cpVect verts[] = { cpv(cd), cpv(cc), cpv(cb), cpv(ca) };
			cpShape *cuttingShape = cpPolyShapeNew(cpSpaceGetStaticBody(_space), 4, verts, cpTransformIdentity, radius * 2);
			cpSpaceShapeQuery(_space, cuttingShape, [](cpShape *collisionShape, cpContactPointSet *points, void *data){
				cut_collector *collector = static_cast<cut_collector*>(data);
				ShapeRef terrainShape = static_cast<Shape*>(cpShapeGetUserData(collisionShape))->shared_from_this();
				collector->shapes.insert(terrainShape);
				collector->groups.insert(terrainShape->getGroup());
			}, &collector);

			cpShapeFree(cuttingShape);

			CI_LOG_D("Collected " << collector.shapes.size() << " shapes (" << collector.groups.size() << " bodies) to cut" );

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

				for (auto &newShape : result) {
					parentage[newShape] = parentGroup;
				}

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
		switch(renderState.mode) {
			case RenderMode::GAME:
				drawGame(renderState);
				break;

			case RenderMode::DEVELOPMENT:
				drawDebug(renderState);
				break;

			case RenderMode::COUNT:
				break;
		}
	}

	void World::step(const time_state &timeState) {
		_staticGroup->step(timeState);
		for (auto &body : _dynamicGroups) {
			body->step(timeState);
		}
	}

	void World::update(const time_state &timeState) {
		_staticGroup->update(timeState);
		for (auto &body : _dynamicGroups) {
			body->update(timeState);
		}
	}

	void World::build(const vector<ShapeRef> &affectedShapes, const map<ShapeRef,GroupBaseRef> &parentage) {

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


				DynamicGroupRef group = make_shared<DynamicGroup>(_space, _material);
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

						const PolyLine2f &shapeContour = shape->getOuterContour().world;
						const PolyLine2f &anchorContour = anchor->getContour();

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

	void World::drawGame(const render_state &renderState) {
		const cpBB frustum = renderState.viewport.getFrustum();
		for (const auto &group : _dynamicGroups) {
			if (cpBBIntersects(frustum, group->getBB())) {
				gl::ScopedModelMatrix smm;
				gl::multModelMatrix(group->getModelview());

				for (const auto &shape : group->getShapes()) {
					gl::color(group->getColor());
					gl::draw(*shape->getTriMesh());
				}
			}
		}

		if (cpBBIntersects(frustum, _staticGroup->getBB())) {
			gl::ScopedModelMatrix smm;
			gl::multModelMatrix(_staticGroup->getModelview());

			for (const auto &shape : _staticGroup->getShapes()) {
				gl::color(_staticGroup->getColor());
				gl::draw(*shape->getTriMesh());
			}
		}

		for (const auto &anchor : _anchors) {
			if (cpBBIntersects(frustum, anchor->getBB())) {
				// anchors are already in world space so we just draw them
				gl::color(ColorA(1,1,1));
				gl::draw(*anchor->getTriMesh());
			}
		}
	}

	void World::drawDebug(const render_state &renderState) {
		const cpBB frustum = renderState.viewport.getFrustum();

		auto drawGroup = [frustum, &renderState](GroupBaseRef group, Color fillColor, Color strokeColor) {
			if (cpBBIntersects(frustum, group->getBB())) {
				gl::ScopedModelMatrix smm;
				gl::multModelMatrix(group->getModelview());

				for (const auto &shape : group->getShapes()) {
					gl::color(fillColor);
					gl::draw(*shape->getTriMesh());

					gl::lineWidth(1);
					gl::color(strokeColor);
					gl::draw(shape->getOuterContour().model);

					for (auto &holeContour : shape->getHoleContours()) {
						gl::draw(holeContour.model);
					}

					{
						float rScale = renderState.viewport.getReciprocalScale();
						float angle = group->getAngle();
						vec3 modelCentroid = vec3(shape->_modelCentroid, 0);
						gl::ScopedModelMatrix smm2;
						gl::multModelMatrix(glm::translate(modelCentroid) * glm::rotate(-angle, vec3(0,0,1)) * glm::scale(vec3(rScale, -rScale, 1)));
						gl::drawString(shape->getName(), vec2(0,0), strokeColor);
					}
				}
			}

			// body draws some debug data - note that game render pass ignores Group::draw
			group->draw(renderState);
		};

		for (const auto &group : _dynamicGroups) {
			drawGroup(group, group->getColor(), group->getColor().lerp(0.5, Color::white()));
		}

		drawGroup(_staticGroup, _staticGroup->getColor(), _staticGroup->getColor().lerp(0.75, Color::white()));

		for (const auto &anchor : _anchors) {
			if (cpBBIntersects(frustum, anchor->getBB())) {
				// anchors are already in world space so we just draw them
				gl::color(ColorA(1,1,1,0.25));
				gl::draw(*anchor->getTriMesh());

				gl::lineWidth(1);
				gl::color(Color(1,1,1));
				gl::draw(anchor->getContour());
			}
		}
	}

#pragma mark - GroupBase


	GroupBase::~GroupBase(){}


#pragma mark - StaticGroup

	/*
		cpSpace *_space;
		cpBody *_body;
		material _material;
		set<ShapeRef> _shapes;
		cpBB _worldBB;
	 */

	StaticGroup::StaticGroup(cpSpace *space, material m):
	_space(space),
	_body(nullptr),
	_material(m),
	_worldBB(cpBBInvalid) {
		_name = "StaticGroup";
		_color = Color(0.1,0.1,0.1);
		_body = cpBodyNewStatic();
		cpBodySetUserData(_body, this);
		cpSpaceAddBody(space,_body);
	}
	
	StaticGroup::~StaticGroup() {
		_shapes.clear();
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

	void StaticGroup::addShape(ShapeRef shape) {

		if (!shape->hasValidTriMesh()) {
			if (!shape->triangulate()) {
				return;
			}
		}

		if (shape->hasValidTriMesh()) {

			shape->_modelCentroid = shape->_outerContour.model.calcCentroid();

			float area = shape->computeArea();
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

				if (!collisionShapes.empty() && cpBBIsValid(modelBB)) {
					cpBBExpand(_worldBB, modelBB);

					if (didCreateNewShapes) {
						for (cpShape *collisionShape : collisionShapes) {
							cpShapeSetFilter(collisionShape, _material.filter);
							cpShapeSetFriction(collisionShape, _material.friction);
							cpSpaceAddShape(_space, collisionShape);
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
		}
	}


#pragma mark - DynamicGroup

	/*
		static size_t _count;

		bool _dynamic;
		material _material;
		cpSpace *_space;
		cpBody *_body;
		cpVect _position;
		cpFloat _angle;
		cpBB _worldBB, _modelBB;
		mat4 _modelview, _modelviewInverse;

		set<ShapeRef> _shapes;

		string _name;
		cpHashValue _hash;
		Color _color;
	 */

	size_t DynamicGroup::_count = 0;

	DynamicGroup::DynamicGroup(cpSpace *space, material m):
	_material(m),
	_space(space),
	_body(nullptr),
	_position(cpv(0,0)),
	_angle(0),
	_worldBB(cpBBInvalid),
	_modelBB(cpBBInvalid),
	_modelview(1),
	_modelviewInverse(1) {
		_name = nextId();
		_hash = hash<string>{}(_name);
		_color = detail::next_random_color();
	}

	DynamicGroup::~DynamicGroup() {
		_shapes.clear(); // will run Shape::dtor freeing cpShapes
		cpCleanupAndFree(_body);
	}

	string DynamicGroup::getName() const {
		stringstream ss;
		ss << "Body[" << _name << "](";
		for (auto &shape : _shapes) {
			ss << shape->getName() << ":";
		}
		ss << ")";
		return ss.str();
	}

	void DynamicGroup::draw(const render_state &renderState) {
		if (renderState.mode == RenderMode::DEVELOPMENT) {
			const float rScale = renderState.viewport.getReciprocalScale();

			gl::ScopedModelMatrix smm2;
			gl::multModelMatrix(translate(vec3(getPosition() + vec2(0,20),0)) * scale(vec3(rScale, -rScale, 1)));

			gl::drawString(getName(), vec2(0,0), Color(1,1,1));
		}
	}

	void DynamicGroup::step(const time_state &timeState) {
		syncToCpBody();
	}

	void DynamicGroup::update(const time_state &timeState) {
	}

	string DynamicGroup::nextId() {
		stringstream ss;
		ss << _count++;
		return ss.str();
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

				const mat4 T = shape->getModelview();
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

		vec2 averageModelSpaceCentroid(0,0);
		for (auto &shape : shapes) {
			shape->_modelCentroid = shape->_outerContour.model.calcCentroid();
			averageModelSpaceCentroid += shape->_modelCentroid;
		}

		averageModelSpaceCentroid /= shapes.size();

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

		_modelview = _modelview * translate(vec3(averageModelSpaceCentroid.x, averageModelSpaceCentroid.y, 0.f));
		_modelviewInverse = inverse(_modelview);

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

		float totalArea = 0, totalMass = 0, totalMoment = 0;
		for (auto &shape : shapes) {
			float area = 0, mass = 0, moment = 0;
			shape->computeMassAndMoment(_material.density, mass, moment, area);
			totalArea += area;
			totalMass += mass;
			totalMoment += moment;
		}


		if (totalArea > detail::MIN_SHAPE_AREA) {

			_body = cpBodyNew(totalMass, totalMoment);
			cpBodySetUserData(_body, this);

			// glm::mat4 is column major
			vec2 position;
			position.x = _modelview[3][0];
			position.y = _modelview[3][1];
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
						cpShapeSetFriction(collisionShape, _material.friction);
						cpSpaceAddShape(_space, collisionShape);
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
				cpSpaceAddBody(_space, _body);
				syncToCpBody();

				if (parentGroup && cpBodyGetType(parentGroup->getBody()) == CP_BODY_TYPE_DYNAMIC) {

					//
					//	Compute world linearvel and angularvel for parent and apply to self
					//

					const float angularVel = cpBodyGetAngularVelocity(parentGroup->getBody());
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
		// extract position and rotation from body and apply to _modelview
		cpVect position = cpBodyGetPosition(_body);
		cpVect rotation = cpBodyGetRotation(_body);
		cpFloat angle = cpBodyGetAngle(_body);

		// determine if we moved/rotated since last step
		const cpFloat Epsilon = 1e-3;
		bool moved = (cpvlengthsq(cpvsub(position, _position)) > Epsilon || abs(angle - _angle) > Epsilon);
		_position = position;
		_angle = angle;

		// mat4 - column major, each column is a vec4
		_modelview = mat4(
						  vec4(rotation.x, rotation.y, 0, 0),
						  vec4(-rotation.y, rotation.x, 0, 0),
						  vec4(0,0,1,0),
						  vec4(position.x, position.y, 0, 1));

		_modelviewInverse = inverse(_modelview);

		// update our world BB
		_worldBB = cpTransformbBB(getModelviewTransform(), _modelBB);

		if (moved) {
			for (auto &shape : _shapes) {
				shape->_worldSpaceShapeContourEdgesDirty = true;
			}
		}
	}

#pragma mark - Anchor

	/*
		cpBody *_staticBody;
		vector<cpShape*> _shapes;

		cpBB _bb;
		material _material;
		PolyLine2f _contour;
		TriMeshRef _trimesh;
	 */

	Anchor::Anchor(const PolyLine2f &contour, material m):
	_staticBody(nullptr),
	_bb(cpBBInvalid),
	_material(m),
	_contour(contour) {

		for (auto &p : _contour.getPoints()) {
			cpBBExpand(_bb, p);
		}

		Triangulator triangulator;
		triangulator.addPolyLine(_contour);
		_trimesh = triangulator.createMesh();
	}

	Anchor::~Anchor() {
		for (auto s : _shapes) {
			cpCleanupAndFree(s);
		}
		_shapes.clear();
		cpCleanupAndFree(_staticBody);
	}

	bool Anchor::build(cpSpace *space) {
		assert(_shapes.empty());

		_staticBody = cpBodyNewStatic();

		cpVect triangle[3];
		for (size_t i = 0, N = _trimesh->getNumTriangles(); i < N; i++) {
			vec2 a,b,c;
			_trimesh->getTriangleVertices(i, &a, &b, &c);
			triangle[0] = cpv(a);
			triangle[1] = cpv(b);
			triangle[2] = cpv(c);

			float area = cpAreaForPoly(3, triangle, 0);
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

			cpSpaceAddBody(space, _staticBody);
			for (auto shape : _shapes) {
				cpSpaceAddShape(space, shape);
				cpShapeSetUserData(shape, this);
				cpShapeSetFilter(shape, _material.filter);
				cpShapeSetFriction(shape, _material.friction);
			}

			return true;
		}

		return false;
	}

#pragma mark - Shape

	size_t Shape::_count = 0;

	string Shape::nextId() {
		stringstream ss;
		ss << _count++;
		return ss.str();
	}

	ShapeRef Shape::fromContour(const PolyLine2f outerContour) {
		return make_shared<Shape>(outerContour);
	}

	vector<ShapeRef> Shape::fromContours(const vector<PolyLine2f> &contourSoup) {

		vector<detail::contour_tree_node_ref> rootNodes = detail::build_contour_tree(contourSoup);
		vector<ShapeRef> shapes;
		for (auto &rn : rootNodes) {
			vector<ShapeRef> built = build_from_contour_tree(rn, 0);
			shapes.insert(end(shapes), begin(built), end(built));
		}

		return shapes;
	}

	vector<ShapeRef> Shape::fromShapes(const vector<Shape2d> &shapeSoup) {
		vector<PolyLine2f> contours;
		for (auto &shape : shapeSoup) {
			for (auto &path : shape.getContours()) {
				contours.push_back(path.subdivide());
			}
		}

		return fromContours(contours);
	}

	/*
		bool _worldSpaceShapeContourEdgesDirty;
		cpHashValue _hash;
		string _name;
		PolyLine2f _worldSpaceOuterContour, _modelSpaceOuterContour;
		vector<PolyLine2f> _worldSpaceHoleContours, _modelSpaceHoleContours;
		TriMeshRef _trimesh;
		vec2 _modelCentroid;

		vector<cpShape*> _shapes;
		GroupWeakRef _body;

		set<poly_edge> _worldSpaceContourEdges;
		cpBB _worldSpaceContourEdgesBB;
	 */


	Shape::Shape(const PolyLine2f &sc):
	_worldSpaceShapeContourEdgesDirty(true),
	_name(nextId()),
	_outerContour(detail::optimize(sc)),
	_modelCentroid(0,0),
	_worldSpaceContourEdgesBB(cpBBInvalid) {
		_hash = hash<string>{}(_name);
		detail::wind_clockwise(_outerContour.world);
	}

	Shape::Shape(const PolyLine2f &sc, const std::vector<PolyLine2f> &hcs):
	_worldSpaceShapeContourEdgesDirty(true),
	_name(nextId()),
	_outerContour(detail::optimize(sc)),
	_holeContours(detail::optimize(hcs)),
	_modelCentroid(0,0),
	_worldSpaceContourEdgesBB(cpBBInvalid) {
		_hash = hash<string>{}(_name);

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

	cpBB Shape::getWorldSpaceContourEdgesBB() {
		updateWorldSpaceContourAndBB();
		return _worldSpaceContourEdgesBB;
	}

	vector<ShapeRef> Shape::subtract(const PolyLine2f &contourToSubtract) const {
		if (!contourToSubtract.getPoints().empty()) {

			//
			// move the shape to subtract from world space to the our model space, and convert to a boost poly
			//

			PolyLine2f transformedShapeToSubtract = detail::transformed(contourToSubtract, getModelviewInverse());
			detail::polygon polyToSubtract = detail::convertPolyLineToBoostGeometry( transformedShapeToSubtract );

			//
			// convert self (outerContour & holeContours) to a boost poly in model space
			//

			detail::polygon thisPoly = detail::convertTerrainShapeToBoostGeometry( const_cast<Shape*>(this)->shared_from_this() );

			//
			// now subtract - results are in model space
			//

			std::vector<detail::polygon> output;
			boost::geometry::difference( thisPoly, polyToSubtract, output );

			//
			// convert output to Shapes - they need to have our modelview applied to them to move them back to world space
			//

			vector<ShapeRef> newShapes = detail::convertBoostGeometryToTerrainShapes( output, getModelview() );

			if (!newShapes.empty()) {
				return newShapes;
			}
		}

		// handle failure case
		vector<ShapeRef> result = { const_cast<Shape*>(this)->shared_from_this() };
		return result;
	}

	void Shape::updateWorldSpaceContourAndBB() {
		if (_worldSpaceShapeContourEdgesDirty) {

			assert(_outerContour.model.size() > 1);

			const mat4 mv = getModelview();
			vec2 worldPoint = mv * _outerContour.model.getPoints().front();

			_worldSpaceContourEdges.clear();
			cpBB bb = cpBBInvalid;

			// the shapeContour is in model coordinates - we need to move each vertex to world
			for (auto current = _outerContour.model.begin(), next = _outerContour.model.begin()+1, end = _outerContour.model.end(); current != end; ++current, ++next) {
				if (next == end) {
					next = _outerContour.model.begin();
				}
				const vec2 worldNext = mv * (*next);
				_worldSpaceContourEdges.insert(poly_edge(worldPoint, worldNext));
				worldPoint = worldNext;
				cpBBExpand(bb, worldPoint);
			}

			_worldSpaceContourEdgesBB = bb;
			_worldSpaceShapeContourEdgesDirty = false;
		}
	}

	bool Shape::triangulate() {

		//
		// note that when a shape is static, its world and model contours are the same
		//

		Triangulator triangulator;
		triangulator.addPolyLine(_outerContour.model);

		for (auto holeContour : _holeContours) {
			triangulator.addPolyLine(holeContour.model);
		}

		_trimesh = triangulator.createMesh();

		return _trimesh->getNumTriangles() > 0;
	}

	float Shape::computeArea() {
		float area = 0;
		cpVect triangle[3];

		for (size_t i = 0, N = _trimesh->getNumTriangles(); i < N; i++) {
			vec2 a,b,c;
			_trimesh->getTriangleVertices(i, &a, &b, &c);

			// find the winding with positive area
			triangle[0] = cpv(a);
			triangle[1] = cpv(b);
			triangle[2] = cpv(c);
			float triArea = length(cross(vec3(b-a,0),vec3(c-a,0))) * 0.5f;

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

	void Shape::computeMassAndMoment(float density, float &mass, float &moment, float &area) {
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
			float triArea = length(cross(vec3(b-a,0),vec3(c-a,0))) * 0.5f;

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

			const float polyMass = abs(density * triArea);

			if (polyMass > 1e-3) {
				const float polyMoment = cpMomentForPoly(polyMass, 3, triangle, cpvzero, 0);
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

			float area = cpAreaForPoly(3, triangle, 0);
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

		modelBB = _shapesBB = bb;
		return _shapes;
	}
	

} // end namespace island
