//
//  Weapon.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 11/24/11.
//  Copyright (c) 2011 TomorrowPlusX. All rights reserved.
//

#include "Weapon.h"

#include "GameConstants.h"
#include "Level.h"
#include "Monster.h"
#include "Player.h"

using namespace ci;
using namespace core;
namespace game {

namespace {

	struct beam_query_data 
	{
		cpCollisionType collisionType;
		cpCollisionType ignoreCollisionType;
		unsigned int filterMask;
		cpShape *ignore;
		Weapon *weapon;
		cpVect start, end, normal;
		GameObject *touchedObject;
		cpShape *touchedShape;
		real dist;
		
		beam_query_data( cpCollisionType ct, cpCollisionType ict, unsigned int fm, cpShape *ig, Weapon *w ):
			collisionType(ct), 
			ignoreCollisionType(ict),
			filterMask(fm),
			ignore(ig),
			weapon(w),
			touchedObject(NULL),
			touchedShape(NULL),
			dist( FLT_MAX )
		{}
	};
	
	void beamQueryCallback(cpShape *shape, cpFloat t, cpVect n, void *data)
	{
		beam_query_data *query = (beam_query_data*) data;

		if ( query->ignore == shape ) return;

		GameObject *obj = (GameObject*) cpShapeGetUserData( shape );		
		if (!obj) return;

		cpCollisionType type = cpShapeGetCollisionType( shape );

		//
		//	If the shape is of a type set to be ignored
		//

		if ( query->ignoreCollisionType && (type == query->ignoreCollisionType)) return;
		
		//
		//	Report collision if user passed 0 for both type and filter, or if
		//	the hit type is same as collisionType or if hit type passes mask with filter.
		//
		
		if ( (query->collisionType && (query->collisionType == type )) ||
		     (query->filterMask && (query->filterMask & type)) ||
			 ( query->collisionType==0 && query->filterMask==0 ))
		{
			bool allowHit = true;
			
			if ( query->weapon && GameObjectType::isMonster( obj->type() ))
			{
				allowHit = ((Monster*)obj)->allowWeaponHit( query->weapon, shape );
			}
			
			if ( allowHit )
			{
				cpSegmentQueryInfo info;
				info.shape = shape;
				info.t = t;
				info.n = n;
				
				real dist = cpSegmentQueryHitDist( query->start, query->end, info );
				if ( dist < query->dist )
				{
					query->touchedObject = (GameObject*) cpShapeGetUserData( shape );
					query->touchedShape = shape;
					query->dist = dist;
					query->normal = n;
				}
			}
		}
	}


}

#pragma mark - WeaponType

namespace WeaponType {

