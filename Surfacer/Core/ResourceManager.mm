//
//  ResourceManager.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 11/26/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#include "ResourceManager.h"

#include <cinder/app/AppBasic.h>

using namespace ci;
namespace core {

namespace {

	const std::string EMPTY_STRING = std::string();

	std::string buffer_to_string( const Buffer &buffer )
	{
		// what a pain in the ass

		boost::shared_ptr<char> sourceBlock( new char[buffer.getDataSize() + 1], checked_array_deleter<char>() );
		memcpy( sourceBlock.get(), buffer.getData(), buffer.getDataSize() );
		sourceBlock.get()[buffer.getDataSize()] = 0; // null terminate

		return std::string( sourceBlock.get() );		
	}

	// sanitize a path input - makes it an absolute path to a folder, returns false if the destination doesn't exist
	bool folderize( fs::path path, fs::path &folderPath ) 
	{
		if ( path.has_filename() && !fs::is_directory( path ))
		{
			path = path.remove_filename();
		}
		
		if ( fs::exists( path ) && fs::is_directory( path ))
		{
			folderPath = fs::absolute( path );
			return true;
		}
		
		return false;
	}
	
	//
	//	Adapted from here: http://cocoawithlove.com/2010/05/finding-or-creating-application-support.html
	//

	NSString *FindOrCreateDirectory( 
		NSSearchPathDirectory searchPathDirectory,
		NSSearchPathDomainMask domainMask,
		NSString *appendComponent,
		NSError **errorOut )
	{
		// Search for the path
		NSArray* paths = NSSearchPathForDirectoriesInDomains( searchPathDirectory, domainMask, YES );

		if ([paths count] == 0)
		{
			// *** creation and return of error object omitted for space
			return nil;
		}
		
		// Normally only need the first path
		NSString *resolvedPath = [paths objectAtIndex:0];
		
		if (appendComponent)
		{
			resolvedPath = [resolvedPath stringByAppendingPathComponent:appendComponent];
		}
		
		// Create the path if it doesn't exist
		NSError *error;
		BOOL success = [[NSFileManager defaultManager]
						createDirectoryAtPath:resolvedPath
						withIntermediateDirectories:YES
						attributes:nil
						error:&error];
		if (!success) 
		{
			if (errorOut)
			{
				*errorOut = error;
			}
			return nil;
		}
		
		// If we've made it this far, we have a success
		if (errorOut)
		{
			*errorOut = nil;
		}

		return resolvedPath;
	}	
	
