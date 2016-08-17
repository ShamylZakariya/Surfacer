#pragma once

//
//  ResourceManager.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 11/26/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#include <list>

#include "Common.h"
#include "SvgObject.h"

#include <cinder/Filesystem.h>
#include <cinder/DataSource.h>
#include <cinder/ImageIO.h>
#include <cinder/gl/Texture.h>
#include <cinder/gl/GlslProg.h>


namespace core {

/**
	@class ResourceManager
	
	ResourceManager doles out "expensive" resources like ci::Texture, ci::gl::GlslProg and sounds, 
	keeping a local copy. The idea is that if a single texture is requested multiple times, only
	one copy exists on the GPU an in memory.
	
	ResourceManager instances are a tree. The Scenario owns the "root" ResourceManager, and each
	time a Level is loaded, it creates one, makes it a child of the Scenario's. When a resource is loaded
	by the level's ResourceManager, it will first check if any parent has that resource, if so, that
	one is doled out. If not, the Level's ResourceManager loads it.
	
	When the level is destroyed, the resources that were loaded by it's ResourceManager are destroyed.
*/
class ResourceManager
{
	public:
	
		static void setDefaultTextureFormat( ci::gl::Texture::Format format ) { _defaultTextureFormat = format; }
		static ci::gl::Texture::Format defaultTextureFormat() { return _defaultTextureFormat; }
		
		static void setDefaultShaderExtensions( const std::string &vert, const std::string &frag )
		{
			_defaultVertexShaderExtension = vert;
			_defaultFragmentShaderExtension = frag;
		}

		static std::string defaultVertexShaderExtension() { return _defaultVertexShaderExtension; }
		static std::string defaultFragmentShaderExtension() { return _defaultFragmentShaderExtension; }

	public:
	
		ResourceManager( ResourceManager *parent );
		~ResourceManager();

		/**
			Get the parent ResourceManager of this ResourceManager, or NULL if there is no parent
		*/
		ResourceManager *parent() const { return _parent; }
		
		/**
			Get the root ResourceManager, or this ResourceManager if this ResourceManager is root.
		*/
		ResourceManager *root() const;

		/**
			Get all children ResourceManagers
		*/
		const std::vector< ResourceManager* > &children() const { return _children; }

		/**
			Remove a child resource manager
		*/
		void removeChild( ResourceManager *child );
		
		// Path management
		
		/**
			Add a search path to this ResourceManager.
			When a file path is requested, the search paths will be searched starting with the end of the path list,
			working backwards. 
			
			Child ResourceManagers will inherit their parents' search paths
		*/
		void pushSearchPath( const ci::fs::path &searchPath );

		/**
			Remove a search path
		*/
		void removeSearchPath( const ci::fs::path &searchPath );
		
		/**
			Get this resource manager's search paths.
			Search paths are in order, such that the first is highest priority, going down to least.
			This does NOT include inherited parent search paths.
		*/
		const std::list< ci::fs::path > &searchPaths() const { return _searchPaths; }
		
		/**
			Find an absolute path in the search paths given a path fragment.
			If @a pathFragment is absolute & exists the method will return true and set @a absolutePath to @a pathFragment.
			
			otherwise, if @a pathFragment is a partial path such as "Level10/Foo.svg" findPath will attempt
			to find the absolute path to the file in the search paths, starting with this ResourceManager's paths, and working up
			through parent ResourceManagers' paths.
			
			If a resolveable path is found it will be written into @a absolutePath 
			and true will be returned. Otherwise, false will be returned.			
		*/
		bool findPath( const ci::fs::path &pathFragment, ci::fs::path &absolutePath ) const;

		
		// Resource Loading
		
		/**
			Attempts to find @a fileNameFragment in the searchPaths; if found, returns a DataSourcePathRef with that file.
			If no fitting resource exists, throws ResourceLoadExc
		*/
		ci::DataSourcePathRef loadResource( const ci::fs::path &fileNameFragment ) const;
		
		const std::string &getString( const ci::fs::path &fileNameFragment );
		
		ci::Surface getSurface( const ci::fs::path &fileNameFragment );
		
		/**
			If this ResourceManager or any of its parents have the texture matching @a fileNameFragment in the search paths,
			it will be returned directly. If not, it will be loaded ( if it exists ), stored, and returned.
			
			If the file desribed doesn't exist or cannot be loaded, throw ResourceLoadExc
		*/
		ci::gl::Texture getTexture( const ci::fs::path &fileNameFragment, 
			ci::gl::Texture::Format format = ResourceManager::defaultTextureFormat(),
			ci::ImageSource::Options options = ci::ImageSource::Options() );
			
		/**
			Cache a texture that you've loaded or generated and want to make accessible via an ID.
			Texture's cached this way can be gotten through getCachedTexture
		*/
		void cacheTexture( const ci::gl::Texture &tex, const std::string &texId );
				
		ci::gl::Texture getCachedTexture( const std::string &texId ) const;

		ci::gl::GlslProg getShader( const ci::fs::path &shaderNameFragment );
		ci::gl::GlslProg getShader( const ci::fs::path &vertexFileNameFragment, const ci::fs::path &fragFileNameFragment );
		
		
		/**
			Load an SvgObject from an svg file on disk.
		*/
		SvgObject getSvg( const ci::fs::path &fileNameFragment );
		
	protected:

		/**
			Add a child resource manager
		*/
		void _addChild( ResourceManager *child );
		
		void _generateSearchPaths( std::list< ci::fs::path > &paths ) const;

		typedef ci::fs::path file_resource_key;
		typedef std::pair< ci::fs::path, ci::fs::path > dual_file_resource_key;

		file_resource_key _fileResourceKey( const ci::fs::path &fileNameFragment ) const;
		dual_file_resource_key _fileResourceKey( const ci::fs::path &fileNameFragment0, const ci::fs::path &fileNameFragment1 ) const;

		const std::string &_findText( const file_resource_key &key ) const;
		ci::Surface _findSurface( const file_resource_key &key ) const;
		ci::gl::Texture _findTexture( const file_resource_key &key ) const;
		ci::gl::GlslProg _findShader( const dual_file_resource_key &key ) const;				
		SvgObject _findSvg( const file_resource_key &key ) const;				
		
	private:
	
		ResourceManager *_parent;
		std::vector< ResourceManager* > _children;
		std::list< ci::fs::path > _searchPaths;

		typedef std::map< file_resource_key, std::string > text_cache;
		typedef std::map< file_resource_key, ci::gl::Texture > texture_cache;
		typedef std::map< std::string, ci::gl::Texture > texture_id_cache;
		typedef std::map< dual_file_resource_key, ci::gl::GlslProg > shader_cache;
		typedef std::map< file_resource_key, SvgObject > svg_cache;
		typedef std::map< file_resource_key, ci::Surface > surface_cache;

		text_cache _texts;
		texture_cache _textures;
		texture_id_cache _cachedTextures;
		shader_cache _shaders;
		svg_cache _svgs;
		surface_cache _surfaces;
		
		static ci::gl::Texture::Format _defaultTextureFormat;
		static std::string _defaultVertexShaderExtension, _defaultFragmentShaderExtension;

};


}