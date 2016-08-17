//
//  DrawDispatcher.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 12/8/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#include "DrawDispatcher.h"
#include "GameObject.h"
#include "Level.h"

#include <cinder/app/AppBasic.h>

using namespace ci;
namespace core {

namespace {

	cpBB gameObjectBBFunc( void *obj )
	{
		return ((GameObject*)obj)->aabb();
	}
	
	void visibleObjectCollector(void *obj1, void *obj2, void *data)
	{
		DrawDispatcher *dispatcher = (DrawDispatcher*) obj1;
		GameObject *object = (GameObject*) obj2;
		DrawDispatcher::visible_object_collector *collector = (DrawDispatcher::visible_object_collector*) data;

		//
		//	We don't want dupes in visibleObjectsSortedForDrawing, so filter based on whether
		//	the object made it into the set
		//

		if( collector->allVisibleObjects.insert(object).second )
		{
			collector->visibleObjectsSortedForDrawing.push_back(object);
		}
	}
	
	inline bool visibleObjectDisplaySorter( GameObject *a, GameObject *b )
	{
		//
		// draw lower layers before higher layers
		//

		if ( a->layer() != b->layer() )
		{
			return a->layer() < b->layer();
		}

		//
		//	If we're here, the objects are on the same layer. So now
		//	we want to group objects with the same batchDrawDelegate
		//

		if ( a->batchDrawDelegate() != b->batchDrawDelegate() )
		{
			return a->batchDrawDelegate() < b->batchDrawDelegate();
		}

		//
		//	If we're here, the two objects have the same ( possibly null )
		//	batchDrawDelegate, so, just order them from older to newer
		//
		
		return a->instanceId() < b->instanceId();
	}
	
}

/*
		cpSpatialIndex *_index;
		std::set< GameObject* > _alwaysVisibleObjects;
		visible_object_collector _collector;
*/

DrawDispatcher::DrawDispatcher():
	_index(cpBBTreeNew( gameObjectBBFunc, NULL ))
{}
	
DrawDispatcher::~DrawDispatcher()
{
	cpSpatialIndexFree( _index );
}

void DrawDispatcher::addObject( GameObject *obj )
{
	switch( obj->visibilityDetermination() )
	{
		case VisibilityDetermination::ALWAYS_DRAW:
			_alwaysVisibleObjects.insert( obj );
			break;
			
		case VisibilityDetermination::FRUSTUM_CULLING:
			cpSpatialIndexInsert( _index, obj, obj->instanceId() );
			break;
			
		case VisibilityDetermination::NEVER_DRAW:
			break;			
	}
}

void DrawDispatcher::removeObject( GameObject *obj )
{
	switch( obj->visibilityDetermination() )
	{
		case VisibilityDetermination::ALWAYS_DRAW:
		{
			_alwaysVisibleObjects.erase( obj );
			break;
		}
			
		case VisibilityDetermination::FRUSTUM_CULLING:
		{
			cpSpatialIndexRemove( _index, obj, obj->instanceId() );
			break;
		}
			
		case VisibilityDetermination::NEVER_DRAW:
			break;			
	}
	
	//
	//	And, just to be safe, remove this object from the current visible set
	//

	_collector.remove(obj);
}

void DrawDispatcher::objectMoved( GameObject *obj )
{
	if ( obj->visibilityDetermination() == VisibilityDetermination::FRUSTUM_CULLING )
	{
		cpSpatialIndexReindexObject( _index, obj, obj->instanceId() );
	}
}

void DrawDispatcher::cull( const render_state &state )
{
	//
	//	clear storage - note: std::vector doesn't free, it keeps space reserved.
	//

	_collector.allVisibleObjects.clear();
	_collector.visibleObjectsSortedForDrawing.clear();
		
	//
	//	Copy over objects which are always visible
	//

	for( GameObjectSet::const_iterator obj(_alwaysVisibleObjects.begin()),end(_alwaysVisibleObjects.end()); obj != end; ++obj )
	{
		_collector.allVisibleObjects.insert(*obj);
		_collector.visibleObjectsSortedForDrawing.push_back(*obj);
	}

	//
	//	Find those objects which use frustum culling and which intersect the current view frustum
	//

	cpSpatialIndexQuery( _index, this, state.viewport.frustum(), visibleObjectCollector, &_collector );
	
	//
	//	Sort them all
	//
	
	std::sort( 
		_collector.visibleObjectsSortedForDrawing.begin(), 
		_collector.visibleObjectsSortedForDrawing.end(), 
		visibleObjectDisplaySorter );

}

void DrawDispatcher::draw( const render_state &state )
{
	render_state renderState = state;
		
	for( GameObjectVec::iterator 
		objIt(_collector.visibleObjectsSortedForDrawing.begin()),
		end(_collector.visibleObjectsSortedForDrawing.end());
		objIt != end;
		++objIt )
	{
		GameObject *obj = *objIt;
		BatchDrawDelegate *delegate = obj->batchDrawDelegate();
		int drawPasses = obj->drawPasses();

		GameObjectVec::iterator newObjIt = objIt;
		for( renderState.pass = 0; renderState.pass < drawPasses; ++renderState.pass )
		{
			if ( delegate )
			{
				newObjIt = _drawDelegateRun(objIt, end, renderState );
			}
			else
			{
				obj->dispatchDraw( renderState );
			}
		}
		
		objIt = newObjIt;		
	} 
}

GameObjectVec::iterator DrawDispatcher::_drawDelegateRun( 
	GameObjectVec::iterator firstInRun, 
	GameObjectVec::iterator storageEnd, 
	const render_state &state )
{
	GameObjectVec::iterator objIt = firstInRun;
	GameObject *obj = *objIt;
	BatchDrawDelegate *delegate = obj->batchDrawDelegate();

	delegate->prepareForBatchDraw( state, obj );
	
	for( ; objIt != storageEnd; ++objIt )
	{	
		//
		//	If the delegate run has completed, clean up after our run
		//	and return the current iterator.
		//

		if ( (*objIt)->batchDrawDelegate() != delegate )
		{
			delegate->cleanupAfterBatchDraw( state, *firstInRun, obj );
			return objIt - 1;
		}

		obj = *objIt;
		obj->dispatchDraw( state );
	} 
	
	//
	//	If we reached the end of storage, run cleanup
	//

	delegate->cleanupAfterBatchDraw( state, *firstInRun, obj );

	return objIt - 1;
}


}