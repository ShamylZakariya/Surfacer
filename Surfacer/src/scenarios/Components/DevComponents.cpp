//
//  DevComponents.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/14/17.
//
//

#include "DevComponents.hpp"

#pragma mark - WorldCartesianGridDrawComponent

/*
	gl::TextureRef _texture;
	double _basePeriod;
	double _periodStep;
	gl::GlslProgRef _shader;
	gl::BatchRef _batch;
*/

shared_ptr<WorldCartesianGridDrawComponent> WorldCartesianGridDrawComponent::create(double basePeriod, double periodStep) {
	auto tex = gl::Texture2d::create(loadImage(app::loadAsset("cartesian_grid.png")), gl::Texture2d::Format().mipmap());
	return make_shared<WorldCartesianGridDrawComponent>(tex,basePeriod, periodStep);
}

WorldCartesianGridDrawComponent::WorldCartesianGridDrawComponent(gl::TextureRef tex, double basePeriod, double periodStep):
_texture(tex),
_basePeriod(basePeriod),
_periodStep(periodStep)
{
	_texture->setWrap(GL_REPEAT, GL_REPEAT);
	_texture->setMagFilter(GL_NEAREST);
	_texture->setMinFilter(GL_LINEAR_MIPMAP_LINEAR);

	auto vsh = CI_GLSL(150,
					   uniform mat4 ciModelViewProjection;
					   uniform mat4 ciModelMatrix;

					   in vec4 ciPosition;

					   out vec2 worldPosition;

					   void main(void){
						   gl_Position = ciModelViewProjection * ciPosition;
						   worldPosition = (ciModelMatrix * ciPosition).xy;
					   }
					   );

	auto fsh = CI_GLSL(150,
					   uniform float AlphaPeriod;
					   uniform float BetaPeriod;
					   uniform float AlphaPeriodAlpha;
					   uniform float BetaPeriodAlpha;
					   uniform float TexelSize;
					   uniform vec4 Color;
					   uniform vec4 AxisColor;
					   uniform sampler2D Tex0;

					   in vec2 worldPosition;

					   out vec4 oColor;

					   vec4 getGridForPeriod(float period) {
						   vec2 texCoord = worldPosition / period;
						   vec2 aTexCoord = abs(texCoord);

						   float onAxis = (1.0 - step(TexelSize, aTexCoord.s)) + (1.0 - step(TexelSize, aTexCoord.t));
						   onAxis = clamp(onAxis, 0.0, 1.0);
						   vec4 color = mix(Color, AxisColor, onAxis);

						   float alpha = texture(Tex0, texCoord).a;
						   color.a *= alpha;
						   return color;
					   }

					   void main(void) {
						   vec4 alphaValue = getGridForPeriod(AlphaPeriod);
						   vec4 betaValue = getGridForPeriod(BetaPeriod);

						   alphaValue.a *= AlphaPeriodAlpha;
						   betaValue.a *= BetaPeriodAlpha;

						   oColor = alphaValue + betaValue;
					   }
					   );

	_shader = gl::GlslProg::create(gl::GlslProg::Format().vertex(vsh).fragment(fsh));
	_batch = gl::Batch::create(geom::Rect().rect(Rectf(-1,-1,1,1)), _shader);
}

namespace {
	inline double calcTexelSize(double scale, double period, double textureSize) {
		return period * scale / textureSize;
	}
}

void WorldCartesianGridDrawComponent::draw(const core::render_state &state) {

	// set up a scale to fill viewport with plane
	cpBB frustum = state.viewport->getFrustum();
	dvec2 centerWorld = v2(cpBBCenter(frustum));
	dvec2 scale = centerWorld - dvec2(frustum.l, frustum.t);


	// find the first visible period
	double textureSize = _texture->getWidth();
	double viewportScale = state.viewport->getScale();

	// find largest period where rendered texels are less than or equal to maxTexelSize
	const double maxTexelSize = 3;
	const double minTexelSize = 1/_periodStep;
	double period = _basePeriod;
	double texelSize = calcTexelSize(viewportScale, period, textureSize);

	while (texelSize < maxTexelSize) {
		period *= _periodStep;
		texelSize = calcTexelSize(viewportScale, period, textureSize);
	}

	// step back one since the above loop oversteps
	period /= _periodStep;
	texelSize /= _periodStep;

	double betaPeriod = period;
	double betaPeriodTexelSize = texelSize;
	double alphaPeriod = betaPeriod / _periodStep;

	// alpha of beta period falls as it approaches max texel size
	double betaPeriodAlpha = 1 - (betaPeriodTexelSize - minTexelSize) / (maxTexelSize - minTexelSize);
	double alphaPeriodAlpha = 1 - betaPeriodAlpha;


	gl::ScopedModelMatrix smm;
	dmat4 modelMatrix = glm::translate(dvec3(centerWorld,0)) * glm::scale(dvec3(scale,1));
	gl::multModelMatrix(modelMatrix);

	_texture->bind(0);
	_shader->uniform("Tex0", 0);
	_shader->uniform("TexelSize", static_cast<float>(1.0 / _texture->getWidth()));
	_shader->uniform("AlphaPeriod", static_cast<float>(alphaPeriod));
	_shader->uniform("BetaPeriod", static_cast<float>(betaPeriod));
	_shader->uniform("AlphaPeriodAlpha", static_cast<float>(alphaPeriodAlpha));
	_shader->uniform("BetaPeriodAlpha", static_cast<float>(betaPeriodAlpha));
	_shader->uniform("Color", ColorA(1,1,1,1));

	// TODO: Fix axis coloring
	_shader->uniform("AxisColor", ColorA(1,1,1,1));

	_batch->draw();
}


