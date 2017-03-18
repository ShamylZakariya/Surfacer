//
//  Core.h
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/14/17.
//
//

#ifndef Core_h
#define Core_h

#define CLASS_NAME(ptr)		((typeid(*(ptr))).name())
#define SMART_PTR(cname)	class cname; typedef std::shared_ptr< class cname > cname ## Ref; typedef std::weak_ptr< class cname > cname ## WeakRef;

#include "Exception.hpp"
#include "MathHelpers.hpp"
#include "Strings.hpp"
#include "ChipmunkHelpers.hpp"
#include "Signals.hpp"
#include "TimeState.hpp"
#include "RenderState.hpp"
#include "Viewport.hpp"
#include "ViewportController.hpp"
#include "InputDispatcher.hpp"
#include "SvgParsing.hpp"
#include "LineSegment.hpp"
#include "SpatialIndex.hpp"
#include "StopWatch.hpp"

#endif /* Core_h */
