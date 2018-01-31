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

SMART_PTR(WorldCartesianGridDrawComponent);

class WorldCartesianGridDrawComponent : public core::DrawComponent {
public:

    static shared_ptr<WorldCartesianGridDrawComponent> create(double basePeriod = 10, double periodStep = 10);

public:

    WorldCartesianGridDrawComponent(gl::TextureRef tex, double basePeriod, double periodStep);

    void onReady(core::ObjectRef parent, core::StageRef stage) override;

    cpBB getBB() const override {
        return cpBBInfinity;
    }

    void draw(const core::render_state &state) override;

    core::VisibilityDetermination::style getVisibilityDetermination() const override {
        return core::VisibilityDetermination::ALWAYS_DRAW;
    }

    int getLayer() const override {
        return -1;
    }

    void setFillColor(ci::ColorA fillColor) {
        _fillColor = fillColor;
    }

    ci::ColorA getFillColor() const {
        return _fillColor;
    }

    void setGridColor(ci::ColorA color) {
        _gridColor = color;
    }

    ci::ColorA getGridColor() const {
        return _gridColor;
    }

protected:

    void setupShaderUniforms();

    void onViewportMotion(const core::Viewport &vp);

private:

    gl::TextureRef _texture;
    double _basePeriod;
    double _periodStep;
    gl::GlslProgRef _shader;
    gl::BatchRef _batch;
    ci::ColorA _fillColor, _gridColor;

};

SMART_PTR(ManualViewportControlComponent);

class ManualViewportControlComponent : public core::InputComponent {
public:

    ManualViewportControlComponent(core::ViewportControllerRef viewportController, int dispatchReceiptIndex = 1000);

    // InputComponent
    void step(const core::time_state &time) override;

    bool mouseDown(const ci::app::MouseEvent &event) override;

    bool mouseUp(const ci::app::MouseEvent &event) override;

    bool mouseMove(const ci::app::MouseEvent &event, const ivec2 &delta) override;

    bool mouseDrag(const ci::app::MouseEvent &event, const ivec2 &delta) override;

    bool mouseWheel(const ci::app::MouseEvent &event) override;

private:

    vec2 _mouseScreen, _mouseWorld;
    core::ViewportControllerRef _viewportController;

};

SMART_PTR(TargetTrackingViewportControlComponent);

class TargetTrackingViewportControlComponent : public core::InputComponent {
public:

    struct tracking {
        // target to look at
        dvec2 world;
        // up vector
        dvec2 up;
        // scale will adjust to fit everything within this radius of look to fit in viewport
        double radius;

        tracking(const dvec2 &w, const dvec2 &u, double r) :
                world(w),
                up(u),
                radius(r) {
        }

        tracking(const tracking &c) :
                world(c.world),
                up(c.up),
                radius(c.radius) {
        }

    };

    class TrackingTarget {
    public:

        virtual tracking getViewportTracking() const = 0;

    };

public:

    TargetTrackingViewportControlComponent(shared_ptr<TrackingTarget> target, core::ViewportControllerRef viewportController, int dispatchReceiptIndex = 1000);

    TargetTrackingViewportControlComponent(core::ViewportControllerRef viewportController, int dispatchReceiptIndex = 1000);

    virtual void setTrackingTarget(shared_ptr<TrackingTarget> target);

    shared_ptr<TrackingTarget> getTrackingTarget() const {
        return _trackingTarget.lock();
    }

    void setTrackingRegion(double trackingAreaRadius, double deadZoneRadius, double correctiveRampPower = 2, double correctiveTightness = 0.99);

    double getTrackingRegionRadius() const {
        return _trackingAreaRadius;
    }

    double getTrackingRegionDeadZoneRadius() const {
        return _trackingAreaDeadZoneRadius;
    }

    double getTrackingRegionCorrectiveRampPower() const {
        return _trackingAreaCorrectiveRampPower;
    }

    // InputComponent
    void step(const core::time_state &time) override;

    bool mouseWheel(const ci::app::MouseEvent &event) override;

protected:

    virtual void _trackNoSlop(const tracking &t, const core::time_state &time);

    virtual void _track(const tracking &t, const core::time_state &time);

private:

    core::ViewportControllerRef _viewportController;
    weak_ptr<TrackingTarget> _trackingTarget;
    double _scale, _trackingAreaRadius, _trackingAreaDeadZoneRadius, _trackingAreaCorrectiveRampPower, _trackingAreaCorrectiveTightness;


};

