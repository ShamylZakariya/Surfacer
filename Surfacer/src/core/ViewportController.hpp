//
//  ViewportController.hpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/16/17.
//
//

#ifndef ViewportController_hpp
#define ViewportController_hpp

#include "Signals.hpp"
#include "TimeState.hpp"
#include "Viewport.hpp"

namespace core {

    SMART_PTR(ViewportController);

    class ViewportController : public signals::receiver {
    public:

        struct tracking_config {
            double positiveY;
            double negativeY;
            double positiveX;
            double negativeX;
            double up;
            double positiveScale;
            double negativeScale;

            tracking_config():
                positiveY(0.99),
                negativeY(0.99),
                positiveX(0.99),
                negativeX(0.99),
                up(0.99),
                positiveScale(1),
                negativeScale(1)
            {}

            tracking_config(double panFactor, double upFactor, double scaleFactor) :
                positiveY(panFactor),
                negativeY(panFactor),
                positiveX(panFactor),
                negativeX(panFactor),
                up(upFactor),
                positiveScale(scaleFactor),
                negativeScale(scaleFactor)
            {}

        };

    public:

        ViewportController(ViewportRef vp);

        virtual ~ViewportController() {
        }

        ///////////////////////////////////////////////////////////////////////////

        virtual void update(const time_state &time);

        ViewportRef getViewport() const {
            return _viewport;
        }

        void setViewport(ViewportRef vp);

        void setTrackingConfig(tracking_config tc) {
            _tracking = tc;
        }

        tracking_config &getTrackingConfig() {
            return _tracking;
        }

        const tracking_config &getTrackingConfig() const {
            return _tracking;
        }

        void setLook(Viewport::look l);

        void setLook(const dvec2 world, const dvec2 up, double scale) {
            setLook(Viewport::look(world, up, scale));
        }

        void setLook(const dvec2 world, const dvec2 up) {
            setLook(Viewport::look(world, up, _target.scale));
        }

        void setLook(const dvec2 world) {
            setLook(Viewport::look(world, _target.up, _target.scale));
        }

        Viewport::look getLook() const {
            return _target;
        }

        void setScale(double scale);
        
        // set the scale, anchoring whatever's under `aboutScreenPoint to remain in same position on the screen.
        // this is a helper for zooming under mouse cursors
        void setScale(double scale, dvec2 aboutScreenPoint);

        double getScale() const {
            return _target.scale;
        }

    protected:

        void _onViewportPropertyChanged(const Viewport &vp);

    private:

        ViewportRef _viewport;
        Viewport::look _target;

        tracking_config _tracking;
        bool _disregardViewportMotion;

    };


} // end namespace core

#endif /* ViewportController_hpp */
