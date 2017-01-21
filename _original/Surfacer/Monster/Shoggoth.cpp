//
//  MotileFluidMonster.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 12/29/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#include "Shoggoth.h"
#include "GameConstants.h"

using namespace ci;
using namespace core;
namespace game {


CLASSLOAD(Shoggoth);

Shoggoth::Shoggoth():
	MotileFluidMonster( GameObjectType::SHOGGOTH )
{
	setName( "Shoggoth" );
}

Shoggoth::~Shoggoth()
{}

void Shoggoth::initialize( const init &i )
{
	MotileFluidMonster::initialize( i );
	_initializer = i;
}

MotileFluid::fluid_type 
Shoggoth::_fluidType() const 
{ 
	return MotileFluid::PROTOPLASMIC_TYPE; 
}


#pragma mark - ShoggothController

ShoggothController::ShoggothController()
{
	setName( "ShoggothController" );
}



} // end namespace game