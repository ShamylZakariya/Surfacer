//
//  Stage.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/27/17.
//
//

#include "Stage.hpp"

#include <cinder/CinderAssert.h>

#include "ChipmunkHelpers.hpp"
#include "Scenario.hpp"

namespace core {
	
#pragma mark - GravitationCalculator

	/*
	 bool _finished;
	 */
	GravitationCalculator::GravitationCalculator():
	_finished(false)
	{}
	
	/*
	 force _force;
	 */
	
	DirectionalGravitationCalculatorRef DirectionalGravitationCalculator::create(dvec2 dir, double magnitude) {
		return make_shared<DirectionalGravitationCalculator>(dir, magnitude);
	}
	
	DirectionalGravitationCalculator::DirectionalGravitationCalculator(dvec2 dir, double magnitude):
	_force(normalize(dir), magnitude)
	{}
	
	GravitationCalculator::force DirectionalGravitationCalculator::calculate(const dvec2 &world) const {
		return _force;
	}
	
	void DirectionalGravitationCalculator::setDir(dvec2 dir) {
		_force.dir = normalize(dir);
	}
	
	void DirectionalGravitationCalculator::setMagnitude(double mag) {
		_force.magnitude = mag;
	}
	
	/*
	 dvec2 _centerOfMass;
	 double _magnitude;
	 double _falloffPower;
	 */
	
	RadialGravitationCalculatorRef RadialGravitationCalculator::create(dvec2 centerOfMass, double magnitude, double falloffPower) {
		return make_shared<RadialGravitationCalculator>(centerOfMass, magnitude, falloffPower);
	}
	
	RadialGravitationCalculator::RadialGravitationCalculator(dvec2 centerOfMass, double magnitude, double falloffPower):
	_centerOfMass(centerOfMass),
	_magnitude(magnitude),
	_falloffPower(max(falloffPower, 0.0))
	{}
	
	// GravitationCalculator
	GravitationCalculator::force RadialGravitationCalculator::calculate(const dvec2 &world) const {
		return calculate(world, _centerOfMass, _magnitude, _falloffPower);
	}
	
	GravitationCalculator::force RadialGravitationCalculator::calculate(const dvec2 &world, const dvec2 &centerOfMass, double magnitude, double falloffPower) const {
		dvec2 p_to_com = centerOfMass - world;
		double dist = length(p_to_com);
		if (dist > 1e-5) {
			p_to_com /= dist;
			dvec2 g = p_to_com * (magnitude / pow(dist, falloffPower));
			double f = length(g);
			return force(g/f, f);
		}
		return force(dvec2(0,0), 0);
	}
	
	void RadialGravitationCalculator::setCenterOfMass(dvec2 centerOfMass) {
		_centerOfMass = centerOfMass;
	}
	
	void RadialGravitationCalculator::setMagnitude(double mag) {
		_magnitude = mag;
	}
	
	void RadialGravitationCalculator::setFalloffPower(double fp) {
		_falloffPower = max(fp, 0.0);
	}
	


#pragma mark - SpaceAccess

	SpaceAccess::SpaceAccess(cpSpace *space, Stage *stage):
	_space(space),
	_stage(stage)
	{}

	void SpaceAccess::addBody(cpBody *body) {
		cpSpaceAddBody(_space, body);
		bodyWasAddedToSpace(body);
	}

	void SpaceAccess::addShape(cpShape *shape) {
		cpSpaceAddShape(_space, shape);
		shapeWasAddedToSpace(shape);
	}

	void SpaceAccess::addConstraint(cpConstraint *constraint) {
		cpSpaceAddConstraint(_space, constraint);
		constraintWasAddedToSpace(constraint);
	}

	GravitationCalculator::force SpaceAccess::getGravity(const dvec2 &world) {
		return _stage->getGravitation(world);
	}


#pragma mark - DrawDispatcher

	namespace {

		cpHashValue getCPHashValue(const DrawComponentRef &dc) {
			return reinterpret_cast<cpHashValue>(dc.get());
		}

		cpHashValue getCPHashValue(DrawComponent *dc) {
			return reinterpret_cast<cpHashValue>(dc);
		}

		cpBB objectBBFunc( void *obj )
		{
			return static_cast<DrawComponent*>(obj)->getBB();
		}

