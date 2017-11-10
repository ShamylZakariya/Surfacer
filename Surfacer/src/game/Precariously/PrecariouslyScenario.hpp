//
//  SurfacerScenario.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 10/5/17.
//

#ifndef PrecariouslyStageScenario_hpp
#define PrecariouslyStageScenario_hpp

#include "Core.hpp"
#include "Terrain.hpp"

namespace precariously {
	
	
	class PrecariouslyScenario : public core::Scenario {
	public:
		
		PrecariouslyScenario(string stageXmlFile);
		virtual ~PrecariouslyScenario();
		
		virtual void setup() override;
		virtual void cleanup() override;
		virtual void resize( ivec2 size ) override;
		
		virtual void step( const core::time_state &time ) override;
		virtual void update( const core::time_state &time ) override;
		virtual void drawScreen( const core::render_state &state ) override;
		
		virtual bool keyDown( const ci::app::KeyEvent &event ) override;
		
		void reset();
		
	private:
		
		string _stageXmlFile;
		
	};
	
}

#endif /* PrecariouslyStageScenario_hpp */
