//
//  TerrainTestScenario.cpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 3/5/17.
//
//

#include "TerrainTestScenario.hpp"
#include "GameApp.hpp"

#include <cinder/Rand.h>

#include "Strings.hpp"

namespace {

	const float CUT_WIDTH = 4;

	PolyLine2f rect(float left, float bottom, float right, float top) {
		PolyLine2f pl;
		pl.push_back(vec2(left,bottom));
		pl.push_back(vec2(right,bottom));
		pl.push_back(vec2(right,top));
		pl.push_back(vec2(left,top));
		pl.setClosed();
		return pl;
	}

	PolyLine2f rect(vec2 center, vec2 size) {
		return rect(center.x - size.x/2, center.y - size.y/2, center.x + size.x/2, center.y + size.y/2);
	}

	terrain::ShapeRef boxShape(vec2 center, vec2 size) {
		return terrain::Shape::fromContour(rect(center.x - size.x/2, center.y - size.y/2, center.x + size.x/2, center.y + size.y/2));
	}

	struct cut {
		vec2 a,b;
		float radius;

		cut(vec2 A, vec2 B, float R):
		a(A),
		b(B),
		radius(R)
		{}

		cut(string desc) {
			desc = core::strings::removeSet(desc, "\t[]toradius:,");

			const char* it = desc.c_str();
			char* end = nullptr;
			int idx = 0;
			for (float f = strtod(it,&end);; f = strtod(it, &end)) {
				if (it == end) {
					break;
				}

				switch(idx) {
					case 0: a.x = f; break;
					case 1: a.y = f; break;
					case 2: b.x = f; break;
					case 3: b.y = f; break;
					case 4: radius = f; break;
				}

				it = end;
				idx++;
			}
		}

		static vector<cut> parse(string cutsDesc) {
			auto cutDescs = core::strings::split(cutsDesc, "\n");
			vector<cut> cuts;
			for (auto cutDesc : cutDescs) {
				cutDesc = strings::strip(cutDesc);
				if (!cutDesc.empty()) {
					cuts.emplace_back(cutDesc);
				}
			}
			return cuts;
		}

	};


	namespace Categories {
		enum Categories {
			TERRAIN = 1 << 30,
			CUTTER = 1 << 29,
			PICK = 1 << 28,
			ANCHOR = 1 << 27
		};
	};

	namespace Filters {
		cpShapeFilter TERRAIN = cpShapeFilterNew(CP_NO_GROUP, Categories::TERRAIN, Categories::TERRAIN | Categories::CUTTER | Categories::PICK | Categories::ANCHOR);
		cpShapeFilter ANCHOR = cpShapeFilterNew(CP_NO_GROUP, Categories::ANCHOR, Categories::TERRAIN);
		cpShapeFilter CUTTER = cpShapeFilterNew(CP_NO_GROUP, Categories::CUTTER, Categories::TERRAIN);
		cpShapeFilter PICK = cpShapeFilterNew(CP_NO_GROUP, Categories::PICK, Categories::TERRAIN);
	}


}

/*
	bool _cutting;
	vec2 _cutterStart, _cutterEnd, _mouseScreen, _mouseWorld;

	ViewportController _cameraController;
 
	terrain::WorldRef _terrainWorld;

	cpSpace *_space;
	cpBody *_mouseBody, *_draggingBody;
	cpConstraint *_mouseJoint;
*/

TerrainTestScenario::TerrainTestScenario():
_cameraController(getCamera()),
_cutting(false),
_space(nullptr),
_mouseBody(nullptr),
_draggingBody(nullptr),
_mouseJoint(nullptr)
{
	_cameraController.setConstraintMask( ViewportController::ConstrainPan | ViewportController::ConstrainScale );
}

TerrainTestScenario::~TerrainTestScenario(){}

void TerrainTestScenario::setup() {

	// go debug for rendering
	setRenderMode(RenderMode::DEVELOPMENT);

	//timeSpatialIndex();

	_space = cpSpaceNew();
	cpSpaceSetDamping(_space, 0.95);
	_mouseBody = cpBodyNewKinematic();

	//testBasicTerrainSetup();
	//testComplexTerrainSetup();
	//testSimpleAnchors();
	//testComplexAnchors();
	//testSimplePartitionedTerrain();
	//exploitGroupingBug();

	//testComplexPartitionedTerrainWithAnchors();
	testFail();
}

