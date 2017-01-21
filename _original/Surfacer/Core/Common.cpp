/*
 *  Common.cpp
 *  ConvexDecomposition2
 *
 *  Created by Shamyl Zakariya on 1/25/11.
 *  Copyright 2011 Shamyl Zakariya. All rights reserved.
 *
 */

#include "Common.h"
#include <cinder/App/AppBasic.h>
#include <cinder/gl/gl.h>
#include <cinder/Rand.h>

using namespace ci;

#if CP_USE_DOUBLES

	#warning Compiling Surfacer with 64-bit double

#else

	#warning Compiling Surfacer with 32-bit float

#endif


