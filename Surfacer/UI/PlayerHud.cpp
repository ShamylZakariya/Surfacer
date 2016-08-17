//
//  PlayerHud.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/26/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "PlayerHud.h"

#include "Filters.h"
#include "GameLevel.h"
#include "GameNotifications.h"
#include "GameScenario.h"
#include "Player.h"

#include <cinder/Text.h>
#include <cinder/Font.h>

using namespace ci;
using namespace core;
namespace game {

namespace {

	real MHeight( const Font &font )
	{
		return font.getGlyphBoundingBox( font.getGlyphChar( 'M' )).getHeight();
	}
	
	Surface8u CreateRotaryDialRing( int outerRadius, int innerRadius )
	{
		//
		//	Firt, scale up to next POT
		//
		
		const int 
			SizePOT = isPow2(outerRadius*2) ? outerRadius*2 : ci::nextPowerOf2( outerRadius*2 ),
			OuterRadiusPOT = SizePOT/2,
			InnerRadiusPOT = int( (real(innerRadius) / real(outerRadius)) * OuterRadiusPOT );

		const Vec2f
			Center( std::floor( SizePOT/2 + 0.5), std::floor( SizePOT/2 + 0.5 ));

		const int
			NumSamples = 5;

		const Vec2f Samples[NumSamples] = {
			Vec2f( 0.0, 0.0),
			Vec2f(-0.5, 0.0),
			Vec2f(+0.5, 0.0),
			Vec2f( 0.0,+0.5),
			Vec2f( 0.0,-0.5)
		};
		
		const real
			OuterRadiusSquared = OuterRadiusPOT * OuterRadiusPOT,
			InnerRadiusSquared = InnerRadiusPOT * InnerRadiusPOT;

		Surface8u ringSurface( SizePOT, SizePOT, true );
		ci::Surface::Iter iterator = ringSurface.getIter();

		while( iterator.line() )
		{
			while( iterator.pixel() )
			{
				const Vec2f Position( iterator.getPos() - Center );
										
				real sum = 0;
				for ( int i = 0; i < NumSamples; i++ )
				{
					const Vec2f Sample = Position + Samples[i];
					real l2 = Sample.lengthSquared();
					if ( l2 < OuterRadiusSquared && l2 > InnerRadiusSquared ) sum++;	
				}
			
				const real Alpha = sum / NumSamples;
				iterator.a() = Alpha * 255;				

				const Vec2f Dir( rotateCCW(normalize( Position )));
				const real Sweep = std::atan2( Dir.y, Dir.x ) / (M_PI * 2);
				iterator.r() = iterator.g() = iterator.b() = std::ceil( Sweep * 255 );
			}
		}
		
		return ringSurface;		
	}

}

/*
		core::Scenario *_scenario;
		RotaryDial *_health, *_power;
		MagnetoBeamQueryOverlay *_magnetoBeamOverlay;
		SelectionList *_weaponList;
		core::DamagedMonitorFilter *_injuryEffectFilterHud;
*/

PlayerHud::PlayerHud( core::Scenario *s ):
	Layer( "PlayerHud", Lifecycle( 3 )),
	_scenario(s),
	_health(NULL),
	_power(NULL),
	_magnetoBeamOverlay(NULL),
	_weaponList(NULL),
	_injuryEffectFilterHud(NULL)
{}

PlayerHud::~PlayerHud()
{}

void PlayerHud::notificationReceived( const Notification &note )
{
	switch( note.identifier() )
	{
		case game::Notifications::PLAYER_INJURED:
			_health->notifyDecrease();
			break;

		case game::Notifications::PLAYER_HEALTH_BOOST:
			_health->notifyIncrease();
			break;

		case game::Notifications::PLAYER_POWER_BOOST:
			_power->notifyIncrease();
			break;
	}
}

void PlayerHud::addedToStack( core::ui::Stack *s )
{
	const ColorA
		InactiveColor = ColorA(0.2, 0.2, 0.2, 1 ),
		HealthColor = ColorA(0.95,0.55,0.6,1) * ColorA(0.8,0.8,0.8,1),
		WeaponColor = ColorA(0.42,0.79,0.94,1) * ColorA(0.8,0.8,0.8,1);

	const Font
		HudFont = Font( "Helvetica Neue", 24 );

	setPadding( 20 );

	RotaryDial::init healthInit, powerInit;
	healthInit.ringColor = powerInit.ringColor = InactiveColor;
	healthInit.innerRadius = powerInit.innerRadius = 34;
	healthInit.outerRadius = powerInit.outerRadius = 40;
	healthInit.font = powerInit.font = HudFont;

	healthInit.valueColor = HealthColor;
	healthInit.iconTex = s->resourceManager()->getTexture( "RotaryDial-HealthIcon.png" );

	powerInit.valueColor = WeaponColor;
	powerInit.iconTex = s->resourceManager()->getTexture( "RotaryDial-PowerIcon.png" );


	_health = new RotaryDial( "Health" );
	_health->initialize( healthInit );
	addView( _health );
	
	_power = new RotaryDial( "Power" );
	_power->initialize( powerInit );
	addView( _power );
	
	_magnetoBeamOverlay = new MagnetoBeamQueryOverlay();
	addView( _magnetoBeamOverlay );
	
	// Listen to Player weapon changes
	GameLevel *level = static_cast<GameLevel*>(_scenario->level());
	Player *player = level->player();

	if ( player )
	{
		player->weaponSelected.connect( this, &PlayerHud::_playerWeaponChanged );
		
		SelectionList::init weaponListInit;
		weaponListInit.color = InactiveColor;
		weaponListInit.selectedColor = WeaponColor;
		weaponListInit.font = HudFont;
		
		_weaponList = new SelectionList( "Weapons" );
		_weaponList->initialize( weaponListInit );
		addView( _weaponList );		
		
		for ( int i = 0; i < WeaponType::COUNT; i++ )
		{
			_weaponList->addItem( SelectionList::item( WeaponType::displayName(i)));
		}
		
		_weaponList->selectItem( WeaponType::displayName( player->weapon()->weaponType()));
	}
	
	//
	// UI filters
	//
	_injuryEffectFilterHud = new DamagedMonitorFilter();
	_injuryEffectFilterHud->setStrength(0);
	_injuryEffectFilterHud->setPixelSizes( 1, 4, 8 );
	_injuryEffectFilterHud->setScanlineStrength(1.0);
	filters()->push( _injuryEffectFilterHud );
	
	
	//
	//	UI is screen composited
	//

	filters()->setBlendMode( BlendMode( GL_ONE, GL_ONE ));
	filters()->setCompositeColor( ColorA(1,1,1,0));
	
		
	resize( size() );
}

void PlayerHud::resize( const Vec2i &newSize )
{
	if ( _health && _power )
	{
		const real 
			Radius = _health->initializer().outerRadius,
			Size = Radius * 2,
			Pad = padding();

		const Vec2r 
			HealthPosition( Radius + Pad, newSize.y - Radius - Pad ),
			PowerPosition( HealthPosition.x + Pad + Size, HealthPosition.y );		
			
	
		cpBB healthBounds, powerBounds;
		cpBBNewCircle(healthBounds, HealthPosition, Radius );
		cpBBNewCircle(powerBounds, PowerPosition, Radius );
		
		_health->setBounds( healthBounds );
		_power->setBounds( powerBounds );
		
		_weaponList->setPosition( PowerPosition + Vec2r( Radius + Pad, -MHeight(_weaponList->initializer().font)/2 ));
	}
}

void PlayerHud::update( const time_state &time )
{
	GameScenario *scenario = static_cast<GameScenario*>(_scenario);
	GameLevel *level = static_cast<GameLevel*>(_scenario->level());
	Player *player = level->player();
	
	if ( player )
	{
		const real 
			Health = player->health()->health(),
			Power = player->battery()->power();
		
		_health->setValue( Health );
		_power->setValue( Power );

		ci::ColorA compositeColor = filters()->compositeColor();
		if ( player->alive() )
		{
			compositeColor = compositeColor.lerp(0.1, ColorA(0.75,0.75,0.75,1) );
		}
		else
		{
			compositeColor *= 0.99;
		}
		
		filters()->setCompositeColor( compositeColor );
	}
	
	//
	//	Set layer opacity to match lifecycle, via filter's composite color
	//
	
	real alpha = 1;
	switch( lifecycle().phase() )
	{
		case ui::Layer::Lifecycle::TRANSITION_IN:
			alpha = lifecycle().transition();
			break;
			
		case ui::Layer::Lifecycle::TRANSITION_OUT:
			alpha = 1 - lifecycle().transition();
			break;
		
		default: break;
	}
	
	
	//
	//	Update the display chaos effect
	//
	const real FrazzleStrength = scenario->playerInjuryEffectStrength();
	_injuryEffectFilterHud->setStrength( FrazzleStrength );
	
	if ( FrazzleStrength > ALPHA_EPSILON && (!(time.step % 2)))
	{
		const real OffsetRange = 64;
		_injuryEffectFilterHud->setRedChannelOffset( Rand::randFloat( OffsetRange ) * Vec2r( Rand::randFloat(-1,1), 0 ));
		_injuryEffectFilterHud->setGreenChannelOffset( Rand::randFloat( OffsetRange/16 ) * Vec2r( Rand::randFloat(-1,1), 0 ));
		_injuryEffectFilterHud->setBlueChannelOffset( Rand::randFloat( OffsetRange/4 ) * Vec2r( Rand::randFloat(-1,1), 0 ));
	}
}

void PlayerHud::draw( const core::render_state &state )
{}

void PlayerHud::_playerWeaponChanged( Player *player, Weapon *weapon )
{
	_weaponList->selectItem( WeaponType::displayName( weapon->weaponType() ));
}

#pragma mark - ImageView 

/*
		bool _layoutNeeded;
		init _initializer;
		Rectf _imageRect; 
*/

ImageView::ImageView( const std::string &name ):
	View( name ),
	_layoutNeeded(true),
	_imageRect(0,0,0,0)
{}

ImageView::~ImageView()
{}

void ImageView::initialize( const init &initializer )
{
	_initializer = initializer;
	_initializer.image.setFlipped(true);
}

void ImageView::boundsChanged( const cpBB &newBounds )
{
	_invalidateLayout();
}

void ImageView::draw( const core::render_state & )
{
	if ( _layoutNeeded ) 
	{
		_layout();
		_layoutNeeded = false;
	}
	
	gl::color( _initializer.color );
	gl::draw( _initializer.image, _imageRect );
}

void ImageView::_invalidateLayout()
{
	_layoutNeeded = true;
}

void ImageView::_layout()
{
	const Vec2r 
		Size = v2r( cpBBSize( bounds())),
		ImageSize = _initializer.image.getSize();

	switch( _initializer.fit )
	{
		case FIT:
		{
			real scale = Size.x / ImageSize.x;
			Vec2r scaledImageSize = ImageSize * scale;
			if ( scaledImageSize.y > Size.y )
			{
				scale = Size.y / ImageSize.y;
				scaledImageSize = ImageSize * scale;
			}

			const Vec2r Range( Size - scaledImageSize );
			_imageRect.set(0,0,scaledImageSize.x, scaledImageSize.y);
			_imageRect.offset( Range * _initializer.alignment );

			break;
		}

		case FILL:
		{
			real scale = Size.x / ImageSize.x;
			Vec2r scaledImageSize = ImageSize * scale;
			if ( scaledImageSize.y < Size.y )
			{
				scale *= Size.y / scaledImageSize.y;
				scaledImageSize = ImageSize * scale;
			}

			const Vec2r Range( Size - scaledImageSize );
			_imageRect.set(0,0,scaledImageSize.x, scaledImageSize.y);
			_imageRect.offset( Range * _initializer.alignment );
		
			break;
		}

		case NATURAL:
		{
			const Vec2r Range( Size - ImageSize );
			
			_imageRect.set(0,0,ImageSize.x, ImageSize.y);
			_imageRect.offset( Range * _initializer.alignment );
		
			break;
		}

		case STRETCH:
		{
			_imageRect.set(0, 0, Size.x, Size.y );
			break;
		} 
	}
}


#pragma mark - RotaryDial

/*
		init _initializer;
		real _value, _targetValue;
		core::util::Spring<real> _scaleSpring;

		ring_vertex_vec _ringVertices; 
		GLuint _ringVbo;
		ci::gl::GlslProg _ringShader;
		GLint _ringRadiansVertexAttrPosition;

		ci::gl::GlslProg _iconShader;
		ci::gl::GlslProg _iconShader;
		core::SvgObject _icon;
		
		ci::gl::Texture _labelTexture;
		ci::Font _labelFont;		

		display_state _displayState;
*/

RotaryDial::RotaryDial( const std::string &name ):
	View( name ),
	_value(0),
	_targetValue(0),
	_scaleSpring( 1,0.0625,0.25 )
{}

RotaryDial::~RotaryDial()
{}

void RotaryDial::initialize( const init &initializer )
{
	_initializer = initializer;
	_value = _targetValue = _initializer.value;

	_scaleSpring.setValue(0);
	_scaleSpring.setTarget(0);
		
	//
	//	Size and prepare the icon
	//

	_icon = _initializer.iconTex;
	if ( _icon )
	{
		const Vec2r IconSize = _icon.getSize();
		const real IconMaxSize = std::max( IconSize.x, IconSize.y );
		_displayState.iconScale = (_initializer.innerRadius*2 * 0.25) / IconMaxSize;
		_displayState.iconSize = IconSize * _displayState.iconScale;
	}
	
	_labelFont = _initializer.font;	
}

void RotaryDial::boundsChanged( const cpBB &newBounds )
{
	_displayState.needsLayout = true;
}

void RotaryDial::addedToLayer( core::ui::Layer *l )
{}

void RotaryDial::update( const core::time_state &time )
{
	_value = lrp<real>( 0.1,_value, _targetValue );
	_scaleSpring.update( time.deltaT );
	
	int whole = std::floor( _value );
	_displayState.fractional = _value - whole;
	if ( whole != _displayState.whole )
	{
		_displayState.whole = std::floor( _value );
		_labelTexture.reset();
	}	
}

void RotaryDial::draw(const core::render_state &state)
{
	glBlendFuncSeparate( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE );

	if ( !_labelTexture )
	{
		_renderLabel( _displayState );
		_displayState.needsLayout = true;
	}

	if ( _displayState.needsLayout )
	{
		_layout( _displayState );
		_displayState.needsLayout = false;
	}

	_displayRing( state, _displayState );
	_displayIcon( state, _displayState );

	if( _displayState.whole > 0 ) 
	{	
		_displayLabel( state, _displayState );	
	}

	glBlendFuncSeparate( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO );
}

void RotaryDial::setValue( real value )
{
	_targetValue = std::max( value, real(0) );
}

real RotaryDial::value() const
{
	return _targetValue;
}

void RotaryDial::notifyIncrease()
{
	//	apply a positive velocity, causing scale to trend upwards. spring will automatically
	//	return to zero over time.
	_scaleSpring.setVelocity(0.0025);
}

void RotaryDial::notifyDecrease()
{
	// decrease spring tweak is much less because injuries tend to be a continuous process, not single events
	_scaleSpring.setVelocity(-0.000625);
}

void RotaryDial::_layout( display_state &displayState )
{
	const Vec2r Center( v2r( cpBBCenter(bounds())));

	if ( displayState.whole > 0 )
	{
		const 
			real Pad = 5,
			TotalWidth = displayState.iconSize.x + displayState.labelSize.x + Pad,
			Left = Center.x - TotalWidth/2;

		displayState.iconPosition.x = Left + displayState.iconSize.x/2;
		displayState.iconPosition.y = Center.y;
		displayState.labelPosition.x = std::floor( Left + displayState.iconSize.x + Pad );
		displayState.labelPosition.y = Center.y - (_labelFont.getAscent() + _labelFont.getDescent())/2;
	}
	else
	{
		displayState.iconPosition = Center;
	}

	const real 
		IconWidth = displayState.iconSize.x * 0.5,
		IconHeight = displayState.iconSize.y * 0.5;

	displayState.iconRect.set( 
		displayState.iconPosition.x - IconWidth, displayState.iconPosition.y + IconHeight,
		displayState.iconPosition.x + IconWidth, displayState.iconPosition.y - IconHeight );
		
	displayState.iconRect.x1 = std::floor(displayState.iconRect.x1 + 0.5);
	displayState.iconRect.x2 = std::floor(displayState.iconRect.x2 + 0.5);
	displayState.iconRect.y1 = std::floor(displayState.iconRect.y1 + 0.5);
	displayState.iconRect.y2 = std::floor(displayState.iconRect.y2 + 0.5);
}

void RotaryDial::_renderLabel( display_state &state )
{
	TextLayout layout;
	layout.setFont( _labelFont );
	layout.clear( ColorA(0,0,0,0));
	layout.setColor( _initializer.valueColor );
	layout.addLine( str(state.whole) );		
	_labelTexture = gl::Texture( layout.render( true, false ) );
	_labelTexture.setFlipped( true );
	
	state.labelSize = Vec2r( _labelTexture.getSize().x, _labelFont.getAscent());
}

void RotaryDial::_displayRing( const core::render_state &state, const display_state &displayState )
{
	if ( !_ringShader )
	{
		_ringShader = resourceManager()->getShader( "RotaryDialRingShader" );
	}

	if ( !_ringTexture )
	{
		const std::string RingTextureID = "RingTexture-" + str( _initializer.outerRadius, 2 ) + "-" + str( _initializer.innerRadius, 2 );
		_ringTexture = resourceManager()->getCachedTexture( RingTextureID );
		
		if ( !_ringTexture )
		{
			gl::Texture::Format fmt;
			fmt.enableMipmapping(false);
			fmt.setMinFilter( GL_LINEAR );
			_ringTexture = gl::Texture( CreateRotaryDialRing( _initializer.outerRadius, _initializer.innerRadius ), fmt );
			resourceManager()->cacheTexture( _ringTexture, RingTextureID );
		}
	}

	const real 
		CurrentValue = displayState.fractional,
		Scale = real(1) + _scaleSpring.value(),
		IncreaseNotificationFactor = saturate(std::pow(std::max(_scaleSpring.value(),real(0)), real(1))) * 2,
		DecreaseNotificationFactor = saturate(std::pow(std::max(-_scaleSpring.value(),real(0)), real(1))) * 2;
	
	const ColorA 
		RingColor = _initializer.ringColor
			.lerp( IncreaseNotificationFactor * 0.5, _initializer.increaseNotificationColor )
			.lerp( DecreaseNotificationFactor * 0.5, _initializer.decreaseNotificationColor ),

		ValueColor = _initializer.valueColor
			.lerp( IncreaseNotificationFactor, _initializer.increaseNotificationColor )
			.lerp( DecreaseNotificationFactor, _initializer.decreaseNotificationColor );

	const cpBB 
		BoundsBB = bounds();

	const Rectf
		BoundsRect( BoundsBB.l, BoundsBB.t, BoundsBB.r, BoundsBB.b );

	_ringShader.bind();
	_ringShader.uniform( "RingTex", 0 );
	_ringShader.uniform( "Value", CurrentValue );
	_ringShader.uniform( "BaseColor", RingColor );
	_ringShader.uniform( "HighlightColor", ValueColor );
	
	_ringTexture.bind(0);

	gl::drawSolidRect( BoundsRect.scaledCentered(Scale) );

	_ringTexture.unbind(0);
	_ringShader.unbind();		
	
}


void RotaryDial::_displayIcon( const core::render_state &state, const display_state &displayState )
{
	if ( _icon )
	{
		if ( !_iconShader )
		{
			_iconShader = resourceManager()->getShader( "RotaryDialIconShader" );
		}
		
		_iconShader.bind();
		_iconShader.uniform( "IconTex", 0 );
		_iconShader.uniform( "Color", _initializer.valueColor );
		
		_icon.bind(0);

		gl::drawSolidRect( displayState.iconRect );

		_icon.unbind(0);
		_iconShader.unbind();		
	}
}

void RotaryDial::_displayLabel( const core::render_state &state, const display_state &displayState )
{
	gl::color(1,1,1 );		
	gl::draw( _labelTexture, displayState.labelPosition );
}

#pragma mark - SelectionList

namespace {

