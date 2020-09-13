//
//  app.cpp
//  spacejunk
//
//  Created by Cass Everitt on 2/2/12.
//  Copyright (c) 2012 n/a. All rights reserved.
//

#include "r3/input.h"
#include "r3/command.h"
#include "r3/common.h"
#include "r3/filesystem.h"
#include "r3/gfxcontext.h"
#include "r3/var.h"
#include "r3/init.h"
#include "r3/texture.h"
#include "r3/draw.h"
#include "r3/console.h"
#include "r3/output.h"
#include "r3/thread.h"


#include "starlist.h"
#include "constellations.h"
#include "render.h"
#include "spacetime.h"
#include "star3map.h"

#include <map>
#include <iostream>
#include <algorithm>

#include <GL/Regal.h>

using namespace std;
using namespace r3;
using namespace star3map;

VarInteger r_windowWidth( "r_windowWidth", "window width", Var_ReadOnly, 0 );
VarInteger r_windowHeight( "r_windowHeight", "window height", Var_ReadOnly, 0 );
VarInteger r_windowDpi( "r_windowDpi", "window dots per inch", Var_ReadOnly, 160 );

VarFloat r_fov( "r_fov", "perspective field of view", 0, 90.0f );

// Austin, TX: { 30.283974,-97.742615 }
VarFloat app_latitude( "app_latitude", "viewer current latitude", Var_Archive, 30.283974f );
VarFloat app_longitude( "app_longitude", "viewer current longitude", Var_Archive, -97.742615f );

VarBool app_cull( "app_cull", "cull stars that are outside the current view", 0, 1 );

VarInteger app_updateMsec( "app_updateMsec", "the frame interval in msec", Var_Archive, 10 );

extern vector< Sprite > stars;
extern vector< Sprite > solarsystem;
extern vector< Lines > constellations;

r3::Texture2D *startex;

VarRotationf app_orientation( "app_orientation", "camera orientation", Var_Archive, Rotationf() );
r3::Matrix4f platformOrientation;



void UpdateLatLon() {
}

void GfxCheckErrors() {
	GLuint err = glGetError();
	if ( err != GL_NO_ERROR ) {
		Output( "gl error: %d", err );
	}
}

namespace star3map {
	void IncrementLoadProgress( const string & text = string() );
    extern GfxContext *drawContext;
}

void AsyncInitMisc() {
    startex = r3::CreateTexture2DFromFile("startex.jpg", TextureFormat_RGBA );
	IncrementLoadProgress( "textures" );
	
	{
		float starColors [] = {
			0.8, 0.8, 1.0, // blue
			1.0, 1.0, 0.8, // light yellow
			1.0, 1.0, 0.6, // yellow
			1.0, 0.8, 0.4, // orange
			1.0, 0.6, 0.3  // red
		};
		vector< star3map::Star > sl;
		ReadStarList( "stars.json", sl );
		map<int, star3map::Star *> sm;
		for ( int i = 0; i < (int)sl.size(); i++ ) {
			Sprite sp;
			star3map::Star & st = sl[i];
			sm[ st.hipnum ] = & st;
			sp.direction = SphericalToCartesian( 1.0f, ToRadians( st.dec ), ToRadians( st.ra ) );
			sp.magnitude = st.mag;
			sp.scale = 1.0f;
			sp.tex = startex;
			sp.name = st.name;
			float f = 3.99f * max( 0.f, min( 1.0f, float( ( st.colorIndex + 0.29 ) / ( 1.41 + 0.29 ) )  ) );
			int ind = f;
			float phase = f - ind;
			ind *= 3;
			Vec3f c0( starColors + ind );
			Vec3f c1( starColors + ind + 3 );
			Vec3f c = c0 * ( 1 - phase ) + c1 * phase;
			sp.color = Vec4f( c.x, c.y, c.z, 1 );
			stars.push_back( sp );
			if( ( i & 0x1f ) == 0x1f ) {
				IncrementLoadProgress( "stars" );
			}
		}
		vector< star3map::Constellation > cl;
		ReadConstellations( "constellations.txt", cl );
		for( int i = 0; i < (int)cl.size(); i++ ) {
			star3map::Constellation & c = cl[ i ];
			star3map::Lines lines;
			lines.name = c.name;
			lines.center = Vec3f( 0, 0, 0 );
			for ( int j = 0; j < (int)c.indexes.size(); j+=2 ) {
				if ( sm.count( c.indexes[ j + 0 ] ) && sm.count( c.indexes[j + 1 ] ) ) {
					for ( int k = 0; k < 2; k++ ) {
						star3map::Star *s = sm[ c.indexes[ j + k ] ];
						lines.vert.push_back( SphericalToCartesian( 1.0, ToRadians( s->dec ), ToRadians( s->ra ) ) );
						lines.center += lines.vert.back();
					}
				} else {
					Output( "One of the following constellation indexes was not found: %d, %d.", c.indexes[j], c.indexes[j+1] );
				}
			}
			lines.center.Normalize();
			lines.limit = 1.0f;
			for ( int j = 0; j < (int)lines.vert.size(); j++ ) {
				lines.limit = min( lines.limit, lines.center.Dot( lines.vert[ j ] ) );
			}
			constellations.push_back( lines );
			if( ( i & 0x3 ) == 0x3 ) {
				IncrementLoadProgress( "constellations" );
			}
		}
	}    
}



