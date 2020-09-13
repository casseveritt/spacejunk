/*
 *  drawstring
 */

/* 
 Copyright (c) 2010-2011 Cass Everitt
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

#include "drawstring.h"
#include "render.h"

#include "r3/common.h"
#include "r3/bounds.h"
#include "r3/draw.h"
#include "r3/output.h"
#include "r3/var.h"

#include <GL/Regal.h>

#include <vector>
#include <map>

using namespace std;
using namespace star3map;
using namespace r3;

extern VarFloat r_fov;
extern VarInteger r_windowWidth;
extern VarInteger r_windowHeight;
extern VarInteger r_windowDpi;

VarInteger app_debugLabels( "app_debugLabels", "draw debugging info for labels", 0, 0 );

namespace {
		
	struct DrawNonOverlappingStringsImpl : public star3map::DrawNonOverlappingStrings {
		vector< OrientedBounds2f > reserved;
		vector< OrientedBounds2f > obs;
		
		void ClearReservations() {
			reserved.clear();
		}
		
		bool CanDrawString( const string & str, const Vec3f & direction, const Vec3f & lookDir, float limit ) {
			if ( lookDir.Dot( direction ) < limit ) {
				return false;
			}
			OrientedBounds2f ob = StringBounds( str, direction );
			for ( int i = 0; i < (int)obs.size(); i++ ) {
				if ( Intersect( ob, obs[ i ] ) ) {
					return false; // intersected, so don't draw this one
				}
			}
			return true;
		}
		
		void ReserveString( const string & str, const Vec3f & direction, const Vec3f & lookDir, float limit ) {
			if ( lookDir.Dot( direction ) < limit ) {
				return;
			}
			OrientedBounds2f ob = StringBounds( str, direction );
			reserved.push_back( ob );
		}

		void DrawString( const string & str, const Vec3f & direction, const Vec3f & lookDir, float limit ) {
			if ( lookDir.Dot( direction ) < limit ) {
				return;
			}
			
			OrientedBounds2f ob = StringBounds( str, direction );
			if ( ob.empty ) {
				return;
			}
				
			for ( int i = 0; i < (int)obs.size(); i++ ) {
				if ( Intersect( ob, obs[ i ] ) ) {
					return; // intersected, so don't draw this one
				}
			}
			for ( int i = 0; i < (int)reserved.size(); i++ ) {
				if ( Intersect( ob, reserved[ i ] ) ) {
					return; // intersected, so don't draw this one
				}
			}
			obs.push_back( ob );
			::DrawString( str, direction );
			if ( app_debugLabels.GetVal() ) {
				if ( app_debugLabels.GetVal() > 1 ) {
					float len = 0;
					for ( int i = 0; i < 4; i++ ) {
						len = max( len, ( ob.vert[ i ] - ob.vert[ (i+1)%4 ] ).Length() );
					}
					if ( len > 10 ) {
						Output( "bounds %d %s: ( %f, %f ), ( %f, %f ), ( %f, %f ), ( %f, %f )",
							   (int)obs.size(), str.c_str(),
							   ob.vert[0].x, ob.vert[0].y,
							   ob.vert[1].x, ob.vert[1].y,
							   ob.vert[2].x, ob.vert[2].y,
							   ob.vert[3].x, ob.vert[3].y );					
					}
				}
                glMatrixPushEXT( GL_MODELVIEW );
                glMatrixLoadIdentityEXT( GL_MODELVIEW );
                glColor4f( 1, 1, 0, 1 );

                glBegin( GL_LINE_STRIP );
				glVertex3fv( ob.vert[0].Ptr() );
				glVertex3fv( ob.vert[1].Ptr() );
				glVertex3fv( ob.vert[2].Ptr() );
				glVertex3fv( ob.vert[3].Ptr() );
				glVertex3fv( ob.vert[0].Ptr() );
                glEnd();
                glMatrixPopEXT( GL_MODELVIEW );
			}
		}
	};
		
}




namespace star3map {
	DrawNonOverlappingStrings * CreateNonOverlappingStrings() {
		return new DrawNonOverlappingStringsImpl();
	}
}


