/*
 *  localize - do string lookups using a json stream fetched from the server and issue 
 *             and issue fire-and-forget queries for strings not in the table for future updates
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

#include "localize.h"
#include "star3map.h"
#include "ujson.h"
#include "status.h"


#include "r3/filesystem.h"
#include "r3/http.h"
#include "r3/socket.h"
#include "r3/thread.h"
#include "r3/time.h"
#include "r3/output.h"
#include "r3/var.h"

#include <map>

using namespace std;
using namespace r3;
using namespace ujson;

VarString app_locale( "app_locale", "current locale", Var_Archive, "en_US" );


namespace {
	Mutex localizeMutex;
	map< string, bool > disabled;
	map< string, string> loc;
	double expiration;
	vector< string > requests;
	
	void ExtractLocFromJson( Json & j ) {
		loc.clear();
		if( j.GetType() != Json::Type_Object ) {
			return;
		}
        vector<string> name;
        j.GetMemberNames( name );
        for( size_t i = 0; i < name.size(); i++ ) {
            Json & jr = j(name[i]);
            if( jr.GetType() != Json::Type_String ) {
                continue;
            }
            loc[ name[i] ] = jr.s == "<null>" ? name[i] : jr.s;            
        }
	}
	
	void ReadLocalizationFile() {
        if( app_locale.GetVal() == "en_US" ) {
            return;
        }                        

		ScopedMutex scm( localizeMutex, R3_LOC );
		string file = app_locale.GetVal() + ".json";
		vector<uchar> data;
		if( FileReadToMemory( file, data ) == false ) {
			Output( "Unable to read localization file %s.", file.c_str() );
			return;
		} else {
			Output( "Read %d bytes from localization file %s", data.size(), file.c_str() );
		}
		if( data.size() < 20 ) {
			Output( "No localization data to read." );
			return;
		}
		Output( "About to parse localized strings." );
		Json * j = ujson::Decode( (const char *)&data[0], (int)data.size() );
		if( j == NULL || j->GetType() != Json::Type_Object ) {
			ujson::Delete( j );
			Output( "json root has wrong type" );
			return;
		}
        { Json & jr = (*j)("expiration"); if( jr.GetType() == Json::Type_Number ) { expiration = jr.n; } }
        ExtractLocFromJson( (*j)("dict") );
		
	}
	
	bool FetchLocalization() {
        if( app_locale.GetVal() == "en_US" ) {
            return false;
        }                        

		map< string, string > params;
		params["locale"] = app_locale.GetVal();
		string url = UrlBuildGet( "http://home.xyzw.us/star3map/localize.php", params );
		Output( "Fetching %s", url.c_str() );
		vector<uchar> data;
		if ( UrlReadToMemory( url, data ) )  {
			star3map::SetStatus( string(" Downloaded Localization" ).c_str() );
		}
		Output( "Fetched %d bytes from %s", data.size(), url.c_str() );
		
		if( data.size() > 20 ) {
			ScopedMutex scm( localizeMutex, R3_LOC );
			string locFile = app_locale.GetVal() + ".json";
			File *file = FileOpenForWrite( locFile );
			if( file != NULL ) {
				file->Write( &data[0], 1, (int)data.size() );
				delete file;
				return true;
			}
		}
		return false;
	}
	
	void RequestTranslation( const string & text ) {
        if( app_locale.GetVal() == "en_US" ) {
            // should really assert here
            return;
        }                        
		map< string, string > params;
		params["locale"] = app_locale.GetVal();
		params["key"] = text;
		string url = UrlBuildGet( "http://home.xyzw.us/star3map/localize.php", params );
		Output( "About to request translation for %s", text.c_str() );
		vector<uchar> data;
		if ( UrlReadToMemory( url, data ) )  {
			Output( "Executed %s", url.c_str() );
		}
	}
	
	struct LocalizeThread : public r3::Thread {
        LocalizeThread() : r3::Thread( "Localize" ) {}
		void Run() {
			if( FetchLocalization() ) {
				ReadLocalizationFile();				
			}
			while( 1 ) {
                condRender.Wait();
				string req = "";
				{
					ScopedMutex scm( localizeMutex, R3_LOC );
					if( requests.size() > 0 ) {
						req = requests.back();
						requests.pop_back();
					}
				}
				if( req.size() > 0 ) {
					RequestTranslation( req );
				}
				SleepMilliseconds( 1000 );
			}
		}
	};
	
	LocalizeThread localizeThread;
	
	bool initialized = false;	
	
	
}

namespace star3map {
	
	// read localized json stream from file, refresh copy from network if current one has expired
	void InitializeLocalize() {
		if ( initialized ) {
			return;
		}
		expiration = 0.0;
		disabled.clear();
		// need a different font for these glyphs
		//disabled[ "ar_AE" ] = true;
		//disabled[ "ar_AR" ] = true;
		//disabled[ "ar_EG" ] = true;
		//disabled[ "ar_KW" ] = true;
		//disabled[ "he_IL" ] = true;
		//disabled[ "iw_IL" ] = true;
		//disabled[ "ro_RO" ] = true; // some missing glyphs
		//disabled[ "th_TH" ] = true; // some missing glyphs
		
				 
		ReadLocalizationFile();
		localizeThread.Start();
		
	}
	
	// Returns key if no localization; issues fire-and-forget query if key not in local database
	string Localize( const string & key ) {
        if( app_locale.GetVal().substr( 0, 3 ) == "en_" ) {
            return key;
        }
		ScopedMutex scm( localizeMutex, R3_LOC );

		if( loc.count( key ) == 0 ) {			
			loc[ key ] = key;
			requests.push_back( key );
			Output( "Localize: %s", key.c_str() );
		}
		// when localized text rendering works, we'll turn this on
		if( disabled.count( app_locale.GetVal() ) == 0 ) {
			return loc[ key ]; 			
		}
		return key; 
	}
	
}


