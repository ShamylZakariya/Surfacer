//
//  GameLevelTestScenario.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/13/17.
//
//

#ifndef GameLevelTestScenario_hpp
#define GameLevelTestScenario_hpp

#include "Core.hpp"

using namespace ci;
using namespace core;

class GameLevelTestScenario : public Scenario {
public:

	GameLevelTestScenario();
	virtual ~GameLevelTestScenario();

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
	virtual bool mouseMove( const ci::app::MouseEvent &event, const ivec2 &delta ) override;
	virtual bool mouseDrag( const ci::app::MouseEvent &event, const ivec2 &delta ) override;
	virtual bool keyDown( const ci::app::KeyEvent &event ) override;
	virtual bool keyUp( const ci::app::KeyEvent &event ) override;

	void reset();

private:

	vec2 _mouseScreen, _mouseWorld;
	
};


#endif /* GameLevelTestScenario_hpp */
