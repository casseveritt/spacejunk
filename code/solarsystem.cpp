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

#include "solarsystem.h"

#include <math.h>

using namespace r3;
using namespace star3map;

namespace {
	
	struct TimeLinear {
		double Val( double day ) const {
			return a + b * day;
		}
		double a; // constant part
		double b; // times current time
	};
	
	struct TimeLinearElements {
		const char *name;
		TimeLinear lonOfAscendingNode;
		TimeLinear inclination;
		TimeLinear argumentOfPeriapsis;
		TimeLinear semiMajorAxis;
		TimeLinear eccentricity;
		TimeLinear meanAnomaly;		
	};

	// Transcribed from Paul Schlyter's very helpful page on how to compute planetary positions.
	// http://stjarnhimlen.se/comp/ppcomp.html
	
	TimeLinearElements storedElements[] = {
		{
			"Sun",
			{   0.0     ,  0.0           },	// lonOfAscendingNode
			{   0.0     ,  0.0           },	// inclination
			{ 282.9404  ,  4.70935E-5    }, // argumentOfPeriapsis
			{   1.0     ,  0.0           }, // semiMajorAxis (AU)
			{   0.016709, -1.15100E-9    }, // eccentricity
			{ 356.0470  ,  0.9856002585  }	// meanAnomaly
		},
		{	
			"Moon",
			{ 125.1228  , -0.0529538083  }, // lonOfAscendingNode
			{   5.1454  ,  0.0           }, // inclination
			{ 318.0634  ,  0.1643573223  },	// argumentOfPeriapsis
			{  60.2666  ,  0.0           },	// semiMajorAxis (Earth radii)
			{   0.054900,  0.0           },	// eccentricity
			{ 115.3654  ,  13.0649929509 }	// meanAnomaly
		},
		{	
			"Mercury",
			{  48.3313  ,  3.24587E-5    }, // lonOfAscendingNode
			{   7.0047  ,  5.00000E-8    }, // inclination
			{  29.1241  ,  1.01444E-5    }, // argumentOfPeriapsis
			{   0.387098,  0.0           }, // semiMajorAxis (AU)
			{   0.205635,  5.59E-10      }, // eccentricity
			{ 168.6562  ,  4.0923344368  }	// meanAnomaly
		},
		{	
			"Venus",
			{  76.6799  ,  2.46590E-5    }, // lonOfAscendingNode
			{   3.3946  ,  2.75000E-8    }, // inclination
			{  54.8910  ,  1.38374E-5    }, // argumentOfPeriapsis
			{   0.723330,  0.0           }, // semiMajorAxis (AU)
			{   0.006773, -1.30200E-9    },	// eccentricity
			{  48.0052  ,  1.6021302244  }	// meanAnomaly
		},
		{	
			"Mars",
			{  49.5574  ,  2.11081E-5    },	// lonOfAscendingNode
			{   1.8497  , -1.78000E-8    },	// inclination
			{ 286.5016  ,  2.92961E-5    },	// argumentOfPeriapsis
			{  1.523688 ,  0.0           },	// semiMajorAxis (AU)
			{  0.093405 ,  2.516000E-9   },	// eccentricity
			{ 18.6021   ,  0.5240207766  }	// meanAnomaly
		},
		{	
			"Jupiter",
			{ 100.4542  ,  2.76854E-5    },	// lonOfAscendingNode
			{   1.3030  , -1.55700E-7    },	// inclination
			{ 273.8777  ,  1.64505E-5    },	// argumentOfPeriapsis
			{   5.20256 ,  0.0			 },	// semiMajorAxis (AU)
			{   0.048498,  4.46900E-9    },	// eccentricity
			{  19.8950  ,  0.0830853001  }	// meanAnomaly
		},
		{	
			"Saturn",
			{ 113.6634  ,  2.38980E-5    },	// lonOfAscendingNode
			{   2.4886  , -1.08100E-7    },	// inclination
			{ 339.3939  ,  2.97661E-5    },	// argumentOfPeriapsis
			{   9.55475 ,  0.0			 },	// semiMajorAxis (AU)
			{   0.055546, -9.49900E-9    },	// eccentricity
			{ 316.9670  ,  0.0334442282  }	// meanAnomaly
		},
		{	
			"Uranus",
			{  74.0005  ,  1.3978E-5     },	// lonOfAscendingNode
			{   0.7733  ,  1.9000E-8	 },	// inclination
			{  96.6612  ,  3.0565E-5	 },	// argumentOfPeriapsis
			{  19.18171 , -1.5500E-8	 },	// semiMajorAxis (AU)
			{   0.047318,  7.4500E-9     },	// eccentricity
			{ 142.5905  ,  0.011725806   }	// meanAnomaly
		},
		{	
			"Neptune",
			{ 131.7806  ,  3.0173E-5     },	// lonOfAscendingNode
			{   1.7700  , -2.5500E-7     },	// inclination
			{ 272.8461  , -6.0270E-6     },	// argumentOfPeriapsis
			{  30.05826 ,  3.3130E-8	 },	// semiMajorAxis (AU)
			{   0.008606,  2.1500E-9	 },	// eccentricity
			{ 260.2471  ,  0.005995147	 }	// meanAnomaly
		}
	};
	
