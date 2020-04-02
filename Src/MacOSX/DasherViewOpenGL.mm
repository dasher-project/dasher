//
//  DasherViewOpenGL.mm
//  MacOSX
//
//  Created by Doug Dickinson on Fri Apr 18 2003.
//  Copyright (c) 2003 Doug Dickinson (dasher AT DressTheMonkey DOT plus DOT com). All rights reserved.
//

#import "DasherViewOpenGL.h"
#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

#import "DasherApp.h"

#import <sys/time.h>

#import "DasherUtil.h"
#import "DasherApp.h"
#import "GLUtils.h"

#import "OpenGLScreen.h"

@interface DasherViewOpenGL (private_methods)
-(void)sendMarker:(int)iMarker;
@end

class COSXDasherScreen : public OpenGLScreen {
  DasherViewOpenGL *dasherView;
public:
  COSXDasherScreen(DasherViewOpenGL *_dasherView,screenint iWidth, screenint iHeight, GLfloat tc_x, GLfloat tc_y, GLuint *textures)
  : OpenGLScreen(iWidth, iHeight, iWidth, iHeight,tc_x,tc_y,textures), dasherView(_dasherView) {
    RegenerateLabels(); //no actual labels, so just initialize fontAttrs
  }
    
  void SendMarker(int iMarker) {
    [dasherView sendMarker:iMarker];    
  }
  
  void Display() {
    OpenGLScreen::Display();
    [[dasherView openGLContext] flushBuffer];
  }
  
  void resize(screenint w, screenint h,GLfloat tc_x,GLfloat tc_y) {
    OpenGLScreen::resize(w,h, w,h, tc_x,tc_y);
  }
  
  ///Override just to make callable from DasherViewOpenGL
  void RegenerateLabels() {
    OpenGLScreen::RegenerateLabels();
  }
  bool IsWindowUnderCursor() override { return YES; }
protected:
  void RenderStringOntoCGContext(NSString *string, CGContextRef context, unsigned int iFontSize, bool bWrap) {
    NSGraphicsContext *old = [NSGraphicsContext currentContext];
    [NSGraphicsContext setCurrentContext:[NSGraphicsContext graphicsContextWithGraphicsPort:context flipped:YES]];

    //white text on (default) transparent background means that when we texture
    //a surface using a colour, the text appears in that colour...
    NSDictionary *attrs = [NSDictionary dictionaryWithObjectsAndKeys:[NSFont fontWithName:dasherView.cachedFontName size:iFontSize], NSFontAttributeName, [NSColor whiteColor], NSForegroundColorAttributeName, nil];
    //dictionaryWith...: does an autorelease - only "alloc" methods do not.
    
    if (bWrap)
      [string drawWithRect:NSMakeRect(0.0, 0.0, GetWidth(), CGFLOAT_MAX) options:NSStringDrawingUsesLineFragmentOrigin attributes:attrs];
    else
      [string drawAtPoint:NSMakePoint(0.0, 0.0) withAttributes:attrs];
    [NSGraphicsContext setCurrentContext:old];  
  }
  
  CGSize TextSize(NSString *str, unsigned int iFontSize, bool bWrap) {
    NSFont *font=[NSFont fontWithName:dasherView.cachedFontName size:iFontSize];
    NSDictionary *attrs =[NSDictionary dictionaryWithObject:(font ? font : [NSFont systemFontOfSize:iFontSize]) forKey:NSFontAttributeName];
    return NSSizeToCGSize(bWrap ? ([str boundingRectWithSize:NSMakeSize(GetWidth(), CGFLOAT_MAX) options:NSStringDrawingUsesLineFragmentOrigin attributes:attrs].size) : ([str sizeWithAttributes:attrs]));
  }
};

@implementation DasherViewOpenGL
@synthesize cachedFontName;

/*
 BEWARE!
 This is doing funny stuff with OpenGL framebuffers - rendering the boxes and the decorations to separate buffers, then compositing the two together.  The buffer switching is in sendMarker:
 */

