/*
 *  transient
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

#include "render.h"
#include "drawstring.h"

#include <GL/Regal.h>

#include "r3/common.h"
#include "r3/draw.h"
#include "r3/output.h"
#include "r3/time.h"
#include "r3/var.h"

#include <map>

using namespace std;
using namespace star3map;
using namespace r3;

VarBool app_pauseAging( "app_pauseAging", "stop aging for dynamic objects - usually for screenshots", 0, false );

namespace {

	const float RampUpTime = .5f;
	const float RampDownTime = 1.0f;
	
	struct DynamicRenderable {
		enum DState {
			DState_Invalid,
			DState_RampUp,
			DState_Hold,
			DState_RampDown,
			DState_Terminate,
			DState_MAX
		};
		DynamicRenderable() {}
		DynamicRenderable( const Vec3f & lDir, const Vec3f & lLookDir, float lLimit, const Vec4f & lColor, float lDuration ) 
		: direction( lDir ), lookDir( lLookDir ), limit( lLimit ), color( lColor ), duration( lDuration ), state( DState_RampUp ) {
			timeStamp = GetSeconds();
			currAlpha = 0.0f;
		} 
		Vec3f direction;
		Vec3f lookDir;
		float limit;
		Vec4f color;
		float currAlpha;
		float duration;
		DState state;
		float timeStamp;
		float lastSeen;
		void age() {
			float currTime = GetSeconds();
			float delta = currTime - timeStamp;
			switch ( state ) {
				case DState_RampUp:
					if ( delta > RampUpTime ) {
						timeStamp += RampUpTime;
						state = DState_Hold;
						delta = RampUpTime;
					}
					currAlpha = ( delta / RampUpTime ) * color.w;
					break;
				case DState_Hold:
					if ( delta > duration ) {
						timeStamp += duration;
						state = DState_RampDown;
					}
					break;
				case DState_RampDown:
					if ( delta > RampDownTime ) {
						timeStamp += RampUpTime;
						state = DState_Terminate;
						delta = RampDownTime;
					}
					currAlpha = ( 1.0f - delta / RampDownTime ) * color.w;
					break;
				case DState_Terminate:
					break;
				default:
					Output( "Something has gone wrong." );
					break;
			}
		}
		void seen() {
			lastSeen = GetSeconds();
		}
	};
	
	struct DynamicLabel : public DynamicRenderable {
		DynamicLabel() {}
		DynamicLabel( const string & lName, const Vec3f & lDir, const Vec3f & lLookDir, float lLimit, const Vec4f & lColor, float lDuration ) 
		: DynamicRenderable( lDir, lLookDir, lLimit, lColor, lDuration ), name( lName ) {
			timeStamp = GetSeconds();
			currAlpha = 0.0f;
		} 
		string name;
		void render( DrawNonOverlappingStrings * nos ) {
			if ( state != DState_Terminate ) {
				glColor4f( color.x, color.y, color.z, currAlpha );
				nos->DrawString( name, direction, lookDir, limit );
			}
		}
	};
	
	map< string, DynamicLabel > dynamicLabels;
		
	struct DynamicLines : public DynamicRenderable {
		DynamicLines() {}
		DynamicLines( Lines *lLines, const Vec3f & lDir, const Vec3f & lLookDir, float lLimit, const Vec4f lColor, float lDuration ) 
		: DynamicRenderable( lDir, lLookDir, lLimit, lColor, lDuration ), lines( lLines ) {
			timeStamp = GetSeconds();
			currAlpha = 0.0f;
		} 
		Lines *lines;
		void render() {
			if ( state != DState_Terminate ) {
				glColor4f( color.x, color.y, color.z, currAlpha );
				glBegin( GL_LINES );
				for( int j = 0; j < (int)lines->vert.size(); j++ ) {
					Vec3f & v = lines->vert[ j ];
					glVertex3f( v.x, v.y, v.z );
				}
				glEnd();
			}
		}
	};
	
	map< Lines *, DynamicLines > dynamicLines;
	
}




namespace star3map {
		
    void AgeDynamicLabels();
	void AgeDynamicLabels() {
		if ( app_pauseAging.GetVal() ) {
			return;
		}
		map< string, DynamicLabel > oldLabels = dynamicLabels;
		dynamicLabels.clear();
		map< string, DynamicLabel >::iterator it;
		int count = 0;
		float currTime = GetSeconds();
		for ( it = oldLabels.begin(); it != oldLabels.end(); ++it ) {
			DynamicLabel & dl = it->second;
			dl.age();
			if ( dl.state != DynamicLabel::DState_Terminate || ( currTime - dl.lastSeen ) < 1.0f ) {
				dynamicLabels[ dl.name ] = dl;
			}
			count++;
		}
	}
	void DrawDynamicLabels( DrawNonOverlappingStrings * nos );	
	void DrawDynamicLabels( DrawNonOverlappingStrings * nos ) {
		map< string, DynamicLabel >::iterator it;
		for ( it = dynamicLabels.begin(); it != dynamicLabels.end(); ++it ) {
			DynamicLabel & dl = it->second;
			dl.render( nos );
		}
	}
	void AgeDynamicLines();	
	void AgeDynamicLines() {
		if ( app_pauseAging.GetVal() ) {
			return;
		}		
		map< Lines *, DynamicLines > oldLines = dynamicLines;
		dynamicLines.clear();
		map< Lines *, DynamicLines >::iterator it;
		int count = 0;
		float currTime = GetSeconds();
		for ( it = oldLines.begin(); it != oldLines.end(); ++it ) {
			DynamicLines & dl = it->second;
			dl.age();
			if ( dl.state != DynamicLabel::DState_Terminate || ( currTime - dl.lastSeen ) < 1.0f ) {
				dynamicLines[ dl.lines ] = dl;
			}
			count++;
		}
	}
	
	void DrawDynamicLines();
    void DrawDynamicLines() {
		map< Lines *, DynamicLines >::iterator it;
		for ( it = dynamicLines.begin(); it != dynamicLines.end(); ++it ) {
			DynamicLines & dl = it->second;
			dl.render();
		}
	}
	
	void DynamicLinesInView( Lines * lines, Vec4f c, Vec4f cl, Vec3f lookDir );
    void DynamicLinesInView( Lines * lines, Vec4f c, Vec4f cl, Vec3f lookDir ) {
		if ( dynamicLines.count( lines ) == 0 ) {
			DynamicLines dl( lines, lines->center, lookDir, 0, c, 4.0f );
			dynamicLines[ lines ] = dl;
		} else {
			dynamicLines[ lines ].seen();
		}		
		if ( dynamicLabels.count( lines->name ) == 0 ) {
			DynamicLabel dl( lines->name, lines->center, lookDir, 0, cl, 2.5f );
			dynamicLabels[ dl.name ] = dl;					
		} else {
			dynamicLabels[ lines->name ].seen();
		}
	}
	
	void DynamicLabelInView( DrawNonOverlappingStrings *nos, const string & label, Vec3f dir, Vec4f c, Vec3f lookDir, float limit );
 	void DynamicLabelInView( DrawNonOverlappingStrings *nos, const string & label, Vec3f dir, Vec4f c, Vec3f lookDir, float limit ) {
		if ( dynamicLabels.count( label )  == 0) {
			if ( nos->CanDrawString( label, dir, lookDir, limit ) ) {
				DynamicLabel dl( label, dir, lookDir, limit, c, 1.0f );
				dynamicLabels[ label ] = dl;
			}
		} else {
			dynamicLabels[ label ].seen();
		}
		
	}
	
}


