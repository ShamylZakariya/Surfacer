//
//  WebkitRenderer.cpp
//  Webkit2Texture
//
//  Created by Shamyl Zakariya on 10/5/12.
//
//

#include <cinder/app/AppBasic.h>

#include "RichText.h"
#include "WebkitRenderer_Impl.h"
#include "ResourceManager.h"


using namespace ci;
namespace core { namespace util { namespace rich_text {

	#pragma mark - Request

	/*
			std::string _templateHTMLFile, _htmlMessage, _bodyClass;
			Result _result;
			bool _submitted;
	*/

	Request::Request( const std::string &templateHTMLFile, const std::string &message, const std::string &bodyClass ):
		_templateHTMLFile(templateHTMLFile),
		_htmlMessage(message),
		_bodyClass(bodyClass),
		_submitted(false)
	{}
	
	Request::~Request()
	{}
	
	ci::Area Request::apply( void *webViewRaw )
	{
		if ( !_bodyClass.empty())
		{
			_setBodyClass( webViewRaw, _bodyClass );
		}
	
		if ( !_htmlMessage.empty() )
		{
			_setContent( webViewRaw, _htmlMessage );
		}
		
		return _contentRect( webViewRaw );
	}
	
	void Request::_setBodyClass( void *webViewRaw, const std::string &bodyClass ) const
	{
		WebView *webView = static_cast<WebView*>(webViewRaw);

		NSString *html = [NSString stringWithUTF8String: bodyClass.c_str()];
		[[webView windowScriptObject] callWebScriptMethod: @"setBodyClass" withArguments: @[html]];
	}

	void Request::_setContent( void *webViewRaw, const std::string &message ) const
	{
		WebView *webView = static_cast<WebView*>(webViewRaw);

		NSString *html = [NSString stringWithUTF8String: message.c_str()];
		[[webView windowScriptObject] callWebScriptMethod: @"setContent" withArguments: @[html]];
	}

	ci::Area Request::_contentRect( void *webViewRaw ) const
	{
		WebView *webView = static_cast<WebView*>(webViewRaw);
		id obj = [[webView windowScriptObject] callWebScriptMethod:@"contentRect" withArguments:nil];

		int	x = [[obj valueForKey:@"left"] intValue],
			y = [[obj valueForKey:@"top"] intValue],
			width = [[obj valueForKey:@"width"] intValue],
			height = [[obj valueForKey:@"height"] intValue];

		return ci::Area( x,y,x+width,y+height );
	}

	#pragma mark - Renderer

	/*
			void *_renderer; // is WebkitRenderer_Impl ObjC object
			std::queue< RequestRef > _activeRequests;
			ResourceManager *_resourceManager;
	*/

	Renderer::Renderer( ResourceManager *rm ):
		_resourceManager(rm)
	{
		WebkitRenderer_Impl *renderer = [[WebkitRenderer_Impl alloc] initWithSize:NSMakeSize(1024, 1024)];
		renderer.renderer = this;

		_renderer = renderer;
	}
	
	Renderer::~Renderer()
	{
		WebkitRenderer_Impl *renderer = static_cast<WebkitRenderer_Impl*>(_renderer);
		[renderer release];
	}
	
	void Renderer::setResourceManager( core::ResourceManager *rm )
	{
		_resourceManager = rm;
		if ( _resourceManager && !_activeRequests.empty() )
		{
			_execute();
		}
	}

	Result Renderer::render( const RequestRef &r )
	{
		//
		//	If the queue is empty, we want to kick off rendering immediately via _execute
		//	(which pops the queue and executes that request). Otherwise, this will get run
		//	after the last queued request is finished.
		//

		const bool shouldExecute = _activeRequests.empty();
		_activeRequests.push(r);

		if ( resourceManager() && shouldExecute )
		{
			_execute();
		}

		return r->result();
	}

	void Renderer::_renderFinished( const RequestRef &r )
	{
		assert( r == _activeRequests.front());
		_activeRequests.pop();

		// fire Result's ready signal
		r->ready( *r );
		
		//
		//	If requests came in while we were rendering, run the first in queue
		//

		if ( resourceManager() && !_activeRequests.empty())
		{
			_execute();
		}
	}
	
	void Renderer::_execute()
	{
		//
		//	Execute the request first in line
		//

		WebkitRenderer_Impl *renderer = static_cast<WebkitRenderer_Impl*>(_renderer);
		RequestRef &request = _activeRequests.front();

		request->_submitted = true;
		[renderer renderRequest: request withResourceManager: _resourceManager];
	}


}}} // end namespace core::util::rich_text
