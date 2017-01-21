#pragma once

//
//  Scenario.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/14/11.
//  Copyright 2011 Shamyl Zakariya. All rights reserved.
//

#include <cinder/app/App.h>
#include "InputDispatcher.h"
#include "Level.h"
#include "Notification.h"
#include "SignalsAndSlots.h"
#include "ResourceManager.h"
#include "UIStack.h"


namespace core {

namespace util
{
	namespace rich_text
	{
		class Renderer;
	}
}



class Scenario : public signals::receiver, public NotificationListener
{
	public:
	
		Scenario();
		virtual ~Scenario();

		virtual void setup(){}
		virtual void shutdown(){}
		virtual void resize( Vec2i size ){}
		virtual void step( const time_state &time );
		virtual void update( const time_state &time );
		virtual void draw( const render_state &state );

		virtual void dispatchSetup();
		virtual void dispatchShutdown();
		virtual void dispatchResize( const Vec2i &size );
		virtual void dispatchStep();
		virtual void dispatchUpdate();
		virtual void dispatchDraw();
		
		/**
			This is the root ResourceManager for the game.
			Each Level instance creates a child ResourceManager of this one.
		*/
		ResourceManager *resourceManager() const { return _resourceManager; }
		
		/**
			Get the ui stack instance
		*/	
		ui::Stack *uiStack() const { return _uiStack; }
		
		/**
			Get the rich text rendering server
		*/
		util::rich_text::Renderer *richTextRenderer() const { return _richTextRenderer; }
		
		/**
			Get the notification dispatcher for broadcasting notifications to all
			GameObjects, Behaviors, and UI elements.
		*/
		NotificationDispatcher *notificationDispatcher() const { return _notificationDispatcher; }
		
		const Viewport& camera() const { return _camera; }
		Viewport& camera() { return _camera; }
		const time_state &time() const { return _time; }
		const time_state &stepTime() const { return _stepTime; }
		const render_state &renderState() const { return _renderState; }

		/**
			Get filter stack which is applied to composited rendered output of level
		*/
		FilterStack *filters() const { return _filters; }
		
		/**
			Get the current level
		*/
		
		Level *level() const { return _level; }
		virtual void setLevel( Level *l );

		void setRenderMode( RenderMode::mode mode );
		RenderMode::mode renderMode() const { return _renderState.mode; }
		
		/**
			Save a screenshot as PNG to @a path
		*/
		void screenshot( const ci::fs::path &folderPath, const std::string &namingPrefix, const std::string format = "png" );
		
	private:
	
		ResourceManager				*_resourceManager;
		NotificationDispatcher		*_notificationDispatcher;
		util::rich_text::Renderer	*_richTextRenderer;
		ui::Stack					*_uiStack;
		Level						*_level;
		FilterStack					*_filters;

		Viewport					_camera;
		time_state					_time, _stepTime;
		render_state				_renderState;
		
};

}