#pragma mark - CameraControlComponent

CameraControlComponent::CameraControlComponent(core::Camera2DInterfaceRef viewport, int dispatchReceiptIndex):
InputComponent(dispatchReceiptIndex),
_viewport(viewport)
{}

bool CameraControlComponent::mouseDown( const ci::app::MouseEvent &event ) {
	_mouseScreen = event.getPos();
	_mouseWorld = getLevel()->getViewport()->screenToWorld(_mouseScreen);

	// capture alt key for re-centering
	if ( event.isAltDown() )
	{
		_viewport->lookAt( _mouseWorld );
		return true;
	}

	// capture space key for dragging
	if ( isKeyDown( app::KeyEvent::KEY_SPACE )) {
		return true;
	}

	return false;
}

bool CameraControlComponent::mouseUp( const ci::app::MouseEvent &event ) {
	return false;
}

bool CameraControlComponent::mouseMove( const ci::app::MouseEvent &event, const ivec2 &delta ) {
	_mouseScreen = event.getPos();
	_mouseWorld = getLevel()->getViewport()->screenToWorld(_mouseScreen);
	return false;
}

bool CameraControlComponent::mouseDrag( const ci::app::MouseEvent &event, const ivec2 &delta ) {
	_mouseScreen = event.getPos();
	_mouseWorld = getLevel()->getViewport()->screenToWorld(_mouseScreen);

	if ( isKeyDown( app::KeyEvent::KEY_SPACE ))
	{
		_viewport->setPan( _viewport->getPan() + dvec2(delta) );
		return true;
	}

	return false;
}

bool CameraControlComponent::mouseWheel( const ci::app::MouseEvent &event ){
	const float zoom = _viewport->getScale(),
	wheelScale = 0.1 * zoom,
	dz = (event.getWheelIncrement() * wheelScale);
	_viewport->setScale( std::max( zoom + dz, 0.1f ), event.getPos() );

	return true;
}

#pragma mark - MousePickComponent

MousePickComponent::MousePickComponent(cpShapeFilter pickFilter, int dispatchReceiptIndex):
InputComponent(dispatchReceiptIndex),
_pickFilter(pickFilter),
_mouseBody(nullptr),
_draggingBody(nullptr),
_mouseJoint(nullptr)
{}

MousePickComponent::~MousePickComponent() {
	releaseDragConstraint();
	cpBodyFree(_mouseBody);
}

void MousePickComponent::onReady(core::GameObjectRef parent, core::LevelRef level) {
	InputComponent::onReady(parent, level);
	_mouseBody = cpBodyNewKinematic();
}

void MousePickComponent::step(const core::time_state &time){
	cpVect mouseBodyPos = cpv(_mouseWorld);
	cpVect newMouseBodyPos = cpvlerp(cpBodyGetPosition(_mouseBody), mouseBodyPos, 0.75);

	cpBodySetVelocity(_mouseBody, cpvmult(cpvsub(newMouseBodyPos, cpBodyGetPosition(_mouseBody)), time.deltaT));
	cpBodySetPosition(_mouseBody, newMouseBodyPos);
}

bool MousePickComponent::mouseDown( const ci::app::MouseEvent &event ) {
	releaseDragConstraint();

	_mouseScreen = event.getPos();
	_mouseWorld = getLevel()->getViewport()->screenToWorld(_mouseScreen);

	if ( isKeyDown( app::KeyEvent::KEY_SPACE )) {
		return false;
	}

	const float distance = 1.f;
	cpPointQueryInfo info = {};
	cpShape *pick = cpSpacePointQueryNearest(getLevel()->getSpace()->getSpace(), cpv(_mouseWorld), distance, _pickFilter, &info);
	if (pick) {
		cpBody *pickBody = cpShapeGetBody(pick);

		if (pickBody && cpBodyGetType(pickBody) == CP_BODY_TYPE_DYNAMIC) {
			cpVect nearest = (info.distance > 0.0f ? info.point : cpv(_mouseWorld));

			_draggingBody = pickBody;
			_mouseJoint = cpPivotJointNew2(_mouseBody, _draggingBody, cpvzero, cpBodyWorldToLocal(_draggingBody,nearest));

			getLevel()->getSpace()->addConstraint(_mouseJoint);

			return true;
		}
	}

	return false;
}

