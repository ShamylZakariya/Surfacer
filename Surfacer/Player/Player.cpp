//
//  Player.cpp
//  Surfacer
//
//  Created by Zakariya Shamyl on 10/11/11.
//  Copyright 2011 TomorrowPlusX. All rights reserved.
//

#include "Player.h"

#include "Fluid.h"
#include "Monster.h"
#include "GameConstants.h"
#include "GameNotifications.h"
#include "PlayerSvgAnimator.h"
#include "PlayerPhysics.h"

#include <cinder/gl/gl.h>
#include <chipmunk/chipmunk_unsafe.h>

using namespace ci;
using namespace core;
namespace game {
  
#pragma mark - Player

CLASSLOAD(Player);

/*
		init _initializer;
		
		PlayerPhysics *_physics;
		PlayerInputComponent *_input;
		PlayerRenderer *_renderer;
		
		Battery *_battery;
		WeaponType::type _weaponType;
		std::vector< Weapon *>_weapons;

		UniversalParticleSystemController::Emitter *_smokeEmitter, *_fireEmitter, *_injuryEmitter;

		Monster *_restrainer;
		seconds_t _restrainedTime, _timeWiggling;
		std::size_t _wiggleCount;
		int _wiggleDirection;
*/

Player::Player():
	Entity( GameObjectType::PLAYER ), 
	_physics(new PogocyclePlayerPhysics()),
	_input(new PlayerInputComponent()),
	_renderer( new PlayerRenderer()),
	_battery(NULL),
	_weaponType(WeaponType::COUNT),
	_weapons( WeaponType::COUNT),
	_smokeEmitter(NULL),
	_fireEmitter(NULL),
	_injuryEmitter(NULL),
	_restrainer(NULL),
	_restrainedTime(0),
	_timeWiggling(0),
	_wiggleCount(0),
	_wiggleDirection(0)
{
	setName( "Player" );
	addComponent( _renderer );
	addComponent( _input );
	addComponent( _physics );

	setDebugColor( ci::ColorA(0.5,0.5,0.5,0.5));
	setLayer(RenderLayer::CHARACTER);
	setVisibilityDetermination( VisibilityDetermination::FRUSTUM_CULLING );
		
	for ( int i = 0; i < WeaponType::COUNT; i++ ) _weapons[i] = NULL;
}

Player::~Player()
{
	// physics are owned by my _physics component, which will be deleted automatically
	aboutToDestroyPhysics(this);
}

void Player::initialize( const init &initializer )
{
	Entity::initialize( initializer );
	_initializer = initializer;
}


void Player::injured( const HealthComponent::injury_info &info )
{
	notificationDispatcher()->broadcast(Notification(Notifications::PLAYER_INJURED, info));
			
	const real 
		WobbleRadius = cpBBRadius( aabb() ) * 0.1;
	
	GameLevel *level = static_cast<GameLevel*>(this->level());
	
	switch( info.type )
	{
		case InjuryType::CUTTING_BEAM_WEAPON:
		case InjuryType::LAVA:
		{
			const Vec2r Wobble( Rand::randFloat( -WobbleRadius, WobbleRadius ), Rand::randFloat( -WobbleRadius, WobbleRadius ) );
			_fireEmitter->emit( 1, info.position + Wobble );
			break;
		}
		
		case InjuryType::ACID:
		{
			const int 
				Count = static_cast<int>( std::ceil( info.grievousness * 8 ));

			Vec2r wobble;
			for ( int i = 0; i < Count; i++ )
			{
				wobble.x = Rand::randFloat( -WobbleRadius, WobbleRadius );
				wobble.y = Rand::randFloat( -WobbleRadius, WobbleRadius );
				_smokeEmitter->emit( 1, info.position + wobble, normalize( wobble ));
			}
			
			break;
		}
		
		case InjuryType::CRUSH:
		case InjuryType::MONSTER_CONTACT:
		{
			const int 
				Count = static_cast<int>( std::ceil( info.grievousness * 32 ));
						
			Vec2r wobble;
			for ( int i = 0; i < Count; i++ )
			{
				wobble.x = Rand::randFloat( -WobbleRadius, WobbleRadius );
				wobble.y = Rand::randFloat( -WobbleRadius, WobbleRadius );
				_injuryEmitter->emit( 1, info.position + wobble, normalize( wobble ));
			}
						
			break;
		}
			
		default: break;
	}
	
	_physics->injured( info );
}

void Player::died( const HealthComponent::injury_info &info )
{
	app::console() << description() << "::died" << std::endl;

	if ( restrainer() )
	{
		restrainer()->releasePlayer();
	}

	_physics->died( info );
	
	if ( _input )
	{
		removeComponent( _input );
		delete _input;
		_input = NULL;
	}
}

void Player::addedToLevel( Level *level )
{
	Entity::addedToLevel(level);
		
	//
	//	Create weapons
	//
	
	_battery = new Battery();
	_battery->setPower( _initializer.batteryPower );
	
	CuttingBeam *cuttingBeam = new CuttingBeam();
	cuttingBeam->initialize( _initializer.cuttingBeamInit );
	cuttingBeam->setBattery( _battery );
	cuttingBeam->objectWasHit.connect( this, &Player::_shotSomethingWithCuttingBeam );
	_weapons[WeaponType::CUTTING_BEAM] = cuttingBeam;
	
	MagnetoBeam *magnetoBeam = new MagnetoBeam();
	magnetoBeam->initialize( _initializer.magnetoBeamInit );
	magnetoBeam->setBattery( _battery );
	magnetoBeam->objectWasHit.connect( this, &Player::_shotSomethingWithMagnetoBeam );
	_weapons[WeaponType::MAGNETO_BEAM] = magnetoBeam;
		
	selectWeaponType(WeaponType::CUTTING_BEAM);
}

void Player::removedFromLevel( Level *removedFrom )
{
	if ( restrainer() )
	{
		restrainer()->releasePlayer();
	}
}

void Player::ready()
{	
	Entity::ready();

	//
	//	Create effect emitter
	//

	GameLevel *gameLevel = static_cast<GameLevel*>(level());
	_smokeEmitter = gameLevel->smokeEffect()->emitter(16);
	_fireEmitter = gameLevel->fireEffect()->emitter(16);
	_injuryEmitter = gameLevel->monsterInjuryEffect()->emitter(16);
}

void Player::update( const time_state &time )
{
	//
	//	Synchronize physics rep
	//
	
	if ( alive() )
	{
		_physics->setSpeed( motion() * _initializer.walkingSpeed );
		_physics->setJumping( jumping() );
		_physics->setCrouching( crouching() );
	}
	
	//
	//	Update weapon directions and position to match player
	//

	{
		const Vec2r 
			WeaponPos = this->position(),
			WeaponEmissionPos = _renderer->barrelEndpointWorld();

		for ( std::vector< Weapon* >::iterator weapon( _weapons.begin()), end( _weapons.end()); weapon != end; ++weapon )
		{
			Weapon *w = *weapon;
			if ( w )
			{
				w->setPosition( WeaponPos );
				w->setEmissionPosition( WeaponEmissionPos );
			}
		}
	}
	
	//
	//	Now, if we're restrained, we need to update our wiggle rate to see if we've escaped the monster holding us
	//
	
	if ( restrained() )
	{
		const seconds_t ElapsedSinceGrab = time.time - _restrainedTime;
		const real WeaponDir = weapon()->direction().x;
		const int CurrentDir = static_cast<int>(sign(WeaponDir < -0.5 ? -1 : (WeaponDir > 0.5 ? 1 : 0)));
		
		if ( CurrentDir != _wiggleDirection )
		{
			_wiggleDirection = CurrentDir;
			_wiggleCount++;
		}
		
//		app::console() << description() << "::update "
//			<< " WeaponDir: " << WeaponDir
//			<< " currentDir: " << CurrentDir
//			<< " _wiggleDirection: " << _wiggleDirection
//			<< " _wiggleCount: " << _wiggleCount
//			<< " time since restraint: " << ElapsedSinceGrab
//			<< std::endl;		
		
		if ( ElapsedSinceGrab > 2 )
		{
			seconds_t wigglesPerSecond = static_cast<seconds_t>(_wiggleCount) / ElapsedSinceGrab;
			if ( wigglesPerSecond >= restrainer()->initializer().playerWiggleRateToEscapeRestraint )
			{
//				app::console() << description() << "::update - _wiggleCount: " << _wiggleCount
//					<< " elapsedSinceGrab: " << ElapsedSinceGrab
//					<< " wigglesPerSecond: " << wigglesPerSecond
//					<< " exceeds restrainer's playerWiggleRateToEscapeRestraint: " << restrainer()->initializer().playerWiggleRateToEscapeRestraint
//					<< std::endl;

				_timeWiggling += time.deltaT;
			}
			else
			{
				_timeWiggling = std::max<seconds_t>( _timeWiggling - time.deltaT, 0 );
			}
		}
		else
		{
			_timeWiggling = 0;
		}
		
		if ( _timeWiggling > 0 )
		{
			app::console() << description() << "::update"
				<< " timeWiggling: " << _timeWiggling
				<< std::endl;
		}
		
		if ( _timeWiggling >= restrainer()->initializer().playerWiggleTimeToEscape )
		{
			restrainer()->releasePlayer();
		}		
	}

	//
	//	Update bounds
	//
		
	cpBB bounds = cpBBInvalid;
	foreach( cpShape *shape, _physics->shapes() )
	{
		cpBBExpand(bounds, cpShapeCacheBB( shape ));
	}

	setAabb( bounds );
}

Vec2r Player::position() const
{
	return _physics->position();
}

Vec2r Player::groundSlope() const
{
	return _physics->groundSlope();
}

bool Player::touchingGround() const
{
	return _physics->touchingGround();
}

cpBody *Player::body() const
{
	return _physics->body();
}

cpBody *Player::footBody() const
{
	return _physics->footBody();
}

cpGroup Player::group() const
{
	return _physics->group();
}

real Player::motion() const
{
	if ( !_input ) return 0;
	return _input->isGoingRight() ? 1 : _input->isGoingLeft() ? -1 : 0;
}

bool Player::crouching() const
{
	return _input && _input->isCrouching();
}

bool Player::jumping() const
{
	return _input && _input->isJumping();
}

void Player::selectWeaponType( WeaponType::type weaponType )
{
	assert( weaponType != WeaponType::COUNT );

	if ( weaponType != _weaponType )
	{
		if ( (_weaponType != WeaponType::COUNT) && _weapons[_weaponType] )
		{
			_weapons[_weaponType]->setFiring(false);
			_weapons[_weaponType]->_wasPutAway();
			removeChild( _weapons[_weaponType] );
		}
		
		_weaponType = weaponType;
	
		if ( _weapons[_weaponType] )
		{
			addChild( _weapons[_weaponType] );
			_weapons[_weaponType]->_wasEquipped();
			weaponSelected(this, _weapons[_weaponType] );
		}
		else
		{
			weaponSelected(this, static_cast<Weapon*>(NULL) );
		}
	}
}

void Player::setRestrainer( Monster *restrainer )
{
	if ( _restrainer )
	{
		_restrainer->willBeRemovedFromLevel.disconnect( this );
	}
	
	_restrainer = restrainer;

	if ( _restrainer )
	{
		_restrainer->willBeRemovedFromLevel.connect( this, &Player::_unrestrain );
		_restrainedTime = level()->time().time;
		app::console() << description() << "::setRestrainer - restrained by " << _restrainer->description() << std::endl;
	}
	else
	{
		app::console() << description() << "::setRestrainer - RELEASED " << std::endl;
	}

	_wiggleCount = 0;
	_wiggleDirection = 0;
	_timeWiggling = 0;
}

void Player::_shotSomethingWithCuttingBeam( const Weapon::shot_info &info )
{
	GameObject *target = info.target;
	if ( target && (GameObjectType::isMonster( target->type() ) || target == this ))
	{
		Entity *entity = (Entity*) info.target;
		HealthComponent *health = entity->health();

		switch( info.weapon->weaponType() )
		{
			case WeaponType::CUTTING_BEAM:
			{
				CuttingBeam *cutter = (CuttingBeam*) info.weapon;

				health->injure( injury(
					InjuryType::CUTTING_BEAM_WEAPON,
					cutter->initializer().attackStrength, 
					info.position, 
					info.shape ));

				break;
			}
			
			default: break;
		}

	}
}

void Player::_shotSomethingWithMagnetoBeam( const Weapon::shot_info &info )
{}

void Player::_unrestrain( GameObject *obj )
{
	if ( obj == _restrainer )
	{
		setRestrainer(NULL);
	}
}

#pragma mark - PlayerRenderer

/*
		ci::gl::GlslProg _shader;
		core::SvgObject _svg;
		PlayerSvgAnimator *_svgAnimator;
*/

PlayerRenderer::PlayerRenderer():
	_svgAnimator(NULL)
{}

PlayerRenderer::~PlayerRenderer()
{
	delete _svgAnimator;
}

void PlayerRenderer::update( const core::time_state &time )
{
	_svgAnimator->update( time );
}

void PlayerRenderer::ready()
{
	if ( !_svg )
	{
		// load svg
		_svg = level()->resourceManager()->getSvg( "Player.svg" );

		const Vec2r DocumentSize = _svg.documentSize();
		const real Scale = player()->initializer().height / DocumentSize.y;
		_svg.setScale( Scale );

		// load svg animator
		_svgAnimator = new PlayerSvgAnimator( player(), _svg );
	}	

	if ( !_shader )
	{
		_shader = level()->resourceManager()->getShader( "SvgPassthroughShader" );
	}
}


Vec2r PlayerRenderer::barrelEndpointWorld()
{
	return _svgAnimator->barrelEndpointWorld();
}

void PlayerRenderer::_drawGame( const render_state &state )
{	
	if ( _svg )
	{

		_shader.bind();
		_svg.draw( state );
		_shader.unbind();		
	}	
}

void PlayerRenderer::_drawDebug( const render_state &state )
{
	Player *player = this->player();
	PlayerPhysics *physics = player->physics();
	
	gl::enableAlphaBlending();	
	
	if ( true )
	{
		_debugDrawChipmunkShapes( state, physics->shapes() );
		_debugDrawChipmunkConstraints( state, physics->constraints() );
		_debugDrawChipmunkBodies( state, physics->bodies() );
	}
	
	//
	//	Draw the ground slope
	//

	{
		const Vec2r
 			Pos = player->position() + Vec2r( 0, -player->initializer().height/2 ),
			Slope = player->groundSlope();

		gl::color( Color(1,0,0));
		gl::drawLine( Pos, Pos + 2*Slope );

		gl::color( Color(0.5,0,0));
		gl::drawLine( Pos, Pos - 2*Slope );
	}	
	
	//
	//	Draw player's UP vector
	//
	
	gl::color( Color(0,1,0));
	gl::drawLine( player->position(), player->position() + player->physics()->up() * player->initializer().height * 0.6 );
	
	
	
	//_drawGame( state );	
}

#pragma mark - PlayerInputComponent

/*
		int _keyRun, _keyLeft, _keyRight, _keyJump, _keyCrouch, _keyFire, _keyNextWeapon;
		bool _leftMouseButtonPressed;
		real _spin, _deltaSpin;
*/

PlayerInputComponent::PlayerInputComponent()
{
	_keyRun = -1;
	_keyLeft = app::KeyEvent::KEY_a;
	_keyRight = app::KeyEvent::KEY_d;

	_keyJump = app::KeyEvent::KEY_w;
	_keyCrouch = app::KeyEvent::KEY_s;
	_keyFire = app::KeyEvent::KEY_SPACE;
	_keyNextWeapon = app::KeyEvent::KEY_TAB;

	monitorKey( _keyLeft );
	monitorKey( _keyRight );
	monitorKey( _keyJump );
	monitorKey( _keyCrouch );
	monitorKey( _keyFire );
	monitorKey( _keyNextWeapon );
	
	monitorKey( app::KeyEvent::KEY_LSHIFT );
}

PlayerInputComponent::~PlayerInputComponent()
{}

void PlayerInputComponent::monitoredKeyDown( int keyCode )
{
	if ( keyCode == _keyNextWeapon )
	{
		player()->selectWeaponType( WeaponType::type( (player()->selectedWeaponType() + 1) % WeaponType::COUNT ));
	}	
}

bool PlayerInputComponent::isRunning() const
{
	#warning Cinder doesn't tell me about lshift or rshift except as modifiers to other keypresses. Not useful in this context :(
	return false;
}

bool PlayerInputComponent::isGoingRight() const
{
	return isKeyDown( _keyRight );
}

bool PlayerInputComponent::isGoingLeft() const
{
	return isKeyDown( _keyLeft );
}

bool PlayerInputComponent::isJumping() const
{
	return isKeyDown( _keyJump );
}

bool PlayerInputComponent::isCrouching() const
{
	return isKeyDown( _keyCrouch );
}

} // end namespace game