void TerrainTestScenario::cleanup() {
	_terrainWorld.reset();

	_cutting = false;

	if (_mouseJoint) {
		cpSpaceRemoveConstraint(_space, _mouseJoint);
		cpConstraintFree(_mouseJoint);
		_mouseJoint = nullptr;
		_draggingBody = nullptr;
	}

	cpBodyFree(_mouseBody);
	cpSpaceFree(_space);
}

void TerrainTestScenario::resize( ivec2 size ){}

void TerrainTestScenario::step( const time_state &time ) {
	cpSpaceStep(_space, time.deltaT);

	cpVect mouseBodyPos = cpv(_mouseWorld);
	cpVect newMouseBodyPos = cpvlerp(cpBodyGetPosition(_mouseBody), mouseBodyPos, 0.75);

	cpBodySetVelocity(_mouseBody, cpvmult(cpvsub(newMouseBodyPos, cpBodyGetPosition(_mouseBody)), time.deltaT));
	cpBodySetPosition(_mouseBody, newMouseBodyPos);

	if (_terrainWorld) {
		_terrainWorld->step(time);
	}
}

void TerrainTestScenario::update( const time_state &time ) {
	_cameraController.step(time);
	if (_terrainWorld) {
		_terrainWorld->update(time);
	}
}

void TerrainTestScenario::draw( const render_state &state ) {
	//const ivec2 screenSize = getWindowSize();
	gl::clear( Color( 0.2, 0.2, 0.2 ) );

	{
		// apply camera modelview
		Viewport::ScopedState cameraState(getCamera());

		drawWorldCoordinateSystem(state);

		if (_terrainWorld) {
			_terrainWorld->draw(state);
		}

		if (_cutting) {
			float radius = (CUT_WIDTH/2) / state.viewport.getScale();
			vec2 dir = _cutterEnd - _cutterStart;
			float len = ::length(dir);
			if (len > 1e-2) {
				dir /= len;
				float angle = atan2(dir.y, dir.x);
				vec2 center = (_cutterStart+_cutterEnd)/2.f;

				mat4 M = translate(vec3(center.x, center.y, 0)) * rotate(angle, vec3(0,0,1));
				gl::ScopedModelMatrix smm;
				gl::multModelMatrix(M);

				gl::color(Color(1,0,1));
				gl::drawSolidRect(Rectf(-len/2, -radius, +len/2, +radius));
			}
		}

		if (_mouseJoint) {
			// we're dragging, so draw mouse body
			const float radius = 2 * getCamera().getReciprocalScale();
			gl::color(Color(1,0,1));
			gl::drawSolidCircle(v2(cpBodyGetPosition(_mouseBody)), radius);
		}
	}

	// now we're in screen space


	// draw fpf/sps
	float fps = core::GameApp::get()->getAverageFps();
	float sps = core::GameApp::get()->getAverageSps();
	string info = core::strings::format("%.1f %.1f", fps, sps);
	gl::drawString(info, vec2(10,10), Color(1,1,1));
}

bool TerrainTestScenario::mouseDown( const ci::app::MouseEvent &event ) {
	releaseMouseDragConstraint();

	_mouseScreen = event.getPos();
	_mouseWorld = getCamera().screenToWorld(_mouseScreen);

	if ( event.isAltDown() )
	{
		_cameraController.lookAt( _mouseWorld );
		return true;
	}

	if ( isKeyDown( app::KeyEvent::KEY_SPACE )) {
		return false;
	}

	const float distance = 1.f;
	cpPointQueryInfo info = {};
	cpShape *pick = cpSpacePointQueryNearest(_space, cpv(_mouseWorld), distance, Filters::PICK, &info);
	if (pick) {
		cpBody *pickBody = cpShapeGetBody(pick);

		if (pickBody && cpBodyGetType(pickBody) == CP_BODY_TYPE_DYNAMIC) {
			cpVect nearest = (info.distance > 0.0f ? info.point : cpv(_mouseWorld));

			_draggingBody = pickBody;
			_mouseJoint = cpPivotJointNew2(_mouseBody, _draggingBody, cpvzero, cpBodyWorldToLocal(_draggingBody,nearest));

			cpSpaceAddConstraint(_space, _mouseJoint);

			return true;
		}
	}

	_cutting = true;
	_cutterStart = _cutterEnd = _mouseWorld;

	return true;
}

