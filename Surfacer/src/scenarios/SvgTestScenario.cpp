//
//  SvgTestScenario.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/6/17.
//
//

#include "SvgTestScenario.hpp"
#include "GameApp.hpp"
#include "Strings.hpp"

namespace {

	class WorldCoordinateSystemDrawComponent : public DrawComponent {
	public:

		WorldCoordinateSystemDrawComponent(int gridSize = 10):
		_gridSize(gridSize){}

		void onReady(GameObjectRef parent, LevelRef level) override{
			DrawComponent::onReady(parent, level);
		}

		cpBB getBB() const override {
			return cpBBInfinity;
		}

		void draw(const core::render_state &state) override {
			const cpBB frustum = state.viewport->getFrustum();
			const int gridSize = _gridSize;
			const int majorGridSize = _gridSize * 10;
			const int firstY = static_cast<int>(floor(frustum.b / gridSize) * gridSize);
			const int lastY = static_cast<int>(floor(frustum.t / gridSize) * gridSize) + gridSize;
			const int firstX = static_cast<int>(floor(frustum.l / gridSize) * gridSize);
			const int lastX = static_cast<int>(floor(frustum.r / gridSize) * gridSize) + gridSize;

			const auto MinorLineColor = ColorA(1,1,1,0.05);
			const auto MajorLineColor = ColorA(1,1,1,0.25);
			const auto AxisColor = ColorA(1,0,0,1);
			gl::lineWidth(1.0 / state.viewport->getScale());

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

		VisibilityDetermination::style getVisibilityDetermination() const override {
			return VisibilityDetermination::ALWAYS_DRAW;
		}

		int getLayer() const override { return -1; }

	private:

		int _gridSize;

	};

	class SvgDrawComponent : public DrawComponent {
	public:
		SvgDrawComponent(util::svg::GroupRef doc):
		_docRoot(doc)
		{}

		cpBB getBB() const override {
			return _docRoot->getBB();
		}

		void draw(const core::render_state &state) override {
			_docRoot->draw(state);
		}

		VisibilityDetermination::style getVisibilityDetermination() const override {
			return VisibilityDetermination::FRUSTUM_CULLING;
		}

		int getLayer() const override { return 0; }

