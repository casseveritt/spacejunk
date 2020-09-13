#import "RGLOpenGLContext.h"


extern "C" void RegalMakeCurrent( CGLContextObj ctxobj );
int appmain();

@implementation RGLOpenGLContext

-(void)makeCurrentContext {
    [super makeCurrentContext];
    RegalMakeCurrent( CGLGetCurrentContext() );
}

@end
