//
//  DevComponents.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/14/17.
//
//

#include "DevComponents.hpp"

using namespace core;
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
					   uniform float AlphaPeriodTexelSize;
					   uniform float AlphaPeriodAlpha;
					   uniform float BetaPeriod;
					   uniform float BetaPeriodTexelSize;
					   uniform float BetaPeriodAlpha;
					   uniform vec4 Color;
					   uniform vec4 AxisColor;
					   uniform sampler2D Tex0;

					   in vec2 worldPosition;

					   out vec4 oColor;

					   /*
						TODO: Fix the haloing in the onAxis determination. It might make sense to switch from 
						attemping to do this in texel space (right now we're using the 0th texel in S & T when
						world position is at zero. This blurs because of mipmapping. Disabling mip interpolation fixes
						the issue... but causes shit moire artifacting.
						The correct fix is likely to be to detect fragment size (think dFdx, dFdy) and light up
						fragments with axis color when inside N fragments of axis lines.
						*/

					   vec4 getGridForPeriod(float period, float texelSize, vec4 gridColor, vec4 axisColor) {
						   vec2 texCoord = worldPosition / period;
						   vec2 aTexCoord = abs(texCoord);

						   // determine if texCoord is on either the x or y axis, "binary" in that value is 0, 1 or 2 for being on both axes
						   // then clamp that to 0->1
						   float onAxis = (1.0 - step(texelSize, aTexCoord.s)) + (1.0 - step(texelSize, aTexCoord.t));
						   onAxis = clamp(onAxis, 0.0, 1.0);

						   // mix between primary color and axis color
						   vec4 color = mix(gridColor, axisColor, onAxis);

						   float alpha = texture(Tex0, texCoord).a;
						   color.a = mix(alpha,1.0,onAxis);

						   return color;
					   }

					   void main(void) {
						   vec4 alphaValue = getGridForPeriod(AlphaPeriod, AlphaPeriodTexelSize, Color, AxisColor);
						   vec4 betaValue = getGridForPeriod(BetaPeriod, BetaPeriodTexelSize, Color, vec4(0,0,0,0));

						   alphaValue.a *= AlphaPeriodAlpha;
						   betaValue.a *= BetaPeriodAlpha;

						   oColor = alphaValue + betaValue;
					   }
					   );

	_shader = gl::GlslProg::create(gl::GlslProg::Format().vertex(vsh).fragment(fsh));
	_batch = gl::Batch::create(geom::Rect().rect(Rectf(-1,-1,1,1)), _shader);
}

void WorldCartesianGridDrawComponent::onReady(GameObjectRef parent, LevelRef level) {
	level->getViewport()->motion.connect(this, &WorldCartesianGridDrawComponent::onViewportMotion);
	setupShaderUniforms();
}

void WorldCartesianGridDrawComponent::draw(const render_state &state) {

	// set up a scale to fill viewport with plane
	cpBB frustum = state.viewport->getFrustum();
	dvec2 centerWorld = v2(cpBBCenter(frustum));
	dvec2 scale = centerWorld - dvec2(frustum.l, frustum.t);

	gl::ScopedModelMatrix smm;
	dmat4 modelMatrix = glm::translate(dvec3(centerWorld,0)) * glm::scale(dvec3(scale,1));
	gl::multModelMatrix(modelMatrix);

	_texture->bind(0);
	_shader->uniform("Tex0", 0);

	_batch->draw();
}

namespace {
	inline double calcTexelScale(double scale, double period, double textureSize) {
		return period * scale / textureSize;
	}
}

