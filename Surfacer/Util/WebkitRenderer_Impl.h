//
//  WebkitRenderer_Impl.h
//  Webkit2Texture
//
//  Created by Shamyl Zakariya on 10/5/12.
//
//

#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>
#import <WebKit/WebKit.h>

#import "RichText.h"

@interface WebkitRenderer_Impl : NSObject
{
	NSWindow *_window;
	WebView *_webView;
	core::util::rich_text::RequestRef _request;
	core::util::rich_text::Renderer *_renderer;
}

- (id) initWithSize: (NSSize) offscreenWindowSize;
- (void) renderRequest: (const core::util::rich_text::RequestRef &) request withResourceManager: (core::ResourceManager*) resourceManager;

@property (readonly,nonatomic) WebView* webView;
@property (readwrite,assign) core::util::rich_text::Renderer* renderer;


@end
