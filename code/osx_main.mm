#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>
#include <OpenGL/gl.h>
#include "lib/math.h"
#include "lib/assert.h"
#include "rendering.h"

struct osx_state {
  bool Running;
  NSWindow *Window;
  NSOpenGLContext *OGLContext;
  GLuint TextureHandle;
  frame_buffer FrameBuffer;
};

@interface PathtracerAppDelegate : NSObject <NSApplicationDelegate>
- (void)setOSXState:(osx_state*)State;
@end

@implementation PathtracerAppDelegate {
  osx_state *OSXState;
}

- (void)setOSXState:(osx_state*)AState {
  OSXState = AState;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender {
  OSXState->Running = false;
  return NSTerminateCancel;
}
@end

@interface PathtracerWindowDelegate : NSObject <NSWindowDelegate>
- (void)setOSXState:(osx_state*)State;
@end

@implementation PathtracerWindowDelegate {
  osx_state *OSXState;
}

- (void)setOSXState:(osx_state*)AState {
  OSXState = AState;
}

- (BOOL)windowShouldClose:(id)sender {
  OSXState->Running = false;
  return NO;
}
@end

static void SetupOSXMenu() {
  NSMenu *Menu = [[NSMenu alloc] init];
  NSMenuItem *QuitItem = [[NSMenuItem alloc] initWithTitle:@"Quit" action:@selector(terminate:) keyEquivalent:@"q"];
  [Menu addItem:QuitItem];

  NSMenuItem *BarItem = [[NSMenuItem alloc] init];
  [BarItem setSubmenu:Menu];

  NSMenu *Bar = [[NSMenu alloc] init];
  [Bar addItem:BarItem];

  [NSApp setMainMenu:Bar];

  [Menu release];
  [QuitItem release];
  [BarItem release];
  [Bar release];
}

static NSWindow* CreateOSXWindow(v2ui16 Resolution) {
  int StyleMask = NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask;

  NSScreen *Screen = [NSScreen mainScreen];
  CGRect Rect = NSMakeRect(
    0,
    0,
    Resolution.X / Screen.backingScaleFactor,
    Resolution.Y / Screen.backingScaleFactor
  );
  NSWindow *Window = [[NSWindow alloc] initWithContentRect:Rect
                                          styleMask:StyleMask
                                            backing:NSBackingStoreBuffered
                                              defer:NO
                                              screen:Screen];
  if(Window == nil) {
    return NULL;
  }

  PathtracerWindowDelegate *Delegate = [[PathtracerWindowDelegate alloc] init];
  Window.delegate = Delegate;
  Window.title = [NSString stringWithUTF8String:"Pathtracer"];

  [Window center];
  [Window makeKeyAndOrderFront:nil];

  return Window;
}

static void ProcessOSXMessages() {
  while(true) {
    NSEvent *Event = [NSApp nextEventMatchingMask:NSAnyEventMask
                                        untilDate:[NSDate distantPast]
                                           inMode:NSDefaultRunLoopMode
                                          dequeue:YES];
    if(Event == nil) {
      return;
    }
    // switch(Event.type) {
    //   default:
    //     break;
    // }
    [NSApp sendEvent:Event];
  }
}

static NSOpenGLContext* CreateOGLContext() {
  NSOpenGLPixelFormatAttribute Attributes[] = {
    NSOpenGLPFADoubleBuffer,
    NSOpenGLPFAOpenGLProfile,
    NSOpenGLProfileVersionLegacy,
    0
  };
  NSOpenGLPixelFormat *PixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:Attributes];
  if(PixelFormat == nil) {
    return NULL;
  }

  NSOpenGLContext *Context = [[NSOpenGLContext alloc] initWithFormat:PixelFormat shareContext:nil];

  GLint Sync = 1;
  [Context setValues:&Sync forParameter:NSOpenGLCPSwapInterval];

  [PixelFormat release];

  return Context;
}

static void CreateTexture(GLuint *TextureHandle) {
  glGenTextures(1, TextureHandle);
  glBindTexture(GL_TEXTURE_2D, *TextureHandle);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
}

static void DestroyTexture(GLuint TextureHandle) {
  glDeleteTextures(1, &TextureHandle);
}

static void InitFrameBuffer(frame_buffer *Buffer) {
  Buffer->Resolution.Dimension.X = 1200;
  Buffer->Resolution.Dimension.Y = 800;
  memsize PixelCount = Buffer->Resolution.Dimension.X * Buffer->Resolution.Dimension.Y;
  Buffer->Pixels = (ui8*)malloc(PixelCount*3);
}

static void TerminateFrameBuffer(frame_buffer *Buffer) {
  free(Buffer->Pixels);
}

int main() {
  osx_state State;
  InitFrameBuffer(&State.FrameBuffer);
  State.Running = true;
  State.Window = nullptr;
  State.OGLContext = nullptr;

  NSApplication *App = [NSApplication sharedApplication];
  PathtracerAppDelegate *AppDelegate = [[PathtracerAppDelegate alloc] init];
  AppDelegate.OSXState = &State;
  App.delegate = AppDelegate;
  App.activationPolicy = NSApplicationActivationPolicyRegular;
  SetupOSXMenu();
  [App finishLaunching];

  State.Window = CreateOSXWindow(State.FrameBuffer.Resolution.Dimension);
  ReleaseAssert(State.Window != NULL, "Could not create window.");

  State.OGLContext = CreateOGLContext();
  ReleaseAssert(State.OGLContext != NULL, "Could not initialize OpenGL.");

  [State.OGLContext makeCurrentContext];
  [State.OGLContext setView:State.Window.contentView];

  CreateTexture(&State.TextureHandle);

#ifdef DEBUG
  [NSApp activateIgnoringOtherApps:YES];
#endif

  glEnable(GL_TEXTURE_2D);

  while(State.Running) {
    ProcessOSXMessages();

    if(State.Window.occlusionState & NSWindowOcclusionStateVisible) {
      Draw(&State.FrameBuffer);

      glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB,
        State.FrameBuffer.Resolution.Dimension.X,
        State.FrameBuffer.Resolution.Dimension.Y,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        State.FrameBuffer.Pixels
      );

      glBegin(GL_QUADS);
      glTexCoord2f(0.0f, 0.0f);
      glVertex2f(-1.0f, -1.0f);
      glTexCoord2f(1.0f, 0.0f);
      glVertex2f(1.0f, -1.0f);
      glTexCoord2f(1.0f, 1.0f);
      glVertex2f(1.0f, 1.0f);
      glTexCoord2f(0.0f, 1.0f);
      glVertex2f(-1.0f, 1.0f);
      glEnd();

      [State.OGLContext flushBuffer];
    }
    else {
      usleep(10000);
    }
  }

  DestroyTexture(State.TextureHandle);

  TerminateFrameBuffer(&State.FrameBuffer);

  {
    PathtracerAppDelegate *D = App.delegate;
    App.delegate = nil;
    [D release];
  }
  {
    PathtracerWindowDelegate *D = State.Window.delegate;
    State.Window.delegate = nil;
    [D release];
  }
  [State.Window release];
  [State.OGLContext release];
}