void WorldCartesianGridDrawComponent::setupShaderUniforms() {
	ViewportRef vp = getLevel()->getViewport();

	// find largest period where rendered texel scale (ratio of one texel to one pixel) is less than or equal to maxTexelScale
	const double textureSize = _texture->getWidth();
	const double viewportScale = vp->getScale();
	const double maxTexelScale = 3;
	const double minTexelScale = 1/_periodStep;

	double period = _basePeriod;
	double texelScale = calcTexelScale(viewportScale, period, textureSize);

	while (texelScale < maxTexelScale) {
		period *= _periodStep;
		texelScale = calcTexelScale(viewportScale, period, textureSize);
	}

	// step back one since the above loop oversteps
	period /= _periodStep;
	texelScale /= _periodStep;

	const double betaPeriod = period;
	const double betaPeriodTexelScale = texelScale;
	const double alphaPeriod = betaPeriod / _periodStep;
	const double alphaPeriodTexelScale = texelScale / _periodStep;

	// alpha of beta period falls as it approaches max texel size, and alpha period is the remainder
	const double betaPeriodAlpha = 1 - (betaPeriodTexelScale - minTexelScale) / (maxTexelScale - minTexelScale);
	const double alphaPeriodAlpha = 1 - betaPeriodAlpha;

	// compute the mip level so we can pass texel size [0...1] to the shader.
	// This is used by shader to detect axis edges
	double alphaPeriodMip = max(ceil(log2(1/alphaPeriodTexelScale)), 0.0);
	double betaPeriodMip = max(ceil(log2(1/betaPeriodTexelScale)), 0.0);

	// note we bump mip up to handle slop
	double alphaPeriodTexelSize = pow(2, alphaPeriodMip+1) / _texture->getWidth();
	double betaPeriodTexelSize = pow(2, betaPeriodMip+1) / _texture->getWidth();

//	app::console() << " viewportScale: " << viewportScale
//		<< "scale: " << dvec2(alphaPeriodTexelScale, betaPeriodTexelScale)
//		<< " mip: " << dvec2(alphaPeriodMip, betaPeriodMip)
//		<< " texelSize: " << dvec2(alphaPeriodTexelSize, betaPeriodTexelSize)
//		<< endl;

	_shader->uniform("Tex0", 0);
	_shader->uniform("AlphaPeriod", static_cast<float>(alphaPeriod));
	_shader->uniform("AlphaPeriodTexelSize", static_cast<float>(alphaPeriodTexelSize));
	_shader->uniform("AlphaPeriodAlpha", static_cast<float>(alphaPeriodAlpha));
	_shader->uniform("BetaPeriod", static_cast<float>(betaPeriod));
	_shader->uniform("BetaPeriodTexelSize", static_cast<float>(betaPeriodTexelSize));
	_shader->uniform("BetaPeriodAlpha", static_cast<float>(betaPeriodAlpha));
	_shader->uniform("Color", ColorA(1,1,1,0.5));

	// TODO: Turn this back to red when I fix haloing
	_shader->uniform("AxisColor", ColorA(1,1,1,1));
}

void WorldCartesianGridDrawComponent::onViewportMotion(const Viewport &vp) {
	setupShaderUniforms();
}


#pragma mark - ManualViewportControlComponent
/*
	vec2 _mouseScreen, _mouseWorld;
	ViewportControllerRef _viewportController;
*/

ManualViewportControlComponent::ManualViewportControlComponent(ViewportControllerRef viewportController, int dispatchReceiptIndex):
InputComponent(dispatchReceiptIndex),
_viewportController(viewportController)
{}

namespace {
	dvec2 rotate(dvec2 up, double by) {
		return glm::rotate(up, by);
	}
}

void ManualViewportControlComponent::step(const time_state &time) {
	const double radsPerSec = 10 * M_PI / 180;
	if (isKeyDown(ci::app::KeyEvent::KEY_q)) {
		Viewport::look look = _viewportController->getLook();
		look.up = rotate(look.up, -radsPerSec * time.deltaT);
		_viewportController->setLook(look);
		//_viewport->setRotation(_viewport->getRotation() - radsPerSec * time.deltaT);
	} else if (isKeyDown(ci::app::KeyEvent::KEY_e)) {
		Viewport::look look = _viewportController->getLook();
		look.up = rotate(look.up, +radsPerSec * time.deltaT);
		_viewportController->setLook(look);
	}
}

bool ManualViewportControlComponent::mouseDown( const ci::app::MouseEvent &event ) {
	_mouseScreen = event.getPos();
	_mouseWorld = getLevel()->getViewport()->screenToWorld(_mouseScreen);

	CI_LOG_D("screen: " << _mouseScreen << " world: " << _mouseWorld);

	// capture alt key for re-centering
	if ( event.isAltDown() )
	{
		_viewportController->setLook( _mouseWorld );
		return true;
	}

	// capture space key for dragging
	if ( isKeyDown( app::KeyEvent::KEY_SPACE )) {
		return true;
	}

	return false;
}

bool ManualViewportControlComponent::mouseUp( const ci::app::MouseEvent &event ) {
	return false;
}

bool ManualViewportControlComponent::mouseMove( const ci::app::MouseEvent &event, const ivec2 &delta ) {
	_mouseScreen = event.getPos();
	_mouseWorld = getLevel()->getViewport()->screenToWorld(_mouseScreen);
	return false;
}

bool ManualViewportControlComponent::mouseDrag( const ci::app::MouseEvent &event, const ivec2 &delta ) {
	_mouseScreen = event.getPos();
	_mouseWorld = getLevel()->getViewport()->screenToWorld(_mouseScreen);

	if ( isKeyDown( app::KeyEvent::KEY_SPACE ))
	{
		dvec2 deltaWorld = _viewportController->getViewport()->screenToWorldDir(dvec2(delta));
		Viewport::look look = _viewportController->getLook();
		look.world -= deltaWorld;
		_viewportController->setLook(look);
		return true;
	}

	return false;
}

bool ManualViewportControlComponent::mouseWheel( const ci::app::MouseEvent &event ){
	const float zoom = _viewportController->getScale(),
	wheelScale = 0.1 * zoom,
	dz = (event.getWheelIncrement() * wheelScale);
	_viewportController->setScale(zoom + dz);

	return true;
}

#pragma mark - TargetTrackingViewportController

