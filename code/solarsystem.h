/*
 *  solarsystem
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

#include "r3/linear.h"

namespace star3map {

	enum SolarSystemBodyEnum {
		SSB_Sun,
		SSB_Moon,
		SSB_Mercury,
		SSB_Venus,
		SSB_Mars,
		SSB_Jupiter,
		SSB_Saturn,
		SSB_Uranus,
		SSB_Neptune,
		SSB_MAX
	};
	
	struct Elements {
		void Update( double newDay );
		
		SolarSystemBodyEnum which;
		// tabular
		const char *name;
		double day;
		double lonOfAscendingNode;
		double inclination;
		double argumentOfPeriapsis;
		double semiMajorAxis;
		double eccentricity;
		double meanAnomaly;
		// derived
		double eccentricAnomaly;
		r3::Vec2d ellipsePos;
		double trueAnomaly;
		double distance;
		double longitude;
		r3::Vec3d barycentricPos;
		r3::Vec3d barycentricPosSph;
		r3::Vec3d geocentricPos;
		r3::Vec3d equatorialPos;
		r3::Vec3d equatorialPosSph;
	};
	
	struct SolarSystem {
		void Update( double newDay );
		double day;
		Elements body[ SSB_MAX ];		
	};

}


