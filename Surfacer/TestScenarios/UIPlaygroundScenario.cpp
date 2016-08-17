//
//  UIPlaygroundScenario.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/28/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "UIPlaygroundScenario.h"

#include <cinder/app/App.h>
#include <cinder/ImageIO.h>
#include <cinder/gl/gl.h>
#include <cinder/Utilities.h>

#include "PlayerHud.h"

/*
		game::RotaryDial *_testDial;
		game::ImageView *_background;
		game::SelectionList *_testList;
		game::RichTextBox *_richTextBox;
		DamagedMonitorFilter *_crtFilter;
		PalettizingFilter *_palettizerFilter;
*/

UIPlaygroundScenario::UIPlaygroundScenario():
	_testDial(NULL),
	_background(NULL),
	_testList(NULL),
	_richTextBox(NULL),
	_crtFilter(NULL),
	_palettizerFilter(NULL)
{
	takeFocus();
	resourceManager()->pushSearchPath( getHomeDirectory() / "Dropbox/Code/Projects/Surfacer/Resources" );
	setRenderMode( RenderMode::GAME );
}

UIPlaygroundScenario::~UIPlaygroundScenario()
{}

// lifecycle
void UIPlaygroundScenario::setup()
{
	GameScenario::setup();
	
	if ( true )
	{
		/*
			I'm attempting to nail down a bug here.
				
			When only layer0 is visible, the background image renders correctly
			When only layer1 is visible, the modal dialog renders correctly
			
			When both are visible, layer0 renders as solid black, and layer1 renders with the alpha fringe
			When orders are reversed, the image renders correctly on top, but the modal beneath is rendered in solid black.
			
			• Could the call to clear() be clearing the screen and not the FBO?
			
			NOTES:
				Disabling Layer's draw-to-FBO-then-blit fixes issue, so it has to be in FBO code
			
		*/
		ui::Layer
			*layer0 = new ui::Layer( "Layer0" ),
			*layer1 = new ui::Layer( "Layer1" );
			
		uiStack()->pushTop(layer0);
		uiStack()->pushTop(layer1);

		if ( true )
		{
			game::ImageView::init backgroundInit;
			backgroundInit.image = resourceManager()->getTexture( "/Users/shamyl/Dropbox/Photos/Awesome/Astronauts.jpeg" );
			backgroundInit.fit = game::ImageView::FILL;
			game::ImageView *background = new game::ImageView( "Background" );
			background->initialize( backgroundInit );
			background->setZIndex(-10);
			background->setBounds( layer0->bounds() );
			
			layer0->addView(background);
		}
		
		if ( true )
		{
			layer1->setPadding(20);

			game::RichTextBox::init rtbInit;
			rtbInit.templateHtmlFile = "MessageBox";
			rtbInit.htmlContent = "<p>This is a message going into a <strong>modal</strong> message box</p>";
			rtbInit.bodyClass = "alert";
			rtbInit.modal = false;
			rtbInit.position = Vec2r(0.9,0.9);
			rtbInit.blankingColor = ci::ColorA(0.5,0.5,0.5,0.75);
		
			game::RichTextBox *richTextBox = new game::RichTextBox();
			richTextBox->initialize( rtbInit );
			richTextBox->fadeIn();
			
			layer1->addView( richTextBox );
		}

		if ( true )
		{
			layer0->filters()->push( ColorAdjustmentFilter::createBlackAndWhite());
		}
		
		if ( true )
		{
			//layer1->filters()->push( GradientMapFilter::create( ci::ColorA( 1,0,0,1), ci::ColorA(0,1,0,1)));
			//layer1->filters()->push( GaussianBlurFilter::create(4));
			//layer1->filters()->push( PalettizingFilter::create( PalettizingFilter::RGB, 3, 3, 3, 3));
			//layer1->filters()->push( PalettizingFilter::create( PalettizingFilter::YUV, 3, 3, 3, 3));
			//layer1->filters()->push( PalettizingFilter::create( PalettizingFilter::HSL, 3, 3, 3, 3));
			//layer1->filters()->push( NoiseFilter::create(true));
		}
		
		layer0->setHidden(false);
		layer1->setHidden(false);
			
	}
	

	if ( false )
	{
		ui::Layer *layer = new ui::Layer( "TestLayer" );
		uiStack()->pushTop( layer );
		Vec2i layerSize = layer->size();
		
		if ( true )
		{
			game::ImageView::init backgroundInit;
			backgroundInit.image = resourceManager()->getTexture( "/Users/shamyl/Dropbox/Photos/Awesome/Astronauts.jpeg" );
			backgroundInit.fit = game::ImageView::FILL;
			_background = new game::ImageView( "Background" );
			_background->initialize( backgroundInit );
			_background->setZIndex(-10);
			_background->setBounds( layer->bounds() );
			layer->addView(_background);
		}
		
		if ( true )
		{
			layer->setPadding( 60 );

			const int Count = 3;
				
			const std::string TemplateHTMLFile = "MessageBox";
				
			const std::string Messages[3] = {
				"<p>This is the default message box style. <strong>Simple and direct</strong>",
				"<p>This is the \"alert\" message box style. <strong>Bright! Noisy!</strong>",
				"<p>This is the \"info\" message box style. <strong>I guess it's OK</strong>",
			};
			
			const std::string BodyClasses[3] = {
				"", "alert", "info"
			};
			
			game::RichTextBox::init rtbInit;
			for ( int i = 0; i < 64; i++ )
			{
				rtbInit.templateHtmlFile = TemplateHTMLFile;
				rtbInit.htmlContent = Messages[i % Count];
				rtbInit.bodyClass = BodyClasses[i % Count];
				rtbInit.displayTime = 5;
				rtbInit.position = Vec2r( Rand::randFloat(),Rand::randFloat() );
			
				game::RichTextBox *richTextBox = new game::RichTextBox();
				richTextBox->initialize( rtbInit );
				richTextBox->setOpacity( 1 );
				richTextBox->fadeIn(1);
				
				layer->addView( richTextBox );
			}
		}
		
		if ( false )
		{
			game::RichTextBox::init rtbInit;
			rtbInit.templateHtmlFile = "MessageBox";
			rtbInit.htmlContent = "<p>This is a message going into a <strong>modal</strong> message box</p>";
			rtbInit.bodyClass = "modal";
			rtbInit.modal = true;
			rtbInit.blankingColor = ci::ColorA(0.5,0.5,0.5,0.75);
		
			game::RichTextBox *richTextBox = new game::RichTextBox();
			richTextBox->initialize( rtbInit );
			richTextBox->fadeIn();
			

			layer->addView( richTextBox );
		}
		

		if ( false )
		{
			const Font DialFont( "Helvetica Neue Light", 24 );
			
			game::RotaryDial::init dialInit;
			dialInit.ringColor = ci::ColorA(1,1,1,0.75);
			dialInit.valueColor = ci::ColorA(0.25,0.75,1,1);
			dialInit.value = 1.5;
			dialInit.innerRadius = 50;
			dialInit.outerRadius = 60;
			dialInit.iconTex = resourceManager()->getTexture( "RotaryDial-HealthIcon.png" );
			
			dialInit.font = DialFont;

			_testDial = new game::RotaryDial( "Dial One" );
			_testDial->initialize( dialInit );

			cpBB bounds;
			cpBBNewCircle( bounds, cpv(80, layerSize.y - 80), 60 );
			_testDial->setBounds( bounds );
			layer->addView( _testDial );
		}
		
		if ( false )
		{
			game::SelectionList::init listInit;
			listInit.font = Font( "Helvetica Neue Light", 24 );
			listInit.color = ci::ColorA(1,1,1,0.5);
			listInit.selectedColor = ci::ColorA(1,0.7,0,1);
			
			game::SelectionList *list = new game::SelectionList( "TestList" );
			list->initialize( listInit );
			
			list->addItem( game::SelectionList::item( "Egg Overeasy" ));
			list->addItem( game::SelectionList::item( "Activia Yogurt" ));
			list->addItem( game::SelectionList::item( "Some Prunes" ));
			list->addItem( game::SelectionList::item( "Green Smoothy" ));
			list->addItem( game::SelectionList::item( "Chicken and Black Beans" ));
			list->addItem( game::SelectionList::item( "Almonds" ));
			list->addItem( game::SelectionList::item( "Salad" ));
			list->addItem( game::SelectionList::item( "A Serving of Protein" ));
			list->addItem( game::SelectionList::item( "A Finger of Scotch" ));
			
			list->setPosition( Vec2r(100,300));
			layer->addView( list );
			
			_testList = list;
									
		}
			
		if ( false )
		{
			_crtFilter = new DamagedMonitorFilter();
			layer->filters()->push(_crtFilter);
		}
		
		if ( false )
		{
			// Gameplay filters
			_palettizerFilter = new PalettizingFilter;
			_palettizerFilter->setColorModel( PalettizingFilter::HSL );
			_palettizerFilter->setPaletteSize(16, 16, 16);
			_palettizerFilter->setStrength(1);
			layer->filters()->push(_palettizerFilter);
		}
	}
}

