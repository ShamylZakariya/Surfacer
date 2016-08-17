#pragma once

//
//  SurfaceGameLevel.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 7/7/11.
//  Copyright 2011 Shamyl Zakariya. All rights reserved.
//

#include "Level.h"

#include "ParticleSystem.h"
#include "Filters.h"
#include "Terrain.h"

namespace game {

class ActionDispatcher;
class Background;
class Fluid;
class HealthComponent;
class PlayerHud;
class Player;
class ViewportController;

/**
	subclass of core::Level with various gameplay behaviors and effects
*/
class GameLevel : public core::Level {

	public:

		struct init : public core::Level::init
		{
			real defaultZoom;
		
			init():
				defaultZoom(10)
			{}
			
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				core::Level::init::initialize(v);
				JSON_READ(v,defaultZoom);
			}
		};

	public:
	
		GameLevel();
		virtual ~GameLevel();
		
		JSON_INITIALIZABLE_INITIALIZE();
	
		// Level

		virtual void initialize( const init &initializer );
		const init &initializer() const { return _initializer; }
		virtual void addedToScenario( core::Scenario * );
		virtual void removedFromScenario( core::Scenario * );
		virtual cpBB bounds() const;
		virtual void ready();
		virtual void update( const core::time_state & );
		virtual void addObject( core::GameObject *object );
		
		// NotificationListener
		virtual void notificationReceived( const core::Notification &note );
		
		// GameLevel

		ViewportController *cameraController() const { return _cameraController; }

		UniversalParticleSystemController *cutTerrainEffect() const { return _cutEffect; }
		UniversalParticleSystemController *dustEffect() const { return _dustEffect; }
		UniversalParticleSystemController *fireEffect() const { return _fireEffect; }
		UniversalParticleSystemController *smokeEffect() const { return _smokeEffect; }
		UniversalParticleSystemController *monsterInjuryEffect() const { return _monsterInjuryEffect; }

		terrain::Terrain* terrain() const { return _terrain; }
		Player *player() const { return _player; }		
		ActionDispatcher *actionDispatcher() const { return _actionDispatcher; }
				
		/**
			Get the Level's acid engine.
			Monsters emit acid sometimes for injury, and sometimes as an attack. Instead of each
			monster having its own acid instance, the Level provides a single one which is shared.
		*/

		Fluid *acid() const { return _acid; }
		virtual void setAcid( Fluid *acid );
		void emitAcid( const Vec2r &position, const Vec2r &vel = Vec2r(0,0));
		void emitAcidInCircularVolume( const Vec2r &position, real radius, const Vec2r &vel = Vec2r(0,0));
		
		/**
			Get the PlayerHud ui::Layer, used to display player stats, etc
		*/
		PlayerHud *playerHudLayer() const { return _playerHudLayer; }
		
		/**
			Get the notification layer. 
			This layer should be used to display gameplay message boxes, info, warnings, etc.
		*/
		core::ui::Layer *notificationLayer() const { return _notificationLayer; }
		
		
		/*
			Create a GameObject or Behavior from Json, and add it.
			The ci::JsonTree should look something like:
{
  "class" : "Battery",
  "enabled" : true,
  "initializer" : {
	"amount" : 10,
	"type" : "BATTERY",
	"position" : {
	  "x" : 199,
	  "y" : 144
	}
  }
}		

			The ci::JsonTree's root level contains:
				- the classname
				- an optional boolean 'enabled' which if false will early exit
				- an object initializer to configure the thing being created.


			If successful, the object is added to the level and is returned. Otherwise,
			if the object couldn't be created or initialized, this method does not add anything
			to the level and returns NULL.
	
		*/
		
		core::Object *create( const ci::JsonTree &recipe );
		
				
	protected:
	
		virtual void _setTerrain( terrain::Terrain *t );
		virtual void _setPlayer( Player * );
	

		void _collision_Object_Terrain( const core::collision_info &info, bool &discard );
		void _collision_Monster_Monster( const core::collision_info &info, bool &discard );		
		void _collision_Monster_Object( const core::collision_info &info, bool &discard );		
		void _terrainWasCut( const Vec2rVec &positions, TerrainCutType::cut_type cutType );
		void _createEffectEmitters();
		
		
	private:
	
		init						_initializer;
		terrain::Terrain				*_terrain;
		Player						*_player;
		Fluid						*_acid;

		UniversalParticleSystemController *_cutEffect, *_dustEffect, *_fireEffect, *_smokeEffect, *_monsterInjuryEffect;

		PlayerHud					*_playerHudLayer;
		core::ui::Layer				*_notificationLayer;
		ActionDispatcher				*_actionDispatcher;
		ViewportController			*_cameraController;
		
};

} // end namespace game