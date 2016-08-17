#pragma once

//
//  Shoggoth.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 12/29/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#include "MotileFluidMonster.h"

namespace game {

class Shoggoth : public MotileFluidMonster 
{
	public:
	
		struct init : public MotileFluidMonster::init
		{
			init()
			{
				health.initialHealth = health.maxHealth = 40;
				health.resistance = 0.5;
			
				fluidInit.radius = 3;
				fluidInit.stiffness = 0.25;
				fluidInit.fluidParticleTexture = "ShoggothParticle.png";
				fluidInit.fluidToneMapTexture = "ShoggothToneMap.png";
				fluidInit.fluidHighlightColor = ci::ColorA(0,0.5,1,0.1);
				fluidInit.fluidHighlightLookupDistance = 1;
				fluidInit.pulseMagnitude = 0.25;
				fluidInit.friction = 10;
				fluidInit.elasticity = 1;
				
				fluidInit.particleScale = 1;
				
				speed = 5;
				attackStrength = 10;
			}
			
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				MotileFluidMonster::init::initialize(v);
			}

		};

	public:
	
		Shoggoth();
		virtual ~Shoggoth();
		
		JSON_INITIALIZABLE_INITIALIZE();
		
		virtual void initialize( const init & );
		const init &initializer() const { return _initializer; }
	
	protected:

		virtual MotileFluid::fluid_type _fluidType() const;
		
	private:

		init _initializer;

};

class ShoggothController : public MotileFluidMonsterController
{
	public:
	
		ShoggothController();
		virtual ~ShoggothController(){}
		
		Shoggoth *yogSothoth() const { return (Shoggoth*) owner(); }

};



} // end namespace game