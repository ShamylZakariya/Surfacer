#pragma once

//
//  DrawDispatcher.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 12/8/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#include "Common.h"
#include "RenderState.h"

namespace core {

class GameObject;


class DrawDispatcher 
{
	public:
	
		typedef std::set< GameObject* > GameObjectSet;
		typedef std::vector< GameObject* > GameObjectVec;

		struct visible_object_collector
		{
			GameObjectSet allVisibleObjects;
			GameObjectVec visibleObjectsSortedForDrawing;
			
			// remove a game object from the collection
			void remove( GameObject *obj )
			{
				allVisibleObjects.erase(obj);
				visibleObjectsSortedForDrawing.erase( 
					std::remove(visibleObjectsSortedForDrawing.begin(),visibleObjectsSortedForDrawing.end(),obj),
					visibleObjectsSortedForDrawing.end());
			}
		};
		
	public:
	
		DrawDispatcher();
		virtual ~DrawDispatcher();
		
		void addObject( GameObject * );
		void removeObject( GameObject * );		
		void objectMoved( GameObject * );
		
		void cull( const render_state & );
		void draw( const render_state & );

		/**
			Check if @a obj was visible in the last call to cull()
		*/
		bool visible( const GameObject *obj ) const { return _collector.allVisibleObjects.count(const_cast<GameObject*>(obj)); }

	private:
	
		/**
			render a run of delegates
			returns iterator to last object drawn
		*/
		GameObjectVec::iterator _drawDelegateRun( GameObjectVec::iterator first, GameObjectVec::iterator storageEnd, const render_state &state );
	
	private:
	
		cpSpatialIndex *_index;
		std::set< GameObject* > _alwaysVisibleObjects;
		visible_object_collector _collector;

};


}