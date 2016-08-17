//
//  YogSothoth.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 1/25/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "YogSothoth.h"
#include "GameConstants.h"

using namespace ci;
using namespace core;
namespace game {

CLASSLOAD(YogSothoth);

YogSothoth::YogSothoth():
	MotileFluidMonster( GameObjectType::YOG_SOTHOTH )
{
	setName( "YogSothoth" );
}

YogSothoth::~YogSothoth()
{}

void YogSothoth::initialize( const init &i )
{
	MotileFluidMonster::initialize( i );
	_initializer = i;
}

MotileFluid::fluid_type 
YogSothoth::_fluidType() const 
{ 
	return MotileFluid::AMORPHOUS_TYPE; 
}


#pragma mark - YogSothothController

YogSothothController::YogSothothController()
{
	setName( "YogSothothController" );
}



} // end namespace game