- (void)sendMarker:(int)iMarker {
  [[self openGLContext] makeCurrentContext];
  glFlush();
  if (iMarker == -1) {
	  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
  } else {
	  //NSLog(@"SendMarker %i\n",iMarker);
	  glDisable(GL_TEXTURE_2D);
	  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, frameBuffers[iMarker]);
	  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, textures[iMarker], 0);
  }
  //glMatrixMode(GL_PROJECTION); glLoadIdentity();
  //glOrtho(0, [self boundsWidth], [self boundsHeight], 0, -1.0, 1.0);
  //glMatrixMode(GL_MODELVIEW); glLoadIdentity();
  glClearColor(1.0, 1.0, 1.0, iMarker == 1 ? 0.0 /*transparent*/ : 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
}

- (void)drawRect:(NSRect)rect {
  //if (![dasherApp aquaDasherControl]->GetBoolParameter(BP_DASHER_PAUSED)) {
    // (ACL) want to redraw even if paused, to get mouse line etc. So invoke
    // standard Dasher redraw procedure (but not too early or it'll crash!)
  if ([dasherApp timer]) {	
    [dasherApp aquaDasherControl]->goddamn(get_time(), false); //E
    
//  [[self openGLContext] flushBuffer]; // (ACL) done in displayCallback
  }
  
}

- (void)mouseEntered:(NSEvent *)theEvent
{
}

- (void)mouseExited:(NSEvent *)theEvent
{
}

- (void)mouseDown:(NSEvent *)e {
  [dasherApp aquaDasherControl]->KeyDown(get_time(), 100+[e buttonNumber]);
}

- (void)mouseUp:(NSEvent *)e {
  [dasherApp aquaDasherControl]->KeyUp(get_time(), 100+[e buttonNumber]);
}

///The following gets called on MacOS 10.6 or later, only, for multitouch.
/// (It won't work on earlier versions, but won't get called, either)
- (void)touchesBeganWithEvent:(NSEvent *)event {
  int totalTouches = [[event touchesMatchingPhase:NSUIntegerMax inView:self] count];
  int newTouches = [[event touchesMatchingPhase:1 inView:self] count];
  //Call KeyDown with id 101, 102, ... for successive new touches _except_the_first_
  // (the first finger, id 100, is handled by MouseDown...)
  for (int i=max(1,totalTouches-newTouches); i<totalTouches; i++)
    [dasherApp aquaDasherControl]->KeyDown(get_time(), 100+i);
}

///The following gets called on MacOS 10.6 or later, only, for multitouch.
/// (It won't work on earlier versions, but won't get called, either)
-(void)touchesEndedWithEvent:(NSEvent *)event {
  int totalTouches = [[event touchesMatchingPhase:NSUIntegerMax inView:self] count];
  //note hardcoding of constant 24 = NSTouchPhaseEnded | NSTouchPhaseCancelled
  // (allows compilation on pre-10.6 systems)
  int endingTouches= [[event touchesMatchingPhase:24 inView:self] count];
  //Call KeyUp with ids 101, 102, ..., highest first, as fingers are lifted from
  // the touchpad. Not we don't do anything for the last finger to be lifted,
  // as this is dealt with by MouseUp with id==100.
  for (int i=totalTouches; i-->max(1,totalTouches-endingTouches);)
    [dasherApp aquaDasherControl]->KeyUp(get_time(), 100+i);
}

-(void)touchesCancelledWithEvent:(NSEvent *)event {
  [self touchesEndedWithEvent:event];
}

- (void)keyDown:(NSEvent *)e {
  [dasherApp handleKeyDown:e];
}

- (void)keyUp:(NSEvent *)e {
  [dasherApp handleKeyUp:e];
}

- (id)initWithFrame:(NSRect)aFrame {
  NSOpenGLPixelFormatAttribute attrs[] =
  {
    NSOpenGLPFADoubleBuffer,
//    NSOpenGLPFADepthSize, (NSOpenGLPixelFormatAttribute)32,
    (NSOpenGLPixelFormatAttribute)0
  };
  
  NSOpenGLPixelFormat* pixFmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
  
  /* Check if initWithAttributes succeeded. */
//  if(pixFmt == nil) {
    pixFmt = [[self class] defaultPixelFormat];
//  }
  
  if (self = [super initWithFrame:aFrame pixelFormat:pixFmt]) {
        // init open gl
    [self gl_init];
    [self setFrameSize:aFrame.size];
    //note these give us framebuffer _references_...
    glGenFramebuffersEXT(2, frameBuffers);
    glGenTextures(2, textures);
    //...i.e. they don't identify any storage at this point!
    fw = fh = -1;

    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    //On MacOS 10.6 and on, we'll look out for multitouch...
    if ([self respondsToSelector:@selector(setAcceptsTouchEvents:)]) {
      [self setAcceptsTouchEvents:YES];
      [self setWantsRestingTouches:YES];
    }
  }
  return self;
}

