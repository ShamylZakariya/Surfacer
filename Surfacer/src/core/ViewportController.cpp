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
        Viewport::look _look;

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
        
        if (_zenoConfig.factor < 1) {
            const double rate = 1.0 / time.deltaT;
            const double f = std::pow(_zenoConfig.factor, rate);
            Viewport::look look = _viewport->getLook();
            
            const dvec2 lookWorldError = _look.world - look.world;
            const dvec2 lookUpError = _look.up - look.up;
            const double scaleError = _look.scale - look.scale;
            
            look.world = look.world + lookWorldError * f;
            look.up = look.up + lookUpError * f;
            look.scale = look.scale + scaleError * f;
            
            _viewport->setLook(look);
        } else {
            _viewport->setLook(_look);
        }        

        _disregardViewportMotion = false;
    }

    void ViewportController::setViewport(ViewportRef vp) {
        if (_viewport) {
            _viewport->onMotion.disconnect(this);
            _viewport->onBoundsChanged.disconnect(this);
        }
        _viewport = vp;
        if (_viewport) {
            _look = _viewport->getLook();
            _viewport->onMotion.connect(this, &ViewportController::_onViewportPropertyChanged);
            _viewport->onBoundsChanged.connect(this, &ViewportController::_onViewportPropertyChanged);
        } else {
            _look = { dvec2(0, 0), dvec2(0, 1), 1 };
        }
    }

    void ViewportController::setLook(const Viewport::look l) {
        _look.world = l.world;

        double len = glm::length(l.up);
        if (len > 1e-3) {
            _look.up = l.up / len;
        } else {
            _look.up = dvec2(0, 1);
        }
    }

    void ViewportController::setScale(double scale) {
        _look.scale = max(scale, 0.0);
    }
    
    void ViewportController::setScale(double scale, dvec2 aboutScreenPoint) {
        setScale(scale);
        _disregardViewportMotion = true;

        
        // determine where aboutScreenPoint is in worldSpace
        dvec2 aboutScreenPointWorld = _viewport->screenToWorld(aboutScreenPoint);
        
        auto previousLook = _viewport->getLook();
        _viewport->setLook(_look);
        
        dvec2 postScaleAboutScreenPointWorld = _viewport->screenToWorld(aboutScreenPoint);
        dvec2 delta = postScaleAboutScreenPointWorld - aboutScreenPointWorld;
        
        _viewport->setLook(previousLook);

        _look.world -= delta;
        _disregardViewportMotion = false;
    }


#pragma mark -
#pragma mark Private

    void ViewportController::_onViewportPropertyChanged(const Viewport &vp) {
        if (!_disregardViewportMotion) {
            _look = vp.getLook();
        }
    }


} // end namespace core
