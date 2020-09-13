/*
 *  starlist.cpp
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

#include "starlist.h"
#include "r3/filesystem.h"
#include "r3/output.h"
#include "r3/parse.h"
#include <math.h>
#include <stdio.h>

using namespace std;
using namespace r3;
using namespace ujson;

namespace {
    double DecodeNumber( Json & j ) {
        return j.GetType() == Json::Type_Number ? j.n : 0;
    }
    string DecodeString( Json & j ) {
        return j.GetType() == Json::Type_String ? j.s : "";
    }
}


namespace star3map {
    
    void DecodeStar( Json *j, Star & star ) {
        if( j == NULL || j->GetType() != Json::Type_Object ) {
            return;
        }
        Json & jr = *j;        
        star.name = DecodeString( jr( "name" ) ); 
        star.hipnum = (int) DecodeNumber( jr( "hip" ) );
        star.mag = (float) DecodeNumber( jr( "mag" ) );
        star.ra = (float) DecodeNumber( jr( "ra" ) );
        star.ra /= 12.0f;
        if ( star.ra > 1.0 ) {
            star.ra -= 2.0f;
        }
        star.ra *= 180.0f;
        
        star.dec = (float) DecodeNumber( jr( "dec" ) );
        star.colorIndex = (float) DecodeNumber( jr( "color" ) );
    }
    
	void IncrementLoadProgress( const string & text = string() );
		
	void ReadStarList( const string & filename, vector<Star> & list ) {
		list.clear();
        vector<unsigned char> buf;
        if( FileReadToMemory( filename, buf ) ) {
            Json * j = Decode( (const char *)&buf[0], (int)buf.size() );
            if( j && j->GetType() == Json::Type_Array ) {
                for( size_t i = 0; i < j->v.size(); i++ ) {
                    if( j->v[i] && j->v[i]->GetType() == Json::Type_Object ) {
                        Star s;
                        DecodeStar( j->v[i], s );
                        list.push_back( s );
                    }                    
                }
            }
            Delete( j );
		}
		Output( "Read %d stars from %s.", (int)list.size(), filename.c_str() );
	}

}
