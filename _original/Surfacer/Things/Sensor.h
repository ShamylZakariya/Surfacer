#pragma once

//
//  Sensor.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/7/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Core.h"
#include "Notification.h"

namespace game { 
	namespace terrain {
		class Island;
	}
}

namespace game 
{
	class Sensor;

	namespace SensorTargetType
	{
		enum type {
			NONE,
			PLAYER,
			TERRAIN,
			POWERCELL
		};
		
		std::string toString( type );
		type fromString( const std::string & );
	}
	
	struct sensor_event 
	{
		Sensor *sensor;
		bool triggered;
		std::string eventName;
							
		sensor_event( Sensor *s, bool t, const std::string &n ):
			sensor(s), 
			triggered(t),
			eventName(n)
		{}
	};
	
	#pragma mark -

	class Sensor : public core::GameObject 
	{
		public:	
				
			struct init : public core::GameObject::init
			{
				std::string eventName;

				// if true, the sensor will fire once and will remove itself (and free itself) on de-activation
				bool once;
			
				Vec2 position;
				real width, height, angle;

				SensorTargetType::type targetType;

				//	if target==TERRAIN, this is the type of voxel this sensor watches for. If this value is
				//	zero it matches all terrain voxel id types
	
				unsigned int voxelId;
								
				init():
					once(false),
					position(0,0),
					width(0),
					height(0),
					angle(0),
					targetType(SensorTargetType::PLAYER),
					voxelId(0)
				{}
				
				//JsonInitializable
				virtual void initialize( const ci::JsonTree &v )
				{
					core::GameObject::init::initialize(v);

					JSON_READ(v, eventName);
					JSON_READ(v, once);
					JSON_READ(v, position);
					JSON_READ(v, width);
					JSON_READ(v, height);
					JSON_READ(v, angle);
					JSON_ENUM_READ(v, SensorTargetType, targetType);
					JSON_READ(v, voxelId);
				}
					
			};
			
			signals::signal< void(const sensor_event &)> sensorTriggered;
	
		public:
		
			Sensor();
			virtual ~Sensor();

			JSON_INITIALIZABLE_INITIALIZE();

			virtual void initialize( const init &i );
			const init &initializer() const { return _initializer; }
	
			// Object
			virtual std::string description() const;

			// GameObject
			virtual void addedToLevel( core::Level *level );
						
			// Sensor
			cpShape *shape() const { return _shape; }
			cpBody *body() const { return _body; }
			bool triggered() const { return _triggerCount > 0; }
			std::string eventName() const { return _initializer.eventName; }
			
		protected:
		
			/**
				Invoked when the sensor is triggered. 
				@param onOrOff represents whether the sensor has triggered on, or off.
				
				Default implementation fires sensorTriggered signal and dispatches
				Notifications::SENSOR_TRIGGERED notification, both of which are passed
				a corresponding sensor event.				
			*/
			virtual void _trigger( bool onOrOff );
		
		private:
		
			void _contactPlayer( const core::collision_info &info, bool &discard );
			void _separatePlayer( const core::collision_info &info );

			void _contactTerrain( const core::collision_info &info, bool &discard );
			void _separateTerrain( const core::collision_info &info );
			bool _islandTriggers( terrain::Island *island );

			void _contactPowerCell( const core::collision_info &info, bool &discard );
			void _separatePowerCell( const core::collision_info &info );
			
			void _onContact( GameObject * );
			void _onSeparate( GameObject * );
			
			bool _isFirstContact( GameObject *obj ); 
			bool _isFinalSeparation( GameObject *obj ); 
			
		private:
		
			init _initializer;
			int _triggerCount;
			cpBody *_body;
			cpShape *_shape;			
			std::map< std::size_t, int > _contacts;
			std::set< std::size_t > _ignorable;
			std::string _description;

	};
	
	class SensorRenderer : public core::DrawComponent
	{
		public:
		
			SensorRenderer();
			virtual ~SensorRenderer();
			
			Sensor *sensor() const { return static_cast<Sensor*>(owner()); }
						
		protected:

			virtual void _drawGame( const core::render_state & );
			virtual void _drawDevelopment( const core::render_state &s );
			virtual void _drawDebug( const core::render_state & );
			
		private:

			ci::gl::Texture _descTex;
	};
	
 
	#pragma mark -
		
	class ViewportFitSensor : public Sensor
	{
		public:
		
			static const std::string FitEventName;

		public:
		
			ViewportFitSensor();
			virtual ~ViewportFitSensor();

			virtual void initialize( const init &i );

			// Object
			virtual std::string description() const;	
			
		private:
		
			init _initializer;
			
	};	
   
  
}
