/*
 *  star3map
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
#include "satellite.h"
#include "spacetime.h"
#include "drawstring.h"
#include "localize.h"
#include "ui/menubar.h"
#include "ntp.h"
#include "status.h"
#include "transient.h"
#include "solarsystem.h"

#include "r3/command.h"
#include "r3/common.h"
#include "r3/console.h"
#include "r3/draw.h"
#include "r3/filesystem.h"
#include "r3/gfxcontext.h"
#include "r3/image.h"
#include "r3/model.h"
#include "r3/modelobj.h"
#include "r3/output.h"
#include "r3/shader.h"
#include "r3/thread.h"
#include "r3/time.h"

#include "r3/var.h"

#include "ujson.h"

#include <GL/Regal.h>

#include <map>

using namespace std;
using namespace star3map;
using namespace r3;

extern void GfxSwapBuffers();
extern void GfxCheckErrors();
extern void AsyncInitMisc();

extern VarFloat r_fov;
extern VarInteger r_windowWidth;
extern VarInteger r_windowHeight;
extern VarInteger r_windowDpi;

extern VarFloat app_latitude;
extern VarFloat app_longitude;
extern VarFloat app_phaseEarthRotation;
extern VarBool app_cull;

extern VarFloat app_ntpTimeOffset;

enum AttrLocations {
  AL_Position = 0,
  AL_Color = 1,
  AL_TexCoord = 2
};

#if ANDROID
VarBool app_license( "app_license", "allow licensed mode", 0, 0 );
#else
VarBool app_license( "app_license", "allow licensed mode", 0, 1 );
#endif
VarBool app_useCompass( "app_useCompass", "use the compass for pointing the phone around the sky", 0, false );
VarBool app_changeLocation( "app_changeLocation", "change current location when moving in globeView", 0, false );
VarBool app_useCoreLocation( "app_useCoreLocation", "use the location services to get global position", 0, false );
VarBool app_showSatellites( "app_showSatellites", "use TLE satellite data to show satellites", 0, true );
VarString app_satelliteUrl( "app_satelliteUrl", "url to use for satellite data", 0, "http://www.celestrak.com/NORAD/elements/visual.txt" );
VarInteger app_maxSatellites( "app_maxSatellites", "maximum number of satellites to display", 0, 40 );
VarFloat app_inputDrag( "app_inputDrag", "drag factor on input for inertia effect", 0, .9 );
VarInteger app_loadProgress( "app_loadProgress", "indicator of loading progress", 0, 0 );
VarInteger app_loadProgressFinal( "app_loadProgressFinal", "largest value of app_loadProgress", Var_Archive, 100 );
VarBool app_nightViewing( "app_nightViewing", "disable green and blue channels when viewing at night", 0, false );

VarFloat app_debugPhase( "app_debugPhase", "phase adjustment", 0, 0.0f );

VarBool app_showStars( "app_showStars", "draw stars", 0, true );
VarBool app_showConstellations( "app_showConstellations", "draw constellations", 0, true );
VarBool app_showPlanets( "app_showPlanets", "draw planets", 0, true );
VarBool app_showGlobe( "app_showGlobe", "draw globe (in globe view)", 0, true );
VarBool app_showDirections( "app_showDirections", "show direction labels", 0, true );
VarBool app_showLabels( "app_showLabels", "draw labels", 0, true );
VarBool app_showHemisphere( "app_showHemisphere", "draw hemisphere", 0, true );
VarBool app_showQR( "app_showQR", "Show the QR code for the app.", 0, false );

Matrix4f Projection;
Matrix4f ModelView;

extern VarFloat app_scale;
extern VarFloat app_starScale;

r3::Condition condRender;

float textDepthBias;

bool GotCompassUpdate = false;
bool GotLocationUpdate = false;

#if APP_spacejunklite
int proTimeLeft = 0;
const int proTimeWindow = 300;
const int proTimeDuty = 60;
#endif

extern void UpdateLatLon();
extern Matrix4f platformOrientation;
Matrix4f orientation;
Matrix4f manualOrientation;

VarFloat app_manualPhi( "app_manualPhi", "phi for manual orientation", 0, 0 );
VarFloat app_manualTheta( "app_manualTheta", "theta for manual orientation", 0, 0 );

enum AsyncLoadEnum {
    AsyncLoad_Synchronous,
    AsyncLoad_SingleContext,
    AsyncLoad_MultiContext
};

VarBool app_fbLoggedIn( "app_fbLoggedIn", "is the user authenticated with facebook", Var_ReadOnly, false );
VarString app_fbId( "app_fbId", "id of the authenticated facebook user", Var_Archive, "" );
VarString app_fbUser( "app_fbUser", "first name of the authenticated facebook user", Var_ReadOnly, "Person" );
VarString app_fbGender( "app_fbGender", "gender of authenticated facebook user", Var_ReadOnly, "male" );

// still a lurking bug on this on iPhone...
#if ANDROID
#define PLATFORM_STRING "Android"
VarInteger app_asyncLoad( "app_asyncLoad", "enable asynchronous loading", 0, AsyncLoad_Synchronous );
#elif __APPLE__
# if IPHONE
#  define PLATFORM_STRING "iOS"
# else
#  define PLATFORM_STRING "MacOS"
# endif
// multi-context is busted on iOS and MacOS, but the bug is probably mine...
VarInteger app_asyncLoad( "app_asyncLoad", "enable asynchronous loading", 0, AsyncLoad_SingleContext ); 
#else
#define PLATFORM_STRING "Windows"
VarInteger app_asyncLoad( "app_asyncLoad", "enable asynchronous loading", 0, AsyncLoad_SingleContext );
#endif
vector< Sprite > stars;
vector< Sprite > solarsystem;
vector< Lines > constellations;
MenuBar menu;

double frameBeginTime;
double frameEndTime;

namespace {
    double t0 = 0.0;
}

namespace star3map {
    
	GfxContext *drawContext;
	GfxContext *loadContext;
	
	Matrix4f RotateTo( const Vec3f & direction );
	
	void IncrementLoadProgress( const string & text = string() );
	string loadingString;
	void DisplayInitializing();
    
	void IncrementLoadProgress( const string & text ) {
		app_loadProgress.SetVal( app_loadProgress.GetVal() + 1 );
        Output( "Load Progress (%d): %s", app_loadProgress.GetVal(), text.c_str() );
		if ( text.size() > 0 ) {
			loadingString = text;
		}
		if ( app_asyncLoad.GetVal() == AsyncLoad_Synchronous ) {
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
			DisplayInitializing();
		} else {
			GfxContext *ctx = app_asyncLoad.GetVal() == AsyncLoad_MultiContext ? loadContext : drawContext;
			ctx->Finish();
			ctx->Release();
			ctx->Acquire();
		}
	}
}


#if IPHONE
extern void OpenURL( const char *url );
#elif ANDROID
extern bool appOpenUrl( const char *url ); 
void OpenURL( const char *url) {
	appOpenUrl( url );
}
#else
void OpenURL( const char *);
void OpenURL( const char *url) {
}
#endif

namespace {
    
    map< string, Texture * > tex;
    map< string, Shader * > shd;
    map< string, Model * > mod;
    map< string, Button * > btn;
    
	vector<Satellite> satellite;
	vector<SatellitePath> satPath;
	SolarSystem ss;
    
    
    
	enum EnumSightingObjectType {
		SOT_Invalid,
		SOT_Satellite,
		SOT_Constellation,
		SOT_Star,
		SOT_Planet,
		SOT_MAX
	};
	
	struct Sighting {
		Sighting() : id( 0 ) {}
		int GetType() const {
			return id & 0xff;
		}
		void SetType( int type ) {
			id &= ~0xff;
			id |= ( type & 0xff );
		}
		int GetNum() const {
			return id >> 8;
		}
		void SetNum( int num ) {
			id &= 0xff;
			id |= ( num << 8 );
		}
		
		int id;
		string name;
		double timestamp;
	};
	
	double sightingsCleanupTimestamp;
	map< int, Sighting > sightings;
    
	void CleanupSightings() {
		if ( ( frameBeginTime - sightingsCleanupTimestamp ) > 300.0 ) {
			vector<int> killList;
			for ( map<int,Sighting>::iterator it = sightings.begin(); it != sightings.end(); ++it ) {
				Sighting & s = it->second;
				if ( ( frameBeginTime - s.timestamp ) > 7200.0 ) {
					killList.push_back( s.id );
				}
			}
			for ( int i = 0; i < killList.size(); i++ ) {
				sightings.erase( killList[ i ] );
			}
			sightingsCleanupTimestamp = frameBeginTime;
		}
	}
	
	
	enum AppModeEnum {
		AM_INVALID,
		AM_Uninitialized,
		AM_Initializing,
		AM_Initialized,
		AM_ViewStars,
		AM_ViewGlobe,
		AM_MAX
	};
	
	AppModeEnum appMode = AM_Uninitialized;
	
	void UpdateManualOrientation();
	
	struct SatelliteSorter {
		
		SatelliteSorter( const vector< Satellite > & satelliteList ) : satList( satelliteList ), lastMaxSatellites( 0 ), num( 0 ) {
		}
		
		struct Comp {
			const vector< Satellite > & satList;
			Vec3f origin;
			Comp( const vector< Satellite > & inSatList, const Vec3f & inOrigin ) : satList( inSatList ), origin( inOrigin ) {
			}
			bool operator() ( int a, int b ) {
				float aspecial = satList[a].special ? 0.0f : 1.0f;
				float bspecial = satList[b].special ? 0.0f : 1.0f;
				float la = ( satList[a].pos - origin ).Length() * aspecial;
				float lb = ( satList[b].pos - origin ).Length() * bspecial;
				return la < lb;
			}
		};
		
		void Sort( const Vec3f & inOrigin ) {
			
			if ( indexes.size() != satList.size() ) {
				indexes.resize( satellite.size() );				
				for ( int i = 0; i < (int)indexes.size(); i++ ) {
					indexes[i] = i;
				}
				lastSortTime = 0.0; // force a re-sort
			}
            if ( app_maxSatellites.GetVal() != lastMaxSatellites ) {
                lastMaxSatellites = app_maxSatellites.GetVal();
                lastSortTime = 0.0; // force a re-sort
            }
            
			double now = GetTime();
            if ( inOrigin == origin && ( now - lastSortTime ) < 5.0 ) {
				return;
			}
            
			lastSortTime = now;
			origin = inOrigin;			
			sort( indexes.begin(), indexes.end(), Comp( satList, origin ) );		
			
			num = min( app_maxSatellites.GetVal(), (int)satList.size() );			
		}
		
		int Count() {
			return num;
		}
		
		int operator[] ( int i ) {
			return indexes[i];
		}
		
		const vector< Satellite > & satList;
		vector<int> indexes;
		double lastSortTime;
        int lastMaxSatellites;
		Vec3f origin;
		int num;
	};
	SatelliteSorter satSorter( satellite );
	
	struct StarVert {
		Vec3f pos;
		uchar c[4];
		Vec2f tc;
	};
	struct EarthVert {
		Vec3f pos;
	};
	
	float magToDiam[] = { 2.0f, 1.5f, 1.25f, 1.0f, .85f, .75f, .5f };
	
	float GetSpriteDiameter( float magnitude ) {
		int mag = max( 0, min( 6, (int)magnitude ) );
		return magToDiam[ mag ];
	}
	
	float magToColor[] = { 1.0f, .9f, .7f, .5f, .3f, .2f, .1f };
	
	float GetSpriteColorScale( float magnitude ) {
		int mag = max( 0, min( 6, (int)magnitude ) );
		return magToColor[ mag ];
	}
	
	void InitStarsModel() {
		{
			Model * m = mod["stars"] = new Model( "stars" );
			vector< StarVert > data;
			for ( int i = 0; i < (int)stars.size(); i++ ) {
				Sprite & s = stars[i];
				if ( s.magnitude > 4 ) {
					continue;
				}
				Vec4f c = s.color * GetSpriteColorScale( s.magnitude ) * 255.f;
				Matrix4f mRot = RotateTo( s.direction );
				Matrix4f mScale;
				float sc = s.scale * GetSpriteDiameter( s.magnitude ) * app_starScale.GetVal() * app_scale.GetVal();
				mScale.SetScale( Vec3f( sc, sc, 1 ) );
				Matrix4f mTrans;
				mTrans.SetTranslate( Vec3f( 0, 0, -1 ) );
				Matrix4f m = mRot * mScale * mTrans;
				StarVert v;
				v.c[0] = c.x; v.c[1] = c.y; v.c[2] = c.z; v.c[3] = c.w;
				v.pos = m * Vec3f( -10, -10, 0 );
				v.tc = Vec2f( 0, 0 );
				data.push_back( v );
				v.pos = m * Vec3f(  10, -10, 0 );
				v.tc = Vec2f( 1, 0 );
				data.push_back( v );
				v.pos = m * Vec3f(  10,  10, 0 );
				v.tc = Vec2f( 1, 1 );
				data.push_back( v );
				v.pos = m * Vec3f( -10,  10, 0 );
				v.tc = Vec2f( 0, 1 );
				data.push_back( v );				
			}
			m->GetVertexBuffer().SetData( (int)data.size() * sizeof( StarVert ), & data[0] );
            int offset = 0;
            m->AddAttributeArray( AttributeArray( GL_VERTEX_ARRAY, 3, GL_FLOAT, GL_FALSE, sizeof( StarVert ), offset ) );
            offset += sizeof( Vec3f );
            m->AddAttributeArray( AttributeArray( GL_COLOR_ARRAY , 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( StarVert ), offset )  );
            offset += 4;
            m->AddAttributeArray( AttributeArray( GL_TEXTURE0    , 2, GL_FLOAT, GL_FALSE, sizeof( StarVert ), offset ) );
            m->SetNumVertexes( data.size() );
			m->SetPrimitive( GL_QUADS );
		}
		IncrementLoadProgress();
	}
	
	void UpdateSolarSystemSprites() {
		ss.Update( GetCurrentSolarDayNumber() );
		for( int i = 0; i < SSB_MAX; ++i ) {
			Sprite & s = solarsystem[i];
			Elements & e = ss.body[ i ];
			Vec3d pos = e.equatorialPos; // e.geocentricPos;
			pos.Normalize();
			s.direction = Vec3f( pos.x, pos.y, pos.z );
		}
	}
	
	void InitSolarSystemSprites() {
		ss.Update( GetCurrentSolarDayNumber() );
		
		float scale[] = { 6, 5, 2, 2, 2, 4, 6, 2, 2, 2 };
		
		for( int i = 0; i < SSB_MAX; ++i ) {
			Sprite s;
			s.name = ss.body[i].name;
			std::string texname;
			for ( int j = 0; j < (int)s.name.size(); j++ ) {
				texname.push_back( tolower( s.name[ j ] ) );
			}
            s.tex = static_cast<Texture2D *>( tex[ texname ] );
			s.scale = 1.0f;
			s.color = Vec4f( 1,1,1,1 );
			s.magnitude = 1;
			s.scale = scale[ i ];
			solarsystem.push_back( s );
			IncrementLoadProgress( "planets" );
		}		
		UpdateSolarSystemSprites();
	}
	
	void UpdateEarthModel() {
		if ( mod.count( "sphere" ) == 0 ) {
			mod["sphere"] = new Model( "globe" );
            Shader * s = shd["earth"] = CreateShaderFromFile( "earth" );
            glBindAttribLocation( s->pgObject, AL_Position, "Vertex" );
            glBindAttribLocation( s->pgObject, AL_Color, "Color" );
            glBindAttribLocation( s->pgObject, AL_TexCoord + 0, "TexCoord0" );
            glLinkProgram( s->pgObject );
            s = shd["earth-lite"] = CreateShaderFromFile( "earth-lite" );
            glBindAttribLocation( s->pgObject, AL_Position, "Vertex" );
            glBindAttribLocation( s->pgObject, AL_Color, "Color" );
            glBindAttribLocation( s->pgObject, AL_TexCoord + 0, "TexCoord0" );
            glLinkProgram( s->pgObject );
            //s->SetUniform( "Sampler0", 0 );
            //s->SetUniform( "Sampler1", 1 );
		}
		static double lastUpdateTime = 0;
		double time = GetTime();
		if ( ( time - lastUpdateTime ) < 60.0 ) {
			return;
		}
		lastUpdateTime = time;
		
		Vec3d sunPos = ss.body[ SSB_Sun ].equatorialPos;
		Vec3f sunDir( sunPos.x, sunPos.y, sunPos.z );
		sunDir.Normalize();
		sunDir = Rotationf( Vec3f( 0, 0, 1 ), -GetCurrentEarthPhase() ).GetMatrix3() * sunDir;
		
		vector<EarthVert> data;
        int jdim = 18;
        int idim = 18;
        for( int face = 0; face < 6; face++ ) {
            Matrix4f xf;
            switch( face ) {
                case 0:
                    xf.MakeIdentity();
                    break;
                case 1:
                    xf = Rotationf( Vec3f( 1, 0, 0 ), ToRadians( 90 ) ).GetMatrix4();
                    break;
                case 2:
                    xf = Rotationf( Vec3f( 0, 0, 1 ), ToRadians( 90 ) ).GetMatrix4() * Rotationf( Vec3f( 1, 0, 0 ), ToRadians( 90 ) ).GetMatrix4();
                    break;
                case 3:
                    xf = Rotationf( Vec3f( 0, 0, 1 ), ToRadians( 180 ) ).GetMatrix4() * Rotationf( Vec3f( 1, 0, 0 ), ToRadians( 90 ) ).GetMatrix4();
                    break;
                case 4:
                    xf = Rotationf( Vec3f( 0, 0, 1 ), ToRadians( 270 ) ).GetMatrix4() * Rotationf( Vec3f( 1, 0, 0 ), ToRadians( 90 ) ).GetMatrix4();
                    break;
                case 5:
                    xf = Rotationf( Vec3f( 1, 0, 0 ), ToRadians( 180 ) ).GetMatrix4();
                    break;
                default:
                    break;
            }

            for ( int j = 0; j < jdim; j++ ) {
                float fj = float( j ) / ( jdim - 1 );
                for( int i = 0; i < idim; i++ ) {
                    float fi = float( i ) / ( idim - 1 );
                    EarthVert v;
                    v.pos = Vec3f( fi - 0.5f, fj - 0.5f, .5f );                    
                    v.pos.Normalize();
                    xf.MultMatrixVec( v.pos );
                    data.push_back( v );
                }
            }
        }
		vector<ushort> index;
        for( int face = 0; face < 6; face++ ) {
            int faceoff = face * idim * jdim;
            for( int j = 0; j < jdim - 1; j++ ) {
                for( int i = 0; i < idim - 1; i++ ) {
                    // lower right tri
                    index.push_back( faceoff + ( j + 0 ) * idim + ( i + 0 ) );
                    index.push_back( faceoff + ( j + 0 ) * idim + ( i + 1 ) );
                    index.push_back( faceoff + ( j + 1 ) * idim + ( i + 1 ) );
                    // upper left tri
                    index.push_back( faceoff + ( j + 0 ) * idim + ( i + 0 ) );
                    index.push_back( faceoff + ( j + 1 ) * idim + ( i + 1 ) );					
                    index.push_back( faceoff + ( j + 1 ) * idim + ( i + 0 ) );
                }
            }            
        }
        Model * s = mod["sphere"];
		s->GetVertexBuffer().SetData( (int)data.size() * sizeof( EarthVert ), & data[0] );
		s->GetIndexBuffer().SetData( (int)index.size() * sizeof( ushort ), & index[0] );
        s->AddAttributeArray( AttributeArray( AL_Position, 3, GL_FLOAT, GL_FALSE, sizeof( Vec3f ), 0 ) );
        s->AddAttributeArray( AttributeArray( AL_TexCoord, 3, GL_FLOAT, GL_FALSE, sizeof( Vec3f ), 0 ) );
		s->SetPrimitive( GL_TRIANGLES );
	}
    
    using namespace ujson;
    
    void LoadConfigTextures( Json & j ) {
        if( j.GetType() != Json::Type_Object ) {
            return;
        }
        vector<string> name;
        j.GetMemberNames( name );
        for( size_t i = 0; i < name.size(); i++ ) {
            string & n = name[i];
            if( j(n).GetType() != Json::Type_Object ) {
                Output( "bad texture config for %s, not a JsonMap", n.c_str() );
                continue;
            }
            Json & t = j(n);
            r3::TextureFormatEnum fmt = TextureFormat_RGBA;
            if( t("format").GetType() == Json::Type_String ) {
                const string & s = t("format").s;
                /*
                 if( s == "LA" ) {
                 fmt = TextureFormat_LA;
                 }
                 */
                if( s == "RGB" ) {
                    fmt = TextureFormat_RGB;
                }
            }
            r3::TextureTargetEnum tgt = TextureTarget_2D;
            if( t( "target" ).GetType() == Json::Type_String ) {
                const string & s = t("target").s;
                if( s == "CUBE" ) {
                    tgt = TextureTarget_Cube;
                }
            }
            switch( tgt ) {
                case r3::TextureTarget_Cube:
                    tex[ n ] = CreateTextureCubeFromFile( t("src").s, fmt );
                    break;
                case r3::TextureTarget_2D:
                default:
                    tex[ n ] = CreateTexture2DFromFile( t("src").s, fmt );                    
                    break;
            }                        
            Output( "Loaded texture %s", n.c_str() );
            IncrementLoadProgress();
        }
    }

    // "nightmode" : { "type" : "toggle", "tex" : "nightmode", "var" : "app_nightViewing" },
    ToggleButton * LoadConfigUiToggle( Json & j ) {
        if( j.GetType() != Json::Type_Object ) {
            return NULL;
        }
        
        if( j("tex").GetType() != Json::Type_String || j("var").GetType() != Json::Type_String ) {
            Output( "Invalid UiToggle definition" );
            return NULL;
        }
        
        return new ToggleButton( static_cast<r3::Texture2D *>( tex[ j("tex").s ] ), j("var").s );

    }

    // "viewmode"  : { "type" : "sequence", "items" : [ { "tex" : "stars", "cmd" : "setAppMode viewStars" } ] },
    Button * LoadConfigUiSequence( Json & j ) {
        if( j( "items" ).GetType() != Json::Type_Array ) {
            return NULL;
        }
        vector<Json *> & v = j( "items" ).v;
        SequenceButton *sb = new SequenceButton();
        for( int i = 0; i < v.size(); i++ ) {
            if( v[i] == NULL || v[i]->GetType() != Json::Type_Object ) {
                Output( "Invalid UiSequence definition at sequence item %d", i ); 
                continue;
            }
            Json & im = *v[i];
            if( im("tex").GetType() != Json::Type_String || im("cmd").GetType() != Json::Type_String ) {
                Output( "Invalid UiSequence definition" );
                continue;
            }
            Texture2D * t = static_cast<Texture2D *>( tex[ im("tex").s ] );
            if( t == NULL ) {
                Output( "Invalid texture reference %s in UiSequence", im("tex").s.c_str() );
                continue;
            }
            sb->items.push_back( SequenceButton::Item( t, im("cmd").s ) );            
        }

        if( sb->items.size() == 0 ) {
            delete sb;
            sb = NULL;
        }
        return sb;
    }
        
    void LoadConfigUi( Json & j ) {
        if( j.GetType() != Json::Type_Object ) {
            return;
        }
        vector<string> name;
        j.GetMemberNames( name );
        for( size_t i = 0; i < name.size(); i++ ) {
            const string & n = name[i];
            if( j(n).GetType() != Json::Type_Object ) {
                Output( "bad UI config for %s, not a JsonMap", n.c_str() );
                continue;                
            }
            Json & ui = j(n);
            if( ui("type").GetType() != Json::Type_String ) {
                Output( "bad UI config for %s, no type member", n.c_str() );
                continue;
            }
            
            if( ui("type").s == "toggle" ) {
                btn[ n ] = LoadConfigUiToggle( ui );
            } else if( ui("type").s == "sequence" ) {
                Output( "Adding UI Sequence for %s", n.c_str() );
                btn[ n ] = LoadConfigUiSequence( ui );
            }
        }
    }
    
    void LoadConfig( const string & filename ) {
        vector<unsigned char> buf;
        if( FileReadToMemory( filename, buf ) ) {
            Json * j = Decode( (const char *)&buf[0], (int)buf.size() );
            //if( DidDecodeSucceed() == false ) {
            //    Output( "Parse of json file %s failed.\n", filename.c_str() );
            //}
            if( j ) { // && json->GetType() == Json::Type_Map ) {
                LoadConfigTextures( (*j)("textures") );
                LoadConfigUi( (*j)( "buttons" ) );
            }
            Delete( j );
		}
    }
    
    
	    
	struct InitializerThread : public r3::Thread {
        InitializerThread() : r3::Thread( "Initializer" ) {}
        
		virtual void Run() {
			
			GfxContext *ctx = app_asyncLoad.GetVal() == AsyncLoad_MultiContext ? loadContext : drawContext;
			
			SetTimeOffsetViaNtp(); // Sadly, we can't trust the system clock on iOS (and perhaps Android).
            
			menu.borderWidth = 0.05f * r_windowDpi.GetVal(); 
			menu.buttonWidth = 0.2f * r_windowDpi.GetVal();		
            
			if( app_asyncLoad.GetVal() != AsyncLoad_Synchronous ) {
				ctx->Acquire();				
			}
			InitializeRender();
			IncrementLoadProgress( "fonts" );
			textDepthBias = float( 1 << 16 ) * -0.025f;
            
			InitializeSpaceTime();
            
            LoadConfig( "config.json" );
			
			InitSolarSystemSprites();
			
			AsyncInitMisc();
            
			InitializeSatellites();
			ComputeSatellitePositions( satellite );
			IncrementLoadProgress( "satellites" );
            
			// construct hemi model
			{			
				Model * m = mod["hemi"] = new Model( "hemi" );
				float data[ 37 * 2 * 5 ];
				float *d = data;
				for( int i = 0; i <= 36; i++ ) {
					Vec3f p0 = SphericalToCartesian( 1.0f, 0.0f, ( 2.0 * R3_PI * i ) / 36.f );
					d[0] = p0.x;
					d[1] = p0.y;
					d[2] = p0.z;
					d[3] = 0.5f;
					d[4] = 0.0f;
					d+=5;
					Vec3f p1 = SphericalToCartesian( 1.0f, ToRadians( 45.0f ), ( 2.0 * R3_PI * i ) / 36.f );
					d[0] = p1.x;
					d[1] = p1.y;
					d[2] = p1.z;
					d[3] = 0.5f;
					d[4] = 0.5f;
					d+=5;
				}
				m->GetVertexBuffer().SetData( 37 * 2 * 5 * sizeof( float ), data );
                m->SetNumVertexes( 37 * 2 );
                m->AddAttributeArray( AttributeArray( GL_VERTEX_ARRAY, 3, GL_FLOAT, GL_FALSE, 5 * sizeof( float ), 0 ) );
                m->AddAttributeArray( AttributeArray( GL_TEXTURE0    , 2, GL_FLOAT, GL_FALSE, 5 * sizeof( float ), 3 * sizeof( float ) ) );
				m->SetPrimitive( GL_TRIANGLE_STRIP );
			} 
			IncrementLoadProgress( "models" );
            
			InitStarsModel();
            
			UpdateEarthModel();
			IncrementLoadProgress();
            
			glTextureParameteriEXT( tex["earth"]->Object(), GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTextureParameteriEXT( tex["earth"]->Object(), GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
			glTextureParameteriEXT( tex["earthnorm"]->Object(), GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTextureParameteriEXT( tex["earthnorm"]->Object(), GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
			tex["daynight"]->ClampToEdge();
            
            
            /*{
                SequenceButton *sb = new SequenceButton();
                sb->items.push_back( SequenceButton::Item( (Texture2D *)tex["viewstars"], "setAppMode viewStars" ) );
                sb->items.push_back( SequenceButton::Item( (Texture2D *)tex["viewglobe"], "setAppMode viewGlobe" ) );
                btn["viewmode"] = sb;
            }*/
            
            /*{
                SequenceButton *sb = new SequenceButton();
                tex["brightest"] = CreateTexture2DFromFile( "brightest.png", TextureFormat_RGBA );
                IncrementLoadProgress();
                sb->items.push_back( SequenceButton::Item( (Texture2D *)tex["brightest"], "chooseBrightestSatellites" ) );
                tex["heart"] = CreateTexture2DFromFile( "heart.png", TextureFormat_RGBA );
                IncrementLoadProgress();                
                sb->items.push_back( SequenceButton::Item( (Texture2D *)tex["heart"], "chooseFavoriteSatellites" ) );
#if ! APP_spacejunklite
                tex["amateur"] = CreateTexture2DFromFile( "amateurradio.png", TextureFormat_RGBA );
                IncrementLoadProgress();                
                sb->items.push_back( SequenceButton::Item( (Texture2D *)tex["amateur"], "chooseAmateurRadioSatellites" ) );
                tex["iridium"] = CreateTexture2DFromFile( "iridium.png", TextureFormat_RGBA );
                IncrementLoadProgress();                
                sb->items.push_back( SequenceButton::Item( (Texture2D *)tex["iridium"], "chooseIridiumSatellites" ) );
#endif
                btn["satlist"] = sb;
            }*/
            
            btn["fblogin"] = new PushButton( "fblogin.png", "loginToFacebook" );
			IncrementLoadProgress();
			btn["fbpublish"] = new PushButton( "fbicon.png", "publishOnFacebook", "logoutOfFacebook" );
			IncrementLoadProgress();
#if APP_spacejunklite
			btn["appstore"] = new PushButton( "star3map_icon.png", "goToAppStore" );
#endif		
            btn["showflyovers"] = new PushButton( "flyover.png", "showFlyovers" );
            
            
			UpdateManualOrientation();
			IncrementLoadProgress("Done");
			if( app_asyncLoad.GetVal() != AsyncLoad_Synchronous ) {
				ctx->Release();
			}
			
			appMode = AM_Initialized;
		}
        
	};
	
	InitializerThread *initThread;
    
	void Initialize() {
		if ( app_asyncLoad.GetVal() != AsyncLoad_Synchronous ) {
			switch( appMode ) {
				case AM_Uninitialized:
					appMode = AM_Initializing;
					if( r_windowWidth.GetVal() <= 640 ) {
						tex["splash"] = CreateTexture2DFromFile( "splash-iphone.png", TextureFormat_RGBA );
					} else {
						tex["splash"] = CreateTexture2DFromFile( "splash-ipad.png", TextureFormat_RGBA );
					}
				{
					SamplerParams samp = tex["splash"]->Sampler();
					samp.mipFilter = TextureFilter_None;
					tex["splash"]->SetSampler( samp );
				}
					InitializeLocalize();
					Output( "Creating Draw GfxContext!" );
					drawContext = CreateGfxContext( NULL );
					Output( "Created Draw GfxContext!" );
					if ( app_asyncLoad.GetVal() == AsyncLoad_MultiContext ) {
						loadContext = CreateGfxContext( drawContext );						
					}
					initThread = new InitializerThread;
					initThread->Start();
					Output( "Fired up InitializerThread, returning." );
					break;
				case AM_Initialized:
					delete initThread;
					initThread = NULL;
					appMode = AM_ViewStars;
					app_loadProgressFinal.SetVal( app_loadProgress.GetVal() );
					EnableStatusMessages();
					Output( "InitializerThread finished." );
					break;
				default:
					break;
			}						
		} else if ( appMode == AM_Uninitialized ) {
			// running the initializer thread without actually starting a new thread
			appMode = AM_Initializing;
			InitializeLocalize();
			Output( "Creating Draw GfxContext!" );
			drawContext = CreateGfxContext( NULL );
			Output( "Created Draw GfxContext!" );
			if( r_windowWidth.GetVal() <= 640 ) {
				tex["splash"] = CreateTexture2DFromFile( "splash-iphone.png", TextureFormat_RGBA );
			} else {
				tex["splash"] = CreateTexture2DFromFile( "splash-ipad.png", TextureFormat_RGBA );
			}
			IncrementLoadProgress();
			initThread = new InitializerThread;
			initThread->Run();
			delete initThread;
			initThread = NULL;
			appMode = AM_ViewStars;
			app_loadProgressFinal.SetVal( app_loadProgress.GetVal() );
			EnableStatusMessages();
		}
	}
	
	void PlaceButtons() {
		menu.Clear();
		menu.rect = Rect( 0, 0, r_windowWidth.GetVal(), r_windowHeight.GetVal() );
#if APP_spacejunklite
		menu.AddButton( btn["appstore"] );
#endif
        menu.AddButton( btn["viewmode"] );
        menu.AddButton( btn["nightmode"] );
		if ( appMode == AM_ViewStars && GotCompassUpdate == true ) {
            menu.AddButton( btn["compass"] );
		} else if ( appMode == AM_ViewGlobe ) {
			menu.AddButton( btn["changelocation"] );
			menu.AddButton( btn["gps"] );
		}
		if ( SatellitesLoaded() && app_showSatellites.GetVal() ) {
            menu.AddButton( btn["satlist"] );
		}
#if HAS_FACEBOOK
		if ( app_fbLoggedIn.GetVal() == false ) {
			menu.AddButton( btn["fblogin"] );
		} else {
			menu.AddButton( btn["fbpublish"] );
		}
#endif
        menu.AddButton( btn["showflyovers"] );
        menu.AddButton( btn["showqr"] );
	}
    
	// Draw something to illustrate the horizon
	void DrawUpHemisphere() {
		//Output("DrawUpHemisphere");
		tex["hemi"]->Bind( 0 );
		tex["hemi"]->Enable( 0 );
        glMultiTexEnviEXT( GL_TEXTURE0, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		glColor4f( 1, 1, 1, 1 ) ;
		mod["hemi"]->Draw();
		tex["hemi"]->Disable( 0 );
	}
	
	void DrawEarth() {
		UpdateEarthModel();
		
        Vec3d sunPos = ss.body[ SSB_Sun ].equatorialPos;
		Vec3f sunDir( sunPos.x, sunPos.y, sunPos.z );
		sunDir.Normalize();
		sunDir = Rotationf( Vec3f( 0, 0, 1 ), -GetCurrentEarthPhase() ).GetMatrix3() * sunDir;

        
		tex["earth"]->Bind( 0 );
		tex["earthnorm"]->Bind( 1 );
#if APP_spacejunklite
        Shader * s = proTimeLeft >= 0 ? shd["earth"] : shd["earth-lite"];
#else
        Shader * s = shd["earth"];
#endif
        glUseProgram( s->pgObject );
        s->SetUniform( "SunDir", sunDir );
        mod["sphere"]->Draw();
        glUseProgram( 0 );
		//tex["daynight"]->Disable( 1 );
		tex["earth"]->Disable( 0 );
	}
    
    void ResetDrawContext( const vector< Token > & tokens ) {
        drawContext = CreateGfxContext( NULL );
	}
	CommandFunc ResetDrawContextCmd( "resetdrawcontext", "reset the draw context", ResetDrawContext );
    
    
	void GoToAppStore( const vector< Token > & tokens ) {
#define VODAFONE 0
#if ! VODAFONE
        OpenURL( "http://home.xyzw.us/star3map/buy.php" );
#else
        OpenURL( "http://home.xyzw.us/star3map/buy.php?carrier=vodafone" );
#endif
#if 0
#if ANDROID
		OpenURL( "market://search?q=pname:us.xyzw.star3map" );
#else
		OpenURL( "http://phobos.apple.com/WebObjects/MZStore.woa/wa/viewSoftware?id=353613186" );
#endif
#endif
	}
	CommandFunc GoToAppStoreCmd( "goToAppStore", "go to star3map in the App Store", GoToAppStore );
    
    
    void ShowFlyovers( const vector< Token > & tokens ) {
        char url[256];
        r3Sprintf( url, "http://home.xyzw.us/star3map/flyovers.html?lat=%f&lon=%f", app_latitude.GetVal(), app_longitude.GetVal() );
        OpenURL( url );
    }
	CommandFunc ShowFlyoversCmd( "showFlyovers", "show next visible satellite flyovers", ShowFlyovers );
    
    
	void ChooseBrightestSatellites( const vector< Token > & tokens ) {
		app_satelliteUrl.SetVal( "http://www.celestrak.com/NORAD/elements/visual.txt" );
	}
	CommandFunc ChooseBrightestSatellitesCmd( "chooseBrightestSatellites", "load the brightest satellites", ChooseBrightestSatellites );
	
	void ChooseFavoriteSatellites( const vector< Token > & tokens ) {
		app_satelliteUrl.SetVal( "http://home.xyzw.us/star3map/favorites.php" );
	}
	CommandFunc ChooseFavoriteSatellitesCmd( "chooseFavoriteSatellites", "load the favorite satellites", ChooseFavoriteSatellites );
	
	void ChooseAmateurRadioSatellites( const vector< Token > & tokens ) {
		app_satelliteUrl.SetVal( "http://www.celestrak.com/NORAD/elements/amateur.txt" );
	}
	CommandFunc ChooseAmateurRadioSatellitesCmd( "chooseAmateurRadioSatellites", "load the amateur radio satellites", ChooseAmateurRadioSatellites );
	
	void ChooseIridiumSatellites( const vector< Token > & tokens ) {
		app_satelliteUrl.SetVal( "http://www.celestrak.com/NORAD/elements/iridium.txt" );
	}
	CommandFunc ChooseIridiumSatellitesCmd( "chooseIridiumSatellites", "load the iridium satellites", ChooseIridiumSatellites );
	
#if HAS_FACEBOOK
	void LoginToFacebook( const vector< Token > & tokens ) {
		platformFacebookLogin();
	}
	CommandFunc LoginToFacebookCmd( "loginToFacebook", "login to Facebook", LoginToFacebook );
	
	void LogoutOfFacebook( const vector< Token > & tokens ) {
		platformFacebookLogout();
	}
	CommandFunc LogoutOfFacebookCmd( "logoutOfFacebook", "logout of Facebook", LogoutOfFacebook );
	
	void PublishOnFacebook( const vector< Token > & tokens ) {
		char buf[256];
		r3Sprintf( buf, "I used Space Junk for " PLATFORM_STRING " tonight.");
		string caption;
		string description;
		if ( sightings.size() > 0 ) {
            r3Sprintf( buf, "I saw " );
			description += buf;
            int count = 0;
			for ( map<int,Sighting>::iterator it = sightings.begin(); it != sightings.end(); ++it ) {
				description += it->second.name;
                if( sightings.size() > 2 && count < (sightings.size() - 1 ) ) {
                    description += ", ";
                } else if ( sightings.size() == 2 && count == 0 ) {
                    description += " ";
                }
                if( count == (sightings.size() - 2 ) ) {
                    description += "and ";
                }
                count++;
			}
			description += " using Space Junk for " PLATFORM_STRING ".";
		} else {
            description = "I used Space Junk for " PLATFORM_STRING " tonight.";
		}
		platformFacebookPublish( caption, description );
	}
	CommandFunc PublishOnFacebookCmd( "publishOnFacebook", "publish current activity on Facebook", PublishOnFacebook );
#endif
	
	bool hasViewedGlobe = false;
	float globeViewLat;
	float globeViewLon;
	float viewStarsFov = 60.0f;
	float viewGlobeFov = 80.0f;
	void SetAppMode( const vector< Token > & tokens ) {
		if ( appMode == AM_ViewGlobe ) {
			viewGlobeFov = r_fov.GetVal();
		} else if ( appMode == AM_ViewStars ) {
			viewStarsFov = r_fov.GetVal();
		}
        
		if ( tokens.size() == 2 ) {
			if ( tokens[1].valString == "viewStars" ) {
				r_fov.SetVal( viewStarsFov );
				appMode = AM_ViewStars;
			} else if ( tokens[1].valString == "viewGlobe" ) {
				r_fov.SetVal( viewGlobeFov );
				appMode = AM_ViewGlobe;
				if ( hasViewedGlobe == false ) {
					hasViewedGlobe = true;
					globeViewLat = app_latitude.GetVal();
					globeViewLon = app_longitude.GetVal();
				}
			}
		}
	}
	CommandFunc SetAppModeCmd( "setAppMode", "set app mode :-)", SetAppMode );
	
	void UpdateManualOrientation() {
		Matrix4f phiMat = Rotationf( Vec3f( 1, 0, 0 ), -ToRadians( app_manualPhi.GetVal() + 90 ) ).GetMatrix4();
		Matrix4f thetaMat = Rotationf( Vec3f( 0, 0, 1 ), -ToRadians( app_manualTheta.GetVal() ) ).GetMatrix4();
		//Output( "Updating manualOrientation from phi=%f theta=%f", app_manualPhi.GetVal(), app_manualTheta.GetVal() );
		manualOrientation = phiMat * thetaMat;
	}
	
}




namespace star3map {
	
	bool touchActive = false;
	Vec2f inputInertia;
    void ApplyInputInertia();
	void ApplyInputInertia() {
		
		inputInertia *= app_inputDrag.GetVal();
		float len = inputInertia.Length();
		
		if ( touchActive == false && len > 1 ) {
			float dx = inputInertia.x;
			float dy = inputInertia.y;
			if ( appMode == AM_ViewGlobe && ( dx != 0 || dy != 0 ) ) {
				globeViewLat = max( -90.f, min( 90.f, globeViewLat - dy ) );
				globeViewLon = ModuloRange( globeViewLon - dx, -180, 180 );
				if ( app_changeLocation.GetVal() ) {
					app_latitude.SetVal( globeViewLat );
					app_longitude.SetVal( globeViewLon );
				}
			} else if ( appMode == AM_ViewStars && app_useCompass.GetVal() == false ) {
				float phi = max( -90.f, min( 90.f, app_manualPhi.GetVal() - dy ) );
				float theta = ModuloRange( app_manualTheta.GetVal() + dx, -180, 180 );
				app_manualPhi.SetVal( phi );
				app_manualTheta.SetVal( theta );
				UpdateManualOrientation();
			}
		}
		
	}
    
	bool DetectClick( bool active, int x, int y );
    bool DetectClick( bool active, int x, int y )  {
		static double positiveEdgeTimestamp;
		static int positiveEdgeX;
		static int positiveEdgeY;
		if ( touchActive == false  && active == true )  {       // positive edge
			positiveEdgeTimestamp = frameBeginTime;
			positiveEdgeX = x;
			positiveEdgeY = y;
		} else if ( touchActive == true && active == false ) {  // negative edge
			if ( ( frameBeginTime - positiveEdgeTimestamp ) < 0.25 &&
                abs( x - positiveEdgeX ) < 10 &&
                abs( y - positiveEdgeY ) < 10 ) {
				return true;
			}
		}
		return false;
	}
    
	void TraceFromClick( int x, int y );	
	void TraceFromClick( int x, int y ) {
		r3::Matrix4f vpscale;
		vpscale.SetScale( Vec3f( r_windowWidth.GetVal() * 0.5f, r_windowHeight.GetVal() * 0.5f, 0.5f ) );
		r3::Matrix4f vpbias;
		vpbias.SetTranslate( Vec3f( 1, 1, 1 ) );
		r3::Matrix4f proj = r3::Perspective( r_fov.GetVal(), float(r_windowWidth.GetVal()) / r_windowHeight.GetVal(), 0.5f, 100.0f );
		float latitude = ToRadians( app_latitude.GetVal() );
		float longitude = ToRadians( app_longitude.GetVal() );
		Matrix4f xout = Rotationf( Vec3f( 0, 1, 0 ), -R3_PI / 2.0f ).GetMatrix4(); // current Lat/Lon now at { 0, 0, 1 }, with z up
		Matrix4f zup = Rotationf( Vec3f( 1, 0, 0 ), -R3_PI / 2.0f ).GetMatrix4();  // current Lat/Lon now at { 1, 0, 0 }, with z up
		Matrix4f lat = Rotationf( Vec3f( 0, 1, 0 ), latitude ).GetMatrix4();       // current Lat/Lon now at { 1, 0, 0 }, with y up
		Matrix4f lon = Rotationf( Vec3f( 0, 0, 1 ), -longitude ).GetMatrix4();
		Matrix4f comp = ( xout * zup * lat * lon );
		Matrix4f mvpv = vpscale * vpbias * proj * orientation * comp;
		Matrix4f ivmvpv = mvpv.Inverse();
		Vec3f clickDir = ivmvpv * Vec3f( x, y, 0.5f );
		clickDir.Normalize();
        
		float phaseEarthRot = GetCurrentEarthPhase();
		Matrix4f phase = Rotationf( Vec3f( 0, 0, 1 ), phaseEarthRot ).GetMatrix4();
        
		Vec3f viewer = SphericalToCartesian( RadiusEarthKm, latitude, longitude );
		
		Sighting sighting;
		sighting.timestamp = frameBeginTime;
		float maxDot = 0.998f;
        
		// satellites
		if ( app_showSatellites.GetVal() ) {
			for ( int i = 0; i < satSorter.Count(); i++ ) {
				Satellite & sat = satellite[ satSorter[ i ] ];
				Vec3f satDir = sat.pos - viewer;
				satDir.Normalize();
				
				float dot = clickDir.Dot( satDir );
				if ( dot <= maxDot ) {
					continue;
				}
				sighting.name = sat.name;
				sighting.SetNum( sat.id );
				sighting.SetType( SOT_Satellite );
				maxDot = dot;
			}
		}
		
		// Need to figure out why this is required...  I'd like everything to be done in the same space.
		clickDir = phase * clickDir;
		
		// constellations
		if ( app_showConstellations.GetVal() ) {
			for ( int i = 0; i < (int)constellations.size(); i++ ) {
				Lines &l = constellations[ i ];
				float dot = clickDir.Dot( l.center );
				if ( dot <= maxDot ) {
					continue;
				}
				sighting.name = l.name;
				sighting.SetNum( i );
				sighting.SetType( SOT_Constellation );
				maxDot = dot;
			}
		}
        
		// stars
		for ( int i = 0; i < (int)stars.size(); i++ ) {
			Sprite & s = stars[i];
			if ( s.magnitude > 4 || s.name.size() == 0 ) {
				continue;
			}
			float dot = clickDir.Dot( s.direction );
			if ( dot <= maxDot ) {
				continue;
			}
			sighting.name = s.name;
			sighting.SetNum( i );
			sighting.SetType( SOT_Star );			
		}
		
		
		if ( app_showPlanets.GetVal() ) {
			for ( int i = 0; i < (int)solarsystem.size(); i++ ) {
				Sprite & s = solarsystem[i];
				float dot = clickDir.Dot( s.direction );
				if ( dot <= maxDot ) {
					continue;
				}
				sighting.name = s.name;
				sighting.SetNum( i );
				sighting.SetType( SOT_Planet );			
			}			
		}
		
		if ( sighting.id != 0 ) {
			if ( sightings.count( sighting.id ) != 0 ) {
				Output( "Removing %d -- %s from sightings.", sighting.id, sighting.name.c_str() );
				sightings.erase( sighting.id );
			} else {
				Output( "Adding %d -- %s to sightings.", sighting.id, sighting.name.c_str() );
				sightings[ sighting.id ] = sighting;				
			}			
		}
		
	}
	
	
	
	bool ProcessInput( bool active, int x, int y ) {
		ScopedGfxContextAcquire ctx( drawContext );
        
		bool handled = menu.ProcessInput( active, x, y );
		
		if ( app_useCoreLocation.GetVal() == true && GotLocationUpdate == false ) {
			Output( "setting useCoreLocation back to false because we have not gotten a location update" );
			app_useCoreLocation.SetVal( false );
		}
		if ( app_useCompass.GetVal() == true && GotCompassUpdate == false ) {
			Output( "setting useCompass back to false because we have not gotten a compass update" );
			app_useCompass.SetVal( false );
		}
		if ( app_showSatellites.GetVal() == true && SatellitesLoaded() == false ) {
			Output( "setting showSatellites to false because we have not gotten TLE data" );
			app_showSatellites.SetVal( false );
		}
		
		if ( handled == false ) {			
			if ( appMode == AM_ViewStars && DetectClick( active, x, y ) && satSorter.Count() > 0 ) {
				TraceFromClick( x, y );
			}			
		}
        
		
		static int prevx;
		static int prevy;
		float dx = 0;
		float dy = 0;
		// If the UI didn't handle the input, then try to use it as "global" input
		if ( active && handled == false ) {
			if ( prevx != 0 && prevy != 0 ) {
				float factor = 0.25 * r_fov.GetVal() / 90.0f;
				dx = factor * ( x - prevx ) ;
				dy = factor * ( y - prevy );
				if ( appMode == AM_ViewGlobe && ( dx != 0 || dy != 0 ) ) {
					globeViewLat = max( -90.f, min( 90.f, globeViewLat - dy ) );
					globeViewLon = ModuloRange( globeViewLon - dx, -180, 180 );
					if ( app_changeLocation.GetVal() ) {
						app_useCoreLocation.SetVal( false );
						app_latitude.SetVal( globeViewLat );
						app_longitude.SetVal( globeViewLon );
					}
				} else if ( appMode == AM_ViewStars && app_useCompass.GetVal() == false ) {
					float phi = max( -90.f, min( 90.f, app_manualPhi.GetVal() - dy ) );
					float theta = ModuloRange( app_manualTheta.GetVal() + dx, -180, 180 );
					app_manualPhi.SetVal( phi );
					app_manualTheta.SetVal( theta );
					UpdateManualOrientation();
				}
			}
			prevx = x;
			prevy = y;
			if ( fabs( float( dx ) ) > fabs( inputInertia.x ) ) {
				inputInertia.x = dx;
			}
			if ( fabs( float( dy ) ) > fabs( inputInertia.y ) ) {
				inputInertia.y = dy;
			}
		}
        
		if ( active == false ) {
			prevx = prevy = 0;
		}
		
		touchActive = active;
		
		return true;
	}
	
	void DisplayViewStars();
	void DisplayViewStars() {
		DrawNonOverlappingStrings *nos = CreateNonOverlappingStrings();
		
		float sightingAlpha = sin( frameBeginTime * R3_PI ) * 0.125f + 0.25f;
		
        { 
            ScopedPushMatrix push( GL_MODELVIEW );
            glMatrixLoadIdentityEXT( GL_MODELVIEW );
            r3::Matrix4f proj = r3::Perspective( r_fov.GetVal(), float(r_windowWidth.GetVal()) / r_windowHeight.GetVal(), 0.5f, 100.0f );
            Projection = proj;
            glMatrixLoadfEXT( GL_PROJECTION, proj.Ptr() );
            
            
            float latitude = ToRadians( app_latitude.GetVal() );
            float longitude = ToRadians( app_longitude.GetVal() );
            Matrix4f xout = Rotationf( Vec3f( 0, 1, 0 ), -R3_PI / 2.0f ).GetMatrix4(); // current Lat/Lon now at { 0, 0, 1 }, with z up
            Matrix4f zup = Rotationf( Vec3f( 1, 0, 0 ), -R3_PI / 2.0f ).GetMatrix4();  // current Lat/Lon now at { 1, 0, 0 }, with z up
            Matrix4f lat = Rotationf( Vec3f( 0, 1, 0 ), latitude ).GetMatrix4();       // current Lat/Lon now at { 1, 0, 0 }, with y up
            Matrix4f lon = Rotationf( Vec3f( 0, 0, 1 ), -longitude ).GetMatrix4();
            float phaseEarthRot = GetCurrentEarthPhase();
            Matrix4f phase = Rotationf( Vec3f( 0, 0, 1 ), -phaseEarthRot ).GetMatrix4();
            
            Matrix4f comp = ( xout * zup * lat * lon * phase );
            
            ModelView = orientation * comp;
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            ScopedEnable blend( GL_BLEND );
            ScopedDisable ddt( GL_DEPTH_TEST );
            
            glColor3f( 1, 1, 1 );
            
            // compute culling info
            Matrix4f mvp = proj * orientation * comp;
            Matrix4f imvp = mvp.Inverse();
            Vec3f lookDir = imvp * Vec3f( 0, 0, -1 );
            lookDir.Normalize();
            Vec3f corner = imvp * Vec3f( 1, 1, 1 );
            corner.Normalize();
            float limit = lookDir.Dot( corner );
            
            float labelLimit = ( limit + 8 ) / 9.0f;
            
            { 
                ScopedPushMatrix push( GL_MODELVIEW );
                glMatrixMultfEXT( GL_MODELVIEW, orientation.Ptr() );
                
                // draw horizon hemisphere indicator
                if ( app_showHemisphere.GetVal() ) {
                    DrawUpHemisphere();	
                }
                
                glMatrixMultfEXT( GL_MODELVIEW, comp.Ptr() );
                
                Matrix3f local = ToMatrix3( comp );
                UpVector = local.GetRow(2); // to orient text correctly		
                
                // draw constellations
                if ( app_showConstellations.GetVal() ) {
                    for ( int i = 0; i < (int)constellations.size(); i++ ) {
                        Lines &l = constellations[ i ];
                        Vec4f c( .5, .5, .7, .5 );
                        Vec4f cl( .5, .5, .7, .8 );
                        Sighting sighting;
                        sighting.SetNum( i );
                        sighting.SetType( SOT_Constellation );
                        if ( sightings.count( sighting.id ) != 0 ) {
                            Vec4f c( .5, .5, .5, sightingAlpha );
                            glColor4fv( c.Ptr() );
                            glBegin( GL_LINES );
                            for( int j = 0; j < (int)l.vert.size(); j++ ) {
                                Vec3f & v = l.vert[ j ];
                                glVertex3f( v.x, v.y, v.z );
                            }
                            glEnd();
                            DrawString( l.name, l.center );
                        } else if ( lookDir.Dot( l.center ) > l.limit ) {
                            DynamicLinesInView( &l, c, cl, lookDir );
                        }
                    }
                }
                AgeDynamicLines();
                DrawDynamicLines();
                
                glColor4f( 1, 1, 1, 1 );
                
                
                // Reserve space for planet labels.
                {
                    for ( int i = 0; i < (int)solarsystem.size(); i++ ) {
                        Sprite & s = solarsystem[i];
                        nos->ReserveString( s.name, s.direction, lookDir, limit );
                    }
                }
                
                UpVector = local.GetRow(2); // to orient text correctly
                
                if ( app_showDirections.GetVal() ) {
                    glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
                    nos->DrawString( "Up", local.GetRow(2), lookDir, 0.3f );
                    nos->DrawString( "Down", -local.GetRow(2), lookDir, 0.3f );
                    glColor4f( 1, 1, 1, 1 );
                    DrawSprite( (Texture2D *)tex["north"], 10, local.GetRow(1) );
                    DrawSprite( (Texture2D *)tex["south"], 10, -local.GetRow(1) );
                    DrawSprite( (Texture2D *)tex["east"], 10, local.GetRow(0) );
                    DrawSprite( (Texture2D *)tex["west"], 10, -local.GetRow(0) );
                }
                
                float dynamicLabelDot = 0.0f;
                string dynamicLabel;
                Vec4f dynamicLabelColor;
                Vec3f dynamicLabelDirection;
                
                static int prev_culled;
                int culled = 0;
                int drew = 0;
                for ( int i = 0; i < (int)stars.size(); i++ ) {
                    Sprite & s = stars[i];
                    if ( s.magnitude > 4 ) {
                        continue;
                    }
                    float dot = lookDir.Dot( s.direction );
                    if ( app_cull.GetVal() && dot < limit ) {
                        culled++;
                        continue;
                    }			
                    drew++;
                    Sighting sighting;
                    sighting.SetNum( i );
                    sighting.SetType( SOT_Star );
                    if ( sightings.count( sighting.id ) != 0 ) {
                        glColor4f( 1, 1, 1, sightingAlpha );
                        DrawString( s.name, s.direction );
                    } else if ( s.name.size() > 0 && dot > labelLimit && dot > dynamicLabelDot && s.magnitude < 2.5 ) {
                        glColor4f( 1, 1, 1, 1 );
                        float c = ( dot - labelLimit ) / ( 1.0 - labelLimit );
                        c = pow( c, 4 );
                        dynamicLabelDot = dot;
                        dynamicLabel = s.name;
                        dynamicLabelColor = Vec4f( 1, 1, 1, c );
                        dynamicLabelDirection = s.direction;
                    }
                }
                
                // draw stars
                if ( app_showStars.GetVal() ) {
                    r3Assert( stars.size() > 0 );
                    Sprite & s = stars[0];
                    s.tex->Bind( 0 );
                    s.tex->Enable( 0 );
                    glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
                    mod["stars"]->Draw();
                    s.tex->Disable( 0 );
                }
                
                // draw satellites
                if ( app_showSatellites.GetVal() ) {
                    double t = GetTime() / 1.0;
                    t = t - floor( t );
                    float r = t + 0;
                    float g = t + 1.0 / 3.0;
                    float b = t + 2.0 / 3.0;
                    r = ( cos( r * R3_PI * 2.0 ) * .5 + .5 ) * .5 + .5;
                    g = ( cos( g * R3_PI * 2.0 ) * .5 + .5 ) * .5 + .5;
                    b = ( cos( b * R3_PI * 2.0 ) * .5 + .5 ) * .5 + .5;
                    glColor4f( r, g, b, 1 );
                    Matrix4f invPhase = Rotationf( Vec3f( 0, 0, 1 ), phaseEarthRot ).GetMatrix4();
                    //Matrix4f phase = Rotationf( Vec3f( 0, 0, 1 ), -phaseEarthRot ).GetMatrix4();
                    Vec3f viewer = invPhase * SphericalToCartesian( RadiusEarthKm, latitude, longitude );
                    

#if APP_spacejunklite
                    Vec3f pos;
                    if( proTimeLeft >= 0 ) {
                        pos = SphericalToCartesian( RadiusEarthKm, latitude, longitude );                
                    } else {
                        pos = SphericalToCartesian( RadiusEarthKm, ToRadians( app_latitude.GetVal() ), ToRadians( app_longitude.GetVal() ) );
                        
                    }
#else
                    Vec3f pos = SphericalToCartesian( RadiusEarthKm, latitude, longitude );                
#endif
                    satSorter.Sort( pos );
                    
                    
                    for ( int i = 0; i < satSorter.Count(); i++ ) {
                        Satellite & sat = satellite[ satSorter[ i ] ];
                        Vec3f dir = invPhase * sat.pos - viewer;
                        dir.Normalize();
                        
                        float dot = lookDir.Dot( dir );
                        if ( app_cull.GetVal() && dot < limit ) {
                            continue;
                        }
                        
                        DrawSprite( (Texture2D *)tex["sat"], 5, dir );
                        
                        Sighting sighting;
                        sighting.SetNum( sat.id );
                        sighting.SetType( SOT_Satellite );
                        if ( sightings.count( sighting.id ) != 0 ) {
                            glColor4f( 1, 1, 1, sightingAlpha );
                            DrawString( sat.name, dir );
                        } else if ( dot > labelLimit && dot > dynamicLabelDot ) {
                            float c = ( dot - labelLimit ) / ( 1.0 - labelLimit );
                            c = pow( c, 4 );
                            dynamicLabelDot = dot;
                            dynamicLabel = sat.name;
                            dynamicLabelColor = Vec4f( 1, 1, 1, c );
                            dynamicLabelDirection = dir;
                        }
                        
                    }
                    
                    glColor4f( 1, 1, 0, .5 );
                    for( int i = 0; i < (int)satPath.size(); i++ ) {
                        SatellitePath & sp = satPath[i];
                        int last = (int)sp.pathPoint.size() - 1;
                        while( last >= 0 && sp.pathPoint[ last ].aboveThreshold == false ) {
                            last--;
                        }
                        if ( sp.special ) {
                            glColor4f( 1, 1, 1, .5f );
                        } else {
                            glColor4f( 1, 1, 0, .5f );					
                        }
                        // hashes
                        glBegin( GL_LINES );
                        for( int j = 0; j < last; j++ ) {
                            if ( ( j & 1 ) == 0 && j < (last - 2) ) {
                                int now = int( sp.pathPoint[ j + 0 ].minutesFromEpoch );
                                int nxt = int( sp.pathPoint[ j + 2 ].minutesFromEpoch );
                                if( now != nxt ) {
                                    j++;
                                    continue;
                                }
                            }
                            Vec3f p =   ( invPhase * sp.pathPoint[j].pos - viewer );
                            p.Normalize();
                            glVertex3f( p.x, p.y, p.z );
                        }
                        glEnd();
                    }
                    
                }
                
                
                AgeDynamicLabels();
                DrawDynamicLabels( nos );
                
                if ( dynamicLabelDot > 0.0f ) {
                    DynamicLabelInView( nos, dynamicLabel, dynamicLabelDirection, Vec4f( 1, 1, 1, 1), lookDir, limit );
                }
                
                
                if ( culled != prev_culled ) {
                    //Output( "Culled %d and drew %d", culled, drew );
                    prev_culled = culled;
                }
                
                nos->ClearReservations();
                {
                    if ( app_showPlanets.GetVal() ) {
                        for ( int i = 0; i < (int)solarsystem.size(); i++ ) {
                            Sprite & s = solarsystem[i];
                            Sighting sighting;
                            sighting.SetNum( i );
                            sighting.SetType( SOT_Planet );
                            Vec4f c = s.color;
                            if ( sightings.count( sighting.id ) != 0 ) {
                                c.w = sightingAlpha;
                            }
                            glColor4fv( c.Ptr() );
                            DrawSprite( s.tex, s.scale, s.direction );
                            if ( app_showLabels.GetVal() ) {
                                nos->DrawString( s.name, s.direction, lookDir, limit );
                            }
                        }
                        
                    }
                }
            }
        }
		delete nos;
	}
    
	void DisplayViewGlobe();	
	void DisplayViewGlobe() {
		DrawNonOverlappingStrings * nos = CreateNonOverlappingStrings();
		
        glDepthFunc( GL_LESS );
        glEnable( GL_DEPTH_TEST );
        { 
            ScopedPushMatrix push( GL_MODELVIEW ); // 0
            glMatrixLoadIdentityEXT( GL_MODELVIEW );
            r3::Matrix4f proj = r3::Perspective( r_fov.GetVal(),
                                                float(r_windowWidth.GetVal()) / r_windowHeight.GetVal(), 0.5f, 100.0f );
            Projection = proj;
            glMatrixLoadfEXT( GL_PROJECTION, proj.Ptr() );
            
            {                 
                r3::Matrix4f trans;
                trans.SetTranslate( Vec3f( 0, 0, -2 ) );
                glMatrixMultfEXT( GL_MODELVIEW, trans.Ptr() );
                
                float latitude = ToRadians( globeViewLat );
                float longitude = ToRadians( globeViewLon );
                Matrix4f xout = Rotationf( Vec3f( 0, 1, 0 ), -R3_PI / 2.0f ).GetMatrix4(); // current Lat/Lon now at { 0, 0, 1 }, with z up
                Matrix4f zup = Rotationf( Vec3f( 1, 0, 0 ), -R3_PI / 2.0f ).GetMatrix4();  // current Lat/Lon now at { 1, 0, 0 }, with z up
                Matrix4f lat = Rotationf( Vec3f( 0, 1, 0 ), latitude ).GetMatrix4();       // current Lat/Lon now at { 1, 0, 0 }, with y up
                Matrix4f lon = Rotationf( Vec3f( 0, 0, 1 ), -longitude ).GetMatrix4();
                //Matrix4f phase = Rotationf( Vec3f( 0, 0, 1 ), -GetCurrentEarthPhase() ).GetMatrix4();
                
                Matrix4f comp = ( xout * zup * lat * lon );
                
                ModelView = trans * comp;
                
                glColor3f( 1, 1, 1 );
                
                { 
                    ScopedPushMatrix push( GL_MODELVIEW ); // 1
                    glMatrixMultfEXT( GL_MODELVIEW, comp.Ptr() );
                    
                    if( app_showGlobe.GetVal() ) {
                        DrawEarth();			
                    }
                    Vec3f lookDir;
                    Vec3f frust[8];
                    {
                        r3::Matrix4f svproj = 
                        r3::Perspective( viewStarsFov, float(r_windowWidth.GetVal()) / r_windowHeight.GetVal(), 0.5f, 100.0f );
                        float svlatitude = ToRadians( app_latitude.GetVal() );
                        float svlongitude = ToRadians( app_longitude.GetVal() );
                        Matrix4f svlat = Rotationf( Vec3f( 0, 1, 0 ), svlatitude ).GetMatrix4();       // current Lat/Lon now at { 1, 0, 0 }, with y up
                        Matrix4f svlon = Rotationf( Vec3f( 0, 0, 1 ), -svlongitude ).GetMatrix4();
                        Matrix4f svcomp = ( xout * zup * svlat * svlon );
                        
                        Matrix4f mvp = svproj * orientation * svcomp;
                        Matrix4f imvp = mvp.Inverse();
                        frust[0] = imvp * Vec3f( -1, -1, -1 );
                        frust[1] = imvp * Vec3f(  1, -1, -1 );
                        frust[2] = imvp * Vec3f(  1,  1, -1 );
                        frust[3] = imvp * Vec3f( -1,  1, -1 );
                        frust[4] = imvp * Vec3f( -1, -1,  1 );
                        frust[5] = imvp * Vec3f(  1, -1,  1 );
                        frust[6] = imvp * Vec3f(  1,  1,  1 );
                        frust[7] = imvp * Vec3f( -1,  1,  1 );
                        lookDir = imvp * Vec3f( 0, 0, -1 );
                        lookDir.Normalize();			
                    }
                    
                    //ScopedEnable smooth( GL_POINT_SMOOTH );
                    ScopedEnable blend( GL_BLEND );
                    ScopedEnable atest( GL_ALPHA_TEST );
                    glAlphaFunc( GL_GEQUAL,  0.1 );
                    
                    Matrix4f currTrans;
                    glGetFloatv( GL_MODELVIEW_MATRIX, currTrans.Ptr() );
                    Matrix4f billboard = ToMatrix4( ToMatrix3( currTrans.Inverse() ) );
                    // current view position
                    {
                        Vec3f currPos = SphericalToCartesian( 1.005, ToRadians( app_latitude.GetVal() ), ToRadians( app_longitude.GetVal() ) );
                        { 
                            ScopedPushMatrix push( GL_MODELVIEW ); // 3
                            Matrix4f trans;
                            trans.SetTranslate( currPos );
                            glMatrixMultfEXT( GL_MODELVIEW, trans.Ptr() );
                            Matrix4f frustScale;
                            frustScale.SetScale( 0.001f );
                            glMatrixMultfEXT( GL_MODELVIEW, frustScale.Ptr() );
                            
                            //Vec3f lookAt = currPos + lookDir * 0.02f;
                            glColor4f( 1, 0, 0, 1 );
                            glBegin( GL_LINES );
                            glVertex3fv( frust[0].Ptr() ); glVertex3fv( frust[4].Ptr() );
                            glVertex3fv( frust[1].Ptr() ); glVertex3fv( frust[5].Ptr() );
                            glVertex3fv( frust[2].Ptr() ); glVertex3fv( frust[6].Ptr() );
                            glVertex3fv( frust[3].Ptr() ); glVertex3fv( frust[7].Ptr() );
                            
                            glVertex3fv( frust[0].Ptr() ); glVertex3fv( frust[1].Ptr() );
                            glVertex3fv( frust[1].Ptr() ); glVertex3fv( frust[2].Ptr() );
                            glVertex3fv( frust[2].Ptr() ); glVertex3fv( frust[3].Ptr() );
                            glVertex3fv( frust[3].Ptr() ); glVertex3fv( frust[0].Ptr() );
                            
                            glVertex3fv( frust[4].Ptr() ); glVertex3fv( frust[5].Ptr() );
                            glVertex3fv( frust[5].Ptr() ); glVertex3fv( frust[6].Ptr() );
                            glVertex3fv( frust[6].Ptr() ); glVertex3fv( frust[7].Ptr() );
                            glVertex3fv( frust[7].Ptr() ); glVertex3fv( frust[4].Ptr() );
                            
                            glEnd();
                        }
                        {
                            glPolygonOffset( 0, textDepthBias );
                            ScopedEnable po( GL_POLYGON_OFFSET_FILL );                            
                            glMultiTexEnviEXT( GL_TEXTURE0 + 0, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
                            DrawSpriteAtLocation( stars[0].tex, currPos, billboard );
                            DrawStringAtLocation( "you are here", currPos, billboard );
                        }
                        
                    }
                    
                    if ( app_showSatellites.GetVal() ) {
                        //glPointSize( 3 );
                        //ApplyTransform( phase );
                        ScopedEnable bl( GL_BLEND );
                        glMultiTexEnviEXT( GL_TEXTURE0 + 0, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

                        
#if APP_spacejunklite
                        Vec3f pos;
                        if( proTimeLeft >= 0 ) {
                            pos = SphericalToCartesian( RadiusEarthKm, latitude, longitude );                
                        } else {
                            pos = SphericalToCartesian( RadiusEarthKm, ToRadians( app_latitude.GetVal() ), ToRadians( app_longitude.GetVal() ) );
                            
                        }
#else
                        Vec3f pos = SphericalToCartesian( RadiusEarthKm, latitude, longitude );                
#endif
                        satSorter.Sort( pos );
                        
                        glColor4f( 1, 1, 0, 1 );
                        

                        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                        for ( int i = 0; i < satSorter.Count(); i++ ) {
                            Satellite & s = satellite[ satSorter[ i ] ];
                            if ( s.special ) {
                                glColor4f( 1, 1, 1, 1 );
                            } else {
                                glColor4f( 1, 1, 0, 1 );					
                            }
                            Vec3f p = s.pos;
                            p /= RadiusEarthKm;
                            DrawSpriteAtLocation( stars[0].tex, p, billboard );
                        }
                        
                        { 
                            ScopedPushMatrix push( GL_MODELVIEW );     // 3
                            ScopedEnable po( GL_POLYGON_OFFSET_FILL );
                            for ( int i = 0; i < satSorter.Count(); i++ ) {
                                Satellite & s = satellite[ satSorter[ i ] ];
                                if ( s.special ) {
                                    glColor4f( 1, 1, 1, 1 );
                                } else {
                                    glColor4f( 1, 1, 0, 1 );					
                                }
                                Vec3f p = s.pos;
                                p /= RadiusEarthKm;
                                DrawStringAtLocation( s.name, p, billboard );
                            }
                        }
                        
                        for( int i = 0; i < (int)satPath.size(); i++ ) {
                            SatellitePath & sp = satPath[i];
                            int last = (int)sp.pathPoint.size() - 1;
                            while( last >= 0 && sp.pathPoint[ last ].aboveThreshold == false ) {
                                last--;
                            }
                            if ( sp.special ) {
                                glColor4f( 1, 1, 1, .5f );
                            } else {
                                glColor4f( 1, 1, 0, .5f );					
                            }
                            // hashes
                            glBegin( GL_LINES );
                            for( int j = 0; j < last; j++ ) {
                                if ( ( j & 1 ) == 0 && j < (last - 2) ) {
                                    int now = int( sp.pathPoint[ j + 0 ].minutesFromEpoch );
                                    int nxt = int( sp.pathPoint[ j + 2 ].minutesFromEpoch );
                                    if( now != nxt ) {
                                        j++;
                                        continue;
                                    }
                                }
                                Vec3f p = sp.pathPoint[j].pos / float( RadiusEarthKm );
                                glVertex3f( p.x, p.y, p.z );
                            }
                            glEnd();
                            // the space between hashes
                            glColor4f( 0, 0, 0, .5 );
                            glBegin( GL_LINES );
                            for( int j = 1; j < last; j++ ) {
                                Vec3f p = sp.pathPoint[j].pos / float( RadiusEarthKm );
                                glVertex3f( p.x, p.y, p.z );
                            }
                            glEnd();				
                        }
                        
                    }
                    
                }
            }
            
            
            {
                ScopedEnable blend( GL_BLEND );
                ScopedEnable atest( GL_ALPHA_TEST );
                
                { 
                    ScopedPushMatrix push( GL_MODELVIEW ); // 2
                    
                    r3::Matrix4f scale;
                    scale.SetScale( Vec3f( 10, 10, 10 ) );
                    glMatrixMultfEXT( GL_MODELVIEW, scale.Ptr() );
                    
                    float latitude = ToRadians( globeViewLat );
                    float longitude = ToRadians( globeViewLon );
                    Matrix4f xout = Rotationf( Vec3f( 0, 1, 0 ), -R3_PI / 2.0f ).GetMatrix4(); // current Lat/Lon now at { 0, 0, 1 }, with z up
                    Matrix4f zup = Rotationf( Vec3f( 1, 0, 0 ), -R3_PI / 2.0f ).GetMatrix4();  // current Lat/Lon now at { 1, 0, 0 }, with z up
                    Matrix4f lat = Rotationf( Vec3f( 0, 1, 0 ), latitude ).GetMatrix4();       // current Lat/Lon now at { 1, 0, 0 }, with y up
                    Matrix4f lon = Rotationf( Vec3f( 0, 0, 1 ), -longitude ).GetMatrix4();
                    Matrix4f phase = Rotationf( Vec3f( 0, 0, 1 ), -GetCurrentEarthPhase() ).GetMatrix4();
                    
                    Matrix4f comp = ( xout * zup * lat * lon * phase);
                    
                    glColor3f( 1, 1, 1 );
                    
                    { 
                        ScopedPushMatrix push( GL_MODELVIEW ); // 3
                        glMatrixMultfEXT( GL_MODELVIEW, comp.Ptr() );
                        glMultiTexEnviEXT( GL_TEXTURE0 + 0, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

                        // draw stars
                        if ( app_showStars.GetVal() ) {
                            stars[0].tex->Bind( 0 );
                            stars[0].tex->Enable( 0 );
                            mod["stars"]->Draw();
                            stars[0].tex->Disable( 0 );
                        }
                        
                        if ( app_showPlanets.GetVal() ) {
                            for ( int i = 0; i < (int)solarsystem.size(); i++ ) {
                                Sprite & s = solarsystem[i];
                                glColor4fv( s.color.Ptr() );
                                DrawSprite( s.tex, s.scale, s.direction );
                            }
                        }                        
                    }
                }
                
            }
        }
        glDisable( GL_DEPTH_TEST );
		delete nos;
	}
    
	void DisplayInitializing() {
		double t = GetTime();
		if( t0 == 0.0 ) {
            t0 = t;
        }
		{
            ScopedDisable sdt( GL_DEPTH_TEST );
            ScopedPushMatrix spush( GL_MODELVIEW );
            glMatrixLoadIdentityEXT( GL_MODELVIEW );
            
            float dt = t - t0;
            float darken = std::max( 0.2f, 1.0f - ( dt * 0.2f ) );
            glColor4f( darken, darken, darken, 1.0f );
            DrawSprite( (Texture2D *)tex["splash"], Bounds2f( -1, -1, 1, 1 ) );

            glDisableIndexedEXT( GL_TEXTURE_2D, 0 );
            
            Matrix4f scale;
            scale.SetScale( Vec3f( .95, .95, .95 ) );
            glMatrixMultfEXT( GL_MODELVIEW, scale.Ptr() );
            
            Matrix4f scaleBias;
            scaleBias.SetScale( Vec3f( 2, 2, 1 ) );
            scaleBias.SetTranslate( Vec3f( -1, -1, 0 ) );
            glMatrixMultfEXT( GL_MODELVIEW, scaleBias.Ptr() );
            
            
            // draw progress bar
            float pct = float( app_loadProgress.GetVal() ) / float( app_loadProgressFinal.GetVal() );
            
            glColor4f( .2f, .2f, 1.0f, 1.0f );
            glBegin( GL_QUADS );
            glVertex2f( 0.0f, 0.50f );
            glVertex2f( pct,  0.50f );
            glVertex2f( pct,  0.52f );
            glVertex2f( 0.0f, 0.52f );
            glEnd();
            
            glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
            
            if ( IsRenderInitialized() ) {
                string ls = loadingString;
                glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                ScopedEnable blend( GL_BLEND );
                DrawString2D( ls, Bounds2f( 0.05, 0.42, 0.95, 0.48 ) ); 
            }
        }
        
		GfxSwapBuffers();
		GfxCheckErrors();
	}
    
    void RenderQR();
    void RenderQR() {
        if( app_showQR.GetVal() == false ) {
            return;
        }
        
        { 
            ScopedPushMatrix push( GL_MODELVIEW );
            
            float w = min( r_windowWidth.GetVal() - 20, r_windowHeight.GetVal() - 20 );
            float ox = ( r_windowWidth.GetVal() - w ) / 2;
            float oy = ( r_windowHeight.GetVal() - w ) / 2; 
            
            Matrix4f scaleBias;
            scaleBias.SetTranslate( Vec3f( ox, oy, 0 ) );
            scaleBias.SetScale( Vec3f( w, w, 1 ) );
            glMatrixMultfEXT( GL_MODELVIEW, scaleBias.Ptr() );
            
            tex["qr"]->Bind( 0 );
            tex["qr"]->Enable( 0 );
            TexEnvCombineAlphaModulate( 0 );
            
            glDisable( GL_BLEND );
            
            
            glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
            glBegin( GL_QUADS );
            glMultiTexCoord2f( GL_TEXTURE0, 0.0f, 0.0f );
            glVertex2f( 0.0f, 0.0f );
            glMultiTexCoord2f( GL_TEXTURE0, 1.0f, 0.0f );
            glVertex2f( 1.0f, 0.0f );
            glMultiTexCoord2f( GL_TEXTURE0, 1.0f, 1.0f );
            glVertex2f( 1.0f,  1.0f );
            glMultiTexCoord2f( GL_TEXTURE0, 0.0f, 1.0f );
            glVertex2f( 0.0f, 1.0f );
            glEnd();
            
            glEnable( GL_BLEND );
            
            tex["qr"]->Disable( 0 );
            
        }
        
    }
    
#if APP_spacejunklite
    void RenderProTimeLeft() {
        int w = r_windowWidth.GetVal();
        int h = r_windowHeight.GetVal();
		float dpiRatio = r_windowDpi.GetVal() / 160.0f;
        
        ScopedEnable scdt( GL_DEPTH_TEST );
        ScopedEnable scbl( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		
		Bounds2f b( 4.0f * dpiRatio, h - 64.f * dpiRatio , w - (4.0f * dpiRatio) , h - 44.f * dpiRatio );
		
        double t = GetTime() - t0;
        float a = 0.6 + 0.4 * sin( 2 * t );
        Vec4f c( 1.0, 1.0, 1.0, a );
        btn["appstore"]->color = c;

        if ( proTimeLeft < 0 ) {
            btn["appstore"]->color = Vec4f( 1,1,1,1 );
        }
        
        glColor4fv( c.Ptr() );
        
        char buf[80];
        string s;
        if( proTimeLeft >= 0 ) {
            s = Localize( "Pro Mode time remaining" );
            r3Sprintf( buf, ": %d", proTimeLeft );            
        } else {
            s = Localize( "Pro Mode begins" );
            r3Sprintf( buf, ": %d", proTimeWindow - proTimeDuty + proTimeLeft );            
            
        }
		DrawLocalizedString2D( s + buf, b, NULL, Align_Mid );
        //glColor4f( 1, 1, 1, 1 );
        
    }
#endif
    
    
    void CheckStatusMessageTriggers();
	void CheckStatusMessageTriggers() {
        
		// check status message triggers
		{
			static bool doStatusUpdates = false;
			string statusMsg;
			{
				static AppModeEnum prevAppMode;
				if ( appMode != prevAppMode ) {
					statusMsg = appMode == AM_ViewGlobe ? "Earth Viewing Mode" : "Star Viewing Mode";
					prevAppMode = appMode;
				}
				static bool prevNightViewing;
				if ( app_nightViewing.GetVal() != prevNightViewing ) {
					statusMsg = app_nightViewing.GetVal() ? "Night Mode: On" : "Night Mode: Off"; 
					prevNightViewing = app_nightViewing.GetVal();
				}
				static bool prevChangingLocation;
				if ( app_changeLocation.GetVal() != prevChangingLocation ) {
					statusMsg = app_changeLocation.GetVal() ? "Drag to change location." : "Done changing location.";
					if( app_changeLocation.GetVal() ) {
						app_useCoreLocation.SetVal( false );
					}
					prevChangingLocation = app_changeLocation.GetVal();
				}
				static bool prevUseCoreLocation;
				if ( app_useCoreLocation.GetVal() != prevUseCoreLocation ) {
					statusMsg = app_useCoreLocation.GetVal() ? "Location Services: Enabled." : "Location Services: Disabled";
					if( app_useCoreLocation.GetVal() ) {
						app_changeLocation.SetVal( false );
					}
					prevUseCoreLocation = app_useCoreLocation.GetVal();
				}
				static bool prevShowSatellites;
				if ( app_showSatellites.GetVal() != prevShowSatellites ) {
					statusMsg = app_showSatellites.GetVal() ? "Satellite Viewing: Enabled" : "Satellite Viewing: Disabled";
					prevShowSatellites = app_showSatellites.GetVal();
				}
				static bool prevUseCompass;
				if ( app_useCompass.GetVal() != prevUseCompass ) {
					statusMsg = app_useCompass.GetVal() ? "Digital Compass Mode" : "Manual Navigation Mode";
					prevUseCompass = app_useCompass.GetVal();
				}
                
				static float prevNtpTimeOffset;
				if ( app_ntpTimeOffset.GetVal() != prevNtpTimeOffset ) {
					char msg[128];
					r3Sprintf( msg, "NTP time adjust = %f", app_ntpTimeOffset.GetVal() );
					statusMsg = msg;
                    SetTimeOffset( app_ntpTimeOffset.GetVal() );
					prevNtpTimeOffset = app_ntpTimeOffset.GetVal();
				}
                
				static bool prevShowQR;
                if ( app_showQR.GetVal() != prevShowQR ) {
                    prevShowQR = app_showQR.GetVal();
                    statusMsg = app_showQR.GetVal() ? "Share Space Junk Pro!" : "Thank you!";
                }
                
                static int prevLicense = 0;
                if ( app_license.GetVal() != prevLicense ) {
                    prevLicense = app_license.GetVal();
                    statusMsg = app_license.GetVal() == 1 ? "License acquired!" : "License acquisition failed.";
                }
                
			}			
			if ( doStatusUpdates && statusMsg.size() > 0 ) {
				SetStatus( statusMsg.c_str() );
			}
			doStatusUpdates = true;
		}
		
	}
	
	
	void Display() {
		Initialize();  // do this once instead?
		
        static int displayCount = 0;
        displayCount++;
        if( ( displayCount % 0x1f ) == 0 ) {
            //Output( "Display()" );
        } 
        condRender.Broadcast();
        
		frameBeginTime = GetTime();
		CleanupSightings();
		menu.Tick();
        
#if APP_spacejunklite
        proTimeLeft = -( int( GetTime() ) % proTimeWindow ) - 1 + proTimeDuty;
        if( proTimeLeft >= 0 ) {
            app_maxSatellites.SetVal( 40 );            
        } else {
            app_maxSatellites.SetVal( 5 );
        }
#endif
        
		ScopedGfxContextAcquire ctx( drawContext );
        glMatrixLoadIdentityEXT( GL_PROJECTION );
        glClearColor( 0, 0, 0, 0 );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		if( appMode == AM_Initializing ) {
			DisplayInitializing();
			return;
		}
        
		CheckStatusMessageTriggers();
		
		UpdateSolarSystemSprites();
		ApplyInputInertia();		
		
		if ( app_nightViewing.GetVal() ) {
			glColorMask( true, false, false, true );
		} 
        
		if ( app_showSatellites.GetVal() ) {
			ComputeSatellitePositions( satellite );
            
			GetSatelliteFlyovers( app_latitude.GetVal(), app_longitude.GetVal(), satPath );
		}
        
		orientation = app_useCompass.GetVal() ? platformOrientation : manualOrientation;
        
		UpdateLatLon();
        
		
		switch ( appMode ) {
			case AM_ViewStars:
				DisplayViewStars();
				break;
			case AM_ViewGlobe:
				DisplayViewGlobe();
				break;
			default:
				break;
		}
        
		// UI
		PlaceButtons();
        
        {
            ScopedDisable dt( GL_DEPTH_TEST );
            ScopedPushMatrix push( GL_MODELVIEW ); // 0
            glMatrixLoadIdentityEXT( GL_MODELVIEW );
            Matrix4f o = Ortho<float>( 0, r_windowWidth.GetVal(), 0, r_windowHeight.GetVal(), -1, 1 );
            glMatrixLoadfEXT( GL_PROJECTION, o.Ptr() );
            
            RenderQR();
#if APP_spacejunklite
            RenderProTimeLeft();
#endif
            
            menu.backgroundColor = Vec4f( 0, 0, 0, appMode == AM_ViewGlobe ? 0.85 : 0.65 );
            menu.Render();		
            
            RenderStatus();            
            
            if ( app_satelliteUrl.GetVal().find( "favorites.php" ) != string::npos ) {
                float w = r_windowWidth.GetVal();
                float h = r_windowHeight.GetVal();
                glColor4f( 1.0, .4, .2, .5 );
                glBlendFunc( GL_SRC_ALPHA,  GL_ONE_MINUS_SRC_ALPHA );
                glEnable( GL_BLEND );
                if( app_fbId.GetVal().size() == 0 ) {
                    DrawString2D( "log in to Facebook for favorites", Bounds2f(  0.0f,  h / 2.0f - 50.f, w, h / 2.0f + 50.f ) );
                } else if ( satellite.size() == 0 ) {
                    DrawString2D( "No favorites selected.", Bounds2f(  0.0f,  h / 2.0f + 50.f, w, h / 2.0f + 100.f ) );
                    DrawString2D( "Manage favorites at", Bounds2f(  0.0f,  h / 2.0f +  0.f, w, h / 2.0f + 50.f ) );
                    DrawString2D( "http://home.xyzw.us/star3map/user.php", Bounds2f(  0.0f,  h / 2.0f - 50.f, w, h / 2.0f +  0.f ) );
                }
                glDisable( GL_BLEND );
            }
            
            if( console != NULL ) {
                console->Draw();
            }
            
        }
        
		static bool firstFrameSwapped = false;
		if ( firstFrameSwapped == false ) {
			firstFrameSwapped = true;
			Output( "Swapping first frame now." );
		}
		
		if ( app_nightViewing.GetVal() ) {
			glColorMask( true, true, true, true );
		} 
        
		frameEndTime = GetTime();
		
		GfxSwapBuffers();
		GfxCheckErrors();
        
	}		
    
	
	
    
}


