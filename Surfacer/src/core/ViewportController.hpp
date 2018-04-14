//
//  ViewportController.hpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/16/17.
//
//

#ifndef ViewportController_hpp
#define ViewportController_hpp

#include <cinder/Perlin.h>

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
        
        struct trauma_config {
            // max translational shake effect
            dvec2 shakeTranslation;
            // max rotational shake effect
            double shakeRotation;
            // amount of trauma lost per second (trauma is ranged between [0,1])
            double traumaDecayRate;

            // amount of trauma baseline lost per second (traumaBaseline is ranged between [0,1])
            double traumaBaselineDecayRate;

            // shake is computed as pow(trauma, shakePower), defaults to 2
            double shakePower;

            // frequency of shake, higher value is higher frequency of shake
            double shakeFrequency;
            
            trauma_config():
                    shakeTranslation(10,10),
                    shakeRotation(5 * M_PI / 180),
                    traumaDecayRate(1),
                    traumaBaselineDecayRate(1),
                    shakePower(2),
                    shakeFrequency(2)
            {}
            
            trauma_config(dvec2 st, double sr, double tdr, double tbdr, double sp, double sf):
                    shakeTranslation(st),
                    shakeRotation(sr),
                    traumaDecayRate(tdr),
                    traumaBaselineDecayRate(tbdr),
                    shakePower(sp),
                    shakeFrequency(sf)
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
            _trackingConfig = tc;
        }

        tracking_config &getTrackingConfig() {
            return _trackingConfig;
        }

        const tracking_config &getTrackingConfig() const {
            return _trackingConfig;
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
        
        void setTraumaConfig(trauma_config tc) {
            _traumaConfig = tc;
        }
        
        trauma_config &getTraumaConfig() { return _traumaConfig; }
        const trauma_config &getTraumaConfig() const { return _traumaConfig; }
        
        // add to the current trauma level. the level will linearly decay at traumaConfig.traumaDecayRate per second
        // the current trauma level is the current trauma + current trauma baseline
        // use addTrauma to handle one-shot "events" like an explosion or injury
        void addTrauma(double t);
        double getTrauma() const { return _traumaLevel; }

        // set a baseline trauma level. the level will linearly decay at traumaConfig.traumaBaselineDecayRate per second
        // the current trauma level is the current trauma + current trauma baseline
        // use setTraumaBaseline to handle continuous trauma events like an earthquake
        void setTraumaBaseline(double tb);
        double getTraumaBaseline() const { return _traumaBaselineLevel; }
        
        // get the current trauma + baseline
        double getCurrentTraumaLevel() const { return saturate(_traumaLevel + _traumaBaselineLevel); }
        double getCurrentTraumaShake() const { return pow(getCurrentTraumaLevel(), _traumaConfig.shakePower); }

    protected:

        void _onViewportPropertyChanged(const Viewport &vp);

    private:

        ViewportRef _viewport;
        Viewport::look _target;
        bool _disregardViewportMotion;
        
        tracking_config _trackingConfig;
        trauma_config _traumaConfig;
        double _traumaLevel, _traumaBaselineLevel;
        vector<ci::Perlin> _traumaPerlinNoiseGenerators;

    };


} // end namespace core

#endif /* ViewportController_hpp */
