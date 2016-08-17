//
//  CuttingTestScenario.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/14/11.
//  Copyright 2011 Shamyl Zakariya. All rights reserved.
//

#include "CuttingTestScenario.h"

#include <cinder/app/App.h>
#include <cinder/ImageIO.h>
#include <cinder/Utilities.h>

#include "Background.h"
#include "Fluid.h"
#include "GameConstants.h"
#include "ParticleSystem.h"
#include "Player.h"
#include "ViewportController.h"

using namespace ci;
using namespace core;
using namespace game;

namespace {

	const bool PARTICLE_EFFECTS_ENABLED = true;

	const bool PLAYER_ENABLED = false;
	const bool FOLLOW_PLAYER = true;
	
	const bool LINE_CUT_ENABLED = true;
	const bool DISK_CUT_ENABLED = false;
	
	const real LevelScale = 0.25;


	const real CutThickness = 2 * LevelScale;
	const real CutStrength = 0.1;
	const real ManualCutStrength = 10; // > 1 triggers infinite strenth cut

	void drawLineSegment( const Vec2r &a, const Vec2r &b, real thickness )
	{
		util::line_segment segment(a, b);
		Vec2r right( segment.dir.y, -segment.dir.x );
		right *= thickness * 0.5;
				
		gl::enableAlphaBlending();
		gl::color( ColorA(1,0.5,0.5,0.5));
		glBegin( GL_QUADS );
		
		gl::vertex( a + right );
		gl::vertex( b + right );
		gl::vertex( b - right );
		gl::vertex( a - right );
		
		glEnd();
	
	}
	
	void BBQueryIslandGatherer(cpShape *shape, void *data)
	{
		if ( cpShapeGetCollisionType( shape ) == CollisionType::TERRAIN )
		{
			GameObject *obj = (GameObject*) cpShapeGetUserData( shape );
			if ( obj->type() == GameObjectType::ISLAND )
			{
				std::set< terrain::Island* > *islands = (std::set< terrain::Island* > *) data;
				islands->insert( (terrain::Island*) obj );
			}
		}
	}
	
	std::string DescribeCutResult( unsigned int result )
	{
		std::vector< std::string > results;
		if ( result )
		{
			if ( result & terrain::Terrain::CUT_AFFECTED_VOXELS )
				results.push_back( "CUT_AFFECTED_VOXELS" );

			if ( result & terrain::Terrain::CUT_AFFECTED_ISLAND_CONNECTIVITY )
				results.push_back( "CUT_AFFECTED_ISLAND_CONNECTIVITY" );

			if ( result & terrain::Terrain::CUT_HIT_FIXED_VOXELS )
				results.push_back( "CUT_HIT_FIXED_VOXELS" );
		}
		else
		{
			results.push_back( "NONE" );
		}
		
		std::string desc;
		for( int i = 0, N = results.size(); i < N; ++i )
		{
			desc += results[i];
			if ( i < N-1 ) desc += " | ";
		}
		
		return desc;
	}
	
}


/*
		cpBody *_mouseBody;
		cpConstraint *_mouseJoint;
		Vec2r _mouseWorld, _previousMouseWorld, _mouseScreen;
		
		
		Vec2rVec _cut;			
		Font _font;
		
		Player *_player;
*/

CuttingTestScenario::CuttingTestScenario():
	_mouseBody(NULL),
	_mouseJoint(NULL),
	_mouseWorld(0,0),
	_previousMouseWorld(0,0),
	_mouseScreen(0,0),
	_font("HelveticaNeue-Medium", 16)
{
	takeFocus();
	resourceManager()->pushSearchPath( getHomeDirectory() / "Dropbox/Code/Projects/Surfacer/Resources" );
}

CuttingTestScenario::~CuttingTestScenario()
{}