bool MousePickComponent::mouseUp( const ci::app::MouseEvent &event ) {
	releaseDragConstraint();
	return false;
}

bool MousePickComponent::mouseMove( const ci::app::MouseEvent &event, const ivec2 &delta ) {
	_mouseScreen = event.getPos();
	_mouseWorld = getLevel()->getViewport()->screenToWorld(_mouseScreen);
	return false;
}

bool MousePickComponent::mouseDrag( const ci::app::MouseEvent &event, const ivec2 &delta ){
	_mouseScreen = event.getPos();
	_mouseWorld = getLevel()->getViewport()->screenToWorld(_mouseScreen);
	return false;
}

void MousePickComponent::releaseDragConstraint() {
	if (_mouseJoint) {
		cpCleanupAndFree(_mouseJoint);
		_draggingBody = nullptr;
	}
}

#pragma mark - MousePickDrawComponent

MousePickDrawComponent::MousePickDrawComponent(ColorA color, float radius):
_color(color),
_radius(radius)
{}

void MousePickDrawComponent::onReady(core::GameObjectRef parent, core::LevelRef level) {
	DrawComponent::onReady(parent,level);
	_pickComponent = getSibling<MousePickComponent>();
}

void MousePickDrawComponent::draw(const core::render_state &renderState) {
	if (shared_ptr<MousePickComponent> pc = _pickComponent.lock()) {
		if (pc->isDragging()) {
			const float radius = _radius * renderState.viewport->getReciprocalScale();
			gl::color(_color);
			gl::drawSolidCircle(pc->getDragPositionWorld(), radius);
		}
	}
}

#pragma mark - MouseCutterComponent

MouseCutterComponent::MouseCutterComponent(terrain::TerrainObjectRef terrain, cpShapeFilter cutFilter, float radius, int dispatchReceiptIndex):
InputComponent(dispatchReceiptIndex),
_cutting(false),
_radius(radius),
_cutFilter(cutFilter),
_terrain(terrain)
{}

bool MouseCutterComponent::mouseDown( const ci::app::MouseEvent &event ) {
	_mouseScreen = event.getPos();
	_mouseWorld = getLevel()->getViewport()->screenToWorld(_mouseScreen);

	if ( isKeyDown( app::KeyEvent::KEY_SPACE )) {
		return false;
	}

	_cutting = true;
	_cutStart = _cutEnd = _mouseWorld;

	return true;
}

bool MouseCutterComponent::mouseUp( const ci::app::MouseEvent &event ){
	if (_cutting) {
		if (_terrain) {
			const float radius = _radius / getLevel()->getViewport()->getScale();
			_terrain->getWorld()->cut(_cutStart, _cutEnd, radius, _cutFilter);
		}

		_cutting = false;
	}
	return false;
}

bool MouseCutterComponent::mouseMove( const ci::app::MouseEvent &event, const ivec2 &delta ) {
	_mouseScreen = event.getPos();
	_mouseWorld = getLevel()->getViewport()->screenToWorld(_mouseScreen);
	return false;
}

bool MouseCutterComponent::mouseDrag( const ci::app::MouseEvent &event, const ivec2 &delta ) {
	_mouseScreen = event.getPos();
	_mouseWorld = getLevel()->getViewport()->screenToWorld(_mouseScreen);
	if (_cutting) {
		_cutEnd = _mouseWorld;
	}
	return false;
}

#pragma mark - MouseCutterDrawComponent

MouseCutterDrawComponent::MouseCutterDrawComponent(ColorA color):
_color(color)
{}

void MouseCutterDrawComponent::onReady(core::GameObjectRef parent, core::LevelRef level){
	DrawComponent::onReady(parent,level);
	_cutterComponent = getSibling<MouseCutterComponent>();
}

void MouseCutterDrawComponent::draw(const core::render_state &renderState){
	if (shared_ptr<MouseCutterComponent> cc = _cutterComponent.lock()) {
		if (cc->isCutting()) {
			float radius = cc->getRadius() / renderState.viewport->getScale();
			vec2 dir = cc->getCutEnd() - cc->getCutStart();
			float len = ::length(dir);
			if (len > 1e-2) {
				dir /= len;
				float angle = atan2(dir.y, dir.x);
				vec2 center = (cc->getCutStart()+cc->getCutEnd()) * 0.5;

				mat4 M = translate(vec3(center.x, center.y, 0)) * rotate(angle, vec3(0,0,1));
				gl::ScopedModelMatrix smm;
				gl::multModelMatrix(M);

				gl::color(_color);
				gl::drawSolidRect(Rectf(-len/2, -radius, +len/2, +radius));
			}
		}

	}
}
