#pragma once

//
//  UIStack.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/26/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Common.h"
#include "Notification.h"
#include "RenderState.h"
#include "SignalsAndSlots.h"
#include "TimeState.h"

#include <list>

namespace core { 

class FilterStack;
class ResourceManager;
class Scenario;

namespace ui {

class Stack;
class Layer;
class View;

typedef std::set<View*> ViewSet;
typedef std::vector<View*> ViewVec;


class Stack : public signals::receiver, public NotificationListener
{
	public:
	
		typedef std::list< Layer* > LayerList;
		typedef std::set< Layer* > LayerSet;
	
	public:
	
		Stack( Scenario * );
		virtual ~Stack();
		
		Scenario *scenario() const { return _scenario; }
		ResourceManager *resourceManager() const { return _resourceManager; }
		NotificationDispatcher *notificationDispatcher() const;
	
		virtual void resize( const Vec2i &newSize );
		virtual void update( const time_state & );
		virtual void draw( const render_state & );
		
		Vec2i size() const { return _size; }
		
		/**
			add a layer to the bottom of the display stack
		*/
		void pushBottom( Layer * );
		
		/**
			Remove layer at bottom of display stack.
			if deleteLayer is true, the layer will be deleted and NULL returned,
			else the removed layer will be returned.
			If the stack is empty, NULL will be returned.
		*/
		Layer *popBottom( bool deleteLayer = false );

		/**
			add a layer to the top of the display stack
		*/
		void pushTop( Layer * );
		
		/**
			Remove layer at top of display stack.
			if deleteLayer is true, the layer will be deleted and NULL returned,
			else the removed layer will be returned.
			If the stack is empty, NULL will be returned.
		*/
		Layer *popTop( bool deleteLayer = false );
		
		
		const LayerList &layers() const { return _layers; }
		LayerList &layers() { return _layers; }
		bool empty() const { return _layers.empty(); }
		
		/**
			Get the layer at top of display stack, or NULL if empty
		*/
		Layer *top() const;

		/**
			Get the layer at bottom of display stack, or NULL if empty
		*/
		Layer *bottom() const;
		
		
	protected:
	
		friend class Layer;
	
		void _layerOrderChanged();
		void _layerRequestedSuicide( Layer *layer );
		void _layerRequestedDeferredRemoval( Layer *layer );
		
	private:
	
		Scenario *_scenario;
		ResourceManager *_resourceManager;
		LayerList _layers;
		LayerSet _suicideLayers, _deferredRemovalLayers;
		Vec2i _size;

};

class Layer : public signals::receiver, public NotificationListener
{
	public:

		class Lifecycle 
		{
			public:

				enum lifecycle_phase {
					TRANSITION_IN,
					SHOWING,
					TRANSITION_OUT
				};
				
				Lifecycle( seconds_t generalDuration = 0.5 ):
					_phase( TRANSITION_IN ),
					_timeInPhase(0)
				{
					_durations[TRANSITION_IN] = generalDuration;
					_durations[SHOWING] = 0;
					_durations[TRANSITION_OUT] = generalDuration;
				}
				
				Lifecycle( 
					seconds_t inDuration, 
					seconds_t showingDuration, 
					seconds_t exitDuration ):
					_phase( TRANSITION_IN ),
					_timeInPhase(0)
				{
					_durations[TRANSITION_IN] = inDuration;
					_durations[SHOWING] = showingDuration;
					_durations[TRANSITION_OUT] = exitDuration;
				}
				
				void update( const time_state & );
				
				lifecycle_phase phase() const { return _phase; }
				void setPhase( lifecycle_phase p );
				
				/**
					Set how long a phase takes to transition.
					If SHOWING phase is non-zero, the Layer will run in the SHOWING phase
					for that number of seconds, and then will switch to TRANSITION_OUT, and will
					then be automatically removed at the end of the cycle.
				*/
				seconds_t phaseTransitionDuration( lifecycle_phase phase ) const { return _durations[phase]; }
				void setPhaseTransitionDuration( lifecycle_phase phase, seconds_t duration ) { _durations[phase] = duration; }

				/**
					Returns a value from [0,1] representing the time-based progress of the transition
					to the current phase.
					For example, if the phase is TRANSITION_IN, and transition() is 0.7, that means we're 70%
					of the way through the TRANSITION_IN lifecycle phase, and a simple implementation would be
					to draw the layer contents at 70% opacity.					
				*/
				seconds_t transition() const;
				
								
				bool atStart() const { return _phase == TRANSITION_IN && transition() <= Epsilon; }
				bool atEnd() const { return _phase == TRANSITION_OUT && transition() >= (seconds_t(1) - Epsilon); }

			private:
			
				lifecycle_phase _phase;
				seconds_t _timeInPhase, _durations[5];

		};

	public:
	
		Layer( const std::string &name, Lifecycle lifecycle = Lifecycle() );
		virtual ~Layer();
		
		const std::string &name() const { return _name; }		
		const Lifecycle &lifecycle() const { return _lifecycle; }