void UIPlaygroundScenario::shutdown()
{
	GameScenario::shutdown();
}

void UIPlaygroundScenario::resize( Vec2i size )
{
	GameScenario::resize(size);
	if( _background) _background->setBounds( _background->layer()->bounds() );

	if ( _testDial )
	{
		cpBB bounds = _testDial->bounds();
		bounds.l = 20;
		bounds.b = size.y - 20 - cpBBHeight(bounds);
		bounds.t = size.y - 20;
		
		_testDial->setBounds( bounds );
	}
	
}

void UIPlaygroundScenario::update( const time_state &time )
{
	GameScenario::update( time );
}

void UIPlaygroundScenario::draw( const render_state &state )
{
	GameScenario::draw( state );
}

// input
bool UIPlaygroundScenario::mouseDown( const app::MouseEvent &event )
{
	return false;
}

bool UIPlaygroundScenario::mouseUp( const app::MouseEvent &event )
{
	return false;
}

bool UIPlaygroundScenario::mouseWheel( const app::MouseEvent &event )
{
	return false;
}

bool UIPlaygroundScenario::mouseMove( const app::MouseEvent &event, const Vec2r &delta )
{
	return false;
}

bool UIPlaygroundScenario::mouseDrag( const app::MouseEvent &event, const Vec2r &delta )
{
	return false;
}

