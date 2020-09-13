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

#include "r3/time.h"

namespace star3map {

	const double MinutesPerDay = 1440.0; 
	const double SecondsPerDay = 86400.0;
	const double EarthRotationsPerSiderealDay = 1.00273790934;
	const double JulianDateAtEpoch = 2451545.0; // (2000 January 1, 12h UT1).
	const double RadiusEarthKm = 6378.135;
	
	extern double epochMinutes;

	inline double GetMinutesFromEpoch( double timeInMinutes ) {
		return  timeInMinutes - epochMinutes;
	}
	
	inline double GetCurrentMinutesFromEpoch() {
		return GetMinutesFromEpoch( r3::GetTime() / 60.0 );
	}
	
	inline double GetJulianDate( double minutesFromEpoch ) {
		double JD = ( minutesFromEpoch / MinutesPerDay ) + JulianDateAtEpoch;
		return JD;
	}
	
	// solar day number starts at 2000 January 1, 0h UT1, so 0.5 days more than what we use for satellites
	inline double GetSolarDayNumber( double timeInMinutes) {
		// doc says d = JD - 2451543.5, which is what the below is... I don't quite understand why
		// there's a 1.5 in there, but it is necessary, or stuff is in the wrong place.
		// Especially the moon, since its period is so much shorter.
		return GetMinutesFromEpoch( timeInMinutes ) / MinutesPerDay + 1.5;
	}
	
	inline double GetCurrentSolarDayNumber() {
		return GetSolarDayNumber( r3::GetTime() / 60.0 );
	}
	
	inline double Frac( double d ) {
		return d - floor( d );
	}
	
	inline double GetThetaG( double minutesFromEpoch ) { 
		double JD = GetJulianDate( minutesFromEpoch );
		double UT = Frac( JD + 0.5 );
		JD -= UT;
		double Tu = ( JD - JulianDateAtEpoch ) / 36525.0;
		double gmst = 24110.54841 + 8640184.812866 * Tu + 0.093104 * Tu * Tu - 6.2e-6 * Tu * Tu * Tu;
		gmst /= SecondsPerDay;
		gmst += EarthRotationsPerSiderealDay * UT;
		gmst = Frac( gmst );
		return gmst * 2.0 * R3_PI;
	}
		
	// Get the rotation of the earth relative to ECI
	inline float GetEarthPhase( double timeInMinutes ) {
		return GetThetaG( timeInMinutes - epochMinutes );
	}
	
	// Get the rotation of the earth relative to ECI
	inline float GetCurrentEarthPhase() {
		return GetEarthPhase( r3::GetTime() / 60.0 );
	}
	
	void InitializeSpaceTime();
}