	private:
		util::svg::GroupRef _docRoot;
	};

}

SvgTestScenario::SvgTestScenario() {
}

SvgTestScenario::~SvgTestScenario() {

}

void SvgTestScenario::setup() {
	auto vsh = CI_GLSL(150,
					   uniform mat4 ciModelViewProjection;

					   in vec4 ciPosition;

					   void main(void){
						   gl_Position = ciModelViewProjection * ciPosition;
					   }
					   );

	auto fsh = CI_GLSL(150,
					   out vec4 oColor;

					   void main(void) {
						   oColor = vec4(1,0,1,1);
					   }
					   );


	_shader = gl::GlslProg::create(gl::GlslProg::Format().vertex(vsh).fragment(fsh));



	setLevel(make_shared<Level>("Hello Svg"));

	// add background renderer
	auto background = GameObject::with("Background", {
		make_shared<WorldCoordinateSystemDrawComponent>()
	});

	getLevel()->addGameObject(background);

	//testSimpleSvgLoad();
	testSimpleSvgGroupOriginTransforms();
}

void SvgTestScenario::cleanup() {
	setLevel(nullptr);
}

void SvgTestScenario::resize( ivec2 size ) {

}

void SvgTestScenario::step( const time_state &time ) {

}

void SvgTestScenario::update( const time_state &time ) {
}

void SvgTestScenario::clear( const render_state &state ) {
	gl::clear( Color( 0.2, 0.2, 0.2 ) );
}

void SvgTestScenario::draw( const render_state &state ) {

	//
	// NOTE: we're in screen space, with coordinate system origin at top left
	//

	float fps = core::GameApp::get()->getAverageFps();
	float sps = core::GameApp::get()->getAverageSps();
	string info = core::strings::format("%.1f %.1f", fps, sps);
	gl::drawString(info, vec2(10,10), Color(1,1,1));

}

bool SvgTestScenario::mouseDown( const ci::app::MouseEvent &event ) {
	_mouseScreen = event.getPos();
	_mouseWorld = getViewport()->screenToWorld(_mouseScreen);

	if ( event.isAltDown() )
	{
		getViewportController()->lookAt( _mouseWorld );
		return true;
	}

	if ( isKeyDown( app::KeyEvent::KEY_SPACE )) {
		return false;
	}

	return true;
}

bool SvgTestScenario::mouseUp( const ci::app::MouseEvent &event ) {
	return false;
}

bool SvgTestScenario::mouseWheel( const ci::app::MouseEvent &event ) {
	float zoom = getViewportController()->getScale(),
	wheelScale = 0.1 * zoom,
	dz = (event.getWheelIncrement() * wheelScale);

	getViewportController()->setScale( std::max( zoom + dz, 0.1f ), event.getPos() );

	return true;
}

bool SvgTestScenario::mouseMove( const ci::app::MouseEvent &event, const ivec2 &delta ) {
	_mouseScreen = event.getPos();
	_mouseWorld = getViewport()->screenToWorld(_mouseScreen);
	return true;
}

bool SvgTestScenario::mouseDrag( const ci::app::MouseEvent &event, const ivec2 &delta ) {
	_mouseScreen = event.getPos();
	_mouseWorld = getViewport()->screenToWorld(_mouseScreen);

	if ( isKeyDown( app::KeyEvent::KEY_SPACE ))
	{
		getViewportController()->setPan( getViewportController()->getPan() + dvec2(delta) );
		return true;
	}

	return true;
}

bool SvgTestScenario::keyDown( const ci::app::KeyEvent &event ) {
	if (event.getChar() == 'r') {
		reset();
		return true;
	} else if (event.getCode() == ci::app::KeyEvent::KEY_SPACE) {

		return true;
	} else if (event.getCode() == ci::app::KeyEvent::KEY_BACKQUOTE) {
		setRenderMode( RenderMode::mode( (int(getRenderMode()) + 1) % RenderMode::COUNT ));
	}
	return false;
}

bool SvgTestScenario::keyUp( const ci::app::KeyEvent &event ) {
	return false;
}

void SvgTestScenario::reset() {
	cleanup();
	setup();
}

#pragma mark - Tests

void SvgTestScenario::testSimpleSvgLoad() {
	auto doc = util::svg::Group::loadSvgDocument(app::loadAsset("svg_tests/group_origin_test.svg"), 1);
	doc->trace();
	getLevel()->addGameObject(GameObject::with("Hello SVG", { make_shared<SvgDrawComponent>(doc)}));
}

void SvgTestScenario::testSimpleSvgGroupOriginTransforms() {
	auto doc = util::svg::Group::loadSvgDocument(app::loadAsset("svg_tests/group_origin_test.svg"), 1);
	auto root = doc->find("Page-1/group_origin_test/root");
	auto green = root->find("green");
	auto green1 = green->find("green-1");
	auto green2 = green->find("green-2");
	auto purple = root->find("purple");
	auto purple1 = purple->find("purple-1");
	auto purple2 = purple->find("purple-2");


	class Wiggler : public core::Component {
	public:
		Wiggler(vector<util::svg::GroupRef> elements):
		_elements(elements)
		{}

		void step(const time_state &time) override {
			double cycle = cos(time.time) * 0.75;
			int dir = 0;
			for (auto group : _elements) {
				group->setAngle(cycle * (dir % 2 == 0 ? 1 : -1));
				dir++;
			}
		}

	private:
		vector<util::svg::GroupRef> _elements;
	};


	auto drawComponent = make_shared<SvgDrawComponent>(doc);
	vector<util::svg::GroupRef> elements = { root, green, purple, green1, green2, purple1, purple2 };
	auto wiggleComponent = make_shared<Wiggler>(elements);

	auto obj = GameObject::with("Hello SVG", { drawComponent, wiggleComponent });
	getLevel()->addGameObject(obj);
}
