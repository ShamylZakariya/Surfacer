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

        /**
            Configurator structor for Zeno control method.
            There are two fields, panFactor and scaleFactor which are
            in the range of [0,1], representing the interpolation distance
            to take from the current viewport value to the controller's set point.

            A panFactor of 0.9 will, for example, cause the error between the controller
            set point and the current viewport value to be reduced by 90% each second.

         */
        struct zeno_config {
            double lookTargetFactor, scaleFactor, lookUpFactor;
            
            static zeno_config withFactor(double f) {
                return zeno_config(f,f,f);
            }
            
            static zeno_config none() {
                return zeno_config(1,1,1);
            }

            zeno_config() :
                    lookTargetFactor(0.99),
                    scaleFactor(0.99),
                    lookUpFactor(0.99) {
            }

            zeno_config(double ltf, double sf, double luf) :
                    lookTargetFactor(ltf),
                    scaleFactor(sf),
                    lookUpFactor(luf) {
            }

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

        void setZenoConfig(zeno_config zc) {
            _zenoConfig = zc;
        }

        zeno_config &getZenoConfig() {
            return _zenoConfig;
        }

        const zeno_config &getZenoConfig() const {
            return _zenoConfig;
        }

        void setLook(Viewport::look l);

        void setLook(const dvec2 world, const dvec2 up, double scale) {
            setLook(Viewport::look(world, up, scale));
        }

        void setLook(const dvec2 world, const dvec2 up) {
            setLook(Viewport::look(world, up, _look.scale));
        }

        void setLook(const dvec2 world) {
            setLook(Viewport::look(world, _look.up, _look.scale));
        }

        Viewport::look getLook() const {
            return _look;
        }

        void setScale(double scale);
        
        // set the scale, anchoring whatever's under `aboutScreenPoint to remain in same position on the screen.
        // this is a helper for zooming under mouse cursors
        void setScale(double scale, dvec2 aboutScreenPoint);

        double getScale() const {
            return _look.scale;
        }

    protected:

        void _onViewportPropertyChanged(const Viewport &vp);

    private:

        ViewportRef _viewport;
        Viewport::look _look;

        zeno_config _zenoConfig;
        bool _disregardViewportMotion;

    };


} // end namespace core

#endif /* ViewportController_hpp */