bool down;
Vec2f p0;

void appmouse( int button, bool isdown, int x, int y ) {
    ScopedReverseGfxContextAcquire rev( drawContext );

    down = isdown;
	if ( down ) {
		p0 = Vec2f( x, y );
	}
	ProcessInput( down, x, y );
}

void appmotion( int x, int y ) {
    ScopedReverseGfxContextAcquire rev( drawContext );
	Vec2f p1( x, y );
	if ( down ) {
		ProcessInput( true, x, y );
	}
}


void regalerr( GLenum err );
void regalerr( GLenum err ) {
    const char * errstr = NULL;
    switch( err ) {
        case GL_INVALID_ENUM: errstr = "INVALID ENUM"; break;
        case GL_INVALID_OPERATION: errstr = "INVALID OPERATION"; break;
        case GL_INVALID_VALUE: errstr = "INVALID_VALUE"; break;
        default:
            fprintf( stderr, "Got a GL error: %d!\n", err );
            break;
    }
    if( errstr ) {
        fprintf( stderr, "Got a GL error: %s\n", errstr );
    }
    ;
}

void appreshape( int width, int height ) {
    reshape( width, height );
    r_windowWidth.SetVal( width );
	r_windowHeight.SetVal( height );
	extern VarFloat app_aspectRatio;
	app_aspectRatio.SetVal( float( width ) / float( height ) );
	glViewport( 0, 0, width, height );
}

int displayCount = 0;

void appdisplay() {
    ScopedReverseGfxContextAcquire rev( drawContext );
    star3map::Display();
    {
        //ScopedGfxContextAcquire fwd( drawContext );
        //void display( bool clear );
        //display( false );
    }
    displayCount++;
    if( displayCount == 300 ) {
        ScopedGfxContextAcquire fwd( drawContext );
        RegalSetErrorCallback( NULL );
    }
}

void appmain() {
    const char *argv[] = { "foo", "bar" };
    r3::Init( 2, argv );
	r3::ExecuteCommand( "bind escape quit" );
    //r3::FileDelete( "default.vars" );
    
    r3::Output( "GL_VERSION: %s", glGetString( GL_VERSION ) );
    RegalSetErrorCallback( regalerr );
}



// appquit command
void AppQuit( const vector< Token > & tokens ) {
	Shutdown();
	exit( 0 );
}
CommandFunc AppQuitCmd( "appquit", "destroys app specific stuff", AppQuit );



