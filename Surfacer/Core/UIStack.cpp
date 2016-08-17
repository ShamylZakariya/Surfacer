//
//  UIStack.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/26/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "UIStack.h"

#include "FilterStack.h"
#include "ResourceManager.h"
#include "Scenario.h"

using namespace ci;
using namespace core;
namespace core { namespace ui {

#pragma mark - Stack

/*
		Scenario *_scenario;
		ResourceManager *_resourceManager;
		LayerList _layers;
		LayerSet _suicideLayers, _deferredRemovalLayers;
		Vec2i _size;
*/

Stack::Stack( Scenario *scenario ):
	_scenario( scenario ),
	_resourceManager( new ResourceManager(scenario->resourceManager()) ),
	_size(0,0)
{}

Stack::~Stack()
{
	for( LayerList::iterator layer(_layers.begin()), end(_layers.end()); layer != end; ++layer )
	{
		delete *layer;
	}
}

NotificationDispatcher *Stack::notificationDispatcher() const
{
	return _scenario->notificationDispatcher();
}

void Stack::resize( const Vec2i &newSize )
{
	if ( newSize != _size )
	{
		_size = newSize;
		for( LayerList::iterator layer(_layers.begin()), end(_layers.end()); layer != end; ++layer )
		{
			(*layer)->_resize( _size );
		}
	}		
}

void Stack::update( const time_state &time )
{
	//
	// all layers get private _updated
	//

	for( LayerList::iterator layer(_layers.begin()), end(_layers.end()); layer != end; ++layer )
	{
		(*layer)->_update( time );
	}

	//
	// if a layer requested suicide, do it
	//

	if ( !_suicideLayers.empty() || !_deferredRemovalLayers.empty() )
	{
		LayerList remaining;
		for( LayerList::iterator layer(_layers.begin()), end(_layers.end()); layer != end; ++layer )
		{
			if ( _suicideLayers.count( *layer ))
			{
				delete *layer;
			}
			else if ( !_deferredRemovalLayers.count(*layer) )
			{
				remaining.push_back(*layer);
			}		
		}
		
		_suicideLayers.clear();
		_deferredRemovalLayers.clear();
		_layers = remaining;

		_layerOrderChanged();
	}
	
}

void Stack::draw( const render_state &state )
{
	gl::pushMatrices();

		gl::setViewport( Area(0,0,_size.x,_size.y) );
		gl::setMatricesWindow( _size.x, _size.y, false );
		
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		
		for( LayerList::iterator layer(_layers.begin()), end(_layers.end()); layer != end; ++layer )
		{
			if ( !(*layer)->hidden() )
			{
				(*layer)->_draw( state );
			}
		}

		glDisable( GL_BLEND );
	
	gl::popMatrices();
}

void Stack::pushBottom( Layer *layer )
{
	_layers.push_front( layer );
	layer->_addedToStack( this );
	layer->_resize( size() );
	_layerOrderChanged();
}

Layer *Stack::popBottom( bool deleteLayer )
{
	if ( !_layers.empty() )
	{
		Layer *bottom = _layers.front();
		_layers.pop_front();
		bottom->_removedFromStack(this);
		
		_layerOrderChanged();
		
		if ( deleteLayer )
		{
			delete bottom;
			return NULL;
		}
		
		return bottom;
	}
	
	return NULL;
}

void Stack::pushTop( Layer *layer )
{
	_layers.push_back( layer );
	layer->_addedToStack( this );
	layer->_resize( size() );
	_layerOrderChanged();
}

Layer *Stack::popTop( bool deleteLayer )
{
	if ( !_layers.empty() )
	{
		Layer *top = _layers.back();
		_layers.pop_back();
		top->_removedFromStack(this);
		
		_layerOrderChanged();
		
		if ( deleteLayer )
		{
			delete top;
			return NULL;
		}
		
		return top;
	}
	
	return NULL;
}

Layer *Stack::top() const
{
	return _layers.empty() ? NULL : _layers.back();
}

Layer *Stack::bottom() const
{
	return _layers.empty() ? NULL : _layers.front();
}

void Stack::_layerOrderChanged()
{
	for( LayerList::iterator layer(_layers.begin()),end(_layers.end()); layer != end; ++layer )
	{
		LayerList::iterator nextLayer = layer;
		(*layer)->_setCovered( ++nextLayer != end );
	}
}

void Stack::_layerRequestedSuicide( Layer *layer )
{
	_suicideLayers.insert(layer);
}

void Stack::_layerRequestedDeferredRemoval( Layer *layer )
{
	_deferredRemovalLayers.insert(layer);
}

#pragma mark - Layer::Lifecycle

void Layer::Lifecycle::setPhase( lifecycle_phase p )
{ 
	if ( p != _phase )
	{
		_phase = p; 
		_timeInPhase = 0; 
	}
}


void Layer::Lifecycle::update( const time_state &time )
{
	_timeInPhase += time.deltaT;
	seconds_t phaseDuration = _durations[_phase];

	switch( _phase )
	{
		case TRANSITION_IN:
		{
			if ( _timeInPhase > phaseDuration )
			{
				setPhase( SHOWING );
			}

			break;
		}
		
		case SHOWING:
		{
			if ( phaseDuration > 0 && _timeInPhase > phaseDuration )
			{
				setPhase( TRANSITION_OUT );
			}

			break;
		}
		
		case TRANSITION_OUT:
		{
			break;
		}
	}
}

seconds_t Layer::Lifecycle::transition() const
{
	return saturate( _timeInPhase / _durations[_phase ]);
}

#pragma mark - Layer

/*
		Stack *_stack;
		std::string _name;
		Lifecycle _lifecycle; 
		bool _covered, _dismissalWillDelete, _hidden;
		Vec2i _size;
		real _padding;
		
		ViewSet _views;
		ViewVec _viewDisplayList;
		std::size_t _viewAdditionCounter;
		
		FilterStack *_filterStack;
*/

Layer::Layer( const std::string &name, Lifecycle lifecycle ):
	_stack(NULL),
	_name(name),
	_lifecycle(lifecycle),
	_covered(false),
	_dismissalWillDelete(false),
	_hidden(false),
	_size(0,0),
	_padding(0),
	_viewAdditionCounter(0),
	_filterStack( new FilterStack(NULL))
{
	_filterStack->setBlendMode( BlendMode( GL_ONE, GL_ONE_MINUS_SRC_ALPHA ));
}

Layer::~Layer()
{
	foreach( View *view, _views )
	{
		delete view;
	}
	
	delete _filterStack;
}

void Layer::dismiss( bool deleteWhenDone )
{
	_dismissalWillDelete = deleteWhenDone;
	_lifecycle.setPhase( Lifecycle::TRANSITION_OUT );
}

void Layer::addView( View *view )
{
	if ( _views.insert(view).second )
	{
		view->_addedToLayer(this);
		view->zIndexChanged.connect( this, &Layer::_viewZIndexChanged );
		view->_setChildIndexInLayer( _viewAdditionCounter++ );
		_updateViewDisplayList();
		
		view->_setReady( isReady() );
	}
}

void Layer::removeView( View *view )
{
	if ( _views.erase(view) )
	{
		view->_removedFromLayer(this);
		view->_setReady( false );
		view->zIndexChanged.disconnect( this );
		_updateViewDisplayList();
	}
}

void Layer::setPadding( real padding )
{
	_padding = std::max<real>( padding, 0 );

	//
	//	Padding affects layout, so notify self and children
	//

	resize( _size );
	for( ViewSet::iterator view(_views.begin()),end(_views.end()); view != end; ++view )
	{
		(*view)->layerResized( _size );
	}
}

void Layer::_resize( const Vec2i &newSize )
{
	if ( newSize != _size )
	{
		_size = newSize;
		_filterStack->resize( newSize );

		resize( newSize );
		for( ViewSet::iterator view(_views.begin()),end(_views.end()); view != end; ++view )
		{
			(*view)->layerResized( newSize );
		}
	}
}

void Layer::_update( const core::time_state &time )
{
	update(time);

	//
	//	Remove dismissed views from the view set
	//

	ViewSet dismissedViews;
	for( ViewSet::iterator view(_views.begin()),end(_views.end()); view != end; ++view )
	{
		View *v = *view;
		if ( v->_dismissed )
		{
			dismissedViews.insert(v);
		}
	}

	if ( !dismissedViews.empty())
	{
		for( ViewSet::iterator view(dismissedViews.begin()),end(dismissedViews.end()); view != end; ++view )
		{
			View *v = *view;

			// this will update the view display list too
			removeView( v );
			if ( v->_dismissedShouldDelete )
			{
				delete v;
			}
		}
	}

	//
	//	Now update visible views
	//

	for( ViewVec::iterator view(_viewDisplayList.begin()),end(_viewDisplayList.end()); view != end; ++view )
	{
		(*view)->_update(time);
	}

	_filterStack->update( time );

	_lifecycle.update( time );	
	if ( _lifecycle.atEnd() )
	{
		if ( _dismissalWillDelete ) _stack->_layerRequestedSuicide( this );
		else _stack->_layerRequestedDeferredRemoval( this );
	}	
}

void Layer::_draw( const render_state &state )
{
	assert( !hidden());

	const bool
		ExecuteFilters = state.mode == RenderMode::GAME,
		BlitRespectsAlpha = true;
	
	gl::pushModelView();
	{
		gl::SaveFramebufferBinding bindingSaver;
		_filterStack->beginCapture( ColorA(0,0,0,0) );
		
		//
		//	Draw
		//

		draw( state );
		
		for( ViewVec::iterator view(_viewDisplayList.begin()),end(_viewDisplayList.end()); view != end; ++view )
		{
			View *v = *view;
			if ( !v->hidden() && !v->_dismissed )
			{
				v->_draw( state );
			}
		}

		//
		//	now run filters
		//
		
		_filterStack->endCapture();

		if ( ExecuteFilters )
		{
			_filterStack->execute( state, true, BlitRespectsAlpha );
		}
		else
		{
			_filterStack->blitToScreen( BlitRespectsAlpha );
		}
		
	}
	gl::popModelView();
}

void Layer::_addedToStack( Stack *s )
{
	_stack = s;
	_filterStack->setResourceManager( _stack->resourceManager() );
	_lifecycle.setPhase( Lifecycle::TRANSITION_IN );

	//
	//	If we have any Views already, let them know we're ready
	//

	const bool Ready = isReady();
	foreach( View *view, _views )
	{
		view->_setReady( Ready );
	}

	addedToStack( s );
}

void Layer::_removedFromStack( Stack *s )
{
	_stack = NULL;
	foreach( View *view, _views )
	{
		view->_setReady( false );
	}

	removedFromStack( s );
}

namespace {