		cpCollisionID visibleObjectCollector(void *obj1, void *obj2, cpCollisionID id, void *data) {
			DrawComponentRef dc = static_cast<DrawComponent*>(obj2)->shared_from_this_as<DrawComponent>();
			if (cpBBIsValid(dc->getBB())) {
				DrawDispatcher::collector *collector = static_cast<DrawDispatcher::collector*>(data);

				//
				//	We don't want dupes in sorted vector, so filter based on whether
				//	the object made it into the set
				//

				if( collector->visible.insert(dc).second )
				{
					collector->sorted.push_back(dc);
				}
			}

			return id;
		}

		bool visibleObjectDisplaySorter( const DrawComponentRef &a, const DrawComponentRef &b )
		{
			//
			// draw lower layers before higher layers
			//

			if ( a->getLayer() != b->getLayer() )
			{
				return a->getLayer() < b->getLayer();
			}

			//
			//	If we're here, the objects are on the same layer. So now
			//	we want to group objects with the same batchDrawDelegate
			//

			if ( a->getBatchDrawDelegate() != b->getBatchDrawDelegate() )
			{
				return a->getBatchDrawDelegate() < b->getBatchDrawDelegate();
			}

			//
			//	If we're here, the two objects have the same ( possibly null )
			//	batchDrawDelegate, so, just order them from older to newer
			//

			return a->getObject()->getId() < b->getObject()->getId();
		}
		
	}

	/*
		 cpSpatialIndex *_index;
		 set<DrawComponentRef> _all, _alwaysVisible, _deferredSpatialIndexInsertion;
		 map<size_t, set<DrawComponentRef>> _drawComponentsById;
		 collector _collector;
	 */

	DrawDispatcher::DrawDispatcher():
	_index(cpBBTreeNew( objectBBFunc, NULL ))
	{}

	DrawDispatcher::~DrawDispatcher()
	{
		cpSpatialIndexFree( _index );
	}

	void DrawDispatcher::add( size_t id, const DrawComponentRef &dc )
	{
		_drawComponentsById[id].insert(dc);

		if (_all.insert(dc).second) {
			switch( dc->getVisibilityDetermination() )
			{
				case VisibilityDetermination::ALWAYS_DRAW:
					_alwaysVisible.insert( dc );
					break;

				case VisibilityDetermination::FRUSTUM_CULLING:
					// wait until ::cull to add to spatial index. This is to accommodate
					// partially constructed Objects
					_deferredSpatialIndexInsertion.insert(dc);
					break;

				case VisibilityDetermination::NEVER_DRAW:
					break;
			}
		}
	}

	void DrawDispatcher::remove(size_t id) {
		for (auto &dc : _drawComponentsById[id]) {
			remove(dc);
		}
	}

	void DrawDispatcher::remove( const DrawComponentRef &dc )
	{
		if (_all.erase(dc)) {
			switch( dc->getVisibilityDetermination() )
			{
				case VisibilityDetermination::ALWAYS_DRAW:
				{
					_alwaysVisible.erase( dc );
					break;
				}

				case VisibilityDetermination::FRUSTUM_CULLING:
				{
					_deferredSpatialIndexInsertion.erase(dc);

					const auto dcp = dc.get();
					const auto hv = getCPHashValue(dc);
					if (cpSpatialIndexContains(_index, dcp, hv)){
						cpSpatialIndexRemove( _index, dcp, hv);
					}
					break;
				}

				case VisibilityDetermination::NEVER_DRAW:
					break;
			}

			_collector.remove(dc);
		}
	}

	void DrawDispatcher::moved( const DrawComponentRef &dc )
	{
		moved(dc.get());
	}

	void DrawDispatcher::moved( DrawComponent *dc ) {
		if ( dc->getVisibilityDetermination() == VisibilityDetermination::FRUSTUM_CULLING )
		{
			cpSpatialIndexReindexObject( _index, dc, getCPHashValue(dc));
		}
	}

