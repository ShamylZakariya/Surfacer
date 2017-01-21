#pragma once

//
//  YogSothoth.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 1/25/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "MotileFluidMonster.h"

namespace game {

class YogSothoth : public MotileFluidMonster 
{
	public:
	
		struct init : public MotileFluidMonster::init
		{
			init()
			{
				health.initialHealth = health.maxHealth = 100;
				health.resistance = 0.5;
				
				fluidInit.fluidHighlightColor = ci::ColorA( 0,0,1,0.5 );
				fluidInit.fluidParticleTexture = "YogSothothParticle.png";
				fluidInit.fluidToneMapTexture = "YogSothothStarsToneMap.png";
				fluidInit.fluidBackgroundTexture = "YogSothothStars.jpg";
				fluidInit.particleScale = 1.5;
				fluidInit.pulsePeriod = 2;
				fluidInit.pulseMagnitude = 0.5;

				speed = 5;
				attackStrength = 20;
			}
			
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				MotileFluidMonster::init::initialize(v);
			}
			
		};

	public:
	
		YogSothoth();
		virtual ~YogSothoth();

		JSON_INITIALIZABLE_INITIALIZE();
		
		virtual void initialize( const init & );
		const init &initializer() const { return _initializer; }
	
	protected:

		virtual MotileFluid::fluid_type _fluidType() const;
		
	private:

		init _initializer;

};

class YogSothothController : public MotileFluidMonsterController
{
	public:
	
		YogSothothController();
		virtual ~YogSothothController(){}
		
		YogSothoth *yogSothoth() const { return (YogSothoth*) owner(); }

};



} // end namespace game