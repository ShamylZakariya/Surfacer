//
//  App.hpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/14/17.
//
//

#ifndef GameApp_hpp
#define GameApp_hpp

#include <stdio.h>
#include "TimeState.hpp"

#include <cinder/app/App.h>

#include "InputDispatcher.hpp"
#include "ChipmunkHelpers.hpp"
#include "Scenario.hpp"


namespace core {

    SMART_PTR(PhysicsLoop);

    class App : public ci::app::App {
    public:

        static void prepareSettings(Settings *settings);

        static App *get() {
            return static_cast<App *>(ci::app::App::get());
        }

    public:

        App();

        virtual ~App();

        // AppBasic
        virtual void setup() override;

        virtual void cleanup() override;

        virtual void update() override;

        virtual void draw() override;

        virtual void resize() override;

        // GameApp
        virtual void step();

        ScenarioRef scenario() const {
            return _scenario;
        }

        void setScenario(ScenarioRef scenario);


        double getAverageSps() const {
            return _averageStepsPerSecond;
        }

    private:

        PhysicsLoopRef _physicsLoop;
        ScenarioRef _scenario;

        unsigned int _stepCount;
        seconds_t _stepCountTime;
        double _averageStepsPerSecond;

    };

/**
	@class PhysicsLoop
	@brief PhysicsLoop is responsible for dispatching calls to step() in App,
	doing its best over time to maintain a target update rate
 */
    class PhysicsLoop {
    public:

        PhysicsLoop(App *app, int rate);

        virtual ~PhysicsLoop(void);

        int rate() const {
            return _rate;
        }

        seconds_t interval() const {
            return _interval;
        }

        /**
         @brief Called by main event loop
         Subclasses will invoke dispatchDisplay() and dispatchStep()
            */
        virtual void step(void) = 0;

    protected:

        const App *app() const {
            return _app;
        }

        inline void dispatchStep() {
            _app->step();
        }

    private:

        App *_app;
        int _rate;
        seconds_t _interval;


    };

    class AdaptivePhysicsLoop : public PhysicsLoop {
    private:

        seconds_t _currentTime, _lastTime, _timeRemainder;

    public:

        AdaptivePhysicsLoop(App *app, int rate);

        virtual ~AdaptivePhysicsLoop(void);

        virtual void step(void);
    };

    class SacredSoftwarePhysicsLoop : public PhysicsLoop {
    private:

        seconds_t _currentTime, _lastTime, _timeRemainder, _cyclesLeftOver;

    public:

        SacredSoftwarePhysicsLoop(App *app, int rate);

        virtual ~SacredSoftwarePhysicsLoop(void);

        virtual void step(void);
    };

} // end namespace core

#endif /* GameApp_hpp */