	void DrawDispatcher::cull( const render_state &state )
	{
		// if we have any deferred insertions to spatial index, do it now
		if (!_deferredSpatialIndexInsertion.empty())
		{
			for (const auto &dc : _deferredSpatialIndexInsertion)
			{
				cpSpatialIndexInsert( _index, dc.get(), getCPHashValue(dc) );
			}
			_deferredSpatialIndexInsertion.clear();
		}


		//
		//	clear storage - note: std::vector doesn't free, it keeps space reserved.
		//

		_collector.visible.clear();
		_collector.sorted.clear();

		//
		//	Copy over objects which are always visible
		//

		for (const auto &dc : _alwaysVisible) {
			_collector.visible.insert(dc);
			_collector.sorted.push_back(dc);
		}

		//
		//	Find those objects which use frustum culling and which intersect the current view frustum
		//

		cpSpatialIndexQuery( _index, this, state.viewport->getFrustum(), visibleObjectCollector, &_collector );

		//
		//	Sort them all
		//

		std::sort(_collector.sorted.begin(), _collector.sorted.end(), visibleObjectDisplaySorter );
	}

	void DrawDispatcher::draw( const render_state &state )
	{
		render_state renderState = state;

		for( vector<DrawComponentRef>::iterator
			dcIt(_collector.sorted.begin()),
			end(_collector.sorted.end());
			dcIt != end;
			++dcIt )
		{
			DrawComponentRef dc = *dcIt;
			DrawComponent::BatchDrawDelegateRef delegate = dc->getBatchDrawDelegate();
			int drawPasses = dc->getDrawPasses();

			vector<DrawComponentRef>::iterator newDcIt = dcIt;
			for( renderState.pass = 0; renderState.pass < drawPasses; ++renderState.pass ) {
				if ( delegate ) {
					newDcIt = _drawDelegateRun(dcIt, end, renderState );
				}
				else
				{
					dc->draw( renderState );
				}
			}

			dcIt = newDcIt;
		}
	}

	vector<DrawComponentRef>::iterator
	DrawDispatcher::_drawDelegateRun(vector<DrawComponentRef>::iterator firstInRun, vector<DrawComponentRef>::iterator storageEnd, const render_state &state ) {
		vector<DrawComponentRef>::iterator dcIt = firstInRun;
		DrawComponentRef dc = *dcIt;
		DrawComponent::BatchDrawDelegateRef delegate = dc->getBatchDrawDelegate();

		delegate->prepareForBatchDraw( state, dc );

		for( ; dcIt != storageEnd; ++dcIt )
		{
			//
			//	If the delegate run has completed, clean up after our run
			//	and return the current iterator.
			//

			if ( (*dcIt)->getBatchDrawDelegate() != delegate )
			{
				delegate->cleanupAfterBatchDraw( state, *firstInRun, dc );
				return dcIt - 1;
			}
			
			dc = *dcIt;
			dc->draw( state );
		} 
		
		//
		//	If we reached the end of storage, run cleanup
		//
		
		delegate->cleanupAfterBatchDraw( state, *firstInRun, dc );
		
		return dcIt - 1;
	}

	
#pragma mark - Stage

	namespace {

		void gravitationCalculatorVelocityFunc(cpBody *body, cpVect gravity, cpFloat damping, cpFloat dt) {
			cpSpace *space = cpBodyGetSpace(body);
			Stage *stage = static_cast<Stage*>(cpSpaceGetUserData(space));
			auto g = stage->getGravitation(v2(cpBodyGetPosition(body)));
			auto magnitude = g.magnitude;

			// objects can define a custom gravity modifier - 0 means no effect, -1 would be repulsive, etc
			if (ObjectRef object = cpBodyGetObject(body)) {
				if (PhysicsComponentRef physics = object->getPhysicsComponent()) {
					magnitude *= physics->getGravityModifier(body);
				}
			}

			cpBodyUpdateVelocity(body, cpv(g.dir * magnitude), damping, dt);
		}

	}

