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

	testSimpleSvgLoad();
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
	if (_svgDocument) {
		Viewport::ScopedState vs(getViewport());
		// TODO Re-enable shader when using GL to draw SVGs
		//gl::ScopedGlslProg sglp(_shader);
		_svgDocument->draw(state);
	}


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
	_svgDocument = util::svg::Group::loadSvgDocument(loadAsset("AnchorTest0.svg"), 1);
	_svgDocument->trace();
}