	struct item_is_named
	{
		const std::string &_name;

		item_is_named( const std::string &name ):_name(name){}
		bool operator()( const SelectionList::item &i ) const
		{
			return i.name == _name;
		}
	};

	SelectionList::item NullItem = SelectionList::item( std::string() );
}

/*
		init _initializer;
		item_vec _items;
		gl::Texture _compositedListTexture;
		Vec2r _position;
		real _lineHeight, _yOffset;
		int _selectedItemIndex;
		core::util::Spring<real> _yOffsetSpring;
*/

SelectionList::SelectionList( const std::string &name ):
	View( name ),
	_position(0,0),
	_lineHeight(0),
	_yOffset(0),
	_selectedItemIndex(-1),
	_yOffsetSpring( 1,0.035,0.25 )
{}
	
SelectionList::~SelectionList()
{}

// Initialization
void SelectionList::initialize( const init &i )
{
	_initializer = i;
	_lineHeight = _initializer.font.getAscent() + _initializer.font.getDescent() + _initializer.font.getLeading();
	_invalidate();
}

// View
void SelectionList::addedToLayer( core::ui::Layer *l )
{}

void SelectionList::update( const core::time_state & )
{}

void SelectionList::draw( const core::render_state &state )
{
	if ( !_compositedListTexture )
	{
		_compositedListTexture = _render( state );

		_yOffset = _lineHeight * std::max(_selectedItemIndex,0) 
			- _compositedListTexture.getHeight() 
			+ _lineHeight 
			- _initializer.font.getDescent();
			
		_yOffsetSpring.setTarget( _yOffset );
	}
	
	real yOffset = _yOffsetSpring.update( state.deltaT );

	gl::color(1,1,1);
	
	glBlendFuncSeparate( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE );
	gl::draw( _compositedListTexture, _position + Vec2r(0,yOffset));
	
	if (state.mode == RenderMode::DEBUG )
	{
		gl::color(1,0,1,0.5);
		gl::drawLine( _position, _position + Vec2r( std::max( _compositedListTexture.getWidth(), 10 ), 0 ));
	}

	glBlendFuncSeparate( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO );
}

// SelectionList
void SelectionList::addItem( const item &item )
{
	_items.push_back(item);
	_invalidate();
}

void SelectionList::removeItem( const std::string &name )
{
	_items.erase( std::remove_if( _items.begin(), _items.end(), item_is_named(name)), _items.end());
	_invalidate();
}

const SelectionList::item_vec &SelectionList::items() const
{
	return _items;
}

void SelectionList::selectItem( const std::string &name )
{
	item_vec::const_iterator pos = std::find_if( _items.begin(), _items.end(), item_is_named(name));
	if ( pos != _items.end() )
	{
		int idx = pos - _items.begin();
		if ( idx != _selectedItemIndex )
		{
			_selectedItemIndex = idx;
			_invalidate();
		}
	}
	else
	{
		if ( _selectedItemIndex >= 0 )
		{
			_selectedItemIndex = -1;
			_invalidate();
		}
	}	
}

void SelectionList::selectItem( int idx )
{
	if ( idx != _selectedItemIndex )
	{
		if ( idx >= 0 && idx < _items.size() )
		{
			_selectedItemIndex = idx;
		}
		else
		{
			_selectedItemIndex = -1;
		}
		
		_invalidate();
	}
}

const SelectionList::item &SelectionList::selectedItem() const
{
	if ( _selectedItemIndex >= 0 && _selectedItemIndex < _items.size() ) return _items[_selectedItemIndex];

	return NullItem;
}

int SelectionList::selectedItemIndex() const
{
	return _selectedItemIndex;
}

void SelectionList::setPosition( const Vec2r &position )
{
	_position = position;
}

Vec2r SelectionList::position() const
{
	return _position;
}

void SelectionList::_invalidate()
{
	_compositedListTexture.reset();
}

gl::Texture SelectionList::_render( const render_state &state ) const
{
	TextLayout layout;
	layout.clear( ColorA( 0,0,0,0 ) );
	layout.setFont( _initializer.font );
	
	for( item_vec::const_iterator item(_items.begin()), end(_items.end()); item != end; ++item )
	{
		int itemIdx = (item - _items.begin());
		layout.setColor( itemIdx == _selectedItemIndex ? _initializer.selectedColor : _initializer.color );
		layout.addLine( item->name );
	}
	
	gl::Texture tex( layout.render( true, false ));
	tex.setFlipped( true );
	return tex;
}
		
#pragma mark - MagnetoBeamQueryOverlay

MagnetoBeamQueryOverlay::MagnetoBeamQueryOverlay():
	View( "MagnetoBeamQueryOverlay" )
{}

MagnetoBeamQueryOverlay::~MagnetoBeamQueryOverlay()
{}

void MagnetoBeamQueryOverlay::addedToLayer( core::ui::Layer *l )
{}

void MagnetoBeamQueryOverlay::update( const core::time_state & )
{}

void MagnetoBeamQueryOverlay::draw( const core::render_state &state )
{
	MagnetoBeam *beam = this->beam();
	if ( beam->active() )
	{
		const MagnetoBeam::tow_handle &CurrentlyAimedAt = beam->towable(beam->beamTarget());
		const MagnetoBeam::tow_handle &CurrentlyTowing = beam->towing();
		
		//
		//	Nothing to draw, early exit
		//

		if ( !CurrentlyTowing && !CurrentlyAimedAt ) return;
		
		cpBody *towBody = beam->towBody();	
		ColorA color;
		Vec2r screenPos;
		const Vec2r ScreenSize = state.viewport.size();

		if ( CurrentlyAimedAt ) 
		{
			if ( CurrentlyAimedAt.towable )
			{
				color = ColorA(0,1,0,0.2);
				screenPos = state.viewport.worldToScreen( v2r( cpBodyGetPos(CurrentlyAimedAt.target.body)));
			}
			else
			{
				color = ColorA(1,0,0,0.2);
				screenPos = state.viewport.worldToScreen( v2r( cpBodyGetPos(towBody)));
			}
		}

		if ( CurrentlyTowing ) 
		{
			color = ColorA(0,1,0,0.6);
			screenPos = state.viewport.worldToScreen( v2r( cpBodyGetPos(towBody)));
		}

		gl::color( color );
		gl::enableAdditiveBlending();

		gl::drawLine( Vec2f( screenPos.x, 0), Vec2f(screenPos.x, ScreenSize.y));
		gl::drawLine( Vec2f( 0, screenPos.y), Vec2f( ScreenSize.x, screenPos.y ));
	}
}

// MagnetoBeamQueryOverlay
MagnetoBeam *MagnetoBeamQueryOverlay::beam() const
{
	GameLevel *level = static_cast<GameLevel*>(scenario()->level());
	Player *player = level->player();

	return static_cast<MagnetoBeam*>(player->weapon(WeaponType::MAGNETO_BEAM));
}

#pragma mark - RichTextBox::DismissListener

bool RichTextBox::DismissListener::mouseDown( const ci::app::MouseEvent &event )
{
	dismiss();
	return true;
}

bool RichTextBox::DismissListener::keyDown( const ci::app::KeyEvent &event )
{
	switch ( event.getCode() )
	{
		case ci::app::KeyEvent::KEY_SPACE:
		case ci::app::KeyEvent::KEY_RETURN:
		case ci::app::KeyEvent::KEY_ESCAPE:
			dismiss();
		
		default: break;
	}

	// gobble all input
	return true;
}

	
#pragma mark - RichTextBox

/*
		init _initializer;
		bool _fadingIn, _fadingOut, _fadingOutDismiss, _fadingOutDelete;
		ci::gl::Texture _renderedText;
		Vec2r _position;
		seconds_t _readyTime, _fadingInStartTime, _fadingInDuration, _fadingOutStartTime, _fadingOutDuration;
		real _alpha, _alphaMultiplier;
		DismissListener *_dismissalListener;
		core::GameObject *_trackingTarget;
*/

RichTextBox::RichTextBox():
	View( "RichTextBox" ),
	_fadingIn(false),
	_fadingOut(false),
	_fadingOutDismiss(false),
	_fadingOutDelete(false),
	_position(0,0),
	_readyTime(0),
	_fadingInStartTime(0),
	_fadingInDuration(0),
	_fadingOutStartTime(0),
	_fadingOutDuration(0),
	_alpha(1),
	_alphaMultiplier(1),
	_dismissalListener(NULL),
	_trackingTarget(NULL)
{}

RichTextBox::~RichTextBox()
{
	delete _dismissalListener;
}

void RichTextBox::initialize( const init &i )
{
	_initializer = i;
	_position = _initializer.position;	
}

void RichTextBox::ready()
{
	//
	//	Fire off a request to render our text
	//

	core::util::rich_text::RequestRef renderRequest = boost::make_shared<core::util::rich_text::Request>(
		_initializer.templateHtmlFile,
		_initializer.htmlContent,
		_initializer.bodyClass);

	renderRequest->ready.connect( this, &RichTextBox::_requestReady );
	scenario()->richTextRenderer()->render( renderRequest );
	
	_readyTime = scenario()->time().time;
	
	//
	//	If this RichTextBox is modal, we need to create a dismissal listener to fadeOut on user input
	//
	
	if ( _initializer.modal )
	{
		_dismissalListener = new DismissListener();
		_dismissalListener->dismiss.connect( this, &RichTextBox::_userDismiss );
		_dismissalListener->takeFocus();
		
		if ( scenario()->level() )
		{
			scenario()->level()->setPaused( true );
		}
	}

	//
	//	If we have a tracking target, find it, and listen for its death signal
	//

	_resolveTrackingTarget();
}

void RichTextBox::layerResized( const Vec2i &newLayerSize )
{
	_updateScreenPosition();
}

void RichTextBox::update( const core::time_state &time )
{
	//
	//	How many seconds more are we required to display for? This will be at least zero.
	//

	const seconds_t
		SecondsRemainingToDisplay = _initializer.minimumDisplayTime - (time.time - _readyTime);

	//
	//	Determine if we've reached our displayTime limit - if so, run fadeOut()
	//

	if ( !_fadingOut && (_initializer.displayTime > 0))
	{
		const seconds_t Elapsed = time.time - _readyTime;
		if ( Elapsed > _initializer.displayTime )
		{
			fadeOut( 1, true, true );
		}
	}


	real
		fadeInAlphaMultiplier = 1,
		fadeOutAlphaMultiplier = 1;

	if ( _fadingIn )
	{
		if ( _fadingInStartTime > 0 )
		{
			const seconds_t
				Elapsed = time.time - _fadingInStartTime;
				
			fadeInAlphaMultiplier = static_cast<real>( saturate( Elapsed / _fadingInDuration ));
			if ( fadeInAlphaMultiplier >= 1 - ALPHA_EPSILON )
			{
				_fadingIn = false;
				_fadingInStartTime = 0;
				_fadingInDuration = 0;
			}
		}
		else
		{
			// if _fadingIn but _fadingInStartTime isn't set that means we don't have our texture yet, so defer...
			fadeInAlphaMultiplier = 0;		
		}
	}

	if ( _fadingOut )
	{
		if ( _fadingOutStartTime > 0 )
		{
			if ( SecondsRemainingToDisplay > 0 )
			{
				_fadingOutStartTime = time.time;
			}
			else
			{
				const seconds_t
					Elapsed = time.time - _fadingOutStartTime,
					Factor = 1 - saturate(Elapsed / _fadingOutDuration);
					
				fadeOutAlphaMultiplier = static_cast<real>( Factor );
				if ( fadeOutAlphaMultiplier <= ALPHA_EPSILON )
				{
					_fadingOut = false;
					_fadingOutStartTime = 0;
					_fadingOutDuration = 0;
					
					if ( _fadingOutDismiss )
					{
						dismiss( _fadingOutDelete );
					}
				}
			}
		}
	}

	_alphaMultiplier = fadeInAlphaMultiplier * fadeOutAlphaMultiplier;
}

void RichTextBox::draw( const core::render_state &state )
{
	if ( !dismissed() && _renderedText )
	{
		ui::Layer *layer = this->layer();

		//
		//	If we're tracking, we update position here in every frame
		//

		if ( GameObject *target = _resolveTrackingTarget() )
		{
			//
			//	Find centroid of the tracking target as the center of its aabb
			//

			const Vec2r
				PositionWorld = target->position(),
				PositionScreen = scenario()->camera().worldToScreen( PositionWorld ),
				TagPositionScreen = Vec2r( PositionScreen.x - _renderedText.getWidth()/2, PositionScreen.y + layer->padding() );

			_screenPosition.x = std::floor( TagPositionScreen.x );
			_screenPosition.y = std::floor( TagPositionScreen.y );

			//
			//	Update bounds
			//

			cpBB newBounds;
			newBounds.l = _screenPosition.x;
			newBounds.r = _screenPosition.x + _renderedText.getWidth();
			newBounds.b = _screenPosition.y;
			newBounds.t = _screenPosition.y + _renderedText.getHeight();
			
			setBounds( newBounds );
		}



		glEnable( GL_BLEND );
		real alpha = _alpha * _alphaMultiplier;

		Vec2i position = _screenPosition;
		if ( _initializer.modal )
		{
			//
			//	Modal text box is centered, and drawn over a solid color
			//

			position.x = layer->size().x / 2 - _renderedText.getWidth() / 2;
			position.y = layer->size().y / 2 - _renderedText.getHeight() / 2;

			ci::ColorA blankingColor = _initializer.blankingColor;
			blankingColor.a *= alpha;

			gl::color( blankingColor );
			glBlendFuncSeparate( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE );
			gl::drawSolidRect( Rectf(0,0,layer->size().x,layer->size().y));
		}

		//
		// webkit generated texture is premultiplied, so use premultiplied blending
		//

		gl::color( alpha,alpha,alpha,alpha );
		glBlendFuncSeparate( GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE );
		glBlendEquation( GL_FUNC_ADD );

		gl::draw( _renderedText, position );

		glBlendFuncSeparate( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE );
	}
}

void RichTextBox::setPosition( const Vec2r &p )
{
	_position = p;
	_updateScreenPosition();
}

void RichTextBox::fadeIn( seconds_t duration )
{
	if ( !_fadingIn )
	{
		_fadingIn = true;
		_alphaMultiplier = 0;
		_fadingInDuration = std::max( duration, static_cast<seconds_t>(0) );

		if ( isReady() )
		{
			_fadingInStartTime = scenario()->time().time;
		}
	}
}

void RichTextBox::fadeOut( seconds_t duration, bool dismissWhenDone, bool deleteWhenDone )
{
	if ( !_fadingOut )
	{
		_fadingOut = true;
		_fadingOutDuration = std::max( duration, static_cast<seconds_t>(0) );
		_fadingOutDismiss = dismissWhenDone;
		_fadingOutDelete = deleteWhenDone;

		if ( isReady() )
		{
			_fadingOutStartTime = scenario()->time().time;
		}
	}
}

void RichTextBox::_userDismiss()
{
	Scenario *s = scenario();
	if ( s )
	{
		// sanity check to make
		if (s->time().time - _readyTime > _initializer.minimumDisplayTime )
		{
			if ( s->level() )
			{
				s->level()->setPaused( false );
			}

			fadeOut( 0.5, true, true );
		}
	}
}

void RichTextBox::_requestReady( const core::util::rich_text::Request &r )
{
	_renderedText = r.result().texture();
	_renderedText.setFlipped( true );

	_updateScreenPosition(); // trigger a layout

	//
	//	If a fade-in or fade-out was requested, now that we have our texture we can start the animation
	//

	if ( _fadingIn )
	{
		_fadingInStartTime = scenario()->time().time;
	}
	
	if ( _fadingOut )
	{
		_fadingOutStartTime = scenario()->time().time;
	}
}

void RichTextBox::_updateScreenPosition()
{
	if ( _renderedText && _initializer.trackingTargetIdentifier.empty() )
	{
		ui::Layer *layer = this->layer();

		//
		//	Update ScreenPosition based on the available range
		//

		const Vec2r
			Pad = Vec2r( layer->padding(), layer->padding() ),
			LayerSize = layer->size() - 2*Pad,
			Range = LayerSize - Vec2r( _renderedText.getWidth(), _renderedText.getHeight() ),
			LowerLeftCorner = Pad + (saturate(_position) * Range);

		_screenPosition.x = std::floor( LowerLeftCorner.x );
		_screenPosition.y = std::floor( LowerLeftCorner.y );

		//
		//	Update bounds
		//

		cpBB newBounds;
		newBounds.l = _screenPosition.x;
		newBounds.r = _screenPosition.x + _renderedText.getWidth();
		newBounds.b = _screenPosition.y;
		newBounds.t = _screenPosition.y + _renderedText.getHeight();
		
		setBounds( newBounds );
	}
}

GameObject *RichTextBox::_resolveTrackingTarget()
{
	if ( _trackingTarget ) return _trackingTarget;
	if ( _initializer.trackingTargetIdentifier.empty() ) return NULL;

	if ( Scenario *s = scenario() )
	{
		if ( Level *level = s->level() )
		{
			_trackingTarget = level->objectById( _initializer.trackingTargetIdentifier );
			if ( _trackingTarget )
			{
				//app::console() << "RichTextBox::_resolveTrackingTargte: " << _trackingTarget->description() << std::endl;
				_trackingTarget->aboutToBeDestroyed.connect(this, &RichTextBox::_releaseTrackingTarget );
			}
		}
	}

	return _trackingTarget;
}

void RichTextBox::_releaseTrackingTarget( core::GameObject *obj )
{
	if ( _trackingTarget )
	{
		assert( obj == _trackingTarget );

		//app::console() << "RichTextBox::_releaseTrackingTarget: " << obj->description() << std::endl;

		_trackingTarget = NULL;
		_initializer.trackingTargetIdentifier.clear();
	}

	dismiss(true);
}



}