bool TerrainTestScenario::mouseUp( const ci::app::MouseEvent &event ) {
	releaseMouseDragConstraint();

	if (_cutting) {
		if (_terrainWorld) {
			const float radius = (CUT_WIDTH/2) / _cameraController.getScale();
			_terrainWorld->cut(_cutterStart, _cutterEnd, radius, Filters::CUTTER);
		}

		_cutting = false;
	}

	return true;
}

bool TerrainTestScenario::mouseWheel( const ci::app::MouseEvent &event ){
	float zoom = _cameraController.getScale(),
	wheelScale = 0.1 * zoom,
	dz = (event.getWheelIncrement() * wheelScale);

	_cameraController.setScale( std::max( zoom + dz, 0.1f ), event.getPos() );

	return true;
}

bool TerrainTestScenario::mouseMove( const ci::app::MouseEvent &event, const vec2 &delta ) {
	_mouseScreen = event.getPos();
	_mouseWorld = getCamera().screenToWorld(_mouseScreen);
	return true;
}

bool TerrainTestScenario::mouseDrag( const ci::app::MouseEvent &event, const vec2 &delta ) {
	_mouseScreen = event.getPos();
	_mouseWorld = getCamera().screenToWorld(_mouseScreen);

	if ( isKeyDown( app::KeyEvent::KEY_SPACE ))
	{
		_cameraController.setPan( _cameraController.getPan() + delta );
		_cutting = false;
		return true;
	}


	if (_cutting) {
		_cutterEnd = _mouseWorld;
	}

	return true;
}

bool TerrainTestScenario::keyDown( const ci::app::KeyEvent &event ) {
	if (event.getChar() == 'r') {
		reset();
		return true;
	} else if (event.getCode() == ci::app::KeyEvent::KEY_SPACE) {
		//testExplicitCut();
		return true;
	} else if (event.getCode() == ci::app::KeyEvent::KEY_BACKQUOTE) {
		setRenderMode( RenderMode::mode( (int(getRenderMode()) + 1) % RenderMode::COUNT ));
	}
	return false;
}

bool TerrainTestScenario::keyUp( const ci::app::KeyEvent &event ) {
	return false;
}

void TerrainTestScenario::reset() {
	cleanup();
	setup();
}

void TerrainTestScenario::testBasicTerrainSetup() {
	_cameraController.lookAt(vec2(0,0));

	terrain::material terrainMaterial(1, 0.5, Filters::TERRAIN);

	_terrainWorld = make_shared<terrain::World>(_space,terrainMaterial);

	vector<terrain::ShapeRef> shapes = {
		terrain::Shape::fromContour(rect(0, 0, 100, 50)),		// 0
		terrain::Shape::fromContour(rect(100,0, 150, 50)),		// 1 to right of 0 - binds to 0
		terrain::Shape::fromContour(rect(-100,0, 0, 50)),		// 2 to left of 0 - binds to 0
		terrain::Shape::fromContour(rect(-10, 50, 110, 100)),	// 3 above 0, but wider
		terrain::Shape::fromContour(rect(-10, 100, 110, 200)), // 4 above 3, binds to 3

		//terrain::Shape::fromContour(rect(200,0,210,0)),		// empty rect - should be garbage collected
	};

	_terrainWorld->build(shapes);
}

void TerrainTestScenario::testComplexTerrainSetup() {
	_cameraController.lookAt(vec2(0,0));

	terrain::material terrainMaterial(1, 0.5, Filters::TERRAIN);
	terrain::material anchorMaterial(1, 1, Filters::ANCHOR);

	_terrainWorld = make_shared<terrain::World>(_space,terrainMaterial);

	const vec2 boxSize(50,50);
	auto boxPos = [boxSize](float x, float y)->vec2 {
		return vec2(x * boxSize.x + boxSize.x/2,y * boxSize.y + boxSize.y/2);
	};

	vector<terrain::ShapeRef> shapes = {
		boxShape(boxPos(0,0),boxSize),
		boxShape(boxPos(1,0),boxSize),
		boxShape(boxPos(2,0),boxSize),
		boxShape(boxPos(3,0),boxSize),
		boxShape(boxPos(3,1),boxSize),
		boxShape(boxPos(3,2),boxSize),
		boxShape(boxPos(3,3),boxSize),
		boxShape(boxPos(2,3),boxSize),
		boxShape(boxPos(1,3),boxSize),
		boxShape(boxPos(0,3),boxSize),
		boxShape(boxPos(0,2),boxSize),
		boxShape(boxPos(0,1),boxSize),
	};

	_terrainWorld->build(shapes);
}

