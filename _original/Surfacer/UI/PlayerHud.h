#pragma once

//
//  PlayerHud.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/26/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "InputDispatcher.h"
#include "SvgObject.h"
#include "UIStack.h"
#include "Spring.h"
#include "RichText.h"

#include <cinder/gl/GlslProg.h>
#include <cinder/gl/Texture.h>


namespace core {

	class GameObject;
	class Scenario;
	class DamagedMonitorFilter;

}

namespace game { 

class Player;
class Weapon;
class MagnetoBeam;

class RotaryDial;
class SelectionList;
class MagnetoBeamQueryOverlay;

class PlayerHud : public core::ui::Layer 
{
	public:
	
		PlayerHud( core::Scenario *s );
		virtual ~PlayerHud();
		
		// NotificationListener
		virtual void notificationReceived( const core::Notification &note );

		// Layer
		virtual void addedToStack( core::ui::Stack *s );
		virtual void resize( const Vec2i &newSize );
		virtual void update( const core::time_state & );
		virtual void draw( const core::render_state & );
		
	private:
	
		void _playerWeaponChanged( Player *, Weapon * );
		
	private:
	
		core::Scenario *_scenario;
		RotaryDial *_health, *_power;
		MagnetoBeamQueryOverlay *_magnetoBeamOverlay;
		SelectionList *_weaponList;
		core::DamagedMonitorFilter *_injuryEffectFilterHud;

};

class ImageView : public core::ui::View
{
	public:
	
		enum fit_type {
			FIT,		/* image is scaled to fit view, no overlap or cropping */
			FILL,		/* image is scaled to fill view */
			NATURAL,	/* image is shown at natural size */
			STRETCH,	/* image is scaled to fit view completely, without respect for aspect ratio */
		};
		
		struct init {
			ci::gl::Texture image;
			fit_type fit;
			Vec2 alignment;	/* goes from 0 to 1 in x and y to position image, with 0.5,0.5 being centered */
			ci::ColorA color;
			
			init():
				fit(FILL),
				alignment(0.5, 0.5),
				color(1,1,1,1)
			{}
		};
				
	public:
	
		ImageView( const std::string &name );
		virtual ~ImageView();

		void initialize( const init &initializer );
		const init &initializer() const { return _initializer; }
		
		// View
		virtual void boundsChanged( const cpBB &newBounds );
		virtual void draw( const core::render_state & );
		
		// ImageView
		void setColor( const ci::ColorA &color ) { _initializer.color = color; }
		ci::ColorA color() const { return _initializer.color; }
		
		void setAlignment( const Vec2 &align ) { _initializer.alignment = saturate( align ); _invalidateLayout(); }
		Vec2 alignment() const { return _initializer.alignment; }
		
		void setFitType( fit_type type ) { _initializer.fit = type; _invalidateLayout(); }
		fit_type fitType() const { return _initializer.fit; }
		
		void setImage( const ci::gl::Texture &img ) { _initializer.image = img; _invalidateLayout(); }
		ci::gl::Texture image() const { return _initializer.image; }

	private:
	
		void _invalidateLayout();
		void _layout();

	private:
	
		bool _layoutNeeded;
		init _initializer;
		ci::Rectf _imageRect; 

};


class RotaryDial : public core::ui::View
{
	public:
	
		struct init {
		
			ci::ColorA ringColor;
			ci::ColorA valueColor;
			ci::ColorA increaseNotificationColor;
			ci::ColorA decreaseNotificationColor;
			real value, innerRadius, outerRadius;
			core::SvgObject iconSvg;
			ci::gl::Texture iconTex;
			ci::Font font;
		
			init():
				ringColor(1,1,1,0.5),
				valueColor(1,0,1,1),
				increaseNotificationColor(1,1,1,1),
				decreaseNotificationColor(1,0,0,1),				
				value(0),
				innerRadius(80),
				outerRadius(100)
			{}		
		};

	public:
	
		RotaryDial( const std::string &name );
		virtual ~RotaryDial();
		
		void initialize( const init &initializer );
		const init &initializer() const { return _initializer; }
		
		// View
		virtual void boundsChanged( const cpBB &newBounds ); 
		virtual void addedToLayer( core::ui::Layer *l );
		virtual void update( const core::time_state & );
		virtual void draw( const core::render_state & );
		
		// RotaryDial
		
		void setValue( real value );
		real value() const;

		void notifyIncrease();
		void notifyDecrease();
		
	protected:

		struct ring_vertex {
			Vec2 position;
			real radians;
		};
		
		typedef std::vector< ring_vertex > ring_vertex_vec;
		
		struct display_state {
			bool needsLayout;
			int whole;
			real fractional;
			Vec2 iconSize, labelSize, iconPosition, labelPosition;
			ci::Rectf iconRect;
			real iconScale;
			
			display_state():
				needsLayout(true),
				whole(0),
				fractional(0),
				iconScale(0)
			{}
		};
								
	protected:
	
		virtual void _layout( display_state & );
		virtual void _renderLabel( display_state & );

		virtual void _displayRing( const core::render_state &, const display_state & );
		virtual void _displayIcon( const core::render_state &, const display_state & );
		virtual void _displayLabel( const core::render_state &, const display_state & );
			
	private:
	
		init _initializer;
		real _value, _targetValue;
		core::util::Spring<real> _scaleSpring;

		ci::gl::Texture _ringTexture;
		ci::gl::GlslProg _ringShader;

		ci::gl::GlslProg _iconShader;
		ci::gl::Texture _icon;
		
		ci::gl::Texture _labelTexture;
		ci::Font _labelFont;		

		display_state _displayState;
};

class SelectionList : public core::ui::View
{
	public:
	
		struct item {
			std::string name;
			boost::any data;
			