		Scenario *scenario() const { return _stack->scenario(); }
		Stack *stack() const { return _stack; }
		ResourceManager *resourceManager() const { return _stack->resourceManager(); }
		NotificationDispatcher *notificationDispatcher() const { return _stack->notificationDispatcher(); }
		
		// returns true if this layer is beneath another layer
		bool covered() const { return _covered; } 
		
		// get the screen size of the Layer
		Vec2i size() const { return _size; }
		cpBB bounds() const { return cpBBNew(0, 0, _size.x, _size.y ); }
		
		virtual void update( const time_state & ){}
		virtual void draw( const render_state & ){}
		virtual void resize( const Vec2i &newSize ){}
		virtual void addedToStack( Stack *s ){}
		virtual void removedFromStack( Stack *s ){}
		
		/**
			Called when this Layer is "ready", e.g., has access to the Scenario, Stack, ResourceManager, etc
		*/
		virtual void ready(){}
		
		/**
			Transition this layer out, removing it from the stack when finished, and if 
			@a deleteWhenDone is true, the layer will be deleted. Otherwise you're responsible 
			to have held on to it to use again layer or delete it yourself
		*/
		void dismiss( bool deleteWhenDone );
		
		void addView( View *view );
		void removeView( View *view );
		const ViewSet &views() const { return _views; }
		
		FilterStack *filters() const { return _filterStack; }

		bool isReady() const { return _stack && _stack->scenario(); }
		
		/*
			Hiding a layer causes it to not be drawn, nor its children, nor its filters.
			It will be skipped entirely by the owning Stack.
		*/
		virtual void setHidden( bool h ) { _hidden = h; }
		bool hidden() const { return _hidden; }

		/*
			Set the padding to be used for this layer.
			Padding doesn't mean anything specific, it's just a way for a layer to say its children should lay
			out their contents with a common padding. Its up to subclasses to use this info meaningfully.
		*/
		void setPadding( real padding );
		real padding() const { return _padding; }

	protected:
	
		friend class Stack;		
		virtual void _setCovered( bool c ) { _covered = c; }
		virtual void _resize( const Vec2i &newSize );
		virtual void _update( const time_state & );
		virtual void _draw( const render_state & );
		virtual void _addedToStack( Stack *s );
		virtual void _removedFromStack( Stack *s );
		
		void _viewZIndexChanged(View*) { _updateViewDisplayList(); }
		virtual void _updateViewDisplayList();
		
	private:

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

};

class View : public signals::receiver, public NotificationListener
{
	public:
	
		signals::signal< void(View*) > zIndexChanged;

	public:
	
		View( const std::string &name );
		virtual ~View();

		const std::string &name() const { return _name; }

		Scenario *scenario() const { return _layer->stack()->scenario(); }		
		Stack *stack() const { return _layer->stack(); }
		Layer *layer() const { return _layer; }
		ResourceManager *resourceManager() const { return _layer->resourceManager(); }		
		NotificationDispatcher *notificationDispatcher() const { return _layer->notificationDispatcher(); }
		
		void setBounds( const cpBB &bounds );
		cpBB bounds() const { return _bounds; }
		
		void setZIndex( int z );
		int zIndex() const { return _zIndex; }
		/**
			Get this view's index in the parent layer - meaning if this was added third to the layer,
			the value will be 2 ( 0-indexed )
		*/
		std::size_t childIndexInLayer() const { return _childIndexInLayer; }

		virtual void boundsChanged( const cpBB &newBounds ){}
		virtual void layerResized( const Vec2i &newLayerSize ){}
		virtual void update( const time_state & ){}
		virtual void draw( const render_state & ){}
		virtual void addedToLayer( Layer *l ){}
		virtual void removedFromLayer( Layer *l ){}

		/**
			Called when this View can assume it has access to the Stack, Scenario, Layer, ResourceManager, etc.
		*/
		virtual void ready(){}
		
		/**
			Return true iff this View is Ready, e.g.  it has access to the Stack, Scenario, Layer, ResourceManager, etc. 
		*/
		bool isReady() const { return _ready; }
		
		/**
			Cause parent layer to remove this view, and optionally delete it.
			Note: This happens in the next update() loop, since otherwise this would be indirectly
			equivalent to calling `delete this`.
		*/
		virtual void dismiss( bool deleteMe = true );
		bool dismissed() const { return _dismissed; }
		
		/**
			Hiding a View prevents it (and its children) from being drawn by the parent layer.
		*/
		virtual void setHidden( bool h ) { _hidden = h; }
		bool hidden() const { return _hidden; }
		
	protected:
	
		friend class Layer;
		void _update( const time_state & );
		void _draw( const render_state & );
		void _addedToLayer( Layer *l );
		void _removedFromLayer( Layer *l );
		void _setReady( bool ready );
		void _setChildIndexInLayer( std::size_t c ) { _childIndexInLayer = c; }

	private:
	
		Layer *_layer;
		bool _ready, _hidden, _dismissed, _dismissedShouldDelete;
		std::string _name;
		cpBB _bounds;
		int _zIndex;
		ci::ColorA _debugColor;
		std::size_t _childIndexInLayer;

};


}} // end namespace ui