	std::string displayName( int weaponType )
	{
		std::string dn("<UNKNOWN>");

		switch( weaponType )
		{
			case CUTTING_BEAM:
				dn = "Cutter";
				break;
				
			case MAGNETO_BEAM:
				dn = "Tractor";
				break;
				
			default: break;
		}
		
		return dn;
	}

}


#pragma mark - Weapon

/*
		bool _active;
		Battery *_battery; 
		init _initializer;
		bool _firing;
		Vec2 _position, _direction;
		real _range;
*/

Weapon::Weapon():
	GameObject( GameObjectType::PLAYER_WEAPON ),
	_active(false),
	_battery(NULL),
	_firing(false),
	_position(0,0),
	_direction(1,0),
	_range(0)
{
	setLayer(RenderLayer::CHARACTER);
}

Weapon::~Weapon()
{
	// note we don't own the battery, the Player does
}

void Weapon::initialize( const init &i )
{
	GameObject::initialize(i);
	_initializer = i;
	_range = _initializer.maxRange;
}

void Weapon::update( const time_state &time )
{
	//
	//	If drawingPower and battery isn't empty, reduce weapon charge. We're allowing weapon charge to fall to zero by checking
	//	if weapon is !empty, rather than if it has enough charge for one more firing timestep. If battery empties, set firing
	//	to false. 
	//

	if ( drawingPower() )
	{
		if ( _battery && !_battery->empty() )
		{
			_battery->setPower( _battery->power() - ( drawingPowerPerSecond() * time.deltaT ));
		}
		else
		{
			setFiring( false );
		}
	}
	
	Player *player = static_cast<Player*>( parent() );
	if ( player->dead() )
	{
		setFiring(false);
	}
}

void Weapon::setFiring( bool on )
{
	//
	//	If user turns on firing, check if there's a bettery and if it's not empty. Otherwise
	//	turn off firing and notify that we tried tof ire an empty gun. 
	//
	
	if ( on )
	{
		if ( _battery && !_battery->empty() ) 
		{
			_firing = true;
		}
		else
		{
			_firing = false;
			attemptToFireEmptyWeapon(this);
		}
	}
	else
	{
		_firing = false;
	}
}


#pragma mark - BeamWeapon

/*
		init _initializer;
		Vec2 _beamEnd;
		raycast_target _beamTarget;
		raycast_query_filter _raycastQF;
*/

BeamWeapon::BeamWeapon():
	_beamEnd(0,0)
{
	setName( "BeamWeapon" );
	setVisibilityDetermination( VisibilityDetermination::FRUSTUM_CULLING );
}

BeamWeapon::~BeamWeapon()
{}

void BeamWeapon::initialize( const init &i )
{
	Weapon::initialize(i);
	_initializer = i;
}

void BeamWeapon::update( const time_state &time )
{
	Weapon::update(time);

	const Vec2 
		position = this->position(),
		direction = this->direction();
		
	const bool firing = this->firing();

	_beamEnd = position + (direction * this->range());	
	_beamTarget = _raycast( position, _beamEnd );

	if ( _beamTarget )
	{
		_beamEnd = _beamTarget.position;

		if ( firing )
		{
			shot_info sinfo;
			sinfo.weapon = this;
			sinfo.target = _beamTarget.object;
			sinfo.shape = _beamTarget.shape;
			sinfo.position = _beamTarget.position;
			sinfo.normal = _beamTarget.normal;
		
			objectWasHit( sinfo );
		}		
	}

	//
	//	Expand Aabb
	//

	cpBB bounds;
	cpBBNewCircle( bounds, position, _initializer.width );
	cpBBExpand( bounds, _beamEnd, _initializer.width );
	setAabb( bounds );		
}

BeamWeapon::raycast_target 
BeamWeapon::_raycast( const Vec2 &start, const Vec2 &end, cpShape *ignore ) const
{
	return raycast( level(), const_cast<BeamWeapon*>(this), start, end, _raycastQF, ignore );
}

BeamWeapon::raycast_target BeamWeapon::raycast( 
	Level *level, 
	Weapon *weapon,
	const Vec2 &start, 
	const Vec2 &end, 
	const raycast_query_filter &filter, 
	cpShape *ignore )
{
	const real
		Offset = 0.01;

	const Vec2	
		Dir = normalize( end - start ),
		Right = Offset * rotateCW( Dir );

	
	//
	//	Cast two tight parallel rays, and average the result normals - this handles
	//	the case where a ray hits a crack between terrain shapes
	//

	beam_query_data 
		query0( filter.collisionType, filter.ignoreCollisionType, filter.collisionTypeMask, ignore, weapon ),
		query1( query0 );
	
	query0.start = cpv(start - Right);
	query0.end = cpv(end - Right);
	cpSpaceSegmentQuery( level->space(), query0.start, query0.end, filter.layers, filter.group, beamQueryCallback, &query0 );

	query1.start = cpv(start + Right);
	query1.end = cpv(end + Right);
	cpSpaceSegmentQuery( level->space(), query1.start, query1.end, filter.layers, filter.group, beamQueryCallback, &query1 );
		
	raycast_target target;
	if ( query0.touchedShape || query1.touchedShape )
	{
		if ( query0.touchedShape )
		{
			target.object = query0.touchedObject;
			target.shape = query0.touchedShape;		
			target.body = cpShapeGetBody( query0.touchedShape );
		}
		else
		{
			target.object = query1.touchedObject;
			target.shape = query1.touchedShape;		
			target.body = cpShapeGetBody( query1.touchedShape );
		}

		//
		//	if the two parallel queries hit close to eachother, average their result;
		//	otherwise, take the closer result
		//
		
		if ( std::abs( query0.dist - query1.dist ) > 2 * Offset )
		{
			if ( query0.dist < query1.dist )
			{
				target.position = start + Dir * query0.dist; 
				target.normal = v2(query0.normal);
			}
			else
			{
				target.position = start + Dir * query1.dist; 
				target.normal = v2(query1.normal) ;
			}			
		}
		else
		{
			real dist = (query0.dist + query1.dist) * 0.5;
			target.position = start + Dir * dist; 
			target.normal = v2(cpvnormalize( query0.normal + query1.normal));
		}
	}

	return target;
}


} //end namespace game
