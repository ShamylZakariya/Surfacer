#pragma once

//
//  JsonUtils.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/22/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Common.h"
#include "StringLib.h"
#include <cinder/Json.h>

namespace core { namespace util {

class JsonInitializable 
{
	public:
	
		JsonInitializable(){}
		virtual ~JsonInitializable(){}

		// JsonInitializable
		virtual void initialize( const ci::JsonTree &v ){}
};


bool isNull( const ci::JsonTree & );
bool isBoolean( const ci::JsonTree & );
bool isNumeric( const ci::JsonTree & );
bool isString( const ci::JsonTree & );

Vec2i JsonToVec2i( const ci::JsonTree &v );
Vec3i JsonToVec3i( const ci::JsonTree &v );
Vec4i JsonToVec4i( const ci::JsonTree &v );
Vec2r JsonToVec2r( const ci::JsonTree &v );
Vec3r JsonToVec3r( const ci::JsonTree &v );
Vec4r JsonToVec4r( const ci::JsonTree &v );
cpBB JsonToCpBB( const ci::JsonTree &v );
ci::Color JsonToColor( const ci::JsonTree &v );
ci::ColorA JsonToColorA( const ci::JsonTree &v );

bool read( const ci::JsonTree &value, const std::string &name, std::string &into );
bool read( const ci::JsonTree &value, const std::string &name, ci::fs::path &into );
bool read( const ci::JsonTree &value, const std::string &name, bool &into );
bool read( const ci::JsonTree &value, const std::string &name, int &into );
bool read( const ci::JsonTree &value, const std::string &name, unsigned int &into );
bool read( const ci::JsonTree &value, const std::string &name, std::size_t &into );
bool read( const ci::JsonTree &value, const std::string &name, real &into );
bool read( const ci::JsonTree &value, const std::string &name, seconds_t &into );bool read( const ci::JsonTree &value, const std::string &name, Vec2i &into );
bool read( const ci::JsonTree &value, const std::string &name, Vec3i &into );
bool read( const ci::JsonTree &value, const std::string &name, Vec4i &into );
bool read( const ci::JsonTree &value, const std::string &name, Vec2r &into );
bool read( const ci::JsonTree &value, const std::string &name, Vec3r &into );
bool read( const ci::JsonTree &value, const std::string &name, Vec4r &into );
bool read( const ci::JsonTree &value, const std::string &name, cpBB &into );
bool read( const ci::JsonTree &value, const std::string &name, ci::Color &into );
bool read( const ci::JsonTree &value, const std::string &name, ci::ColorA &into );

bool read( const ci::JsonTree &value, const std::string &name, JsonInitializable &into );
bool read( const ci::JsonTree &value, int idx, JsonInitializable &into );

/**
	attempt to read name @a name from Json::value @a value, and if it's there && and convertable to R via T, writes it into @a into.
	If no such value exists, or isn't convertible to the desired type, this will leave the existing value in @a into alone.
*/
template< typename R, typename T >
bool read_conv( const ci::JsonTree &value, const std::string &name, R &into )
{
	T temp;
	if ( read( value, name, temp ))
	{
		into = static_cast<R>(temp);
		return true;
	}
	
	return false;
}


}} // end namespace core::util

// helpful macros
#define JSON_READ( jsv, param ) core::util::read(jsv, #param, param );
#define JSON_CAST_READ( jsv, toType, intermediateType, param ) core::util::read_conv<toType,intermediateType>(jsv, #param, param );

#define JSON_ENUM_READ( jsv, EnumType, param )\
if(core::util::isNumeric(jsv[#param])) { core::util::read_conv<EnumType::type,int>(jsv, #param, param ); }\
else if ( core::util::isString(jsv[#param])) { param = EnumType::fromString( jsv[#param].getValue() ); }\

/**
	declares a default initialize() implementation for JsonInitializable which creates the class instance's init() struct,
	initializes it with the ci::JsonTree, and then calls the object's initialize() method with that initializer.
	
	REQUIRES:
		- Class derives from JsonInitializable
		- Class specifies a struct init{} which derives from JsonInitializable
		- Class specifies a method initialize() which takes the class's init{} struct.
	
*/
#define JSON_INITIALIZABLE_INITIALIZE() virtual void initialize( const ci::JsonTree &v ) { init i; i.initialize(v); this->initialize(i); }

