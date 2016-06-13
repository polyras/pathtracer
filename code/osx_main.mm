#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>
#include <OpenGL/gl.h>
#include <dispatch/dispatch.h>
#include <new>
#include <thread>
#include "lib/math.h"
#include "lib/assert.h"
#include "rendering.h"
#include "scene1.h"

#define THREAD_COUNT 4

struct osx_state {
  bool Running;
  NSWindow *Window;
  NSOpenGLContext *OGLContext;
  GLuint TextureHandle;
  color *RenderBuffer;
  resolution WindowResolution;
  resolution RenderResolution;
  scene Scene;
  memsize TileCount;
  dispatch_semaphore_t WorkerSemaphore;
  dispatch_semaphore_t MainSemaphore;
  std::thread Threads[THREAD_COUNT];
  std::atomic<memsize> NextTileIndex;
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

static void InitPixelBuffer(osx_state *State) {
  memsize PixelCount = State->RenderResolution.CalcCount();
  State->RenderBuffer = new (std::nothrow) color[PixelCount];
  ReleaseAssert(State->RenderBuffer != nullptr, "Could not allocate render buffer.");
}

static void TerminateFrameBuffer(osx_state *State) {
  delete[] State->RenderBuffer;
  State->RenderBuffer = nullptr;
}

static void WorkerMain(osx_state *State) {
  for(;;) {
    dispatch_semaphore_wait(State->WorkerSemaphore, DISPATCH_TIME_FOREVER);

    for(;;) {
      ui16 TileIndex = State->NextTileIndex.load(std::memory_order_relaxed);
      if(TileIndex == UI16_MAX) {
        return;
      }

      TileIndex = State->NextTileIndex.fetch_add(1, std::memory_order_relaxed);
      if(TileIndex < State->TileCount) {
        RenderTile(State->RenderBuffer, &State->Scene, TileIndex);
        if(TileIndex + 1 == State->TileCount) {
          dispatch_semaphore_signal(State->MainSemaphore);
        }
      }
      else {
        break;
      }
    }
  }
}

static void CreateThreads(osx_state *State) {
  for(memsize I=0; I<THREAD_COUNT; ++I) {
    State->Threads[I] = std::thread(WorkerMain, State);
  }
}

static void DestroyThreads(osx_state *State) {
  State->NextTileIndex.store(UI16_MAX, std::memory_order_relaxed);

  for(memsize I=0; I<THREAD_COUNT; ++I) {
    dispatch_semaphore_signal(State->WorkerSemaphore);
  }

  for(memsize I=0; I<THREAD_COUNT; ++I) {
    State->Threads[I].join();
  }
}

int main() {
  osx_state State;
  State.Running = true;
  State.Window = nullptr;
  State.OGLContext = nullptr;
  State.WindowResolution.Dimension.Set(1200, 800);
  State.RenderResolution.Dimension.Set(160, 120);
  InitPixelBuffer(&State);

  InitScene1(&State.Scene);

  NSApplication *App = [NSApplication sharedApplication];
  PathtracerAppDelegate *AppDelegate = [[PathtracerAppDelegate alloc] init];
  AppDelegate.OSXState = &State;
  App.delegate = AppDelegate;
  App.activationPolicy = NSApplicationActivationPolicyRegular;
  SetupOSXMenu();
  [App finishLaunching];

  State.Window = CreateOSXWindow(State.WindowResolution.Dimension);
  ReleaseAssert(State.Window != NULL, "Could not create window.");

  State.OGLContext = CreateOGLContext();
  ReleaseAssert(State.OGLContext != NULL, "Could not initialize OpenGL.");

  [State.OGLContext makeCurrentContext];
  [State.OGLContext setView:State.Window.contentView];

  CreateTexture(&State.TextureHandle);

#ifdef DEBUG
  [NSApp activateIgnoringOtherApps:YES];
#endif

  glEnable(GL_FRAMEBUFFER_SRGB);
  glEnable(GL_TEXTURE_2D);

  State.TileCount = InitRendering(State.RenderResolution);
  State.WorkerSemaphore = dispatch_semaphore_create(0);
  ReleaseAssert(State.WorkerSemaphore != NULL, "Could not create worker semaphore.");
  State.MainSemaphore = dispatch_semaphore_create(0);
  ReleaseAssert(State.MainSemaphore != NULL, "Could not create main semaphore.");
  CreateThreads(&State);

  while(State.Running) {
    ProcessOSXMessages();

    if(State.Window.occlusionState & NSWindowOcclusionStateVisible) {
      State.NextTileIndex.store(0, std::memory_order_relaxed);
      for(memsize I=0; I<THREAD_COUNT; ++I) {
        dispatch_semaphore_signal(State.WorkerSemaphore);
      }
      dispatch_semaphore_wait(State.MainSemaphore, DISPATCH_TIME_FOREVER);

      glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB,
        State.RenderResolution.Dimension.X,
        State.RenderResolution.Dimension.Y,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        State.RenderBuffer
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

  DestroyThreads(&State);
  dispatch_release(State.WorkerSemaphore);
  dispatch_release(State.MainSemaphore);
  TerminateRendering();

  DestroyTexture(State.TextureHandle);

  TerminateFrameBuffer(&State);

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