	/*
	 cpSpace *_space;
	 SpaceAccessRef _spaceAccess;
	 bool _ready, _paused;
	 ScenarioWeakRef _scenario;
	 set<ObjectRef> _objects;
	 map<size_t,ObjectRef> _objectsById;
	 time_state _time;
	 string _name;
	 DrawDispatcherRef _drawDispatcher;
	 cpBodyVelocityFunc _bodyVelocityFunc;
	 vector<GravitationCalculatorRef> _gravities;
	 
	 set<collision_type_pair> _monitoredCollisions;
	 map<collision_type_pair, vector<EarlyCollisionCallback>> _collisionBeginHandlers, _collisionPreSolveHandlers;
	 map<collision_type_pair, vector<LateCollisionCallback>> _collisionPostSolveHandlers, _collisionSeparateHandlers;
	 map<collision_type_pair, vector<ContactCallback>> _contactHandlers;
	 map<collision_type_pair, vector<pair<ObjectRef, ObjectRef>>> _syntheticContacts;
	 */

	Stage::Stage(string name):
	_space(cpSpaceNew()),
	_ready(false),
	_paused(false),
	_time(0,0,0),
	_name(name),
	_drawDispatcher(make_shared<DrawDispatcher>()),
	_bodyVelocityFunc(cpBodyUpdateVelocity)
	{
		// some defaults
		cpSpaceSetGravity(_space, cpvzero);
		cpSpaceSetIterations( _space, 30 );
		cpSpaceSetDamping( _space, 0.95 );
		cpSpaceSetSleepTimeThreshold( _space, 1 );
		cpSpaceSetIdleSpeedThreshold(_space, 0.05);
		cpSpaceSetUserData(_space, this);
		setCpBodyVelocityUpdateFunc(gravitationCalculatorVelocityFunc);

		_spaceAccess = SpaceAccessRef(new SpaceAccess(_space, this));
		_spaceAccess->bodyWasAddedToSpace.connect(this, &Stage::onBodyAddedToSpace);
		_spaceAccess->shapeWasAddedToSpace.connect(this, &Stage::onShapeAddedToSpace);
		_spaceAccess->constraintWasAddedToSpace.connect(this, &Stage::onConstraintAddedToSpace);
		
	}

	Stage::~Stage() {
		// these all have to be freed before we can free the space
		_objects.clear();
		_objectsById.clear();
		_drawDispatcher.reset();

		cpSpaceFree(_space);
	}

	void Stage::setPaused(bool paused) {
		if (paused != isPaused()) {
			_paused = isPaused();
			if (isPaused()) {
				signals.onStagePaused(shared_from_this_as<Stage>());
			} else {
				signals.onStageUnpaused(shared_from_this_as<Stage>());
			}
		}
	}

	void Stage::resize( ivec2 newSize )
	{}

	void Stage::step( const time_state &time ) {
		if (!_ready) {
			onReady();
		}

		if (!_paused) {
			// step the chipmunk space
			cpSpaceStep(_space, time.deltaT);

			// dispatch any synthetic contacts which were generated
			dispatchSyntheticContacts();
	
			// step objects and dispose of those which are finished
			{
				vector<ObjectRef> moribund;
				for (auto &obj : _objects) {
					if (!obj->isFinished()) {
						obj->step(time);
					} else {
						moribund.push_back(obj);
					}
				}
				
				if (!moribund.empty()) {
					for (auto &obj : moribund) {
						removeObject(obj);
					}
				}
			}
		}
	}

	void Stage::update( const time_state &time ) {
		if (!_paused) {
			_time = time;
		}

		if (!_ready) {
			onReady();
		}

		if (!_paused) {
			
			{
				// update gravitation calculators and dispose of finished
				vector<GravitationCalculatorRef> moribund;
				for (const auto &gravity : _gravities) {
					if (!gravity->isFinished()) {
						gravity->update(time);
					} else {
						moribund.push_back(gravity);
					}
				}
				
				if (!moribund.empty()) {
					for (const auto &obj : moribund) {
						removeGravity(obj);
					}
				}
			}
			
			{
				// update objects and dispose of finished
				vector<ObjectRef> moribund;
				for (auto &obj : _objects) {
					if (!obj->isFinished()) {
						obj->update(time);
					} else {
						moribund.push_back(obj);
					}
				}

				if (!moribund.empty()) {
					for (auto &obj : moribund) {
						removeObject(obj);
					}
				}
			}
		}

		// cull visibility sets
		if (ScenarioRef scenario = _scenario.lock()) {
			_drawDispatcher->cull(scenario->getRenderState());
		}
	}