void CuttingTestScenario::setup()
{
	GameScenario::setup();

	_mouseBody = cpBodyNew(INFINITY, INFINITY);

	GameLevel::init levelInit;	
	levelInit.gravity = Vec2r( 0, -9.8 );

	GameLevel *level = new GameLevel();
	level->initialize( levelInit );

	setLevel( level );

	ResourceManager *rm = level->resourceManager();

	terrain::Terrain::init terrainInit;
	terrainInit.scale = LevelScale;
	terrainInit.sectorSize = Vec2i( 64, 64 );
	terrainInit.levelImage = "Stalactites.png";
	terrainInit.materialTexture = "BlueTerrain.png";
	terrainInit.materialColor = Color( 54.0/255.0, 80/255.0, 95/255.0 ).lerp(0.1, Color::white());		
	terrainInit.greebleTextureAtlas = "BlueTerrainGreebleAtlas.png";
	terrainInit.greebleSize = 0.5;
	terrainInit.greebleTextureIsMask = true;
	
	terrain::Terrain *terrain = new terrain::Terrain();
	terrain->initialize( terrainInit );
	level->addObject( terrain );


	if ( PLAYER_ENABLED )
	{
		Player::init playerInit;
		playerInit.height = 2.5;
		playerInit.width = 1;
		playerInit.position = Vec2r(3.5 * 64, 5.5 * 64 ) * LevelScale;

		playerInit.cuttingBeamInit.cutStrength = 0.1;
		playerInit.cuttingBeamInit.attackStrength = 10;
		playerInit.cuttingBeamInit.width = LevelScale;
		playerInit.cuttingBeamInit.cutWidth = 2*LevelScale;
		
		Player *player = new Player();
		player->initialize( playerInit );
		level->addObject( player );
	}

	Background::init backgroundInit;
	backgroundInit.clearColor = Color(0.2,0.2,0.2);
	backgroundInit.sectorSize = 64;
	backgroundInit.addLayer( "Dark-Paper-Background.png", 1.1 );
	
	Background *background = new Background();
	background->initialize( backgroundInit );
	level->addObject( background );
		
	level->camera().lookAt( v2r(cpBBCenter( level->bounds())), 10 );
}

void CuttingTestScenario::shutdown()
{
	GameScenario::shutdown();
	setLevel( NULL );
}

void CuttingTestScenario::resize( Vec2i size )
{
	GameScenario::resize( size );
}

void CuttingTestScenario::update( const time_state &time )
{
	GameScenario::update(time);

	cpVect mouseBodyPos = cpv(_mouseWorld),
	       mouseBodyVel = cpvmult(cpvsub(mouseBodyPos, cpv(_previousMouseWorld)), time.deltaT);

	cpBodySetPos( _mouseBody, mouseBodyPos );
	cpBodySetVel( _mouseBody, mouseBodyVel );
	_previousMouseWorld = v2r(mouseBodyVel);

	GameLevel *level = this->gameLevel();
	if ( !level->cameraController()->tracking() )
	{
		const real dp = 10.0f;
		if ( isKeyDown( app::KeyEvent::KEY_UP ))	level->cameraController()->setPan( level->cameraController()->pan() + Vec2r( 0,   -dp ));
		if ( isKeyDown( app::KeyEvent::KEY_DOWN ))	level->cameraController()->setPan( level->cameraController()->pan() + Vec2r( 0,   +dp ));
		if ( isKeyDown( app::KeyEvent::KEY_LEFT ))	level->cameraController()->setPan( level->cameraController()->pan() + Vec2r( +dp, 0 ));
		if ( isKeyDown( app::KeyEvent::KEY_RIGHT ))	level->cameraController()->setPan( level->cameraController()->pan() + Vec2r( -dp, 0 ));		
	}
}

