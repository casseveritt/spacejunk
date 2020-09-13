#include "Regal.h"
#include <stdio.h>



static void regalerr( GLenum err ) {
    static int errcount = 0;
    const char * errstr = NULL;
    switch( err ) {
        case GL_INVALID_ENUM: errstr = "INVALID ENUM"; break;
        case GL_INVALID_OPERATION: errstr = "INVALID OPERATION"; break;
        default:
            fprintf( stderr, "Got a GL error: %d!\n", err );
            break;
    }
    if( errstr ) {
        fprintf( stderr, "Got a GL error: %s\n", errstr );
    }
    
    if( errcount > 50 ) {
        RegalSetErrorCallback( NULL );
    }
}



// Need this because glut links against the real CGL...
void regal_make_current() {
    RegalMakeCurrent( CGLGetCurrentContext() );
    RegalSetErrorCallback( regalerr );
}

void draw_something() {
    
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    
    glUseProgram( 0 );
    glMatrixPushEXT( GL_PROJECTION );
    glMatrixLoadIdentityEXT( GL_PROJECTION );
    glMatrixPushEXT( GL_MODELVIEW );
    glMatrixLoadIdentityEXT( GL_MODELVIEW );
    {
        glColor3f( 1, 1, 1 );
        glBegin( GL_TRIANGLES );
        for( int i = 0; i < 100; i++ ) {
            glVertex3f( -1.1, -1.1, (i - 50) / 25.f );
            glVertex3f(  1.1, -1.1, (i - 50) / 25.f  );
            glVertex3f(  0.0, 1.0, (i - 50) / 25.f  );
        }
        glEnd();
    }
    glMatrixPopEXT( GL_PROJECTION );
    glMatrixPopEXT( GL_MODELVIEW );

}