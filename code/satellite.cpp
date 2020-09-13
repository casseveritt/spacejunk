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

#include "star3map.h"

#include "render.h"
#include "satellite.h"
#include "spacetime.h"
#include "status.h"

#include "sgp4/sgp4io.h"
#include "sgp4/sgp4unit.h"
#include "sgp4/sgp4ext.h"

#include "r3/filesystem.h"
#include "r3/http.h"
#include "r3/linear.h"
#include "r3/output.h"
#include "r3/thread.h"
#include "r3/time.h"
#include "r3/var.h"

using namespace std;
using namespace star3map;
using namespace r3;

extern VarBool app_showSatellites;
VarInteger app_satellitePathMaxTime( "app_satellitePathMaxSteps", "maximum length of satellite paths in seconds", 0, 15 * 60 );
VarInteger app_satellitePathTimeStep( "app_satellitePathTimeStep", "number of seconds per satellite path time step", 0, 5 );

extern VarString app_satelliteUrl;

namespace {
	r3::Mutex mutex;

	vector<uchar> specialTles;
	
	struct SatRecord {
		SatRecord() : special( false ), numErrors( 0 ) {
		}
		elsetrec orbitalElements;
		string name;
		bool special;
		int numErrors;		
	};

	bool satsLoaded;
	vector< SatRecord > satrec;
		
	struct SatellitePathThread : public r3::Thread {

		SatellitePathThread() : r3::Thread("SatellitePath"), currLL( 0, 0 ), newLL( 0, 0 ) {
		}
		
