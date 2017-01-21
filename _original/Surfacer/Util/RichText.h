#pragma once

//
//  Rich Text.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 10/5/12.
//
//


#include <cinder/gl/gl.h>
#include <cinder/gl/Texture.h>
#include <boost/smart_ptr/make_shared.hpp>
#include <queue>

#include "Exception.h"
#include "SignalsAndSlots.h"

namespace core {

	class ResourceManager;

}

namespace core { namespace util { namespace rich_text {

	class RenderException : public core::Exception
	{
		public:
			
			RenderException( const std::string &what ):
				Exception(what)
			{}

			virtual ~RenderException() throw()
			{}
	};
	
	/**
		@class Result
		Rendering requests will return a Result instance, which will at some point
		in the future have a texture which you can render to screen. Or if an error
		occurred, a RenderException will be thrown with a description of the problem.
	*/
	class Result
	{
		public:

			/*
				This is public because of interoperability with ObjC - if I had more time I'd
				make an abstraction which allows this to be private, as it should be.
				Suffice to say, don't touch this directly. Use the methods above to
				get the texture, error, etc.
			*/
		
			struct _result
			{
				ci::gl::Texture texture;
				std::string errorMessage;
			};
			
			boost::shared_ptr< _result > sharedResult;
		
		public:
		
			Result():
				sharedResult( boost::make_shared<_result>() )
			{}
			
			Result( const Result &r ):
				sharedResult( r.sharedResult )
			{}
			
			Result &operator=( const Result &rhs )
			{
				sharedResult = rhs.sharedResult;
				return *this;
			}
				
			/**
				Get the rendered rich text as a texture.
				This will return a null texture until the rendered texture is available.
				If an error occurred during rendering ( say, bad file, whatever ) an
				exception will be thrown, describing the problem.
			*/
			ci::gl::Texture texture() const
			{
				if ( !sharedResult->errorMessage.empty())
				{
					throw RenderException( sharedResult->errorMessage );
				}
				
				return sharedResult->texture;
			}

			std::string errorMessage() const { return sharedResult->errorMessage; }
			bool error() const { return !sharedResult->errorMessage.empty(); }
		
			operator bool() const {
				return sharedResult->texture;
			}

		
	};

	/**
		@class Request
		
		A request represents a request to load and render an HTML file bundled in the app, with the optional
		ability to set the content of that file, set the body class ( think message boxes with "alert", or "warning" styles )
		and the ability to determine the minimum subrect to render to texture.
		
		Your html should have three functions: setBodyClass, setContent, and contentRect, which might look something like so:
		
		@code
function setBodyClass( c )
{
	document.getElementsByTagName('body')[0].className = c;
}

function setContent( c )
{
	document.getElementById('content').innerHTML = c;
}

function contentRect()
{
	var outerFrame = document.getElementById('outerFrame');
	return {
		top: outerFrame.offsetTop,
		left: outerFrame.offsetLeft,
		width: outerFrame.offsetWidth,
		height: outerFrame.offsetHeight
	}
}
		@endcode
		
		And your markup might look something like this:
@code
<body>
	<div id="outerFrame" class="outer">
		<div class="inner" id="content">
			<p>Something</p>
		</div>
	</div>
</body>
@endcode

		The default Request implementation works so long as setBodyClass, setContent and contentRect can be meaningfully implemented for your markup. 
		If you need something fancier, you may want to subclass Request and override Request::_setBodyClass, Request::_setContent and Request::_contentRect.
		
	*/
	class Request
	{
		public:

			/**
				Signal fired when this Request is ready, and the result texture is available
			*/
			signals::signal<void(const Request &)> ready;
	
		public:
		
			Request(
				const std::string &templateHTMLFile,
				const std::string &htmlMessage = std::string(),
				const std::string &bodyClass = std::string() );

			virtual ~Request();
			
			std::string templateHTMLFile() const { return _templateHTMLFile; }
			std::string htmlMessage() const { return _htmlMessage; }
			std::string bodyClass() const { return _bodyClass; }
			
			bool submitted() const { return _submitted; }
			
			const Result &result() const { return _result; }
			Result &result() { return _result; }

			virtual ci::Area apply( void *webViewRaw );

		protected:

			virtual void _setBodyClass( void *webViewRaw, const std::string &bodyClass ) const;

			/*
				@param webViewRaw An ObjC WebView instance, cast as a void pointer so as to not make other C++ code unhappy seeing this header.
				@param htmlMessage The HTML to apply to the template HTML document
			*/
			virtual void _setContent( void *webViewRaw, const std::string &htmlMessage ) const;

			/*
				@param webViewRaw An ObjC WebView instance, cast as a void pointer so as to not make other C++ code unhappy seeing this header.
				@return the bounding rectangle of the content in the HTML document
			*/
			virtual ci::Area _contentRect( void *webViewRaw ) const;
			
		private:

			// disabled, and undefined
			Request();
			Request( const Request &c );
			Request &operator= ( const Request &rhs );

		private:
		
			friend class Renderer;

			std::string _templateHTMLFile, _htmlMessage, _bodyClass;
			Result _result;
			bool _submitted;
			
	};
	
	typedef boost::shared_ptr< Request > RequestRef;
		
	/**
		@class Renderer
		
		You should have ONE singleton-ish renderer, and when you want to render some rich html text call render() passing
		a Request instance. You will get a Result which is like a "future", in that at some point down the road, the rendering
		(which is done asynchronously) will finish, and you will have a ci::gl::Texture instance which you can render.		
	*/
	class Renderer
	{
		public:
		
			Renderer( ResourceManager *rm );
			~Renderer();
	
			void setResourceManager( ResourceManager *rm );
			ResourceManager *resourceManager() const { return _resourceManager; }


			/**
				Render the HTML file specified by Request.
				Returns a Result object which may have a null texture value, but which will at some point
				down the road have a non-null texture value, when the renderer has finished the request.
			*/
			Result render( const RequestRef & );
			
		public:
			
			/**
				Called when a render is complete.
				This has to be public owing to the fact that the backend is all ObjC... don't call it.
			*/
			void _renderFinished( const RequestRef & );

		private:

			void _execute();
			
		private:
		
			void *_renderer; // is WebkitRenderer_Impl ObjC object
			std::queue< RequestRef > _activeRequests;
			ResourceManager *_resourceManager;
	
	};


}}} // end namespace core::util::rich_text
