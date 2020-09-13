/*
 *  star3map
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

#ifndef __STAR3MAP_STAR3MAP_H__
#define __STAR3MAP_STAR3MAP_H__

#include "render.h"
#include "starlist.h"
#include "r3/var.h"
#include "r3/thread.h"
#include <string>

#if ANDROID || IPHONE 
# define HAS_FACEBOOK 1
#endif

#define APP_VERSION_STRING "2.1.0"

#if HAS_FACEBOOK

extern r3::VarBool app_fbLoggedIn;
extern r3::VarString app_fbId;
extern r3::VarString app_fbUser;
extern r3::VarString app_fbGender;
extern r3::VarString app_locale;

void platformFacebookLogin();
void platformFacebookLogout();
void platformFacebookPublish( const std::string & caption, const std::string & description );

#endif 

extern r3::Condition condRender;


namespace star3map {

	void Display();	
	bool ProcessInput( bool active, int x, int y );
	
	inline float ModuloRange( float f, float lower, float upper ) {
		float delta = upper - lower;
		float fndiff = ( f - lower ) / delta;
		float fnfrac = fndiff - floor( fndiff );
		return fnfrac * delta + lower;
	}
	
}

#endif //__STAR3MAP_STAR3MAP_H__
