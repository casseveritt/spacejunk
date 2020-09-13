//
//  RegalView.mm
//
//  Created by Cass Everitt on 3/15/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import "RegalView.h"
#import "RGLOpenGLContext.h"
#include "render.h"
#include <stdio.h>
#include <sstream>
#include "r3/input.h"


RegalView *regalView = NULL;

void GfxSwapBuffers() {
    if( regalView == NULL ) {
        return;
    }
    //[[regalView openGLContext] flushBuffer];
}

int appmouse( int button, bool isdown, int x, int y );
int appmotion( int x, int y );
int appreshape( int width, int height );
int appdisplay();
int appmain();


@implementation RegalView

bool maincalled = false;
bool resized = false;
bool drawn = false;

int w, h;

NSTimer *timer = nil;

- (id)initWithCoder:(NSCoder *)aDecoder {

    [super initWithCoder: aDecoder];
	NSOpenGLPixelFormatAttribute attr[] = {
        //NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersionLegacy,      //   pick this 
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,   //   or this
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAAccelerated,
		NSOpenGLPFAAlphaSize, 8,
		NSOpenGLPFAColorSize, 32,
		NSOpenGLPFADepthSize, 24,
		NSOpenGLPFAStencilSize, 0,
		0 
	};	
	NSOpenGLPixelFormat *fmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:attr];
    [self setPixelFormat: fmt];
    RGLOpenGLContext *rglCtx = [[RGLOpenGLContext alloc] initWithFormat:fmt shareContext:nil];
    [self setOpenGLContext: rglCtx];
    [rglCtx setView:self];
	timer = [NSTimer scheduledTimerWithTimeInterval:0.01 target:self selector:@selector	(render) userInfo:nil repeats:YES];
    drawn = false;
    if( regalView == NULL ) {
        regalView = self;
    }
    NSRect rect = [[[self window] contentView] frame];
    w = rect.size.width;
    h = rect.size.height;
    return self;
}


- (void)drawRect:(NSRect)dirtyRect {
    NSRect rect = [[[self window] contentView] frame];
    if( drawn == false ) {
        appmain();
    }
    drawn = true;
    if( resized ) {
        appreshape( rect.size.width, rect.size.height );
        resized = false;
    }
    appdisplay();
    [[self openGLContext] flushBuffer];

}

- (void) render {
	[self setNeedsDisplay:YES];
}

- (void)viewDidMoveToWindow
{
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(windowResized:) name:NSWindowDidResizeNotification
                                               object:[self window]];
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (void)mouseDown:(NSEvent *)theEvent
{
    NSPoint p = [ [self window] mouseLocationOutsideOfEventStream ];
    NSInteger b = [theEvent buttonNumber];
    appmouse( b, true, p.x, p.y );
}

- (void)mouseUp:(NSEvent *)theEvent
{
    NSPoint p = [ [self window] mouseLocationOutsideOfEventStream ];
    NSInteger b = [theEvent buttonNumber];
    appmouse( b, false, p.x, p.y );
}

- (void) mouseDragged:(NSEvent *)theEvent
{
    NSPoint p = [ [self window] mouseLocationOutsideOfEventStream ];
    appmotion( p.x, p.y);  
}


#define KEY_INFO( mod, chg, mask, name, ss ) if( mod & mask ) ss << name << " "

- (void)flagsChanged:(NSEvent *)theEvent
{
    static unsigned int oldMod = 0;
    unsigned int mod = [theEvent modifierFlags];
    //unsigned int chg = mod ^ oldMod;
    std::stringstream ss( std::stringstream::out );
    KEY_INFO( mod, chg, NSAlphaShiftKeyMask, "AlphaShift", ss );
    KEY_INFO( mod, chg, NSShiftKeyMask, "Shift", ss );
    KEY_INFO( mod, chg, NSControlKeyMask, "Control", ss );
    KEY_INFO( mod, chg, NSAlternateKeyMask, "Alt", ss );
    KEY_INFO( mod, chg, NSCommandKeyMask, "Command", ss );
    KEY_INFO( mod, chg, NSFunctionKeyMask, "Func", ss );
    //fprintf( stderr, "mod: %s\n", ss.str().c_str() );
    //fprintf( stderr, "modifier flags: %x, old: %x, changed: %x\n", mod, oldMod, chg );
    oldMod = mod;
    
}