bool UIPlaygroundScenario::keyDown( const app::KeyEvent &event )
{
	switch( event.getCode() )
	{
		case app::KeyEvent::KEY_BACKQUOTE:
		{
			setRenderMode( RenderMode::mode( (int(renderMode()) + 1) % RenderMode::COUNT ));
			break;
		}

		case app::KeyEvent::KEY_F12:
		{
			screenshot( getHomeDirectory() / "Desktop", "UIPlaygroundScenario" );
			break;
		}

		default: break;
	}

	if ( _crtFilter )
	{
		switch( event.getCode() )
		{
			case app::KeyEvent::KEY_LEFTBRACKET:
				_crtFilter->setStrength( _crtFilter->strength() - 0.1 );
				app::console() << "crt strength: " << _crtFilter->strength() << std::endl;
				break;

			case app::KeyEvent::KEY_RIGHTBRACKET:
				_crtFilter->setStrength( _crtFilter->strength() + 0.1 );
				app::console() << "crt strength: " << _crtFilter->strength() << std::endl;
				break;
				
			case app::KeyEvent::KEY_r:
			{
				Vec2r offset(1,1);
				if ( event.isShiftDown() ) offset *= -1;
				_crtFilter->setRedChannelOffset( _crtFilter->redChannelOffset() + offset );
				break;
			}
			
			case app::KeyEvent::KEY_g:
			{
				Vec2r offset(1,1);
				if ( event.isShiftDown() ) offset *= -1;
				_crtFilter->setGreenChannelOffset( _crtFilter->greenChannelOffset() + offset );
				break;
			}

			case app::KeyEvent::KEY_b:
			{
				Vec2r offset(1,1);
				if ( event.isShiftDown() ) offset *= -1;
				_crtFilter->setBlueChannelOffset( _crtFilter->blueChannelOffset() + offset );
				break;
			}

			case app::KeyEvent::KEY_e:
			{
				Vec3r delta(1,0,0);
				if ( event.isShiftDown() ) delta *= -1;
				_crtFilter->setPixelSizes( _crtFilter->pixelSizes() + delta );
				break;
			}

			case app::KeyEvent::KEY_f:
			{
				Vec3r delta(0,1,0);
				if ( event.isShiftDown() ) delta *= -1;
				_crtFilter->setPixelSizes( _crtFilter->pixelSizes() + delta );
				break;
			}

			case app::KeyEvent::KEY_v:
			{
				Vec3r delta(0,0,1);
				if ( event.isShiftDown() ) delta *= -1;
				_crtFilter->setPixelSizes( _crtFilter->pixelSizes() + delta );
				break;
			}
			
			default: break;
		}
				
	}
	
	if ( _background )
	{
		switch( event.getCode() )
		{
			case app::KeyEvent::KEY_UP:
			
				_background->setAlignment( _background->alignment() + Vec2r( 0,0.1 ));
				app::console() << "Background.alignment: " << _background->alignment() << std::endl;
				break;

			case app::KeyEvent::KEY_DOWN:
				_background->setAlignment( _background->alignment() + Vec2r( 0,-0.1 ));
				app::console() << "Background.alignment: " << _background->alignment() << std::endl;
				break;

			case app::KeyEvent::KEY_LEFT:
				_background->setAlignment( _background->alignment() + Vec2r( -0.1, 0 ));
				app::console() << "Background.alignment: " << _background->alignment() << std::endl;
				break;

			case app::KeyEvent::KEY_RIGHT:
				_background->setAlignment( _background->alignment() + Vec2r( +0.1, 0 ));
				app::console() << "Background.alignment: " << _background->alignment() << std::endl;
				break;

			default: break;
		}
	}
	
	if ( _testList )
	{
		switch( event.getCode() )
		{
			case app::KeyEvent::KEY_UP:
				_testList->selectItem( std::max( _testList->selectedItemIndex() - 1, 0 ));	
				break;

			case app::KeyEvent::KEY_DOWN:
				_testList->selectItem( std::min( _testList->selectedItemIndex() + 1, int(_testList->items().size() - 1)));	
				break;

			default: break;
		}
	}
	
	if ( _testDial )
	{
		switch( event.getCode() )
		{
			case app::KeyEvent::KEY_UP:
				_testDial->setValue( _testDial->value() + 0.1 );
				break;

			case app::KeyEvent::KEY_DOWN:
				_testDial->setValue( _testDial->value() - 0.1 );
				break;
				
			case app::KeyEvent::KEY_SPACE:
				if ( event.isShiftDown() )
				{
					_testDial->setValue( _testDial->value() - 1 );
					_testDial->notifyDecrease();
				}
				else
				{
					_testDial->setValue( _testDial->value() + 1 );
					_testDial->notifyIncrease();
				}

			default: break;
		}
	}
	
	if ( _palettizerFilter )
	{
		Vec3i delta(0,0,0);
		switch( event.getCode() )
		{
			case app::KeyEvent::KEY_x: 
				delta.x = 1;
				break;

			case app::KeyEvent::KEY_y: 
				delta.y = 1;
				break;

			case app::KeyEvent::KEY_z: 
				delta.z = 1;
				break;
			
			default: break;
		}
		
		if ( event.isShiftDown() ) delta *= -1;
		_palettizerFilter->setPaletteSize( _palettizerFilter->paletteSize() + delta );
		app::console() << "PaletteSize: " << _palettizerFilter->paletteSize() << std::endl;
	}

	
	return false;
}

bool UIPlaygroundScenario::keyUp( const app::KeyEvent &event )
{
	return false;
}