		void Run() {
			bool showSatellites = app_showSatellites.GetVal();
			Vec3f currViewerPos;
			bool filling;
			while( 1 ) {
                condRender.Wait();
				filling = false;
				{				
					ScopedMutex scmutex( mutex, R3_LOC );
					float deltaLL = ( currLL - newLL ).Length();
					if ( deltaLL > 0.01f || (int) paths.size() == 0 || showSatellites != app_showSatellites.GetVal() ) {
						//Output( "Updating viewer position - prev( %f, %f ), new( %f, %f ).", currLL.x, currLL.y, newLL.x, newLL.y );
						if ( deltaLL > 0.01f ) {
							Output( "Update reason: deltaLL = %f", deltaLL );							
						}
						//if ( paths.size() == 0 ) {
						//	Output( "Update reason: paths.size() == 0" );							
						//}
						if ( showSatellites != app_showSatellites.GetVal() ) {
							Output( "Update reason: app_showSatellites toggled" );														
						}
						currLL = newLL;
						showSatellites = app_showSatellites.GetVal();
						paths.clear();
						for ( int i = 0; i < (int)satrec.size(); i++ ) {
							SatellitePath path;
							path.name = satrec[i].name;
							path.special = satrec[i].special;
							paths.push_back( path );
						}
						currViewerPos = SphericalToCartesian( RadiusEarthKm, ToRadians( currLL.x ), ToRadians( currLL.y ) );
					}

					if ( showSatellites ) {
						
						Vec3f zenith = currViewerPos;
						zenith.Normalize();
						float minDot = cos( ToRadians( 65.f ) );
						float minDotSpecial = cos( ToRadians( 75.f ) );
						
						
						const int maxPath = app_satellitePathMaxTime.GetVal() / app_satellitePathTimeStep.GetVal();
						const int maxPathSpecial = 2 * maxPath;
						const double pathIncr = app_satellitePathTimeStep.GetVal() / 60.0;
						
						// snap mfe to a grid, so synchronization with other running copies of star3map is apparent 
						double mfe = floor( GetCurrentMinutesFromEpoch() / ( 2 * pathIncr ) ) * 2 * pathIncr;
						
						
						
						for ( int i = 0; i < (int)satrec.size(); i++ ) {
							SatRecord & sr = satrec[i];
							SatellitePath & path = paths[i];
							
							if ( sr.numErrors > 10 ) {
								if ( path.pathPoint.size() > 0 ) {
									path.pathPoint.clear();
								}
								continue;
							}
						
							int points = (int)path.pathPoint.size();
							int currMaxPath = sr.special ? maxPathSpecial : maxPath;
							float currMinDot = sr.special ? minDotSpecial : minDot;
							
							// remove all old PathPoints if the current time is past the furthest prediction
							if ( points > 4 && path.pathPoint.back().minutesFromEpoch < ( mfe - 2 * pathIncr ) ) {
								path.pathPoint.clear();
							}
							// otherwise remove at most one old PathPoint from each satellite on each iteration of the main thread loop
							else if ( points >= 4 && path.pathPoint[0].minutesFromEpoch < ( mfe - 2 * pathIncr ) ) {
								PathPoint & pp = path.pathPoint.front();
								if ( pp.aboveThreshold ) {
									path.aboveThresholdCount--;
								}							
								path.pathPoint.pop_front();
								path.pathPoint.pop_front();
								points -= 2;
							}
							
							// add at most one new PathPoint to each satellite on each iteration of the main thread loop
							// -- if the path size < maxPath or the last position is still high enough in the sky from
							//    the viewer position
							double nextTime;

							if ( points >= currMaxPath && path.pathPoint.back().aboveThreshold == false ) {
								continue;
							}		
							
							if ( points > 0 ) {
								nextTime = path.pathPoint[0].minutesFromEpoch + points * pathIncr; 
							} else {
								nextTime = mfe;
							}
							
							
							// definitely adding a new PathPoint to this satellite now
							double ro[3];
							double vo[3];
							elsetrec & srec = sr.orbitalElements;
							double minutesFromSatEpoch = nextTime - ( ( srec.jdsatepoch - JulianDateAtEpoch ) * MinutesPerDay );
							
							sgp4( wgs72, srec, minutesFromSatEpoch, ro, vo );
							float phaseEarthRot = GetThetaG( nextTime );
							Matrix4f phase = Rotationf( Vec3f( 0, 0, 1 ), -phaseEarthRot ).GetMatrix4();
							PathPoint pp;
							pp.minutesFromEpoch = nextTime;
							pp.pos = phase * Vec3f( ro[0], ro[1], ro[2] );
							Vec3f sdir = pp.pos - currViewerPos;
							sdir.Normalize();
							if ( sdir.Dot( zenith ) >= currMinDot ) {
								pp.aboveThreshold = true;
								path.aboveThresholdCount++;
							}
							path.pathPoint.push_back( pp );
							if( srec.error > 0 ) {
								Output( "Sat %s error %d at %lf mins from sat epoch. (Error count = %d.)", path.name.c_str(), srec.error, minutesFromSatEpoch, sr.numErrors );
								sr.numErrors++;
							}
							points++;
							if ( points >= currMaxPath ) {
								//Output( "Sat %s adding point (%d).", path.name.c_str(), points );								
							}
							
							filling = true; // since we added a new PathPoint this time through the main loop, sleep for less time
						}
					
						
					}
				}
				SleepMilliseconds( filling ? 50 : 1000 );
			}
				
		}
		
		void CopyPaths( vector< SatellitePath > & p ) {
			ScopedMutex scmutex( mutex, R3_LOC );
			p.clear();
			int sz = (int)paths.size();
			for ( int i = 0; i < sz; i++ ) {
				SatellitePath & path = paths[i];
				if ( path.aboveThresholdCount > 0 ) {
					p.push_back( path );
				}
			}
		}

		Vec2f currLL;
		Vec2f newLL;
		vector< SatellitePath > paths;
	};
	
	SatellitePathThread satPathThread;
	
