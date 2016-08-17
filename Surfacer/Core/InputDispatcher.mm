//
//  InputDispatcher.mm
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/20/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "InputDispatcher.h"

#import <ApplicationServices/ApplicationServices.h>
#import <Cocoa/Cocoa.h>


using namespace ci;
namespace core {

void InputDispatcher::hideMouse()
{
	if ( !_mouseHidden )
	{
		//app::console() << "InputDispatcher::hideMouse - HIDING MOUSE" << std::endl;
		[NSCursor hide];
		_mouseHidden = true;
	}
}

void InputDispatcher::unhideMouse()
{
	if ( _mouseHidden )
	{
		//app::console() << "InputDispatcher::unhideMouse - SHOWING MOUSE" << std::endl;
		[NSCursor unhide];
		_mouseHidden = false;
	}
}

}