	void Stage::draw( const render_state &state ) {
		//
		//	This may look odd, but Stage is responsible for Pausing, not the owner Scenario. This means that
		//	when the game is paused, the stage is responsible for sending a mock 'paused' time object that reflects
		//	the time when the game was paused. I can't do this at the Scenario stage because Scenario may have UI
		//	which needs correct time updates.
		//

		render_state localState = state;
		localState.time = _time.time;
		localState.deltaT = _time.deltaT;

		_drawDispatcher->draw( localState );
	}

	void Stage::drawScreen( const render_state &state ) {
		for (const auto &dc : _drawDispatcher->all()) {
			dc->drawScreen(state);
		}
	}

	void Stage::addObject(ObjectRef obj) {
		CI_ASSERT_MSG(!obj->getStage(), "Can't add a Object that already has been added to this or another Stage");

		size_t id = obj->getId();

		_objects.insert(obj);
		_objectsById[id] = obj;

		for (auto &dc : obj->getDrawComponents()) {
			_drawDispatcher->add(id, dc);
		}

		obj->onAddedToStage(shared_from_this_as<Stage>());

		if (_ready) {
			obj->onReady(shared_from_this_as<Stage>());
		}

	}

	void Stage::removeObject(ObjectRef obj) {
		CI_ASSERT_MSG(obj->getStage().get() == this, "Can't remove a Object which isn't a child of this Stage");

		size_t id = obj->getId();

		_objects.erase(obj);
		_objectsById.erase(id);
		_drawDispatcher->remove(id);

		obj->onRemovedFromStage();
	}

	ObjectRef Stage::getObjectById(size_t id) const {
		auto pos = _objectsById.find(id);
		if (pos != _objectsById.end()) {
			return pos->second;
		} else {
			return nullptr;
		}
	}

	vector<ObjectRef> Stage::getObjectsByName(string name) const {

		vector<ObjectRef> result;
		copy_if(_objects.begin(), _objects.end(), back_inserter(result),[&name](const ObjectRef &object) -> bool {
			return object->getName() == name;
		});

		return result;
	}

	ViewportRef Stage::getViewport() const {
		return getScenario()->getViewport();
	}

	ViewportControllerRef Stage::getViewportController() const {
		return getScenario()->getViewportController();
	}
	
	void Stage::addGravity(const GravitationCalculatorRef &gravityCalculator) {
		removeGravity(gravityCalculator);
		_gravities.push_back(gravityCalculator);
	}

	void Stage::removeGravity(const GravitationCalculatorRef &gravityCalculator) {
		_gravities.erase(remove(_gravities.begin(), _gravities.end(), gravityCalculator), _gravities.end());
	}

	GravitationCalculator::force Stage::getGravitation(dvec2 world) const {
		
		if (!_gravities.empty()) {
			dvec2 gravity(0,0);
			for (const auto &calc : _gravities) {
				gravity += calc->calculate(world).getForce();
			}
			
			double magnitude = length(gravity);
			if (magnitude > 1e-5) {
				dvec2 dir = gravity / magnitude;
				return GravitationCalculator::force(dir, magnitude);
			}
		}
		
		return GravitationCalculator::force(dvec2(0,0),0);
	}


	namespace detail {

		cpBool Stage_collisionBeginHandler(cpArbiter *arb, struct cpSpace *space, cpDataPointer data) {
			Stage *stage = static_cast<Stage*>(data);

			cpShape *a = nullptr, *b = nullptr;
			cpArbiterGetShapes(arb, &a, &b);

			// if any handlers are defined for this pairing, dispatch
			cpBool accept = cpTrue;
			const Stage::collision_type_pair ctp(cpShapeGetCollisionType(a),cpShapeGetCollisionType(b));
			auto pos = stage->_collisionBeginHandlers.find(ctp);
			if (pos != stage->_collisionBeginHandlers.end()) {
				ObjectRef ga = cpShapeGetObject(a);
				ObjectRef gb = cpShapeGetObject(b);
				for (const auto &cb : pos->second) {
					if (!cb(ctp, ga, gb, arb)) {
						accept = cpFalse;
					}
				}
			}

			if (!stage->onCollisionBegin(arb)) {
				accept = cpFalse;
			}

			return accept;
		}

