//
//  WebkitRenderer_Impl.m
//  Webkit2Texture
//
//  Created by Shamyl Zakariya on 10/5/12.
//
//

#import "WebkitRenderer_Impl.h"
#import "ResourceManager.h"

@implementation WebkitRenderer_Impl

@synthesize webView = _webView;
@synthesize renderer = _renderer;

- (id) initWithSize: (NSSize) offscreenWindowSize;
{
	if ((self = [super init]))
	{
		NSRect windowFrame = NSMakeRect(0, 0, offscreenWindowSize.width, offscreenWindowSize.height);
		_window = [[NSWindow alloc] initWithContentRect: windowFrame styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:NO];
		_window.backgroundColor = [NSColor colorWithDeviceWhite:0 alpha:0];

		_webView = [[WebView alloc] initWithFrame: windowFrame ];
		_webView.drawsBackground = NO;
		_webView.shouldUpdateWhileOffscreen = YES;
		_webView.mainFrame.frameView.allowsScrolling = NO;

		_window.contentView = _webView;
		_webView.frameLoadDelegate = self;
	}
	
	return self;
}

- (void) dealloc
{
	[_webView release];
	[_window release];
	[super dealloc];
}

- (void) renderRequest: (const core::util::rich_text::RequestRef &) request withResourceManager: (core::ResourceManager*) resourceManager
{
	_request = request;
	if ( _request )
	{
		NSURL *templateURL = [[NSBundle mainBundle]
			URLForResource: [NSString stringWithUTF8String: _request->templateHTMLFile().c_str()]
			withExtension:@"html"];

		//
		//	If we have a resource manager, and NSBundle couldn't find the template, use resource manager
		//
		if ( resourceManager && !templateURL )
		{
			ci::fs::path pathToTemplate;
			if(resourceManager->findPath( _request->templateHTMLFile() + ".html", pathToTemplate ))
			{
				templateURL = [NSURL fileURLWithPath: [NSString stringWithUTF8String: pathToTemplate.c_str()]];
			}
		}


		if ( templateURL )
		{
			NSURLRequest *req = [NSURLRequest requestWithURL: templateURL ];
			[_webView.mainFrame loadRequest: req];
		}
		else
		{
			_request->result().sharedResult->errorMessage =
				std::string("Unable to find template HTML file ") + _request->templateHTMLFile();
		}
	}
}

#pragma mark WebView FrameLoadDelegate

- (void)webView:(WebView *)sender didFailLoadWithError:(NSError *)error forFrame:(WebFrame *)frame
{
	NSLog( @"webView: %@ didFailWithError: %@", sender, error );

	if ( _request )
	{
		_request->result().sharedResult->errorMessage = [[error localizedDescription] UTF8String];
	}
}

- (void)webView:(WebView *)sender didFailProvisionalLoadWithError:(NSError *)error forFrame:(WebFrame *)frame
{
	NSLog( @"webView: %@ didFailProvisionalLoadWithError: %@", sender, error );

	if ( _request )
	{
		_request->result().sharedResult->errorMessage = [[error localizedDescription] UTF8String];
	}
}

- (void)webView:(WebView *)sender didFinishLoadForFrame:(WebFrame *)frame
{
	if ( _request )
	{
		//
		//	Now we're going to let the requestor set the message, and pull out the minimum rect the content
		//
		
		const ci::Area
			ContentArea = _request->apply( _webView );

		const int
			Width = ContentArea.getWidth(),
			Height = ContentArea.getHeight();

		
		//
		//	Copy out image from just the minimum height area - note contentArea is in HTML's native top-down coordinate system,
		//	but the rectangle we're rendering into needs to be in Quartz's bottom-up coordinate system.
		//

		NSRect viewBounds = [_webView bounds];
		NSRect subRect = NSMakeRect(
			ContentArea.getX1(),
			viewBounds.size.height - ContentArea.getY1() - Height,
			Width,
			Height);

		NSBitmapImageRep *rep = [[NSBitmapImageRep alloc]
			initWithBitmapDataPlanes: NULL
			pixelsWide: Width
			pixelsHigh: Height
			bitsPerSample: 8
			samplesPerPixel: 4
			hasAlpha: YES
			isPlanar: NO
			colorSpaceName: NSDeviceRGBColorSpace
			bitmapFormat: 0
			bytesPerRow: Width * 4
			bitsPerPixel: 32];

		[_webView cacheDisplayInRect: subRect toBitmapImageRep: rep];
		
		//
		//	Now, create a texture from the captured image, and pass it to the shared result
		//
		
		{
			ci::gl::Texture::Format textureFormat;
			textureFormat.setTargetRect();
			
			_request->result().sharedResult->texture = ci::gl::Texture(
				[rep bitmapData],
				GL_RGBA,
				ContentArea.getWidth(),
				ContentArea.getHeight(),
				textureFormat
			);
			
			[rep release];
		}
		
		//
		//	We're done
		//

		core::util::rich_text::RequestRef finishedRequest = _request;
		_request.reset();

		_renderer->_renderFinished( finishedRequest );
	}
}


@end
