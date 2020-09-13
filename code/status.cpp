/*
 *  status
 */


/* 
 Copyright (c) 2010 Cass Everitt
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or
 without modification, are permitted provided that the following
 conditions are met:
 
 * Redistributions of source code must retain the above
 copyright notice, this list of conditions and the following
 disclaimer.
 
 * Redistributions in binary form must reproduce the above
 copyright notice, this list of conditions and the following
 disclaimer in the documentation and/or other materials
 provided with the distribution.
 
 * The names of contributors to this software may not be used
 to endorse or promote products derived from this software
 without specific prior written permission. 
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 POSSIBILITY OF SUCH DAMAGE. 
 
 
 Cass Everitt
 */

#include "status.h"
#include "render.h"
#include <GL/Regal.h>

#include "r3/draw.h"
#include "r3/font.h"
#include "r3/time.h"
#include "r3/var.h"

#include <string>

using namespace r3;
using namespace star3map;
using namespace std;

extern VarInteger r_windowWidth;
extern VarInteger r_windowHeight;
extern VarInteger r_windowDpi;
extern VarString app_locale;

namespace {

	const float messageDuration = 3.0f;
	
	Font *font;
	float startTime;
	float lastUpdateTime;
	string statusMsg;
	
	bool initialized = false;
	void Initialize() {
		if( initialized ) {
			return;
		}
        
        string fontName = "DroidSans.ttf";
        string fallback = "";
        string locale = app_locale.GetVal();
        if( locale.find( "ar_" ) != string::npos ) {
            fallback = fontName;
            fontName = "DroidSansArabic.ttf";
        } else if( locale.find( "he_" ) != string::npos || locale.find( "iw_" ) != string::npos ) {
            fallback = fontName;
            fontName = "DroidSansHebrew.ttf";
        } else if( locale.find( "th_" ) != string::npos ) {
            fallback = fontName;
            fontName = "DroidSansThai.ttf";
        } else if( locale.find( "zh_" ) != string::npos ||
                  locale.find( "ko_" ) != string::npos ||
                  locale.find( "ja_" ) != string::npos ||
                  0 // locale.find( "ru_" ) != string::npos
                  ) {
            fallback = fontName;
            fontName = "DroidSansFallback.ttf";
        }

		font = r3::CreateStbFont( fontName, fallback, 24 );
		//font = r3::CreateStbFont( "LiberationSans-Regular.ttf", 24 );
		
		initialized  = true;
	}
	
	bool enable = false;
}



namespace star3map {

	void EnableStatusMessages() {
		enable = true;
	}
	void DisableStatusMessages(){
		enable = false;
	}
	
	void SetStatus( const char *msg ) {
		if( enable == false ) {
			return;
		}
		lastUpdateTime = GetSeconds();
		if ( statusMsg.size() == 0 ) {
			startTime = lastUpdateTime;
		}
		statusMsg = msg;
	}
	
	
	void RenderStatus() {
		if( enable == false ) {
			return;
		}
		if ( statusMsg.size() == 0 ) {
			return;
		}
		Initialize();
		
		float t = GetSeconds();
		float alphaRampIn = min( 1.0f, ( t - startTime ) * 2.0f );
		float alphaRampOut = min( 1.0f, max( 0.0f, lastUpdateTime + messageDuration - t ) );
		float alpha = min( alphaRampIn, alphaRampOut );
		
		
		float w = r_windowWidth.GetVal();
		//float h = r_windowHeight.GetVal();
		float dpiRatio = r_windowDpi.GetVal() / 160.0f;

        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glEnable( GL_BLEND );
		
		Bounds2f b(  4.0f * dpiRatio, 4.f * dpiRatio , w - (4.0f * dpiRatio) , 24.f * dpiRatio );
		Bounds2f bp = b;
		bp.Inset( -4.0f * dpiRatio );
				   
		glColor4f( 0.0f, 0.0f, 0.0f, 0.6f * alpha );
		glBegin( GL_QUADS );
		glVertex2f( bp.ll.x, bp.ll.y);
		glVertex2f( bp.ur.x, bp.ll.y );
		glVertex2f( bp.ur.x, bp.ur.y );
		glVertex2f( bp.ll.x, bp.ur.y );
		glEnd();
		
		
		glColor4f( 0.8f, 0.8f, 0.8f, 0.8 * alpha );

		DrawString2D( statusMsg, b , font );
        glDisable( GL_BLEND );

		if( alphaRampOut == 0.0f ) {
			statusMsg.clear();
		}
		
	}

}