		cpBool Stage_collisionPreSolveHandler(cpArbiter *arb, struct cpSpace *space, cpDataPointer data) {
			Stage *stage = static_cast<Stage*>(data);

			cpShape *a = nullptr, *b = nullptr;
			cpArbiterGetShapes(arb, &a, &b);

			// if any handlers are defined for this pairing, dispatch
			cpBool accept = cpTrue;
			const Stage::collision_type_pair ctp(cpShapeGetCollisionType(a),cpShapeGetCollisionType(b));
			auto pos = stage->_collisionPreSolveHandlers.find(ctp);
			if (pos != stage->_collisionPreSolveHandlers.end()) {
				ObjectRef ga = cpShapeGetObject(a);
				ObjectRef gb = cpShapeGetObject(b);
				for (const auto &cb : pos->second) {
					if (!cb(ctp, ga, gb, arb)) {
						accept = cpFalse;
					}
				}
			}

			if (!stage->onCollisionPreSolve(arb)) {
				accept = cpFalse;
			}

			return accept;
		}

		void Stage_collisionPostSolveHandler(cpArbiter *arb, struct cpSpace *space, cpDataPointer data) {
			Stage *stage = static_cast<Stage*>(data);

			cpShape *a = nullptr, *b = nullptr;
			cpArbiterGetShapes(arb, &a, &b);

			// if any handlers are defined for this pairing, dispatch
			const Stage::collision_type_pair ctp(cpShapeGetCollisionType(a),cpShapeGetCollisionType(b));
			ObjectRef ga = cpShapeGetObject(a);
			ObjectRef gb = cpShapeGetObject(b);

			// dispatch to post solve handlers
			{
				auto pos = stage->_collisionPostSolveHandlers.find(ctp);
				if (pos != stage->_collisionPostSolveHandlers.end()) {
					for (const auto &cb : pos->second) {
						cb(ctp, ga, gb, arb);
					}
				}
			}

			// dispatch to generic contact handlers
			{
				auto pos = stage->_contactHandlers.find(ctp);
				if (pos != stage->_contactHandlers.end()) {
					for (const auto &cb : pos->second) {
						cb(ctp, ga, gb);
					}
				}
			}

			stage->onCollisionPostSolve(arb);
		}

		void Stage_collisionSeparateHandler(cpArbiter *arb, struct cpSpace *space, cpDataPointer data) {
			Stage *stage = static_cast<Stage*>(data);

			cpShape *a = nullptr, *b = nullptr;
			cpArbiterGetShapes(arb, &a, &b);

			// if any handlers are defined for this pairing, dispatch
			const Stage::collision_type_pair ctp(cpShapeGetCollisionType(a),cpShapeGetCollisionType(b));
			auto pos = stage->_collisionSeparateHandlers.find(ctp);
			if (pos != stage->_collisionSeparateHandlers.end()) {
				ObjectRef ga = cpShapeGetObject(a);
				ObjectRef gb = cpShapeGetObject(b);
				for (const auto &cb : pos->second) {
					cb(ctp, ga, gb, arb);
				}
			}

			stage->onCollisionSeparate(arb);
		}

	}

	Stage::collision_type_pair Stage::addCollisionMonitor(cpCollisionType a, cpCollisionType b) {
		collision_type_pair ctp(a,b);
		if (_monitoredCollisions.insert(ctp).second) {
			cpSpace * space = getSpace()->getSpace();
			cpCollisionHandler *handler = cpSpaceAddCollisionHandler(space, a, b);
			handler->userData = this;
			handler->beginFunc = detail::Stage_collisionBeginHandler;
			handler->preSolveFunc = detail::Stage_collisionPreSolveHandler;
			handler->postSolveFunc = detail::Stage_collisionPostSolveHandler;
			handler->separateFunc = detail::Stage_collisionSeparateHandler;
		}
		return ctp;
	}

	void Stage::addCollisionBeginHandler(cpCollisionType a, cpCollisionType b, EarlyCollisionCallback cb) {
		const auto ctp = addCollisionMonitor(a,b);
		_collisionBeginHandlers[ctp].push_back(cb);
	}

	void Stage::addCollisionPreSolveHandler(cpCollisionType a, cpCollisionType b, EarlyCollisionCallback cb) {
		const auto ctp = addCollisionMonitor(a,b);
		_collisionPreSolveHandlers[ctp].push_back(cb);
	}