	void ReadSatelliteFile( const std::string & filename ) {
		ScopedMutex scmutex( mutex, R3_LOC );
		satPathThread.paths.clear();
		
		if ( filename.size() == 0 ) {
			satrec.clear();
			return;
		}
		
		File *f = FileOpenForRead( "satellite_" + filename );
		if ( f ) {
			satrec.clear();
			vector<string> twoLineElements;
			while( f->AtEnd() == false ) {
				twoLineElements.push_back( f->ReadLine() );
			}
			delete f;
			Output( "ReadSatelliteData: Read %d lines from %s", (int)twoLineElements.size(), filename.c_str() );

			if( ( twoLineElements.size() % 3 ) > 0 ) {
				twoLineElements.pop_back();
			}
			
			if ( filename == "visual.txt" ) {
				string specialLine;
				for( int i = 0; i < specialTles.size(); i++ ) {
					if ( specialTles[i] == '\n' ) {
						if ( specialLine.size() > 1 ) {
							twoLineElements.push_back( specialLine );
						}
						specialLine = "";
					} else {
						specialLine.push_back( specialTles[i] );					
					}
				}				
			}
			
			double startmfe, stopmfe, deltamin; // dummies
			for ( int i = 2; i < (int)twoLineElements.size(); i+=3 ) {
				string s0 = twoLineElements[ i-2 ];
				int last = (int)s0.size() - 1;
				char lc = s0[ last ];
				while( last >= 0 && ( lc == ' ' || lc == '\t' || lc == 0 || lc == 13 ) ) {
					last--;
					lc = s0[last];
				}
				last++;
				s0 = s0.substr( 0, last );
				
				char s1[130], s2[130];
				strncpy( s1, twoLineElements[i-1].c_str(), 130 );
				strncpy( s2, twoLineElements[i-0].c_str(), 130 );
				
				SatRecord sr;
				sr.name = s0;
				// do a few common name translations
				if ( sr.name == "HST" ) {
					sr.name = "Hubble Space Telescope";
					sr.special = true;
				} else if ( sr.name == "ISS (ZARYA)" ) {
					sr.name = "International Space Station";
					sr.special = true;
				} else if ( sr.name.substr( 0, 3 ) == "STS" ) {
					sr.name = "Space Shuttle";
					sr.special = true;
				} else if ( sr.name == "NANOSAILD" ) {
					sr.name = "NanoSail-D";
					sr.special = true;
				} else if ( sr.name == "FASTSAT" ) {
					sr.name = "FastSat";
					sr.special = true;
				}
				
				twoline2rv( s1, s2, 'm', 'm', 'i', wgs72, startmfe, stopmfe, deltamin, sr.orbitalElements );
				double ro[3];
				double vo[3];
				sgp4( wgs72, sr.orbitalElements, 0.0, ro, vo );
				satrec.push_back( sr );
			}
			
			string satList;
			if ( filename == "visual.txt" ) {
				satList = "brightest";
			}
			if ( filename == "amateur.txt" ) {
				satList = "amateur radio";
			}
			if ( filename == "iridium.txt" ) {
				satList = "Iridium Constellation";
			}
			char buf[200];
			r3Sprintf( buf, "Loaded %d %s satellites.", (int)satrec.size(), satList.c_str() );
			Output( "Loaded %d %s satellites.", (int)satrec.size(), satList.c_str() );
			SetStatus( buf );
			
		}
	}
	
	struct SatelliteReadThread : public r3::Thread {
        SatelliteReadThread() : r3::Thread( "SatelliteRead" ) {}

		void ReadSpecials() {
			string specialTlesUrl = "http://home.xyzw.us/star3map/special_tles.txt";
			Output( "About to fetch... special TLEs" );
			if ( UrlReadToMemory( specialTlesUrl, specialTles ) )  {
				SetStatus( string(" Fetched: special TLEs" ).c_str() );
				Output( "Fetched... special TLEs" );
			}			
		}
		
		string UrlToFilename( const string & url ) {			
			string s = url.substr( url.rfind('/') ).substr( 1 );
			return s;
		}
		
