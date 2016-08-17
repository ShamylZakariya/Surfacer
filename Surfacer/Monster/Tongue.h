#pragma once

//
//  Tongue.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 7/12/12.
//
//

#include "Monster.h"

namespace game {

class Tongue : public Monster
{
	public:
	
		struct init : public Monster::init
		{
			real density;
			real length;
			real thickness;
			real detachForceScale;
			seconds_t timeToGrabPlayer;
			seconds_t timeBetweenPlayerGrabs;
			int segmentCount;
			ci::ColorA color;
			
			// if parent && perent body are specified, the tongue will be attached to them, originating at their position
			// otherwise tongue will be attached to a static body at @a origin
			GameObject *anchoredTo;
			cpBody *anchoredToBody;
					
			init():
				density(40),
				length(20),
				thickness(1),
				detachForceScale(INFINITY),
				timeToGrabPlayer(0.25),
				timeBetweenPlayerGrabs(2),
				segmentCount(10),
				color(0.1,0.1,0.1,1),
				anchoredTo(NULL),
				anchoredToBody(NULL)
			{
				extroTime = 4;
				health.initialHealth = health.maxHealth = 10;
				health.resistance = 0.1;
				attackStrength = 0;
			}
						
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				Monster::init::initialize(v);

				JSON_READ( v, density );
				JSON_READ( v, length);
				JSON_READ( v, thickness);
				JSON_READ( v, detachForceScale);
				JSON_READ( v, timeToGrabPlayer );
				JSON_READ( v, color);
			}
		};
		
		struct segment {
			cpConstraint *pivot, *pin, *angularLimit;
			cpShape *shape;
			cpBody *body;
			real length;
			real radius;
			real distance;
			
			segment():
				pivot(NULL),
				pin(NULL),
				angularLimit(NULL),
				shape(NULL),
				body(NULL),
				length(0),
				radius(0),
				distance(0)
			{}
		};
		
		typedef std::vector<segment> segment_vec;
		
	public:
	
		Tongue();
		virtual ~Tongue();
		
		JSON_INITIALIZABLE_INITIALIZE();
				
		virtual void initialize( const init &i );
		const init &initializer() const { return _initializer; }
		
		// GameObject
		virtual void addedToLevel( core::Level *level );
		virtual void removedFromLevel( core::Level *removedFrom );
		virtual void ready();
		virtual void update( const core::time_state & );
		
		// Entity
		virtual void injured( const HealthComponent::injury_info &info );
		virtual void died( const HealthComponent::injury_info &info );
		
		// Monster
		virtual Vec2r position() const;
		virtual Vec2r up() const;
		virtual Vec2r eyePosition() const;
		virtual cpBody *body() const;
		virtual void restrainPlayer();
		virtual void releasePlayer();

		
		// Tongue
		void attachTo( Monster *parent, cpBody *body, const Vec2r &positionWorld );
		void attach( const Vec2r &positionWorld );
		bool attached() const;
		cpBody *attachedTo() const;
		void detach();
		
		bool playerBeingHeld() const { return !_grabConstraints.empty(); }
		
		void setExtension( real extension ) { _extension = saturate(extension); }
		real extension() const { return _extension; }

		const segment_vec segments() const { return _segments; }

	protected:
	
		friend class TongueRenderer;
	
		void _createShapes( real segmentLength );
		bool _hasShapes() const;

		virtual void _anchorAboutToDestroyPhysics( GameObject * );
		virtual void _parentWillBeRemovedFromLevel( GameObject * );
		virtual void _parentDied( const HealthComponent::injury_info & );

		//
		//	Barnacle dispatches injury differently, depending on tongue or body contact.
		//

		virtual void _monsterPlayerContact( const core::collision_info &, bool &discard );
		virtual void _monsterPlayerPostSolve( const core::collision_info & );
		virtual void _monsterPlayerSeparate( const core::collision_info & );

		
	private:
		
		init _initializer;
		segment_vec _segments;
		real _extension, _contractionExtension, _currentExtension, _pivotForce, _segmentLength;
		int _playerContactCount;
		bool _initContraction, _grabbingPlayer, _releasingPlayer;
		seconds_t _birthTime, _detachTime, _timeTouchingPlayer, _timeReleasedPlayer;
		cpConstraintSet _grabConstraints;
	
};

class TongueRenderer : public core::DrawComponent
{
	public:
	
		TongueRenderer();
		virtual ~TongueRenderer();
		
		virtual void draw( const core::render_state &state );
		
		Tongue *tongue() const { return (Tongue*) owner(); }

	protected:
	
		void _drawGame( const core::render_state &state );
		void _drawDebug( const core::render_state &state );
		
		void _updateSpline( const core::render_state &state );
		void _updateTrianglulation( const core::render_state &state );
		
	private:

		struct vertex {
			ci::Vec2f position;

			vertex()
			{}

			vertex( const ci::Vec2f &p ):
				position(p)
			{}
		};
		
		struct spline_rep {
			Vec2fVec positions, dirs, splinePositions;
		};
		
		typedef std::vector<vertex> vertex_vec;

	private:

		GLuint _vbo;
		ci::gl::GlslProg _shader;

		spline_rep _splineRep;
		vertex_vec _triangleStrip;
		
};




}