/*
 *  satellite
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

#ifndef __STAR3MAP_SATELLITE_H__
#define __STAR3MAP_SATELLITE_H__

#include <string>
#include <vector>
#include <deque>
#include "r3/linear.h"

namespace star3map {
	
	struct Satellite {
		r3::Vec3f pos;
		std::string name;
		int id;
		bool special;
	};
	
	struct PathPoint {
		PathPoint() : minutesFromEpoch( 0.0 ), aboveThreshold( false ) {}
		r3::Vec3f pos;
		double minutesFromEpoch;
		bool aboveThreshold;
	};

	struct SatellitePath {
		SatellitePath() : aboveThresholdCount( 0 ) {}
		std::deque< PathPoint > pathPoint;
		std::string name;
		bool special;
		int aboveThresholdCount;
	};
	
	void InitializeSatellites();

	bool SatellitesLoaded();
	void ComputeSatellitePositions( std::vector<Satellite> & satellites );
	void GetSatelliteFlyovers( float lat, float lon, std::vector<SatellitePath> & paths );
	
}




#endif // __STAR3MAP_SATELLITE_H__