/*
	core::ViewportControllerRef _viewportController;
	weak_ptr<TrackingTarget> _trackingTarget;
	double _scale, _trackingAreaRadius, _trackingAreaDeadZoneRadius, _trackingAreaCorrectiveRampPower, _trackingAreaCorrectiveTightness;
*/

TargetTrackingViewportControlComponent::TargetTrackingViewportControlComponent(shared_ptr<TrackingTarget> target, core::ViewportControllerRef viewportController, int dispatchReceiptIndex):
InputComponent(dispatchReceiptIndex),
_viewportController(viewportController),
_trackingTarget(target),
_scale(1),
_trackingAreaRadius(0),
_trackingAreaDeadZoneRadius(0),
_trackingAreaCorrectiveRampPower(0),
_trackingAreaCorrectiveTightness(0)
{}

TargetTrackingViewportControlComponent::TargetTrackingViewportControlComponent(ViewportControllerRef viewportController, int dispatchReceiptIndex):
InputComponent(dispatchReceiptIndex),
_viewportController(viewportController),
_scale(1),
_trackingAreaRadius(0),
_trackingAreaDeadZoneRadius(0),
_trackingAreaCorrectiveRampPower(0),
_trackingAreaCorrectiveTightness(0)
{}

void TargetTrackingViewportControlComponent::setTrackingTarget(shared_ptr<TrackingTarget> target) {
	_trackingTarget = target;
}

void TargetTrackingViewportControlComponent::setTrackingRegion(double trackingAreaRadius, double deadZoneRadius, double correctiveRampPower, double tightness) {
	_trackingAreaRadius = max(trackingAreaRadius, 0.0);
	_trackingAreaDeadZoneRadius = clamp(deadZoneRadius, 0.0, trackingAreaRadius * 0.9);
	_trackingAreaCorrectiveRampPower = max(correctiveRampPower, 1.0);
	_trackingAreaCorrectiveTightness = clamp(tightness, 0.0, 1.0);
}

// InputComponent
void TargetTrackingViewportControlComponent::step(const time_state &time) {
	if (auto target = _trackingTarget.lock()) {
		const auto t = target->getViewportTracking();
		if (_trackingAreaRadius > 0) {
			_track(t, time);
		} else {
			_trackNoSlop(t, time);
		}
	}
}

bool TargetTrackingViewportControlComponent::mouseWheel( const ci::app::MouseEvent &event ) {
	if (auto target = _trackingTarget.lock()) {
		const double wheelScale = 0.1 * _scale;
		_scale += (event.getWheelIncrement() * wheelScale);
		return true;
	}
	return false;
}

void TargetTrackingViewportControlComponent::_trackNoSlop(const tracking &t, const time_state &time) {
	const ivec2 size = _viewportController->getViewport()->getSize();
	const double scale = min(size.x, size.y) / (2 * t.radius);

	_viewportController->setLook(Viewport::look(t.world, t.up));
	_viewportController->setScale(scale * _scale);
}

void TargetTrackingViewportControlComponent::_track(const tracking &t, const time_state &time) {
	Viewport::look currentLook = _viewportController->getLook();

	const double outerDistance = distance(t.world, currentLook.world) + t.radius;
	if (outerDistance > _trackingAreaDeadZoneRadius) {
		// compute error as factor from 0 (no error) to 1
		double error = outerDistance - _trackingAreaDeadZoneRadius;
		error = min(error, _trackingAreaRadius);
		error = (error - _trackingAreaDeadZoneRadius) / (_trackingAreaRadius - _trackingAreaDeadZoneRadius);

		// this is the distance the viewport has to move (this step)
		double correction = (outerDistance - _trackingAreaDeadZoneRadius) * pow(error,_trackingAreaCorrectiveRampPower)  * _trackingAreaCorrectiveTightness;

		// apply correction to look world target
		dvec2 deltaLook = normalize(t.world - currentLook.world) * correction;
		currentLook.world += deltaLook;
		currentLook.up = t.up;

		// now apply scale and look::up
		const ivec2 size = _viewportController->getViewport()->getSize();
		const double scale = min(size.x, size.y) / (2 * _trackingAreaRadius);

		_viewportController->setLook(currentLook);
		_viewportController->setScale(scale * _scale);
	}
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

void MousePickComponent::onReady(GameObjectRef parent, LevelRef level) {
	InputComponent::onReady(parent, level);
	_mouseBody = cpBodyNewKinematic();
}

void MousePickComponent::step(const time_state &time){
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

void MousePickDrawComponent::onReady(GameObjectRef parent, LevelRef level) {
	DrawComponent::onReady(parent,level);
	_pickComponent = getSibling<MousePickComponent>();
}

void MousePickDrawComponent::draw(const render_state &renderState) {
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

void MouseCutterDrawComponent::onReady(GameObjectRef parent, LevelRef level){
	DrawComponent::onReady(parent,level);
	_cutterComponent = getSibling<MouseCutterComponent>();
}

void MouseCutterDrawComponent::draw(const render_state &renderState){
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