void TerrainTestScenario::testSimpleAnchors() {
	_cameraController.lookAt(vec2(0,0));

	terrain::material terrainMaterial(1, 0.5, Filters::TERRAIN);
	terrain::material anchorMaterial(1, 1, Filters::ANCHOR);

	_terrainWorld = make_shared<terrain::World>(_space,terrainMaterial);

	vector<terrain::ShapeRef> shapes = {
		terrain::Shape::fromContour(rect(0, 0, 100, 50)),		// 0
		terrain::Shape::fromContour(rect(100,0, 150, 50)),		// 1 to right of 0 - binds to 0
		terrain::Shape::fromContour(rect(-100,0, 0, 50)),		// 2 to left of 0 - binds to 0
	};

	vector<terrain::AnchorRef> anchors = {
		terrain::Anchor::fromContour(rect(40,20,60,30), anchorMaterial),
		terrain::Anchor::fromContour(rect(200,20,260,30), anchorMaterial)
	};

	_terrainWorld->build(shapes, anchors);
}

void TerrainTestScenario::testComplexAnchors() {
	_cameraController.lookAt(vec2(0,0));

	terrain::material terrainMaterial(1, 0.5, Filters::TERRAIN);
	terrain::material anchorMaterial(1, 1, Filters::ANCHOR);

	cpSpaceSetGravity(_space, cpv(0,-9.8 * 10));
	_terrainWorld = make_shared<terrain::World>(_space,terrainMaterial);

	const vec2 boxSize(50,50);
	auto boxPos = [boxSize](float x, float y)->vec2 {
		return vec2(x * boxSize.x + boxSize.x/2,y * boxSize.y + boxSize.y/2);
	};

	vector<terrain::ShapeRef> shapes = {
		boxShape(boxPos(0,0),boxSize),
		boxShape(boxPos(1,0),boxSize),
		boxShape(boxPos(2,0),boxSize),
		boxShape(boxPos(3,0),boxSize),
		boxShape(boxPos(3,1),boxSize),
		boxShape(boxPos(3,2),boxSize),
		boxShape(boxPos(3,3),boxSize),
		boxShape(boxPos(2,3),boxSize),
		boxShape(boxPos(1,3),boxSize),
		boxShape(boxPos(0,3),boxSize),
		boxShape(boxPos(0,2),boxSize),
		boxShape(boxPos(0,1),boxSize),

		boxShape(boxSize * 2.f, boxSize)
	};

	vector<terrain::AnchorRef> anchors = {
		terrain::Anchor::fromContour(rect(vec2(25,25), vec2(10,10)), anchorMaterial)
	};

	_terrainWorld->build(shapes, anchors);
}

void TerrainTestScenario::testSimplePartitionedTerrain() {

	ci::Rand rng;

	auto ring = [&rng](vec2 center, float radius, int subdivisions, float wobbleRange) -> PolyLine2f {
		PolyLine2f polyLine;
		for (int i = 0; i < subdivisions; i++) {
			float j = static_cast<float>(i)/static_cast<float>(subdivisions);
			float r = j * M_PI * 2;
			vec2 p = center + (vec2(cos(r), sin(r)) * (radius + rng.nextFloat(-wobbleRange, +wobbleRange)));
			polyLine.push_back(p);
		}
		polyLine.setClosed();
		return polyLine;
	};

	_cameraController.lookAt(vec2(0,0));

	const auto terrainMaterial = terrain::material(1, 0.5, Filters::TERRAIN);
	_terrainWorld = make_shared<terrain::World>(_space,terrainMaterial);

	auto rings = vector<PolyLine2f> {
		ring(vec2(0,0), 500, 600, 0),
		ring(vec2(0,0), 400, 600, 0)
	};

	auto shapes = terrain::Shape::fromContours(rings);
	//auto shapes = vector<terrain::ShapeRef> { boxShape(vec2(0,0), vec2(500,500)) };

	auto partitionedShapes = terrain::World::partition(shapes, vec2(0,0), 130);

	_terrainWorld->build(partitionedShapes);
}

