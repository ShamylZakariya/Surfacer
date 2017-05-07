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

#include "DevComponents.hpp"

namespace {

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
	setLevel(make_shared<Level>("Hello Svg"));

	auto cameraController = GameObject::with("ViewportControlComponent", {
		make_shared<ManualViewportControlComponent>(getViewportController())
	});

	auto grid = GameObject::with("Grid", { WorldCartesianGridDrawComponent::create(1) });

	getLevel()->addGameObject(grid);
	getLevel()->addGameObject(cameraController);

	testSimpleSvgLoad();
	//testSimpleSvgGroupOriginTransforms();
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

void SvgTestScenario::drawScreen( const render_state &state ) {

	//
	// NOTE: we're in screen space, with coordinate system origin at top left
	//

	float fps = core::GameApp::get()->getAverageFps();
	float sps = core::GameApp::get()->getAverageSps();
	string info = core::strings::format("%.1f %.1f", fps, sps);
	gl::drawString(info, vec2(10,10), Color(1,1,1));

	stringstream ss;
	Viewport::look look = getViewport()->getLook();
	double scale = getViewport()->getScale();

	ss << std::setprecision(3) << "world (" << look.world.x << ", " << look.world.y  << ") scale: " << scale << " up: (" << look.up.x << ", " << look.up.y << ")";
	gl::drawString(ss.str(), vec2(10,40), Color(1,1,1));

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

void SvgTestScenario::reset() {
	cleanup();
	setup();
}

#pragma mark - Tests

void SvgTestScenario::testSimpleSvgLoad() {
	auto doc = util::svg::Group::loadSvgDocument(app::loadAsset("svg_tests/contour_soup_test.svg"), 1);
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

	root->setPosition(-root->getDocumentSize()/2.0);

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