	void Stage::addCollisionPostSolveHandler(cpCollisionType a, cpCollisionType b, LateCollisionCallback cb) {
		const auto ctp = addCollisionMonitor(a,b);
		_collisionPostSolveHandlers[ctp].push_back(cb);
	}

	void Stage::addCollisionSeparateHandler(cpCollisionType a, cpCollisionType b, LateCollisionCallback cb) {
		const auto ctp = addCollisionMonitor(a,b);
		_collisionSeparateHandlers[ctp].push_back(cb);
	}

	void Stage::addContactHandler(cpCollisionType a, cpCollisionType b, ContactCallback cb) {
		const auto ctp = addCollisionMonitor(a,b);
		_contactHandlers[ctp].push_back(cb);
	}
	
	void Stage::registerContactBetweenObjects(cpCollisionType a, const ObjectRef &ga, cpCollisionType b, const ObjectRef &gb) {
		collision_type_pair ctp(a, b);
		_syntheticContacts[ctp].push_back(std::make_pair(ga, gb));
	}
	
	Stage::query_nearest_result Stage::queryNearest(dvec2 point, cpShapeFilter filter, double maxDistance) {
		query_nearest_result result;
		
		cpPointQueryInfo pointQueryInfo;
		cpShape *shape = cpSpacePointQueryNearest(_space, cpv(point), maxDistance, filter, &pointQueryInfo);
		if (shape) {
			result.point = v2(pointQueryInfo.point);
			result.distance = pointQueryInfo.distance;
			result.shape = shape;
			result.body = cpShapeGetBody(shape);
			result.object = cpShapeGetObject(shape);
		}
		
		return result;
	}
	
	Stage::query_segment_result Stage::querySegment(dvec2 a, dvec2 b, double radius, cpShapeFilter filter) {
		query_segment_result result;
		cpSegmentQueryInfo info;
		cpShape *shape = cpSpaceSegmentQueryFirst(_space, cpv(a), cpv(b), radius, filter, &info);
		if (shape) {
			result.point = v2(info.point);
			result.normal = v2(info.normal);
			result.shape = shape;
			result.body = cpShapeGetBody(shape);
			result.object = cpShapeGetObject(shape);
		}
		return result;
	}

	void Stage::setCpBodyVelocityUpdateFunc(cpBodyVelocityFunc f) {
		_bodyVelocityFunc = f ? f : cpBodyUpdateVelocity;

		// update every body in sim
		cpSpaceEachBody(_space, [](cpBody *body, void *data){
			auto l = static_cast<Stage*>(data);
			cpBodySetVelocityUpdateFunc(body, l->_bodyVelocityFunc);
		}, this);
	}

	void Stage::addedToScenario(ScenarioRef scenario) {
		_scenario = scenario;
	}

	void Stage::removeFromScenario() {
		_scenario.reset();
	}

	void Stage::onReady() {
		CI_ASSERT_MSG(!_ready, "Can't call onReady() on Stage that is already ready");

		const auto self = shared_from_this_as<Stage>();
		for (auto &obj : _objects) {
			obj->onReady(self);
		}
		_ready = true;
	}

	void Stage::onBodyAddedToSpace(cpBody *body) {
		//CI_LOG_D("onBodyAddedToSpace body: " << body);
		cpBodySetVelocityUpdateFunc(body, getCpBodyVelocityUpdateFunc());
	}

	void Stage::onShapeAddedToSpace(cpShape *shape) {
		//CI_LOG_D("onShapeAddedToSpace shape: " << shape);
	}

	void Stage::onConstraintAddedToSpace(cpConstraint *constraint) {
		//CI_LOG_D("onConstraintAddedToSpace constraint: " << constraint);
	}

	void Stage::dispatchSyntheticContacts() {
		for (auto &group : _syntheticContacts) {
			const auto ctp = group.first;
			const auto pos = _contactHandlers.find(ctp);
			if (pos != _contactHandlers.end()) {
				for (const auto &handler : pos->second) {
					for (auto &objectPair : group.second) {
						handler(ctp, objectPair.first, objectPair.second);
					}
				}
			}
		}
		_syntheticContacts.clear();
	}
}