void CuttingTestScenario::draw( const render_state &state )
{
	GameScenario::draw( state );

	//
	//	Render any cuts that are in-process
	//

	if ( !_cut.empty() )
	{
		gl::pushModelView();
		camera().set();

			if ( LINE_CUT_ENABLED )
			{
				for ( int i = 0; i < _cut.size() - 1; i++ )
				{
					drawLineSegment( _cut[i], _cut[i+1], CutThickness );
				}
			}
			
			if ( DISK_CUT_ENABLED )
			{
				Vec2r center = _cut.front();
				real radius = _cut.back().distance( _cut.front());

				gl::enableAlphaBlending();
				gl::color( ColorA(1,0,0,0.5));
				gl::drawSolidCircle( center, radius, 24 );
				gl::color( ColorA(1,0,0,1));
				gl::drawStrokedCircle( center, radius, 24 );
			}
	
		gl::popModelView();
	}


//	/*
//		We're bypassing GameScenario::draw to draw cutting overlays here
//	*/
//
//	gl::pushModelView();
//
//		camera().set();
//		level()->draw( state );
//
//		//
//		//	Render any cuts that are in-process
//		//
//
//		if ( !_cut.empty() )
//		{
//			if ( LINE_CUT_ENABLED )
//			{
//				for ( int i = 0; i < _cut.size() - 1; i++ )
//				{
//					drawLineSegment( _cut[i], _cut[i+1], CutThickness );
//				}
//			}
//			
//			if ( DISK_CUT_ENABLED )
//			{
//				Vec2r center = _cut.front();
//				real radius = _cut.back().distance( _cut.front());
//
//				gl::enableAlphaBlending();
//				gl::color( ColorA(1,0,0,0.5));
//				gl::drawSolidCircle( center, radius, 24 );
//				gl::color( ColorA(1,0,0,1));
//				gl::drawStrokedCircle( center, radius, 24 );
//			}
//			
//		}	
//
//	gl::popModelView();
//
//	uiStack()->draw( state );
}

#pragma mark - Input

bool CuttingTestScenario::mouseDown( const app::MouseEvent &event )
{		
	//
	//	if mousedown occurs on a body, we'll setup a mouse joint to drag that body around,
	//	otherwise we'll create a slice to cut the world geometry				
	//
	
	Viewport &camera( level()->camera());
	cpSpace *space = level()->space();

	_mouseScreen = event.getPos();
	_mouseWorld = camera.screenToWorld( _mouseScreen );
	
	app::console() << "Mouse down screen: " << _mouseScreen << " world: " << _mouseWorld << std::endl;

	if ( event.isAltDown() )
	{
		camera.lookAt( _mouseWorld, camera.zoom(), camera.viewportCenter() );
		return true;			
	}

	if ( isKeyDown( app::KeyEvent::KEY_SPACE )) return false;

	cpShape *shape = cpSpacePointQueryFirst( space, cpv(_mouseWorld), CP_ALL_LAYERS, CP_NO_GROUP);
	if(shape)
	{
		cpBody *body = shape->body;
		if ( body && cpBodyGetUserData(body) && !cpBodyIsStatic(body) )
		{
			GameObject *gameObject = (GameObject*) cpBodyGetUserData(body);
			
			app::console() << "Clicked game object: " << gameObject->description() << std::endl;
			
			terrain::IslandGroup *group = dynamic_cast<terrain::IslandGroup*>(gameObject);
			if ( group )
			{
				_mouseJoint = cpPivotJointNew( body, _mouseBody, cpv(_mouseWorld));
				_mouseJoint->maxForce = cpBodyGetMass( body ) * std::max(level()->initializer().gravity.length(),real(1)) * 1000;
				cpSpaceAddConstraint( space, _mouseJoint );
				
				return true;
			}
		}
	}
	else
	{
		_cut.clear();
		_cut.push_back( _mouseWorld );				
		return true;
	}
	
	return false;
}

