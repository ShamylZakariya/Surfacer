//
//  EasingTestScenario.cpp
//  Tests
//
//  Created by Shamyl Zakariya on 3/4/18.
//

#include "EasingTestScenario.hpp"
#include "DevComponents.hpp"
#include "ImageProcessing.hpp"
#include "Easing.hpp"


using namespace ci;
using namespace core;

namespace {
    
    typedef function<double(double)> EaseFunction;

    class EaseRenderer : public DrawComponent {
    public:
        
        EaseRenderer(string name, const EaseFunction &easer, int steps, dvec2 position, int displaySize):
        _name(name),
        _position(position),
        _displaySize(displaySize)
        {
            for (int i = 0; i <= steps; i++) {
                double step = static_cast<double>(i) / static_cast<double>(steps);
                double value = easer(step);
                _computedEase.push_back(vec2(step, value));
            }
            _computedEase.setClosed(false);
            _bb = cpBBNew(position.x, position.y, position.x+displaySize, position.y+displaySize);
        }
        
        cpBB getBB() const override {
            return _bb;
        }
        
        void draw(const render_state &renderState) override {
            gl::ScopedModelMatrix smm;
            gl::translate(dvec3(_position.x, _position.y, 0));
            
            gl::enableAlphaBlending();
            gl::color(0.1,0.1,0.1,0.9);
            gl::drawSolidRect(Rectf(0,0,_displaySize, _displaySize));
            
            gl::color(1,0,1,1);
            gl::lineWidth(1);
            {
                gl::ScopedModelMatrix smm2;
                gl::scale(_displaySize, _displaySize);
                gl::draw(_computedEase);
            }
            
            {
                gl::ScopedModelMatrix smm3;
                gl::scale(renderState.viewport->getReciprocalScale(), -renderState.viewport->getReciprocalScale());
                gl::drawString(_name, dvec2(0,10));
            }
        }
        
        VisibilityDetermination::style getVisibilityDetermination() const override {
            return VisibilityDetermination::FRUSTUM_CULLING;
        }
        
        int getLayer() const override { return 100; }
        
        // Component
        void onReady(ObjectRef parent, StageRef stage) override {
            DrawComponent::onReady(parent, stage);
            notifyMoved();
        }
        
    private:
        
        cpBB _bb;
        string _name;
        dvec2 _position;
        PolyLine2 _computedEase;
        int _displaySize;

    };
    
    const int EASE_STEPS = 256;
    
    struct EaseDesc {
        string name;
        EaseFunction fn;
    };
    
    ObjectRef ease(const EaseDesc &d, dvec2 pos, double size) {
        return Object::with(d.name, {make_shared<EaseRenderer>(d.name, d.fn, EASE_STEPS, pos, size)});
    }
    
    double add_ease_row(const StageRef &stage, const initializer_list<EaseDesc> &descs, double y) {
        dvec2 p(0, y);
        double size = 100;
        double step = 100;
        for(const EaseDesc &ed : descs) {
            stage->addObject(ease(ed, p, size));
            p.x += size + step;
        }
        
        return y + size + step;
    }
    
}

#define EASE_DESC(fn_name) {#fn_name, util::easing::fn_name<double>}

EasingTestScenario::EasingTestScenario()
{
}

EasingTestScenario::~EasingTestScenario() {
}

void EasingTestScenario::setup() {
    setStage(make_shared<Stage>("Image Processing Tests"));
    
    getStage()->addObject(Object::with("ViewportControlComponent", {
        make_shared<ManualViewportControlComponent>(getViewportController())
    }));
    
    auto grid = WorldCartesianGridDrawComponent::create(1);
    grid->setFillColor(ColorA(0.2, 0.22, 0.25, 1.0));
    grid->setGridColor(ColorA(1, 1, 1, 0.025));
    getStage()->addObject(Object::with("Grid", {grid}));
    
    auto stage = getStage();
    double y = 0;
    y = add_ease_row(stage, {EASE_DESC(linear)}, y);
    y = add_ease_row(stage, {EASE_DESC(expo_ease_out), EASE_DESC(expo_ease_in), EASE_DESC(expo_ease_in_out), EASE_DESC(expo_ease_out_in)}, y);
    y = add_ease_row(stage, {EASE_DESC(circ_ease_out), EASE_DESC(circ_ease_in), EASE_DESC(circ_ease_in_out), EASE_DESC(circ_ease_out_in)}, y);
    y = add_ease_row(stage, {EASE_DESC(quad_ease_out), EASE_DESC(quad_ease_in), EASE_DESC(quad_ease_in_out), EASE_DESC(quad_ease_out_in)}, y);
    y = add_ease_row(stage, {EASE_DESC(sine_ease_out), EASE_DESC(sine_ease_in), EASE_DESC(sine_ease_in_out), EASE_DESC(sine_ease_out_in)}, y);
    y = add_ease_row(stage, {EASE_DESC(cubic_ease_out), EASE_DESC(cubic_ease_in), EASE_DESC(cubic_ease_in_out), EASE_DESC(cubic_ease_out_in)}, y);
    y = add_ease_row(stage, {EASE_DESC(quart_ease_out), EASE_DESC(quart_ease_in), EASE_DESC(quart_ease_in_out), EASE_DESC(quart_ease_out_in)}, y);
    y = add_ease_row(stage, {EASE_DESC(quint_ease_out), EASE_DESC(quint_ease_in), EASE_DESC(quint_ease_in_out), EASE_DESC(quint_ease_out_in)}, y);
    y = add_ease_row(stage, {EASE_DESC(elastic_ease_out), EASE_DESC(elastic_ease_in), EASE_DESC(elastic_ease_in_out), EASE_DESC(elastic_ease_out_in)}, y);
    y = add_ease_row(stage, {EASE_DESC(bounce_ease_out), EASE_DESC(bounce_ease_in), EASE_DESC(bounce_ease_in_out), EASE_DESC(bounce_ease_out_in)}, y);
    y = add_ease_row(stage, {EASE_DESC(back_ease_out), EASE_DESC(back_ease_in), EASE_DESC(back_ease_in_out), EASE_DESC(back_ease_out_in)}, y);
    
}

void EasingTestScenario::cleanup() {
    setStage(nullptr);
}

void EasingTestScenario::resize(ivec2 size) {
}

void EasingTestScenario::step(const time_state &time) {
}

void EasingTestScenario::update(const time_state &time) {
}

void EasingTestScenario::clear(const render_state &state) {
    gl::clear(Color(0.2, 0.2, 0.2));
}

void EasingTestScenario::draw(const render_state &state) {
    Scenario::draw(state);
}

void EasingTestScenario::drawScreen(const render_state &state) {
    Scenario::drawScreen(state);
}

bool EasingTestScenario::keyDown(const ci::app::KeyEvent &event) {
    
    switch(event.getChar()) {
        case 'r':
            reset();
            return true;
    }
    
    return false;
}

void EasingTestScenario::reset() {
    cleanup();
    setup();
}
