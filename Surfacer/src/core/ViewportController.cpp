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
     bool _disregardViewportMotion;
     
     tracking_config _trackingConfig;
     trauma_config _traumaConfig;
     double _traumaLevel;
     vector<ci::Perlin> _traumaPerlinNoiseGenerators;
     */

    ViewportController::ViewportController(ViewportRef vp) :
            _viewport(nullptr),
            _disregardViewportMotion(false),
            _traumaLevel(0)
    {
        setViewport(vp);
        
        // uniquely-seeded perlin noise for shake x, shake y, and shake rotation
        for (int i = 0; i < 3; i++) {
            _traumaPerlinNoiseGenerators.emplace_back(ci::Perlin(4,i));
        }
    }

    void ViewportController::update(const time_state &time) {
        _disregardViewportMotion = true;
        
        // update viewport
                
        const double rate = 1.0 / time.deltaT;
        Viewport::look look = _viewport->getLook();
        
        const dvec2 lookWorldError = _target.world - look.world;
        if (lookWorldError.x > 0) {
            const double f = std::pow(_trackingConfig.positiveX, rate);
            look.world.x += f * lookWorldError.x;
        } else {
            const double f = std::pow(_trackingConfig.negativeX, rate);
            look.world.x += f * lookWorldError.x;
        }

        if (lookWorldError.y > 0) {
            const double f = std::pow(_trackingConfig.positiveY, rate);
            look.world.y += f * lookWorldError.y;
        } else {
            const double f = std::pow(_trackingConfig.negativeY, rate);
            look.world.y += f * lookWorldError.y;
        }
        
        {
            const dvec2 lookUpError = _target.up - look.up;
            const double f = std::pow(_trackingConfig.up, rate);
            look.up += f * lookUpError;
            look.up = normalize(look.up);
        }
        
        const double scaleError = _target.scale - look.scale;
        if (scaleError > 0) {
            const double f = std::pow(_trackingConfig.positiveScale, rate);
            look.scale += f * scaleError;
        } else {
            const double f = std::pow(_trackingConfig.negativeScale, rate);
            look.scale += f * scaleError;
        }
        
        // apply trauma shake
        const double shake = pow(_traumaLevel, _traumaConfig.shakePower);
        if (shake > 0) {
            double t = time.time * _traumaConfig.shakeFrequency;
            const double dx = _traumaPerlinNoiseGenerators[0].noise(t) * shake * _traumaConfig.shakeTranslation.x;
            const double dy = _traumaPerlinNoiseGenerators[1].noise(t) * shake * _traumaConfig.shakeTranslation.y;
            const double dr = _traumaPerlinNoiseGenerators[2].noise(t) * shake * _traumaConfig.shakeRotation;
            look.world.x += dx;
            look.world.y += dy;
            look.up = rotate(look.up, dr);
        }

        _viewport->setLook(look);

        _disregardViewportMotion = false;
        
        // now decay trauma
        _traumaLevel = saturate(_traumaLevel - _traumaConfig.traumaDecayRate * time.deltaT);
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
    
    void ViewportController::addTrauma(double trauma) {
        _traumaLevel = saturate<double>(_traumaLevel + trauma);
    }

#pragma mark -
#pragma mark Private

    void ViewportController::_onViewportPropertyChanged(const Viewport &vp) {
        if (!_disregardViewportMotion) {
            _target = vp.getLook();
        }
    }


} // end namespace core