	bool zIndexComparator( View *a, View *b )
	{
		if ( a->zIndex() != b->zIndex() )
		{
			return a->zIndex() < b->zIndex();
		}
		
		return a->childIndexInLayer() < b->childIndexInLayer();
	}

}

void Layer::_updateViewDisplayList()
{
	_viewDisplayList.resize(_views.size() );
	std::copy( _views.begin(), _views.end(), _viewDisplayList.begin() );
	std::sort( _viewDisplayList.begin(), _viewDisplayList.end(), zIndexComparator );
}

#pragma mark - View

/*
		Layer *_layer;
		bool _ready, _hidden, _dismissed, _dismissedShouldDelete;
		std::string _name;
		cpBB _bounds;
		int _zIndex;
		ci::ColorA _debugColor;
		std::size_t _childIndexInLayer;
*/


View::View( const std::string &name ):
	_layer(NULL),
	_ready(false),
	_hidden(false),
	_dismissed(false),
	_dismissedShouldDelete(false),
	_name(name),
	_bounds(cpBBNew(0,0,0,0)),
	_zIndex(0),
	_debugColor( RandomColor(), 0.25 ),
	_childIndexInLayer(0)
{}

View::~View()
{}

void View::setBounds( const cpBB &bounds )
{
	_bounds = bounds;
	boundsChanged(bounds);
}

void View::setZIndex( int z )
{
	if ( z != _zIndex )
	{
		_zIndex = z;
		zIndexChanged(this);
	}
}

void View::dismiss( bool deleteMe )
{
	_dismissed = true;
	_dismissedShouldDelete = true;
}

void View::_update( const time_state &time )
{
	update(time);
}

void View::_draw( const render_state &state )
{
	assert( !hidden());

	if ( state.mode == RenderMode::DEBUG )
	{
		cpBB bounds = this->bounds();
		gl::color( _debugColor );
		gl::drawStrokedRect( Rectf( bounds.l, bounds.b, bounds.r, bounds.t ));
	}

	draw( state );
}

void View::_addedToLayer( Layer *l )
{
	_layer = l;
	addedToLayer(l);
}

void View::_removedFromLayer( Layer *l )
{
	_layer = NULL;
	removedFromLayer(l);
}

void View::_setReady( bool ready )
{
	if ( ready != _ready )
	{
		_ready = ready;
		if ( _ready ) this->ready();
	}
}


}}