	// spherical maps { rho, phi, theta } to { x, y, z } in the Vec3
	Vec3d SphericalToCartesian( const Vec3d & s ) {
		return Vec3d( cos( s.y ) * cos( s.z ), cos( s.y ) * sin( s.z ), sin( s.y ) ) * s.x;
	}
	
	// spherical maps { rho, phi, theta } to { x, y, z } in the Vec3
	Vec3d CartesianToSpherical( const Vec3d & c ) {
		return Vec3d( c.Length(), atan2( c.z, sqrt( c.x * c.x + c.y * c.y ) ), atan2( c.y, c.x ) );
	}
	

	
	double Frac( double d ) {
		return d - floor( d );
	}
	
	double Modulo( double d, double period ) {
		return Frac( d / period ) * period;
	}
	

	// Computes (approximate) eccentric anomaly from mean anomaly (M) and eccentricity (e).
	double EccentricAnomaly( double M, double e ) {
		double E = M + e * sin( M ) * ( 1.0 + e * cos( M ) );
		
		if ( e > 0.05 ) {
			double E0 = E;
			const double limit = ToRadians( 0.001 );
			for ( int i = 0; i < 10; i++ ) { // quit after some number of iterations, even if we didn't converge
				E = E0 - ( E0 - e * sin( E0 ) - M ) / ( 1.0 - e * cos( E0 )  );
				if ( abs( E - E0 ) < limit ) {
					break;
				}
				E0 = E;
			}			
		}
		return Modulo( E, 2.0 * R3_PI );
	}
	
	// Computes cartesian coordinates in "ellipse local" space from 
	// semiMajorAxis (a), eccentric anomaly (E), and eccintricity (e).
	// In "ellipse local" space, the massive body is at the origin and periapsis is on the positive x axis.
	Vec2d EllipsePosition( double a, double E, double e ) {
		Vec2d r;
		r.x = a * ( cos( E ) - e );
		r.y = a * ( sqrt( 1.0 - e * e ) * sin( E ) );
		return r;
	}
	
	// Compute the cartesian coordinates in "barycentric" space.  That is, the space of the massive object.
	Vec3d BarycentricPosition( double dist, double N, double i, double lon ) {
		Vec3d r;
		r.x = dist * ( cos( N ) * cos( lon ) - sin( N ) * sin( lon ) * cos( i ) );
		r.y = dist * ( sin( N ) * cos( lon ) + cos( N ) * sin( lon ) * cos( i ) );
		r.z = dist * ( sin( lon ) * sin( i ) );
		return r;
	}