enum KeyCode {
    kVK_Return                    = 0x24,
    kVK_Tab                       = 0x30,
    kVK_Space                     = 0x31,
    kVK_Delete                    = 0x33,
    kVK_Escape                    = 0x35,
    kVK_Command                   = 0x37,
    kVK_Shift                     = 0x38,
    kVK_CapsLock                  = 0x39,
    kVK_Option                    = 0x3A,
    kVK_Control                   = 0x3B,
    kVK_RightShift                = 0x3C,
    kVK_RightOption               = 0x3D,
    kVK_RightControl              = 0x3E,
    kVK_Function                  = 0x3F,
    kVK_F17                       = 0x40,
    kVK_VolumeUp                  = 0x48,
    kVK_VolumeDown                = 0x49,
    kVK_Mute                      = 0x4A,
    kVK_F18                       = 0x4F,
    kVK_F19                       = 0x50,
    kVK_F20                       = 0x5A,
    kVK_F5                        = 0x60,
    kVK_F6                        = 0x61,
    kVK_F7                        = 0x62,
    kVK_F3                        = 0x63,
    kVK_F8                        = 0x64,
    kVK_F9                        = 0x65,
    kVK_F11                       = 0x67,
    kVK_F13                       = 0x69,
    kVK_F16                       = 0x6A,
    kVK_F14                       = 0x6B,
    kVK_F10                       = 0x6D,
    kVK_F12                       = 0x6F,
    kVK_F15                       = 0x71,
    kVK_Help                      = 0x72,
    kVK_Home                      = 0x73,
    kVK_PageUp                    = 0x74,
    kVK_ForwardDelete             = 0x75,
    kVK_F4                        = 0x76,
    kVK_End                       = 0x77,
    kVK_F2                        = 0x78,
    kVK_PageDown                  = 0x79,
    kVK_F1                        = 0x7A,
    kVK_LeftArrow                 = 0x7B,
    kVK_RightArrow                = 0x7C,
    kVK_DownArrow                 = 0x7D,
    kVK_UpArrow                   = 0x7E
};

int KeycodeToKeysym( int kc ) {
    switch( kc ) {
        case kVK_F1: return XK_F1;
        case kVK_F2: return XK_F2;
        case kVK_F3: return XK_F3;
        case kVK_F4: return XK_F4;
        case kVK_F5: return XK_F5;
        case kVK_F6: return XK_F6;
        case kVK_F7: return XK_F7;
        case kVK_F8: return XK_F8;
        case kVK_F9: return XK_F9;
        case kVK_F10: return XK_F10;
        case kVK_F11: return XK_F11;
        case kVK_F12: return XK_F12;
        case kVK_LeftArrow: return XK_Left;
        case kVK_RightArrow: return XK_Right;
        case kVK_DownArrow: return XK_Down;
        case kVK_UpArrow: return XK_Up;
        default: break;
    }
    return 0;
}

- (void)keyDown:(NSEvent *)theEvent
{
    /*
     int rawkey = [theEvent keyCode];
     int key;
     switch( key ) {
     case kVK_ANSI_A: key = 'a'; break;
     default: key = rawkey; break;
     }
     fprintf( stderr, "Got keycode %c (%d)\n", key, key );
     */
    int rawkey = [theEvent keyCode];
    int ks = KeycodeToKeysym( rawkey );
    NSString * s = [theEvent characters];
    int k = r3::AsciiToKey( [s UTF8String][0] );
    fprintf( stderr, "Key down %c (%d) - keyCode: %x\n", k, k, rawkey);
    r3::CreateKeyEvent( ks != 0 ? ks : k, r3::KeyState_Down );
}

- (void)keyUp:(NSEvent *)theEvent
{
    /*
     int rawkey = [theEvent keyCode];
     int key;
     switch( key ) {
     case kVK_ANSI_A: key = 'a'; break;
     default: key = rawkey; break;
     }
     fprintf( stderr, "Got keycode %c (%d)\n", key, key );
     */
    int rawkey = [theEvent keyCode];    
    int ks = KeycodeToKeysym( rawkey );
    NSString * s = [theEvent characters];
    int k = r3::AsciiToKey( [s UTF8String][0] );
    fprintf( stderr, "Key up %c (%d) - keyCode: %x\n", k, k, rawkey);
    r3::CreateKeyEvent( ks != 0 ? ks : k, r3::KeyState_Up );
}



- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [super dealloc];
}

- (void)windowResized:(NSNotification *)notification;
{
    NSRect rect = [[[self window] contentView] frame];
    NSRect crect = NSRectFromCGRect( CGRectMake( 0, 0, rect.size.width, rect.size.height ) );
    [self setFrame: crect];
    [self lockFocus];
    resized = rect.size.width != w || rect.size.width != h;
    [self unlockFocus];
}

@end
