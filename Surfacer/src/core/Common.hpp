//
//  Common.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/27/17.
//
//

#ifndef Common_h
#define Common_h

using namespace std;

#define CLASS_NAME(ptr)		((typeid(*(ptr))).name())
#define SMART_PTR(cname)	class cname; typedef std::shared_ptr< class cname > cname ## Ref; typedef std::weak_ptr< class cname > cname ## WeakRef;

#endif /* Common_h */
