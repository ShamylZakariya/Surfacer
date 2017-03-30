//
//  LevelTestScenario.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/28/17.
//
//

#ifndef LevelTestScenario_hpp
#define LevelTestScenario_hpp

#include "Core.hpp"

using namespace ci;
using namespace core;

class LevelTestScenario : public Scenario {
public:

	LevelTestScenario();
	virtual ~LevelTestScenario();

	virtual void setup() override;
	virtual void cleanup() override;
	virtual void resize( ivec2 size ) override;

	virtual void step( const time_state &time ) override;
	virtual void update( const time_state &time ) override;
	virtual void clear( const render_state &state ) override;
	virtual void draw( const render_state &state ) override;

	virtual bool mouseDown( const ci::app::MouseEvent &event ) override;
	virtual bool mouseUp( const ci::app::MouseEvent &event ) override;
	virtual bool mouseWheel( const ci::app::MouseEvent &event ) override;
	virtual bool mouseMove( const ci::app::MouseEvent &event, const vec2 &delta ) override;
	virtual bool mouseDrag( const ci::app::MouseEvent &event, const vec2 &delta ) override;
	virtual bool keyDown( const ci::app::KeyEvent &event ) override;
	virtual bool keyUp( const ci::app::KeyEvent &event ) override;

	void reset();

protected:

	void drawWorldCoordinateSystem(const render_state &state);


private:

	ViewportController _cameraController;
	vec2 _mouseScreen, _mouseWorld;

};

#endif /* LevelTestScenario_hpp */
