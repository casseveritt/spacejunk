/*
 *  spacetime
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

#include <stdlib.h> // for setenv

#include "r3/time.h"

namespace {
	bool initialized = false;		
}

namespace star3map {

	
	double epochMinutes;

    void InitializeSpaceTime();
	void InitializeSpaceTime() {
		if ( initialized ) {
			return;
		}
		
		struct tm epoch_tm, curr_tm;
		time_t gmt = time_t( r3::GetTime() );
		curr_tm = *Gmtime( &gmt );
		epoch_tm = curr_tm;
		epoch_tm.tm_sec = 0;
		epoch_tm.tm_min = 0;
		epoch_tm.tm_hour = 12;
		epoch_tm.tm_mday = 1;
		epoch_tm.tm_mon = 0;
		epoch_tm.tm_year = 2000 - 1900;
		epoch_tm.tm_isdst = -1; // timegm() will try to figure it out if negative
		
#if __APPLE__
		// we operate on gmtime only...
		setenv( "TZ", "", 1 );
		tzset();		
		time_t epoch = timegm( & epoch_tm );
#elif ANDROID
		setenv( "TZ", "", 1 );
		tzset();
		time_t epoch = timegm64( & epoch_tm );
#elif _WIN32
		_putenv_s( "TZ", "GST" );
		_tzset();
		time_t epoch = mktime( & epoch_tm );
#endif
		//Output( "Epoch = %s", ctime( & epoch ) );
		
		epochMinutes = double( epoch ) / 60.0;
		initialized = true;
	}
		
}