	NSString * ApplicationSupportDirectory()
	{
		NSString *executableName =
			[[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleExecutable"];

		NSError *error;
		NSString *result = FindOrCreateDirectory( NSApplicationSupportDirectory, NSUserDomainMask, executableName, &error );

		if (error)
		{
			NSLog(@"Unable to find or create application support directory:\n%@", error);
		}

		return result;
	}

}

/*
		ResourceManager *_parent;
		std::vector< ResourceManager* > _children;
		std::list< fs::path > _searchPaths;

		typedef std::map< file_resource_key, gl::Texture > texture_cache;
		typedef std::map< std::string, gl::Texture > texture_id_cache;
		typedef std::map< dual_file_resource_key, gl::GlslProg > shader_cache;
		typedef std::map< file_resource_key, SvgObject > svg_cache;
		typedef std::map< file_resource_key, Surface > surface_cache;

		texture_cache _textures;
		texture_id_cache _cachedTextures;
		shader_cache _shaders;
		svg_cache _svgs;
		surface_cache _surfaces;
		
		static gl::Texture::Format _defaultTextureFormat;
		static std::string _defaultVertexShaderExtension, _defaultFragmentShaderExtension;
*/

gl::Texture::Format ResourceManager::_defaultTextureFormat;
std::string ResourceManager::_defaultVertexShaderExtension = "vert";
std::string ResourceManager::_defaultFragmentShaderExtension = "frag";

ResourceManager::ResourceManager( ResourceManager *parent ):
	_parent(parent)
{
	if ( _parent )
	{
		_parent->_addChild( this );
	}
	else
	{
		//
		//	Automatically configure the two base search paths:
		//		- the user's ~/Library/Application Support/AppName folder
		//		- the app's Resources/ folder
		//	TIME FOR SOME OBJC!
		//
		
		NSString *appSupport = ApplicationSupportDirectory();
		if ( appSupport )
		{
			pushSearchPath( fs::path( [appSupport cStringUsingEncoding:NSUTF8StringEncoding] ));
		}

		pushSearchPath( fs::path( [[[NSBundle mainBundle] resourcePath] cStringUsingEncoding: NSUTF8StringEncoding] ));
	}
}

ResourceManager::~ResourceManager()
{
	foreach( ResourceManager *r, _children )
	{
		delete r;
	}	
}
		
ResourceManager *ResourceManager::root() const
{
	ResourceManager *r = const_cast<ResourceManager*>(this);
	while( r->parent() )
	{
		r = r->parent();
	}
	
	return r;
}

void ResourceManager::removeChild( ResourceManager *child )
{
	if ( child->parent() == this )
	{
		_children.erase( std::remove( _children.begin(), _children.end(), child ), _children.end());
		child->_parent = NULL;
	}
}

#pragma mark - Search Path Management

void ResourceManager::pushSearchPath( const fs::path &searchPath )
{
	fs::path folder;
	if ( folderize( searchPath, folder ))
	{
		_searchPaths.push_front( folder );
		app::console() << "ResourceManager::pushSearchPath - added: " << folder << std::endl;
	}
	else
	{
		app::console() << "ResourceManager::pushSearchPath - Unable to folderize: \"" << searchPath << "\"" << std::endl;
	}
}

void ResourceManager::removeSearchPath( const fs::path &searchPath )
{
	fs::path folder;
	if ( folderize( searchPath, folder ))
	{
		_searchPaths.erase( std::remove( _searchPaths.begin(), _searchPaths.end(), folder ), _searchPaths.end());
	}
}

bool ResourceManager::findPath( const fs::path &pathFragment, fs::path &absolutePath ) const
{
	if ( pathFragment.is_absolute() && fs::exists( pathFragment ))
	{
		absolutePath = pathFragment;
		return true;
	}
	
	std::list< fs::path > paths;
	_generateSearchPaths( paths );
	
	//
	//	Now, for each path, append fileNameFragment and if it exists, we're good to go
	//
	
	foreach( fs::path &path, paths )
	{
		fs::path testPath = path / pathFragment;
		if ( fs::exists( testPath ))
		{
			absolutePath = testPath;
			return true;
		}
	}	
	
	//
	//	Found nothing!
	//
	
	return false;
}

DataSourcePathRef ResourceManager::loadResource( const fs::path &fileNameFragment ) const
{
	fs::path fullPath;
	if ( !findPath( fileNameFragment, fullPath ))
	{
		throw app::ResourceLoadExc( fileNameFragment.string() );
	}

	return DataSourcePath::create( fullPath.string() );	
}

const std::string &ResourceManager::getString( const fs::path &fileNameFragment )
{
	file_resource_key key = _fileResourceKey( fileNameFragment );

	//
	//	See if self, or any parents have this text.
	//

	ResourceManager *rm = this;
	while ( rm )
	{
		const std::string &text( rm->_findText( key ) );
		if ( !text.empty() ) return text;
	
		rm = rm->parent();
	}

	DataSourceRef resource = loadResource(fileNameFragment);
	if ( resource && resource->getBuffer() )
	{
		_texts[key] = buffer_to_string(resource->getBuffer());
		return _texts[key];
	}
	
	return EMPTY_STRING;
}

Surface ResourceManager::getSurface( const fs::path &fileNameFragment )
{
	file_resource_key key = _fileResourceKey( fileNameFragment );

	//
	//	See if self, or any parents have this surface.
	//

	ResourceManager *rm = this;
	Surface surface;
	while ( rm && !surface )
	{
		surface = rm->_findSurface( key );
		rm = rm->parent();
	}

	//
	//	Looks like we don't have it, so, load & store it
	//

	if ( !surface )
	{
		surface = Surface( loadImage( loadResource( fs::path(key) )));
		_surfaces[key] = surface;
	}
	
	return surface;
}

gl::Texture ResourceManager::getTexture( 
	const fs::path &fileNameFragment, 
	gl::Texture::Format format,
	ImageSource::Options options )
{
	file_resource_key key = _fileResourceKey( fileNameFragment );

	//
	//	See if self, or any parents have this texture.
	//

	ResourceManager *rm = this;
	gl::Texture texture;
	while ( rm && !texture )
	{
		texture = rm->_findTexture( key );
		rm = rm->parent();
	}

	//
	//	Looks like we don't have it, so, load & store it
	//

	if ( !texture )
	{
		texture = gl::Texture( loadImage( loadResource( fs::path(key) ), options ), format );
		_textures[key] = texture;
	}
	
	return texture;
}

void ResourceManager::cacheTexture( const gl::Texture &tex, const std::string &texId )
{
	_cachedTextures[texId] = tex;
}

gl::Texture ResourceManager::getCachedTexture( const std::string &texId ) const
{
	texture_id_cache::const_iterator pos( _cachedTextures.find(texId));
	return pos != _cachedTextures.end() ? pos->second : gl::Texture();
}

gl::GlslProg ResourceManager::getShader( const fs::path &shaderNameFragment )
{
	return getShader( 
		fs::path( shaderNameFragment.string() + "." + _defaultVertexShaderExtension ),
		fs::path( shaderNameFragment.string() + "." + _defaultFragmentShaderExtension )
	);
}

gl::GlslProg ResourceManager::getShader( const fs::path &vertexFileNameFragment, const fs::path &fragFileNameFragment )
{
	dual_file_resource_key key = _fileResourceKey( vertexFileNameFragment, fragFileNameFragment );
	
	//
	//	See if self, or any parents have this shader.
	//

	ResourceManager *rm = this;
	gl::GlslProg shader;
	while ( rm && !shader )
	{
		shader = rm->_findShader( key );
		rm = rm->parent();
	}

	//
	//	Looks like we don't have it, so, load & store it
	//

	if ( !shader )
	{
		try 
		{
			shader = gl::GlslProg( loadResource(fs::path(key.first)), loadResource(fs::path(key.second)) );
			_shaders[key] = shader;
		}
		catch ( const gl::GlslProgCompileExc &e )
		{
			app::console() << "[" << vertexFileNameFragment.filename() << ", " 
				<< fragFileNameFragment.filename() << "]\n\t" << e.what() << std::endl;
		}
		
	}
	
	return shader;
}

SvgObject ResourceManager::getSvg( const fs::path &fileNameFragment )
{
	file_resource_key key = _fileResourceKey( fileNameFragment );

	//
	//	See if self, or any parents have this texture.
	//

	ResourceManager *rm = this;
	SvgObject svg;
	while ( rm && !svg )
	{
		svg = rm->_findSvg( key );
		rm = rm->parent();
	}

	//
	//	Looks like we don't have it, so, load & store it
	//

	if ( !svg )
	{
		svg = SvgObject( loadResource( key ), 1 );
		_svgs[key] = svg;
	}
	
	return svg;
}


#pragma mark - Protected

void ResourceManager::_addChild( ResourceManager *child )
{
	if ( child->parent() )
	{
		child->parent()->removeChild( child );
	}

	child->_parent = this;
	_children.push_back( child );		
}

void ResourceManager::_generateSearchPaths( std::list< fs::path > &paths ) const
{
	paths = _searchPaths;
	
	ResourceManager *r = parent();
	while( r )
	{
		foreach( const fs::path &p, r->searchPaths() )
		{
			paths.push_back( p );
		}
		
		r = r->parent();
	}	
}

ResourceManager::file_resource_key ResourceManager::_fileResourceKey( const fs::path &fileNameFragment ) const
{
	fs::path fullFilePath;
	if ( findPath( fileNameFragment, fullFilePath ))
	{
		return fullFilePath;
	}
	
	return file_resource_key();
}

ResourceManager::dual_file_resource_key ResourceManager::_fileResourceKey( const fs::path &fileNameFragment0, const fs::path &fileNameFragment1 ) const
{
	fs::path fullFilePath0, fullFilePath1;
	if ( findPath( fileNameFragment0, fullFilePath0 ) && findPath( fileNameFragment1, fullFilePath1 ))
	{
		return dual_file_resource_key( fullFilePath0, fullFilePath1 );
	}

	return dual_file_resource_key();
}

const std::string &ResourceManager::_findText( const file_resource_key &key ) const
{
	if ( !key.empty() )
	{
		text_cache::const_iterator pos = _texts.find( key );
		if ( pos != _texts.end() )
		{
			return pos->second;
		}
	}
	
	return EMPTY_STRING;
}


Surface ResourceManager::_findSurface( const file_resource_key &key ) const
{
	if ( !key.empty() )
	{
		surface_cache::const_iterator pos = _surfaces.find( key );
		if ( pos != _surfaces.end() )
		{
			return pos->second;
		}
	}
	
	return Surface();
}

gl::Texture ResourceManager::_findTexture( const file_resource_key &key ) const
{
	if ( !key.empty() )
	{
		texture_cache::const_iterator pos = _textures.find( key );
		if ( pos != _textures.end() )
		{
			return pos->second;
		}
	}
	
	return gl::Texture();
}

gl::GlslProg ResourceManager::_findShader( const dual_file_resource_key &key ) const
{
	if ( !key.first.empty() && !key.second.empty() )
	{
		shader_cache::const_iterator pos = _shaders.find( key );
		if ( pos != _shaders.end() )
		{
			return pos->second;
		}
	}
	
	return gl::GlslProg();
}

SvgObject ResourceManager::_findSvg( const file_resource_key &key ) const
{
	if ( !key.empty() )
	{
		svg_cache::const_iterator pos = _svgs.find( key );
		if ( pos != _svgs.end() )
		{
			return pos->second;
		}
	}
	
	return SvgObject();
}


}