void TerrainTestScenario::testComplexPartitionedTerrainWithAnchors() {

	//cpSpaceSetGravity(_space, cpv(0,-9.8 * 10));

	ci::Rand rng;

	auto ring = [&rng](vec2 center, float radius, int subdivisions, float wobbleRange) -> PolyLine2f {
		PolyLine2f polyLine;
		for (int i = 0; i < subdivisions; i++) {
			float j = static_cast<float>(i)/static_cast<float>(subdivisions);
			float r = j * M_PI * 2;
			vec2 p = center + (vec2(cos(r), sin(r)) * (radius + rng.nextFloat(-wobbleRange, +wobbleRange)));
			polyLine.push_back(p);
		}
		polyLine.setClosed();
		return polyLine;
	};

	_cameraController.lookAt(vec2(0,0));

	const auto terrainMaterial = terrain::material(1, 0.5, Filters::TERRAIN);
	terrain::material anchorMaterial(1, 1, Filters::ANCHOR);

	_terrainWorld = make_shared<terrain::World>(_space,terrainMaterial);

	auto rings = vector<PolyLine2f> {
		ring(vec2(0,0), 500, 600, 0),
		ring(vec2(0,0), 400, 600, 0)
	};

	auto shapes = terrain::Shape::fromContours(rings);
	auto partitionedShapes = terrain::World::partition(shapes, vec2(0,0), 130);

	vector<terrain::AnchorRef> anchors = {
		terrain::Anchor::fromContour(rect(vec2(0,-450), vec2(10,10)), anchorMaterial)
	};

	_terrainWorld->build(partitionedShapes, anchors);

}

void TerrainTestScenario::testFail() {
	testComplexPartitionedTerrainWithAnchors();

	// this sequence of cuts causes trouble
	string cutDescriptions = R"(
	[ -155.994,   84.944] to [ -483.285,  336.033] radius: 2.49663
	[ -269.808,  242.250] to [ -417.349,  337.285] radius: 1.04977
	[ -282.409,  262.727] to [ -387.420,  333.609] radius: 1.04977
	[ -273.484,  267.452] to [ -384.795,  360.387] radius: 1.04977
	[ -258.257,  279.004] to [ -382.695,  379.289] radius: 1.04977
	[ -238.839,  283.203] to [ -354.876,  391.365] radius: 1.04977
	[ -242.515,  270.077] to [ -318.648,  404.491] radius: 1.04977
	[ -244.090,  291.604] to [ -319.173,  461.722] radius: 1.04977
	[ -229.388,  295.805] to [ -280.319,  426.018] radius: 1.04977
	[ -219.937,  313.656] to [ -261.417,  435.994] radius: 1.04977
	[ -217.312,  325.208] to [ -256.166,  439.145] radius: 1.04977
	[ -209.961,  324.158] to [ -250.916,  442.295] radius: 1.04977
	[ -204.710,  325.727] to [ -245.139,  445.965] radius: 1.04977
	[ -201.462,  334.242] to [ -238.086,  443.261] radius: 0.852035
	[ -197.629,  340.204] to [ -232.550,  456.889] radius: 0.852035
	[ -189.538,  335.093] to [ -221.903,  460.722] radius: 0.852035
	[ -333.052,  208.613] to [ -432.703,  278.028] radius: 0.852035
	[ -339.440,  203.503] to [ -448.034,  272.492] radius: 0.852035
	)";

	auto cuts = cut::parse(cutDescriptions);

	// we know the last cut causes weirdness so cut all but the last
	for (int i = 0; i < cuts.size() - 1; i++) {
		_terrainWorld->cut(cuts[i].a, cuts[i].b, cuts[i].radius, Filters::CUTTER);
	}

	cut lastCut = cuts[cuts.size()-1];
	_terrainWorld->cut(lastCut.a, lastCut.b, lastCut.radius, Filters::CUTTER);


//	// this leaves shape 31 improperly cut - there's a tiny tab on lower left which should be excised but isn't
//	_terrainWorld->cut(vec2(-339.440,  203.503), vec2(-448.034,  272.492), 0.852035, Filters::CUTTER);
}

