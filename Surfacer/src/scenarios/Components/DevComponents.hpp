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

	static shared_ptr<WorldCartesianGridDrawComponent> create(double baseRepeatPeriod = 100);

public:

	WorldCartesianGridDrawComponent(gl::TextureRef tex, double baseRepeatPeriod);

	cpBB getBB() const override {
		return cpBBInfinity;
	}

	void draw(const core::render_state &state) override;

	core::VisibilityDetermination::style getVisibilityDetermination() const override {
		return core::VisibilityDetermination::ALWAYS_DRAW;
	}

	int getLayer() const override { return -1; }

private:

	gl::TextureRef _texture;
	double _baseRepeatPeriod;
	gl::GlslProgRef _shader;
	gl::BatchRef _batch;
	
};




class CameraControlComponent : public core::InputComponent {
public:

	CameraControlComponent(core::Camera2DInterfaceRef viewport, int dispatchReceiptIndex = 1000);

	bool mouseDown( const ci::app::MouseEvent &event ) override;
	bool mouseUp( const ci::app::MouseEvent &event ) override;
	bool mouseMove( const ci::app::MouseEvent &event, const ivec2 &delta ) override;
	bool mouseDrag( const ci::app::MouseEvent &event, const ivec2 &delta ) override;
	bool mouseWheel( const ci::app::MouseEvent &event ) override;

private:

	vec2 _mouseScreen, _mouseWorld;
	core::Camera2DInterfaceRef _viewport;

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
