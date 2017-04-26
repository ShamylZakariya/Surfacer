//
//  DevComponents.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/14/17.
//
//

#ifndef DevComponents_hpp
#define DevComponents_hpp

#include "Core.hpp"
#include "Terrain.hpp"

class WorldCartesianGridDrawComponent : public core::DrawComponent {
public:

	static shared_ptr<WorldCartesianGridDrawComponent> create(double basePeriod = 10, double periodStep = 10);

public:

	WorldCartesianGridDrawComponent(gl::TextureRef tex, double basePeriod, double periodStep);

	void onReady(core::GameObjectRef parent, core::LevelRef level) override;

	cpBB getBB() const override {
		return cpBBInfinity;
	}

	void draw(const core::render_state &state) override;

	core::VisibilityDetermination::style getVisibilityDetermination() const override {
		return core::VisibilityDetermination::ALWAYS_DRAW;
	}

	int getLayer() const override { return -1; }

protected:

	void setupShaderUniforms();
	void onViewportMotion(const core::Viewport &vp);

private:

	gl::TextureRef _texture;
	double _basePeriod;
	double _periodStep;
	gl::GlslProgRef _shader;
	gl::BatchRef _batch;
	
};

class ManualViewportControlComponent : public core::InputComponent {
public:

	ManualViewportControlComponent(core::ViewportControllerRef viewportController, int dispatchReceiptIndex = 1000);

	// InputComponent
	void step(const core::time_state &time) override;
	bool mouseDown( const ci::app::MouseEvent &event ) override;
	bool mouseUp( const ci::app::MouseEvent &event ) override;
	bool mouseMove( const ci::app::MouseEvent &event, const ivec2 &delta ) override;
	bool mouseDrag( const ci::app::MouseEvent &event, const ivec2 &delta ) override;
	bool mouseWheel( const ci::app::MouseEvent &event ) override;

private:

	vec2 _mouseScreen, _mouseWorld;
	core::ViewportControllerRef _viewportController;

};

class TargetTrackingViewportControlComponent : public core::InputComponent {
public:

	struct tracking {
		// target to look at
		dvec2 world;
		// up vector
		dvec2 up;
		// scale will adjust to fit everything within this radius of look to fit in viewport 
		double radius;

		tracking(const dvec2 &w, const dvec2 &u, double r):
		world(w),
		up(u),
		radius(r)
		{}

		tracking(const tracking &c):
		world(c.world),
		up(c.up),
		radius(c.radius)
		{}

	};

	class TrackingTarget {
	public:

		virtual tracking getViewportTracking() const = 0;

	};

public:

	TargetTrackingViewportControlComponent(shared_ptr<TrackingTarget> target, core::ViewportControllerRef viewportController, int dispatchReceiptIndex = 1000);
	TargetTrackingViewportControlComponent(core::ViewportControllerRef viewportController, int dispatchReceiptIndex = 1000);

	virtual void setTrackingTarget(shared_ptr<TrackingTarget> target);
	shared_ptr<TrackingTarget> getTrackingTarget() const { return _trackingTarget.lock(); }

	void setTrackingRegion(double trackingAreaRadius, double deadZoneRadius, double correctiveRampPower=2, double correctiveTightness=0.99);
	double getTrackingRegionRadius() const { return _trackingAreaRadius; }
	double getTrackingRegionDeadZoneRadius() const { return _trackingAreaDeadZoneRadius; }
	double getTrackingRegionCorrectiveRampPower() const { return _trackingAreaCorrectiveRampPower; }

	// InputComponent
	void step(const core::time_state &time) override;
	bool mouseWheel( const ci::app::MouseEvent &event ) override;

protected:

	virtual void _trackNoSlop(const tracking &t, const core::time_state &time);
	virtual void _track(const tracking &t, const core::time_state &time);

private:

	core::ViewportControllerRef _viewportController;
	weak_ptr<TrackingTarget> _trackingTarget;
	double _scale, _trackingAreaRadius, _trackingAreaDeadZoneRadius, _trackingAreaCorrectiveRampPower, _trackingAreaCorrectiveTightness;


};

class MousePickComponent : public core::InputComponent {
public:

	MousePickComponent(cpShapeFilter pickFilter, int dispatchReceiptIndex = 0);

	virtual ~MousePickComponent();

	void onReady(core::GameObjectRef parent, core::LevelRef level) override;
	void step(const core::time_state &time) override;
	bool mouseDown( const ci::app::MouseEvent &event ) override;
	bool mouseUp( const ci::app::MouseEvent &event ) override;
	bool mouseMove( const ci::app::MouseEvent &event, const ivec2 &delta ) override;
	bool mouseDrag( const ci::app::MouseEvent &event, const ivec2 &delta ) override;

	bool isDragging() const { return _mouseJoint != nullptr; }
	dvec2 getDragPositionWorld() const { return v2(cpBodyGetPosition(_mouseBody)); }

protected:

	void releaseDragConstraint();


private:

	cpShapeFilter _pickFilter;
	cpBody *_mouseBody, *_draggingBody;
	cpConstraint *_mouseJoint;
	vec2 _mouseScreen, _mouseWorld;

};

class MousePickDrawComponent : public core::DrawComponent {
public:

	MousePickDrawComponent(ColorA color = ColorA(1,1,1,0.5), float radius=4);

	void onReady(core::GameObjectRef parent, core::LevelRef level) override;
	cpBB getBB() const override { return cpBBInfinity; }
	void draw(const core::render_state &renderState) override;
	core::VisibilityDetermination::style getVisibilityDetermination() const override { return core::VisibilityDetermination::ALWAYS_DRAW; }
	int getLayer() const override { return numeric_limits<int>::max(); };

private:

	ColorA _color;
	float _radius;
	weak_ptr<MousePickComponent> _pickComponent;

};

class MouseCutterComponent : public core::InputComponent {
public:

	MouseCutterComponent(terrain::TerrainObjectRef terrain, cpShapeFilter cutFilter, float radius, int dispatchReceiptIndex = 0);

	bool mouseDown( const ci::app::MouseEvent &event ) override;
	bool mouseUp( const ci::app::MouseEvent &event ) override;
	bool mouseMove( const ci::app::MouseEvent &event, const ivec2 &delta ) override;
	bool mouseDrag( const ci::app::MouseEvent &event, const ivec2 &delta ) override;

	bool isCutting() const { return _cutting; }
	float getRadius() const { return _radius; }
	dvec2 getCutStart() const { return _cutStart; }
	dvec2 getCutEnd() const { return _cutEnd; }

private:

	bool _cutting;
	float _radius;
	cpShapeFilter _cutFilter;
	vec2 _mouseScreen, _mouseWorld, _cutStart, _cutEnd;
	terrain::TerrainObjectRef _terrain;

};

class MouseCutterDrawComponent : public core::DrawComponent {
public:

	MouseCutterDrawComponent(ColorA color = ColorA(1,1,1,0.5));

	void onReady(core::GameObjectRef parent, core::LevelRef level) override;
	cpBB getBB() const override { return cpBBInfinity; }
	void draw(const core::render_state &renderState) override;
	core::VisibilityDetermination::style getVisibilityDetermination() const override { return core::VisibilityDetermination::ALWAYS_DRAW; }
	int getLayer() const override { return numeric_limits<int>::max(); };

private:

	ColorA _color;
	weak_ptr<MouseCutterComponent> _cutterComponent;

};


#endif /* DevComponents_hpp */
