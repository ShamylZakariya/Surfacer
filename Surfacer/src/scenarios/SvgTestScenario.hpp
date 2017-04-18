//
//  SvgTestScenario.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/6/17.
//
//

#ifndef SvgTestScenario_hpp
#define SvgTestScenario_hpp

#include <cinder/gl/GlslProg.h>

#include "Core.hpp"
#include "Svg.hpp"


using namespace ci;
using namespace core;

class SvgTestScenario : public Scenario {
public:

	SvgTestScenario();
	virtual ~SvgTestScenario();

	virtual void setup() override;
	virtual void cleanup() override;
	virtual void resize( ivec2 size ) override;

	virtual void step( const time_state &time ) override;
	virtual void update( const time_state &time ) override;
	virtual void clear( const render_state &state ) override;
	virtual void draw( const render_state &state ) override;

	virtual bool keyDown( const ci::app::KeyEvent &event ) override;

	void reset();

protected:

	void testSimpleSvgLoad();
	void testSimpleSvgGroupOriginTransforms();

};

#endif /* SvgTestScenario_hpp */