SMART_PTR(MousePickComponent);

class MousePickComponent : public core::InputComponent {
public:

    MousePickComponent(cpShapeFilter pickFilter, int dispatchReceiptIndex = 0);

    virtual ~MousePickComponent();

    void onReady(core::ObjectRef parent, core::StageRef stage) override;

    void step(const core::time_state &time) override;

    bool mouseDown(const ci::app::MouseEvent &event) override;

    bool mouseUp(const ci::app::MouseEvent &event) override;

    bool mouseMove(const ci::app::MouseEvent &event, const ivec2 &delta) override;

    bool mouseDrag(const ci::app::MouseEvent &event, const ivec2 &delta) override;

    bool isDragging() const {
        return _mouseJoint != nullptr;
    }

    dvec2 getDragPositionWorld() const {
        return v2(cpBodyGetPosition(_mouseBody));
    }

protected:

    void releaseDragConstraint();


private:

    cpShapeFilter _pickFilter;
    cpBody *_mouseBody, *_draggingBody;
    cpConstraint *_mouseJoint;
    vec2 _mouseScreen, _mouseWorld;

};

SMART_PTR(MousePickDrawComponent);

class MousePickDrawComponent : public core::DrawComponent {
public:

    MousePickDrawComponent(ColorA color = ColorA(1, 1, 1, 0.5), float radius = 4);

    void onReady(core::ObjectRef parent, core::StageRef stage) override;

    cpBB getBB() const override {
        return cpBBInfinity;
    }

    void draw(const core::render_state &renderState) override;

    core::VisibilityDetermination::style getVisibilityDetermination() const override {
        return core::VisibilityDetermination::ALWAYS_DRAW;
    }

    int getLayer() const override {
        return numeric_limits<int>::max();
    };

private:

    ColorA _color;
    float _radius;
    weak_ptr<MousePickComponent> _pickComponent;

};

SMART_PTR(KeyboardDelegateComponent);

class KeyboardDelegateComponent : public core::InputComponent {
public:

    typedef function<void(int keyCode)> KeyHandler;

    static KeyboardDelegateComponentRef create(int dispatchReceiptIndex, const initializer_list<int> keycodes);

public:
    KeyboardDelegateComponent(int dispatchReceiptIndex, const initializer_list<int> keycodes);

    KeyboardDelegateComponentRef onPress(KeyHandler h);

    KeyboardDelegateComponentRef onRelease(KeyHandler h);

    void monitoredKeyDown(int keyCode) override;

    void monitoredKeyUp(int keyCode) override;

private:
    function<void(int keyCode)> _upHandler, _downHandler;
};

SMART_PTR(MouseDelegateComponent);

class MouseDelegateComponent : public core::InputComponent {
public:

    typedef function<bool(dvec2 screen, dvec2 world, const ci::app::MouseEvent &event)> MousePressHandler;
    typedef function<bool(dvec2 screen, dvec2 world, const ci::app::MouseEvent &event)> MouseReleaseHandler;
    typedef function<bool(dvec2 screen, dvec2 world, ivec2 deltaScreen, dvec2 deltaWorld, const ci::app::MouseEvent &event)> MouseMoveHandler;
    typedef function<bool(dvec2 screen, dvec2 world, ivec2 deltaScreen, dvec2 deltaWorld, const ci::app::MouseEvent &event)> MouseDragHandler;

    static MouseDelegateComponentRef create(int dispatchReceiptIndex);

public:

    MouseDelegateComponent(int dispatchReceiptIndex);

    MouseDelegateComponentRef onPress(MousePressHandler);

    MouseDelegateComponentRef onRelease(MouseReleaseHandler);

    MouseDelegateComponentRef onMove(MouseMoveHandler);

    MouseDelegateComponentRef onDrag(MouseDragHandler);

    bool mouseDown(const ci::app::MouseEvent &event) override;

    bool mouseUp(const ci::app::MouseEvent &event) override;

    bool mouseMove(const ci::app::MouseEvent &event, const ivec2 &delta) override;

    bool mouseDrag(const ci::app::MouseEvent &event, const ivec2 &delta) override;

private:

    MousePressHandler _pressHandler;
    MouseReleaseHandler _releaseHandler;
    MouseMoveHandler _moveHandler;
    MouseDragHandler _dragHandler;

};

#endif /* DevComponents_hpp */