		void Run() {
			string satelliteUrl;
			while( 1 ) {
                condRender.Wait();
				if( satelliteUrl != app_satelliteUrl.GetVal() ) {
					satelliteUrl = app_satelliteUrl.GetVal();
					string satelliteFile = UrlToFilename( satelliteUrl );
					File *f = FileOpenForRead( "satellite_" + satelliteFile );
					string fullUrl = satelliteUrl;
					bool fetch = false;
					if( fullUrl.find( ".php" ) != string::npos ) {
#if HAS_FACEBOOK
					    if( app_fbId.GetVal().size() > 0 ) {
							fullUrl += "?id=";
							fullUrl += app_fbId.GetVal();
							fetch = true;
						} else {
							ReadSatelliteFile("");
							continue;
						}
#else
						ReadSatelliteFile("");
						continue;
#endif
					} 
					fetch = fetch || f == NULL;
					if ( f ) {
						double age = ( GetTime() - GetTimeOffset() ) - f->GetModifiedTime() ;
						Output( "Reading %s - age %f hours", satelliteFile.c_str(), float( age / 3600.0 ) );
						fetch = fetch || age > 3600.0; // fetch if the file is over an hour old
						delete f;
						// if the file exists, go ahead and read this one first, even if it's stale
						ReadSpecials();
						ReadSatelliteFile( satelliteFile );
					}
					if ( fetch ) {
						vector<uchar> data;
						Output( "About to fetch... %s", fullUrl.c_str() );
						if ( UrlReadToMemory( fullUrl, data ) )  {
							ScopedMutex scmutex( mutex, R3_LOC );
							File *f = FileOpenForWrite( "satellite_" + satelliteFile );
							if ( f ) {
								f->Write( &data[0], 1, (int)data.size() );
								delete f;
							} else {
								Output( "FileOpenForWrite() failed!");
							}				
							SetStatus( string("Fetched: " + satelliteUrl ).c_str() );
							Output( "Fetched... %s", satelliteUrl.c_str() );
						}
						ReadSpecials();
						SleepMilliseconds( 500 );
						ReadSatelliteFile( satelliteFile );
					}
				}
				SleepMilliseconds( 100 );				
			}
		}
	};
	
	SatelliteReadThread satReadThread;
	
	bool initialized = false;	
		
}

namespace star3map {

	void InitializeSatellites() {
		if ( initialized ) {
			return;
		}
		
		InitializeSpaceTime();		
		satPathThread.Start();
		satReadThread.Start();
		initialized = true;
	}
	
	bool SatellitesLoaded() {
		satsLoaded = satsLoaded || satrec.size() > 0;
		return satsLoaded;
	}
	
	void ComputeSatellitePositions( std::vector<Satellite> & satellites ) {
		satellites.clear();
		double mfe = GetCurrentMinutesFromEpoch();
		ScopedMutex scmutex( mutex, R3_LOC );
		
		float phaseEarthRot = GetThetaG( mfe );
		Matrix4f phase = Rotationf( Vec3f( 0, 0, 1 ), -phaseEarthRot ).GetMatrix4();
	
		int sz = (int)satrec.size();		
		
		for ( int i = 0; i < sz; i++ ) {
			SatRecord sr = satrec[i];
			if( sr.numErrors > 10 ) {
				continue;
			}
			Satellite sat;
			sat.name = sr.name;
			sat.id = (int)sr.orbitalElements.satnum;
			sat.special = sr.special;
			double ro[3];
			double vo[3];
			elsetrec & srec = sr.orbitalElements;
			double minutesFromSatEpoch = mfe - ( ( srec.jdsatepoch - JulianDateAtEpoch ) * 1440.0 );
			sgp4( wgs72, srec, minutesFromSatEpoch, ro, vo );
			if ( srec.error > 0 ) {
				Output( "Sat %s error %d at %lf mins from sat epoch. (Error count = %d.)", sat.name.c_str(), srec.error, minutesFromSatEpoch, sr.numErrors );
				sr.numErrors++;
			}
			sat.pos = phase * Vec3f( ro[0], ro[1], ro[2] );
			satellites.push_back( sat );
		}
	}
	
	void GetSatelliteFlyovers( float lat, float lon, std::vector<SatellitePath> & paths ) {
		satPathThread.CopyPaths( paths );
		Vec2f ll( lat, lon );
		if ( ll != satPathThread.newLL ) {
			ScopedMutex scmutex( mutex, R3_LOC );
			satPathThread.newLL = ll;
		}
	}
	
}


