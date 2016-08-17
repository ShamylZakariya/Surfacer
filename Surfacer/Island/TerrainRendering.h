#pragma once

//
//  IslandRendering.h
//  PhysicsIntegration4
//
//  Created by Shamyl Zakariya on 5/10/11.
//  Copyright 2011 Shamyl Zakariya. All rights reserved.
//

#include "GameObject.h"
#include <cinder/gl/Vbo.h>

namespace game { namespace terrain {

class Island;
struct triangle;
struct perimeter_greeble_vertex;

class IslandRenderer : public core::DrawComponent 
{
	public:

		IslandRenderer();
		virtual ~IslandRenderer();
		
		void draw( const core::render_state &state );
		void reset();

	private:
	
		void _render_Geometry_Game( Island *island, const core::render_state &state );
		void _render_Geometry_Development( Island *island, const core::render_state &state );
		void _render_Geometry_Debug( Island *island, const core::render_state &state );

		void _render_DebugVoxels( Island *island, const core::render_state &state );
		void _render_DebugOverlay( Island *island, const core::render_state &state );

		void _render_Greebling_Game( Island *island, const core::render_state &state );
		void _render_Greebling_Development( Island *island, const core::render_state &state );
		void _render_Greebling_Debug( Island *island, const core::render_state &state );

	private:
	
		GLuint _geometryVbo, _geometryVboVertexCount, _greeblingVbo, _greeblingVboVertexCount;
};
	
}} // end namespace game::terrain