	void ComputeMoonPerturbation( SolarSystem *ss ) {
		Elements & sun = ss->body[ SSB_Sun ];
		Elements & moon = ss->body[ SSB_Moon ];
		double Ms = sun.meanAnomaly;
		double Mm = moon.meanAnomaly;
		double Nm = moon.lonOfAscendingNode;
		double ws = sun.argumentOfPeriapsis;
		double wm = moon.argumentOfPeriapsis;
		double Ls = Ms + ws;                    // mean longitude of the sun
		double Lm = Mm + wm + Nm;               // mean longitude of the moon
		double D = Lm - Ls;						// mean elongation of the moon
		double F = Lm - Nm;						// argument of latitude for the moon
		double lonAdjust = 0.0;
		lonAdjust += -1.274 * sin( Mm - 2 * D );      // the Evection
		lonAdjust += +0.658 * sin( 2 * D );           // the Variation
		lonAdjust += -0.186 * sin( Ms );              // the Yearly Equation
		lonAdjust += -0.059 * sin( 2 * Mm - 2 * D ); 
		lonAdjust += -0.057 * sin( Mm - 2 * D + Ms );
		lonAdjust += +0.053 * sin( Mm + 2 * D );
		lonAdjust += +0.046 * sin( 2 * D - Ms );
		lonAdjust += +0.041 * sin( Mm - Ms );
		lonAdjust += -0.035 * sin( D );               // the Parallactic Equation
		lonAdjust += -0.031 * sin( Mm + Ms );
		lonAdjust += -0.015 * sin( 2 * F - 2 * D );
		lonAdjust += +0.011 * sin( Mm -  4 * D );
		moon.barycentricPosSph.z += ToRadians( lonAdjust );
		double latAdjust = 0.0;
		latAdjust += -0.173 * sin( F - 2 * D );
		latAdjust += -0.055 * sin( Mm - F - 2 * D );
		latAdjust += -0.046 * sin( Mm + F - 2 * D );
		latAdjust += +0.033 * sin( F + 2 * D );
		latAdjust += +0.017 * sin( 2 * Mm + F );
		moon.barycentricPosSph.y += ToRadians( latAdjust );
		double distAdjust = 0.0;
		distAdjust += -0.58 * cos( Mm - 2 * D );
		distAdjust += -0.46 * cos( 2 * D );
		moon.barycentricPosSph.x += distAdjust;
		moon.distance += distAdjust;
		// move spherical adjustments back to cartesian
		moon.barycentricPos = SphericalToCartesian( moon.barycentricPosSph );
	}
	
	void ComputeJupiterSaturnUranusPerturbation( SolarSystem *ss ) {
		Elements & jupiter = ss->body[ SSB_Jupiter ];
		Elements & saturn = ss->body[ SSB_Saturn ];
		Elements & uranus = ss->body[ SSB_Uranus ];
		double Mj = jupiter.meanAnomaly;
		double Ms = saturn.meanAnomaly;
		double Mu = uranus.meanAnomaly;

		// jupiter
		double jLonAdjust = 0.0;
		jLonAdjust += -0.332 * sin( 2 * Mj - 5 * Ms - ToRadians( 67.6 ) );
		jLonAdjust += -0.056 * sin( 2 * Mj - 2 * Ms + ToRadians( 21.0 ) );
		jLonAdjust += +0.042 * sin( 3 * Mj - 5 * Ms + ToRadians( 21.0 ) );
		jLonAdjust += -0.036 * sin( Mj - 2 * Ms );
		jLonAdjust += +0.022 * cos( Mj - Ms );
		jLonAdjust += +0.023 * sin( 2 * Mj - 3 * Ms + ToRadians( 52.0 ) );
		jLonAdjust += -0.016 * sin( Mj - 5 * Ms - ToRadians( 69.0 ) );
		jupiter.barycentricPosSph.z += ToRadians( jLonAdjust );
		jupiter.barycentricPos = SphericalToCartesian( jupiter.barycentricPosSph );

		// saturn
		double sLonAdjust = 0.0;
		sLonAdjust += +0.812 * sin( 2 * Mj - 5 * Ms - ToRadians( 67.6 ) );
		sLonAdjust += -0.229 * cos( 2 * Mj - 4 * Ms - ToRadians( 2.0 ) );
		sLonAdjust += +0.119 * sin( Mj - 2 * Ms - ToRadians( 3.0 ) );
		sLonAdjust += +0.046 * sin( 2 * Mj - 6 * Ms - ToRadians( 69.0 ) );
		sLonAdjust += +0.014 * sin( Mj - 3 * Ms + ToRadians( 32.0 ) );
		saturn.barycentricPosSph.z += ToRadians( sLonAdjust );
		double sLatAdjust = 0.0;
		sLatAdjust += -0.020 * cos( 2 * Mj - 4 * Ms - ToRadians( 2.0 ) );
		sLatAdjust += +0.018 * sin( 2 * Mj - 6 * Ms - ToRadians( 49.0 ) );
		saturn.barycentricPosSph.y += ToRadians( sLatAdjust );
		saturn.barycentricPos = SphericalToCartesian( saturn.barycentricPosSph );
		
		// uranus
		double uLonAdjust = 0.0;
		uLonAdjust += +0.040 * sin(Ms - 2*Mu + ToRadians( 6.0 ) );
		uLonAdjust += +0.035 * sin(Ms - 3*Mu + ToRadians( 33.0 ) );
		uLonAdjust += -0.015 * sin(Mj - Mu + ToRadians( 20.0 ) );
		uranus.barycentricPosSph.z += ToRadians( uLonAdjust );
		uranus.barycentricPos = SphericalToCartesian( uranus.barycentricPosSph );
	}
	
