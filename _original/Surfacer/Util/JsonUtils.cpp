//
//  JsonUtils.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/22/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "JsonUtils.h"
#include "SvgParsing.h"

using namespace ci;
namespace core { namespace util {

bool isNull( const ci::JsonTree &js )
{
	if ( js.hasChildren() ) return false;
	return js.getValue().empty();
}

bool isBoolean( const ci::JsonTree &js )
{
	if ( js.hasChildren() ) return false;

	try
	{
		js.getValue<bool>();
		return true;
	}
	catch( const JsonTree::ExcNonConvertible & )
	{}
}

bool isNumeric( const JsonTree &js )
{
	if ( js.hasChildren() ) return false;

	try
	{
		js.getValue<real>();
		return true;
	}
	catch( const JsonTree::ExcNonConvertible & )
	{}
	
	return false;
}

bool isString( const JsonTree &js )
{
	if ( js.hasChildren() ) return false;
	return !isNumeric(js);
}

#pragma mark -

Vec2i JsonToVec2i( const JsonTree &v )
{
	const JsonTree 
		X = v["x"],
		Y = v["y"];

	return Vec2i( X.getValue<int>(), Y.getValue<int>());
}

Vec3i JsonToVec3i( const JsonTree &v )
{
	const JsonTree 
		X = v["x"],
		Y = v["y"],
		Z = v["z"];

	return Vec3i( X.getValue<int>(), Y.getValue<int>(), Z.getValue<int>());
}

Vec4i JsonToVec4i( const JsonTree &v )
{
	const JsonTree 
		X = v["x"],
		Y = v["y"],
		Z = v["z"],
		W = v["w"];

	return Vec4i( X.getValue<int>(), Y.getValue<int>(), Z.getValue<int>(), W.getValue<int>());
}

Vec2 JsonToVec2( const JsonTree &v )
{
	const JsonTree 
		X = v["x"],
		Y = v["y"];
		
	return Vec2( X.getValue<real>(), Y.getValue<real>());
}

Vec3 JsonToVec3( const JsonTree &v )
{
	const JsonTree 
		X = v["x"],
		Y = v["y"],
		Z = v["z"];
		
	return Vec3( X.getValue<real>(), Y.getValue<real>(), Z.getValue<real>());
}

Vec4 JsonToVec4( const JsonTree &v )
{
	const JsonTree 
		X = v["x"],
		Y = v["y"],
		Z = v["z"],
		W = v["w"];

	return Vec4( X.getValue<real>(), Y.getValue<real>(), Z.getValue<real>(), W.getValue<real>());
}

cpBB JsonToCpBB( const JsonTree &v )
{
	const JsonTree 
		L = v["l"],
		B = v["b"],
		R = v["r"],
		T = v["t"];

	return cpBBNew( L.getValue<real>(), B.getValue<real>(), R.getValue<real>(), T.getValue<real>() );
}

Color JsonToColor( const JsonTree &v )
{
	if ( v.hasChildren() )
	{
		const JsonTree 
			R = v["l"],
			G = v["b"],
			B = v["r"];

		return Color( R.getValue<real>(), G.getValue<real>(), B.getValue<real>());
	}
	else
	{
		Color color;
		if ( svg::parseColor( v.getValue(), color ) )
		{
			return color;
		}
	}

	throw JsonTree::ExcNonConvertible(v);
	return Color();
}

ColorA JsonToColorA( const JsonTree &v )
{
	if ( v.hasChildren() )
	{
		const JsonTree 
			R = v["l"],
			G = v["b"],
			B = v["r"],
			A = v["t"];
			
		return ColorA( R.getValue<real>(), G.getValue<real>(), B.getValue<real>(), A.getValue<real>());
	}
	else
	{
		ColorA color;
		if ( svg::parseColor( v.getValue(), color ) )
		{
			return color;
		}
	}

	throw JsonTree::ExcNonConvertible(v);
	return Color();
}

#pragma mark - read

namespace {


	template< typename T >
	bool _read( const ci::JsonTree &value, const std::string &name, T &into )
	{
		try
		{
			into = value[name].getValue<T>();
			return true;
		}
		catch( const std::exception & )
		{}
			
		return false;
	}


}

bool read( const JsonTree &value, const std::string &name, std::string &into )
{
	try
	{
		into = value[name].getValue();
		return true;
	}
	catch( const std::exception & )
	{}
	
	return false;
}

bool read( const JsonTree &value, const std::string &name, fs::path &into )
{
	try
	{
		into = value[name].getValue();
		return true;
	}
	catch( const std::exception & )
	{}
	
	return false;
}

bool read( const ci::JsonTree &value, const std::string &name, bool &into )
{
	return _read( value, name, into );
}

bool read( const ci::JsonTree &value, const std::string &name, int &into )
{
	return _read( value, name, into );
}

bool read( const ci::JsonTree &value, const std::string &name, unsigned int &into )
{
	return _read( value, name, into );
}

bool read( const ci::JsonTree &value, const std::string &name, std::size_t &into )
{
	return _read( value, name, into );
}

bool read( const ci::JsonTree &value, const std::string &name, real &into )
{
	return _read( value, name, into );
}

bool read( const ci::JsonTree &value, const std::string &name, seconds_t &into )
{
	return _read( value, name, into );
}

bool read( const JsonTree &value, const std::string &name, Vec2i &into )
{
	try
	{
		into = JsonToVec2i(value[name]);
		return true;
	}
	catch( const std::exception & )
	{}
	
	return false;
}

bool read( const JsonTree &value, const std::string &name, Vec3i &into )
{
	try
	{
		into = JsonToVec3i(value[name]);
		return true;
	}
	catch( const std::exception & )
	{}
	
	return false;
}

bool read( const JsonTree &value, const std::string &name, Vec4i &into )
{
	try
	{
		into = JsonToVec4i(value[name]);
		return true;
	}
	catch( const std::exception & )
	{}
	
	return false;
}

bool read( const JsonTree &value, const std::string &name, Vec2 &into )
{
	try
	{
		into = JsonToVec2(value[name]);
		return true;
	}
	catch( const std::exception & )
	{}
	
	return false;
}

bool read( const JsonTree &value, const std::string &name, Vec3 &into )
{
	try
	{
		into = JsonToVec3(value[name]);
		return true;
	}
	catch( const std::exception & )
	{}
	
	return false;
}

bool read( const JsonTree &value, const std::string &name, Vec4 &into )
{
	try
	{
		into = JsonToVec4(value[name]);
		return true;
	}
	catch( const std::exception & )
	{}
	
	return false;
}

bool read( const JsonTree &value, const std::string &name, cpBB &into )
{
	try
	{
		into = JsonToCpBB(value[name]);
		return true;
	}
	catch( const std::exception & )
	{}
	
	return false;
}

bool read( const JsonTree &value, const std::string &name, Color &into )
{
	try
	{
		into = JsonToColor(value[name]);
		return true;
	}
	catch( const std::exception & )
	{}
	
	return false;
}

bool read( const JsonTree &value, const std::string &name, ColorA &into )
{
	try
	{
		into = JsonToColorA(value[name]);
		return true;
	}
	catch( const std::exception & )
	{}
	
	return false;
}


bool read( const ci::JsonTree &value, const std::string &name, JsonInitializable &into )
{
	try
	{
		into.initialize(value[name]);
		return true;
	}
	catch( const std::exception & )
	{}
	
	return false;
}


bool read( const ci::JsonTree &value, int idx, JsonInitializable &into )
{
	try
	{
		into.initialize(value[idx]);
		return true;
	}
	catch( const std::exception & )
	{}
	
	return false;
}


}}
