//
//  Viewport.cpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/13/17.
//
//

#include "Viewport.hpp"

namespace core {
    
    namespace {
        const double MinScale = 1e-5;
    }

    /*
        int _width, _height;
        look _look;
        dmat4 _viewMatrix, _inverseViewMatrix, _projectionMatrix, _inverseProjectionMatrix, _viewProjectionMatrix, _inverseViewProjectionMatrix;
     */

    Viewport::Viewport() :
            _width(0),
            _height(0) {
        setSize(app::getWindowWidth(), app::getWindowHeight());
        setLook(dvec2(0, 0));
    }

    void Viewport::setSize(int width, int height) {
        if (_width != width || _height != height) {

            _width = width;
            _height = height;

            //update & notify
            _updateMatrices();
            onMotion(*this);
            onBoundsChanged(*this);
        }
    }

    void Viewport::setLook(const look &l) {
        if (distanceSquared(l.world, _look.world) > 1e-6 || distanceSquared(l.up, _look.up) > 1e-6 || abs(l.scale - _look.scale) > 1e-6) {
            _look.world = l.world;

            // normalize up, falling back to (0,1) for invalid
            double len = glm::length(l.up);
            if (len > 1e-4) {
                _look.up = l.up / len;
            } else {
                _look.up = dvec2(0, 1);
            }
            
            _look.scale = max<double>(l.scale, MinScale);

            // update & notify
            _updateMatrices();
            onMotion(*this);
        }
    }

    cpBB Viewport::getFrustum() const {

        // frustum is axis-aligned box in world space wrapping the edges of the screen

        const double width = getWidth(), height = getHeight();
        cpBB bb = cpBBInvalid;
        bb = cpBBExpand(bb, _inverseViewProjectionMatrix * dvec2(0, 0));
        bb = cpBBExpand(bb, _inverseViewProjectionMatrix * dvec2(width, 0));
        bb = cpBBExpand(bb, _inverseViewProjectionMatrix * dvec2(width, height));
        bb = cpBBExpand(bb, _inverseViewProjectionMatrix * dvec2(0, height));

        return bb;
    }

    void Viewport::set() {
        gl::multProjectionMatrix(_projectionMatrix);
        gl::multViewMatrix(_viewMatrix);
    }

    void Viewport::_updateMatrices() {
        double rs = 1.0 / _look.scale;
        _projectionMatrix = glm::translate(dvec3(getCenter(), 0)) * glm::ortho(-rs, rs, -rs, rs, -1.0, 1.0);
        _inverseProjectionMatrix = glm::inverse(_projectionMatrix);

        _viewMatrix = glm::lookAt(dvec3(_look.world, 1), dvec3(_look.world, 0), dvec3(_look.up, 0));
        _inverseViewMatrix = inverse(_viewMatrix);

        _viewProjectionMatrix = _projectionMatrix * _viewMatrix;
        _inverseViewProjectionMatrix = glm::inverse(_viewProjectionMatrix);
    }


}