	void ComputeGeocentricPosition( SolarSystem *ss ) {
		Elements & sun = ss->body[ SSB_Sun ];
		sun.geocentricPos = sun.barycentricPos;
		ss->body[ SSB_Moon ].geocentricPos = ss->body[ SSB_Moon ].barycentricPos;
		for ( int i = SSB_Mercury; i < SSB_MAX; i++ ) {
			Elements & body = ss->body[i];
			body.geocentricPos = body.barycentricPos + sun.barycentricPos;
		}		
	}

	void ComputeEquatorialPosition( SolarSystem *ss ) {
		TimeLinear eclDeg = { 23.4393, -3.563E-7 }; 
		double ecl = ToRadians( eclDeg.Val( ss->day ) );
		Matrix3d m = Rotationd( Vec3d( 1, 0, 0 ), ecl ).GetMatrix3();
		
		for ( int i = 0; i < SSB_MAX; i++ ) {
			Elements & body = ss->body[i];
			Vec3d & g = body.geocentricPos;
			Vec3d & e = body.equatorialPos;
			e = m * g;
			body.equatorialPosSph = CartesianToSpherical( e );
		}		
	}
}

namespace star3map {
		
	//Elements solarSystemBodies[ SSB_MAX ];

	void Elements::Update( double newDay ) {
		const TimeLinearElements & se = storedElements[ which ];
		day = newDay;
		name = se.name;
		lonOfAscendingNode = ToRadians( Modulo( se.lonOfAscendingNode.Val( day ), 360.0 ) );
		inclination = ToRadians( Modulo( se.inclination.Val( day ), 360.0 ) );
		argumentOfPeriapsis = ToRadians( Modulo( se.argumentOfPeriapsis.Val( day ), 360.0 ) );
		semiMajorAxis = se.semiMajorAxis.Val( day );
		eccentricity = se.eccentricity.Val( day );
		meanAnomaly = ToRadians( Modulo( se.meanAnomaly.Val( day ), 360.0 ) );
		// derived
		eccentricAnomaly = EccentricAnomaly( meanAnomaly, eccentricity );
		ellipsePos = EllipsePosition( semiMajorAxis, eccentricAnomaly, eccentricity );
		trueAnomaly = atan2( ellipsePos.y, ellipsePos.x );
		distance = ellipsePos.Length();
		longitude = trueAnomaly + argumentOfPeriapsis;
		barycentricPos  = BarycentricPosition( distance, lonOfAscendingNode, inclination, longitude );
		barycentricPosSph = CartesianToSpherical( barycentricPos );
	}
	
	void SolarSystem::Update( double newDay ) {
		if ( day == newDay ) {
			return;
		}
		day = newDay;
		for ( int i = 0; i < SSB_MAX; i++ ) {
			body[i].which = SolarSystemBodyEnum( i );
			body[i].Update( day );
		}
		
		ComputeMoonPerturbation( this );
		ComputeJupiterSaturnUranusPerturbation( this );
		
		ComputeGeocentricPosition( this );
		ComputeEquatorialPosition( this );
		
	}
		
}


