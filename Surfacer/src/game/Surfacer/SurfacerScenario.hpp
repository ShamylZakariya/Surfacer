//
//  SurfacerScenario.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 10/5/17.
//

#ifndef SurfacerStageScenario_hpp
#define SurfacerStageScenario_hpp

#include "Core.hpp"
#include "Terrain.hpp"

namespace surfacer {
	
	
	class SurfacerScenario : public core::Scenario {
	public:
		
		SurfacerScenario(string stageXmlFile);
		virtual ~SurfacerScenario();
		
		virtual void setup() override;
		virtual void cleanup() override;
		virtual void resize( ivec2 size ) override;
		
		virtual void step( const core::time_state &time ) override;
		virtual void update( const core::time_state &time ) override;
		virtual void clear( const core::render_state &state ) override;
		virtual void drawScreen( const core::render_state &state ) override;
		
		virtual bool keyDown( const ci::app::KeyEvent &event ) override;
		
		void reset();
		
	private:
		
		string _stageXmlFile;
		
	};
	
}

#endif /* SurfacerStageScenario_hpp */