- (void)userDefaultsDidChange:(NSNotification *)aNote {
  //TODO also flush caches (i.e. glyphs) if alphabet changed?
  NSString *newFont=[[NSUserDefaults standardUserDefaults] objectForKey:@"DasherFont"];
  if (![newFont isEqualToString:self.cachedFontName]) {
    self.cachedFontName=newFont;
    aquaDasherScreen->RegenerateLabels();
    [dasherApp redraw];
  }
}

- (void)adjustTrackingRect {
// need this in order to get mouseEntered and Exited
//  [self removeTrackingRect:trackingRectTag];
//  trackingRectTag = [self addTrackingRect:[self frame] owner:self userData:nil assumeInside:[self isDashing]];
}

- (BOOL)isFlipped {
  return YES;
//  return NO;
}

- (BOOL)isOpaque {
  return YES;
}

- (void)awakeFromNib {
//  aquaDasherScreen = new COSXDasherScreen(self);
  [dasherApp setDasherView:self];
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(userDefaultsDidChange:) name:NSUserDefaultsDidChangeNotification object:nil];
  self.cachedFontName=[[NSUserDefaults standardUserDefaults] objectForKey:@"DasherFont"];

  NSSize sz = [self bounds].size;
  aquaDasherScreen = new COSXDasherScreen(self, sz.width, sz.height, tc_x, tc_y, textures);
  [dasherApp changeScreen:aquaDasherScreen];
  
  [self adjustTrackingRect];
}

- (BOOL)acceptsFirstResponder {
  return NO;
}

- (void)dealloc {
  self.cachedFontName=nil;
  
  [[self openGLContext] makeCurrentContext];
    
  [super dealloc];
}

- (void)gl_init {
  [[self openGLContext] makeCurrentContext];
  //glClearColor(1.0, 1.0, 1.0, 1.0);
  glShadeModel(GL_FLAT);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  //glEnable(GL_TEXTURE_2D);
}

- (void)reshape {
  NSSize sz=[self bounds].size;
  screenint w=sz.width,h=sz.height;
  [self gl_reshape:w :h];
  aquaDasherScreen->resize(w, h, tc_x, tc_y);
  //"reshape" gets called at shutdown, after applicationWillTerminate
  // has deleted the aquaDasherControl...!
  if (COSXDasherControl *pIntf = [dasherApp aquaDasherControl])
    pIntf->ScreenResized(aquaDasherScreen);
    
  [self adjustTrackingRect];  
}

- (void) gl_reshape:(int)w :(int)h
{
  [[self openGLContext] makeCurrentContext];
  int tw = P2Ceiling(w), th = P2Ceiling(h);
  if (tw!=fw || th!=fh) {
    //need to (re)allocate storage for framebuffer...
    for (int i=0; i<2; i++)
    {
      glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, frameBuffers[i]);
      glBindTexture(GL_TEXTURE_2D, textures[i]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tw, th, 0, GL_RGBA, GL_UNSIGNED_BYTE, nil);
      glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, textures[i], 0);
      if(glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT)
      {
        NSLog(@"failed to make complete framebuffer object %x", glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT));
        //...and do what, to report failure? we can't render anything, so Dasher is useless...?
      }
    }
    fw=tw; fh=th;
  }
  tc_x = w/(double)tw;
  tc_y = h/(double)th;
  
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
//  glOrtho(0, w, 0, h, -1.0, 1.0);
  glOrtho(0, w, h, 0, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

- (NSPoint)mouseLocation {
  NSPoint p = [[self window] mouseLocationOutsideOfEventStream];
  return [self convertPoint:p fromView:nil];
}

- (void)redisplay {
  [self setNeedsDisplay:YES];
}







@end
