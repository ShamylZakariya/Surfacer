//
//  ViewportController.cpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/16/17.
//
//

#include "ViewportController.hpp"

namespace core {

    /*
        ViewportRef _viewport;
        Viewport::look _target;

        zeno_config _zenoConfig;
        bool _disregardViewportMotion;
     */

    ViewportController::ViewportController(ViewportRef vp) :
            _viewport(nullptr),
            _disregardViewportMotion(false) {
        setViewport(vp);
    }

    void ViewportController::update(const time_state &time) {
        _disregardViewportMotion = true;
                
        const double rate = 1.0 / time.deltaT;
        Viewport::look look = _viewport->getLook();
        
        const dvec2 lookWorldError = _target.world - look.world;
        if (lookWorldError.x > 0) {
            const double f = std::pow(_tracking.positiveX, rate);
            look.world.x += f * lookWorldError.x;
        } else {
            const double f = std::pow(_tracking.negativeX, rate);
            look.world.x += f * lookWorldError.x;
        }

        if (lookWorldError.y > 0) {
            const double f = std::pow(_tracking.positiveY, rate);
            look.world.y += f * lookWorldError.y;
        } else {
            const double f = std::pow(_tracking.negativeY, rate);
            look.world.y += f * lookWorldError.y;
        }
        
        {
            const dvec2 lookUpError = _target.up - look.up;
            const double f = std::pow(_tracking.up, rate);
            look.up += f * lookUpError;
            look.up = normalize(look.up);
        }
        
        const double scaleError = _target.scale - look.scale;
        if (scaleError > 0) {
            const double f = std::pow(_tracking.positiveScale, rate);
            look.scale += f * scaleError;
        } else {
            const double f = std::pow(_tracking.negativeScale, rate);
            look.scale += f * scaleError;
        }
        
        _viewport->setLook(look);

        _disregardViewportMotion = false;
    }

    void ViewportController::setViewport(ViewportRef vp) {
        if (_viewport) {
            _viewport->onMotion.disconnect(this);
            _viewport->onBoundsChanged.disconnect(this);
        }
        _viewport = vp;
        if (_viewport) {
            _target = _viewport->getLook();
            _viewport->onMotion.connect(this, &ViewportController::_onViewportPropertyChanged);
            _viewport->onBoundsChanged.connect(this, &ViewportController::_onViewportPropertyChanged);
        } else {
            _target = { dvec2(0, 0), dvec2(0, 1), 1 };
        }
    }

    void ViewportController::setLook(const Viewport::look l) {
        _target.world = l.world;

        double len = glm::length(l.up);
        if (len > 1e-3) {
            _target.up = l.up / len;
        } else {
            _target.up = dvec2(0, 1);
        }
    }

    void ViewportController::setScale(double scale) {
        _target.scale = max(scale, 0.0);
    }
    
    void ViewportController::setScale(double scale, dvec2 aboutScreenPoint) {
        setScale(scale);
        _disregardViewportMotion = true;

        
        // determine where aboutScreenPoint is in worldSpace
        dvec2 aboutScreenPointWorld = _viewport->screenToWorld(aboutScreenPoint);
        
        auto previousLook = _viewport->getLook();
        _viewport->setLook(_target);
        
        dvec2 postScaleAboutScreenPointWorld = _viewport->screenToWorld(aboutScreenPoint);
        dvec2 delta = postScaleAboutScreenPointWorld - aboutScreenPointWorld;
        
        _viewport->setLook(previousLook);

        _target.world -= delta;
        _disregardViewportMotion = false;
    }


#pragma mark -
#pragma mark Private

    void ViewportController::_onViewportPropertyChanged(const Viewport &vp) {
        if (!_disregardViewportMotion) {
            _target = vp.getLook();
        }
    }


} // end namespace core