			item( const std::string &i, const boost::any &d = boost::any() ):
				name(i),
				data(d)
			{}
			
			operator bool() const { return !name.empty(); }
		};
		
		struct init {
			ci::Font font;
			ci::ColorA selectedColor;
			ci::ColorA color;
		};
		
		typedef std::vector< item > item_vec;

	public:
	
		SelectionList( const std::string &name );
		virtual ~SelectionList();
		
		// Initialization
		virtual void initialize( const init &i );
		const init &initializer() const { return _initializer; }
		
		// View
		virtual void addedToLayer( core::ui::Layer *l );
		virtual void update( const core::time_state & );
		virtual void draw( const core::render_state & );
		
		// SelectionList
		void addItem( const item &item );
		void removeItem( const std::string &name );
		const item_vec &items() const;
		
		void selectItem( const std::string &name );
		// pass < 0 to select nothing
		void selectItem( int idx );
		const item &selectedItem() const;
		int selectedItemIndex() const;
		
		void setPosition( const Vec2 &position );
		Vec2 position() const;
		
		real lineHeight() const { return _lineHeight; }
		
	protected:
	
		virtual void _invalidate();
		virtual ci::gl::Texture _render( const core::render_state &state ) const;
		
	private:

		init _initializer;
		item_vec _items;
		ci::gl::Texture _compositedListTexture;
		Vec2 _position;
		real _lineHeight, _yOffset;
		int _selectedItemIndex;
		core::util::Spring<real> _yOffsetSpring;
};

class MagnetoBeamQueryOverlay : public core::ui::View 
{
	public:
	
		MagnetoBeamQueryOverlay();
		virtual ~MagnetoBeamQueryOverlay();
		
		// View
		virtual void addedToLayer( core::ui::Layer *l );
		virtual void update( const core::time_state & );
		virtual void draw( const core::render_state & );
		
		// MagnetoBeamQueryOverlay
		MagnetoBeam *beam() const;
	
};

class RichTextBox : public core::ui::View
{
	public:

		struct init {
		
			// if true, the box will be centered on screen, will display a blanking layer, and will pause
			// the game while displayed. Will go away and unpause game when clicked.
			bool modal;
		
			/**
				position of box relative to screen
				a value of 0,0 would put the box at the lower-left corner of the screen,
				and a value of 1,1 would put the box all the way to the top-right of the screen.
			*/
			Vec2 position;

			/**
				identifier of a GameObject.
				If this GameObject exists, the RichTextBox will be placed above the target on screen.
				If/When the GameObject is destroyed, the RichTextBox will dispose of itself automatically.
			*/
			std::string trackingTargetIdentifier;

			// number of seconds to display, before fading out and removing self from layer
			seconds_t displayTime;
			
			//	min number of seconds this richtext box will display. If non-zero, any calls to fadeOut() ( or displayTime timeout )
			//	will be deferred until the minimumDisplayTime elapses
			seconds_t minimumDisplayTime;

			// name of template HTML file, minus '.html' extension
			std::string templateHtmlFile;
			std::string htmlContent;
			std::string bodyClass;
			
			//	color of blanking layer drawn behind modal RichTextBox
			ci::ColorA blankingColor;


			init():
				modal(false),
				position(0,0),
				displayTime(0),
				minimumDisplayTime(1),
				blankingColor(0.4,0.4,0.4,0.8)
			{}
		};

	public:
	
		RichTextBox();
		virtual ~RichTextBox();
		
		virtual void initialize( const init &i );
		const init &initializer() const { return _initializer; }

		// View
		virtual void layerResized( const Vec2i &newLayerSize );
		virtual void update( const core::time_state & );
		virtual void ready();
		virtual void draw( const core::render_state & );
		
		// RichTextBox

		/**
			Position is relative, in range [0,1] where 0 if left/bottom and 1 is right/top of screen.
		*/
		void setPosition( const Vec2 &p );
		Vec2 position() const { return _position; }
		
		void setOpacity( real opacity ) { _alpha = saturate(opacity); }
		real opacity() const { return _alpha; }
		
		/**
			Causes this RichTextBox to fade in from zero to opacity() over a period of @a duration seconds.
			If the text isn't ready yet ( due to async nature of the rich text rendering service ) the animation
			will wait until text is rendered, then will fade in.
		*/
		void fadeIn( seconds_t duration = 1 );

		/**
			Causes this RichTextBox to fade from opacity() to zero over a period of @a duration seconds.
			If the text isn't ready yet ( due to async nature of the rich text rendering service ) the animation
			will wait until text is rendered, then will fade in.
		*/
		void fadeOut( seconds_t duration = 1, bool dismissWhenDone = true, bool deleteWhenDone = true );

	private:
	
		class DismissListener : public core::InputListener
		{
			public:
			
				signals::signal<void()> dismiss;
		
			public:
			
				DismissListener(){}
				virtual ~DismissListener(){}

				virtual bool mouseDown( const ci::app::MouseEvent &event );
				virtual bool keyDown( const ci::app::KeyEvent &event );
		};
	
		void _userDismiss();
		void _requestReady( const core::util::rich_text::Request & );
		void _updateScreenPosition();
		core::GameObject *_resolveTrackingTarget();
		void _releaseTrackingTarget( core::GameObject * );
		
	private:

		init _initializer;
		bool _fadingIn, _fadingOut, _fadingOutDismiss, _fadingOutDelete;
		ci::gl::Texture _renderedText;
		Vec2 _position;
		Vec2i _screenPosition;
		seconds_t _readyTime, _fadingInStartTime, _fadingInDuration, _fadingOutStartTime, _fadingOutDuration;
		real _alpha, _alphaMultiplier;
		DismissListener *_dismissalListener;
		core::GameObject *_trackingTarget;
	
};

}