bool CuttingTestScenario::mouseUp( const app::MouseEvent &event )
{
	Viewport &camera( level()->camera());
	cpSpace *space = level()->space();
	GameLevel *level = static_cast<GameLevel*>(this->level());

	if ( _mouseJoint )
	{
		cpSpaceRemoveConstraint( space, _mouseJoint );
		cpConstraintFree( _mouseJoint );
		_mouseJoint = NULL;
		
		return true;
	}
	else if (_cut.size() > 1 ) 
	{				
		_appendToCut( camera.screenToWorld( event.getPos() ) );

		if ( level->terrain() )
		{
			if ( LINE_CUT_ENABLED )
			{
				unsigned int result = level->terrain()->cutLine( 
					_cut.front(), _cut.back(), 
					CutThickness, 
					ManualCutStrength, 
					TerrainCutType::MANUAL );
					
				app::console() << "Line cut [" << _cut.front() << " -> " << _cut.back() << "] result: " << DescribeCutResult(result) << std::endl;
			}
			
			if ( DISK_CUT_ENABLED )
			{
				Vec2r position = _cut.front();
				real radius = position.distance( _cut.back());

				std::set< terrain::Island* > islands;
				cpBB cutBB;
				cpBBNewCircle( cutBB, position, radius );
			
				cpSpaceBBQuery( 
					level->space(), 
					cutBB, 
					CP_ALL_LAYERS, 
					CP_NO_GROUP, 
					BBQueryIslandGatherer, 
					&islands );
					
				app::console() << "Gathered " << islands.size() << " islands" << std::endl;
					
				foreach( terrain::Island *island, islands )
				{
					app::console() << "\tCutting island " << island << std::endl;
					
					unsigned int result = level->terrain()->cutDisk( 
						position, 
						radius,
						ManualCutStrength,
						TerrainCutType::MANUAL,
						island );

					app::console() << "Disk cut @" << position << " r: " << radius << " result: " << DescribeCutResult(result) << std::endl;
				}	
				
			}
		}
		
		_cut.clear();		
		return true;
	}

	_cut.clear();		
	return false;
}	

bool CuttingTestScenario::mouseWheel( const app::MouseEvent &event )
{
	GameLevel *level = static_cast<GameLevel*>(this->level());
	real zoom = level->cameraController()->zoom(),
		 wheelScale = 0.1 * zoom,
		 dz = (event.getWheelIncrement() * wheelScale);
	
	if ( level->player() && FOLLOW_PLAYER )
	{
		// zoom about tank
		level->cameraController()->setZoom( std::max( zoom + dz, real(0.1) ), level->camera().worldToScreen( level->player()->position() ) );
	}
	else
	{
		// zoom about mouse cursor
		level->cameraController()->setZoom( std::max( zoom + dz, real(0.1) ), event.getPos() );
	}
	
	return true;
}

bool CuttingTestScenario::mouseMove( const app::MouseEvent &event, const Vec2r &delta )
{
	Viewport &camera( level()->camera());

	_mouseScreen = event.getPos();
	_mouseWorld = camera.screenToWorld( _mouseScreen );
	return true;
}

bool CuttingTestScenario::mouseDrag( const app::MouseEvent &event, const Vec2r &delta )
{
	Viewport &camera( level()->camera());

	Vec2r screen = event.getPos();
	_mouseWorld = camera.screenToWorld( screen );

	Vec2r dp = screen - _mouseScreen;
	_mouseScreen = screen;
	
	if ( isKeyDown( app::KeyEvent::KEY_SPACE ))
	{
		gameLevel()->cameraController()->setPan( gameLevel()->cameraController()->pan() + delta );
		_cut.clear();
		return true;
	}
	else if ( !_mouseJoint )
	{
		_appendToCut( _mouseWorld );
		return true;
	}
	
	return false;
}	

bool CuttingTestScenario::keyDown( const app::KeyEvent &event )
{
	//app::console() << "CuttingTestScenario::keyDown code: " << event.getCode() << std::endl;
	GameLevel *level = static_cast<GameLevel*>(this->level());

	switch( event.getCode() )
	{
		case app::KeyEvent::KEY_BACKQUOTE:
		{
			setRenderMode( RenderMode::mode( (int(renderMode()) + 1) % RenderMode::COUNT ));
			return true;
		}
		
		case app::KeyEvent::KEY_TAB:
		{
			level->terrain()->setRenderVoxelsInDebug( !level->terrain()->renderVoxelsInDebug());
			app::console() << (level->terrain()->renderVoxelsInDebug() ? "ENABLED" : "DISABLED") << " voxel rendering" << std::endl;
			return true;
		}
		
		case app::KeyEvent::KEY_s:
		{
			if ( event.isMetaDown() )
			{
				screenshot( getHomeDirectory() / "Desktop", "CuttingTestScenario" );
			}

			return true;
		}

		default: break;
	}
	
	return false;
}

bool CuttingTestScenario::keyUp( const app::KeyEvent &event )
{
	//app::console() << "CuttingTestScenario::keyUp code: " << event.getCode() << std::endl;

	switch( event.getCode() )
	{
		default: break;
	}
	
	return false;
}