void TerrainTestScenario::timeSpatialIndex() {

	ci::Rand rng;

	auto generator = [&rng](cpBB bounds, int count) -> vector<cpBB> {
		vector<cpBB> out;

		const float sizeMin = (bounds.r - bounds.l) / 50;
		const float sizeMax = sizeMin * 2;

		for (int i = 0; i < count; i++) {
			vec2 origin = vec2(rng.nextFloat(bounds.l, bounds.r), rng.nextFloat(bounds.b, bounds.t));
			float size = rng.nextFloat(sizeMin, sizeMax);

			cpBB bb = cpBBNew(origin.x-size/2, origin.y-size/2, origin.x+size/2, origin.y+size/2);
			out.push_back(bb);
		}

		return out;
	};

	auto timeUsingSpatialIndex = [](const vector<cpBB> bbs, core::SpatialIndex<int> &index) -> double {
		core::StopWatch timer;
		index.clear();

		int tick = 0;
		for (auto bb : bbs) {
			index.insert(bb, tick++);
		}

		int count = 0;

		for (auto queryBB : bbs) {
			auto neighbors = index.sweep(queryBB);
			count += neighbors.size();
		}

		return timer.mark();
	};

	auto timeUsingBruteForce = [](const vector<cpBB> bbs) -> double {
		core::StopWatch timer;

		int count = 0;
		const auto last = bbs.end();
		for (auto it = bbs.begin(); it != last; ++it) {
			for (auto it2 = bbs.begin(); it2 != last; ++it2) {
				if (it != it2 && cpBBIntersects(*it, *it2)) {
					count ++;
				}
			}
		}

		double m = timer.mark();
		app::console() << "timeUsingBruteForce count: " << count << std::endl;
		return m;
	};

	auto performTimingRun = [&generator, &timeUsingSpatialIndex, &timeUsingBruteForce](int count) {
		const cpBB bounds = cpBBNew(-1000, -1000, 1000, 10000);
		vector<cpBB> bbs = generator(bounds, count);
		core::SpatialIndex<int> index(50);
		const int runs = 10;

		double sumSpatialIndex = 0;
		double sumBruteForce = 0;

		for (int i = 0; i < runs; i++) {
			sumSpatialIndex += timeUsingSpatialIndex(bbs, index);
			sumBruteForce += timeUsingBruteForce(bbs);
		}

		const double spatialIndexAverageTime = sumSpatialIndex / runs;
		const double bruteForceAverageTime = sumBruteForce / runs;

		app::console() << "For " << runs << " runs over " << count << " items:" << endl;
		app::console() << "\tspatialIndex average time: " << spatialIndexAverageTime << " seconds" << endl;
		app::console() << "\tbruteForce average time: " << bruteForceAverageTime << " seconds" << endl;
		app::console() << "\tratio spatialIndex/bruteForce: " << (spatialIndexAverageTime/bruteForceAverageTime) << endl;
		app::console() << endl << endl;
	};

	app::console() << "------------------------------------" << endl << "PERFORMING PERF MEASUREMENTS" << endl;
	performTimingRun(450);
}

void TerrainTestScenario::drawWorldCoordinateSystem(const render_state &state) {

	const cpBB frustum = state.viewport.getFrustum();
	const int gridSize = 10;
	const int majorGridSize = 100;
	const int firstY = static_cast<int>(floor(frustum.b / gridSize) * gridSize);
	const int lastY = static_cast<int>(floor(frustum.t / gridSize) * gridSize) + gridSize;
	const int firstX = static_cast<int>(floor(frustum.l / gridSize) * gridSize);
	const int lastX = static_cast<int>(floor(frustum.r / gridSize) * gridSize) + gridSize;

	const auto MinorLineColor = ColorA(1,1,1,0.05);
	const auto MajorLineColor = ColorA(1,1,1,0.25);
	const auto AxisColor = ColorA(1,0,0,1);
	gl::lineWidth(1.0 / state.viewport.getScale());

	for (int y = firstY; y <= lastY; y+= gridSize) {
		if (y == 0) {
			gl::color(AxisColor);
		} else if ( y % majorGridSize == 0) {
			gl::color(MajorLineColor);
		} else {
			gl::color(MinorLineColor);
		}
		gl::drawLine(vec2(firstX, y), vec2(lastX, y));
	}

	for (int x = firstX; x <= lastX; x+= gridSize) {
		if (x == 0) {
			gl::color(AxisColor);
		} else if ( x % majorGridSize == 0) {
			gl::color(MajorLineColor);
		} else {
			gl::color(MinorLineColor);
		}
		gl::drawLine(vec2(x, firstY), vec2(x, lastY));
	}

}

void TerrainTestScenario::releaseMouseDragConstraint() {
	if (_mouseJoint) {
		cpSpaceRemoveConstraint(_space, _mouseJoint);
		cpConstraintFree(_mouseJoint);
		_mouseJoint = nullptr;
		_draggingBody = nullptr;
	}
}
