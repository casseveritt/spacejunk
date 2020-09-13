/*
 *  render
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
#include "localize.h"

#include "r3/draw.h"
#include "r3/font.h"
//#include "r3/gl.h"
#include "r3/output.h"
#include "r3/var.h"

#include <GL/Regal.h>

using namespace r3;
using namespace std;

VarString app_font( "app_font", "main app font", 0, "DroidSansFallback.ttf" );
//VarString app_font( "app_font", "main app font", 0, "LiberationSans-Regular.ttf" );
VarInteger app_fontSize( "app_fontSize", "main app font rasterization size", Var_Archive, 12 );
VarFloat app_fontScale( "app_fontScale", "main app font rendering scale", Var_Archive, 12 );

VarFloat app_starScale( "app_starScale", "scale of star sprite rendering", Var_Archive, 5 );
VarFloat app_scale( "app_scale", "scale of stuff in angle space", Var_Archive, 0.0001f );
VarFloat app_aspectRatio( "app_aspectRatio", "aspect ratio of the screen", 0, 1.0f );

using namespace star3map;

extern VarFloat r_fov;
extern VarString app_locale;
extern VarInteger r_windowHeight;

namespace {
	
	Font *font;
	float fov;
	float fovFontScale;

	int transformVersion = 0;
	vector< Matrix4f > transformStack;

}



namespace star3map {

	// Our object space is z-up (which is the zenith)  
    Matrix4f RotateTo( const Vec3f & );
	Matrix4f RotateTo( const Vec3f & direction ) {
		Matrix4f m;
		m.MakeIdentity();
		Vec3f dx = UpVector.Cross( direction );
		dx.Normalize();
		Vec3f dup = direction.Cross( dx );
		Quaternionf q( Vec3f( 0, 0, -1 ), Vec3f( 0, 1, 0), direction, dup );
		q.GetValue(m);
		return m;
	}
	
	
	
	Vec3f UpVector;
    
    Matrix4f GetTransform() {
        Matrix4f m;
        glGetFloatv( GL_MODELVIEW_MATRIX, m.Ptr() );
        return m;
    }
	
	
	void DrawQuad( float radius, const Vec3f & direction ) {
		Matrix4f m = RotateTo( direction );
		glMatrixPushEXT( GL_MODELVIEW );
		glMatrixMultfEXT( GL_MODELVIEW, m.Ptr() );
		glMatrixScalefEXT( GL_MODELVIEW, radius * app_scale.GetVal(), radius * app_scale.GetVal(), 1.f );
		glMatrixTranslatefEXT( GL_MODELVIEW, 0, 0, -1 );
		r3::DrawQuad( -10, -10, 10, 10 );
        glMatrixPopEXT( GL_MODELVIEW );
	}

	void DrawSprite( Texture2D *tex, r3::Bounds2f bounds ) {
		float s = float( tex->Width() ) / float( tex->PaddedWidth() );
		float t = float( tex->Height() ) / float( tex->PaddedHeight() );
		tex->Bind( 0 );
		tex->Enable( 0 );
        glMultiTexEnviEXT( GL_TEXTURE0, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		r3::DrawTexturedQuad( bounds.Min().x, bounds.Min().y, bounds.Max().x, bounds.Max().y, 0, 0, s, t );
		tex->Disable( 0 );
	}
	
	void DrawSprite( Texture2D *tex, float radius, const Vec3f & direction ) {
		// beef up the width and height since the texture will make it
		// effectively smaller
        if( tex == NULL ) {
            Output( "DrawSprite got NULL texture." );
            return;
        }
		radius *= app_starScale.GetVal();
		Matrix4f m = RotateTo( direction );
		glMatrixPushEXT( GL_MODELVIEW );
		glMatrixMultfEXT( GL_MODELVIEW, m.Ptr() );
		glMatrixScalefEXT( GL_MODELVIEW, radius * app_scale.GetVal(), radius * app_scale.GetVal(), 1.f );
		glMatrixTranslatefEXT( GL_MODELVIEW, 0, 0, -1 );
		tex->Bind( 0 );
		tex->Enable( 0 );
		r3::DrawSprite( -10, -10, 10, 10 );
		tex->Disable( 0 );
        glMatrixPopEXT( GL_MODELVIEW );
	}
	bool renderInitialized = false;
    void InitAndUpdate();
	void InitAndUpdate() {
		if ( font == NULL ) {
			string fontName = "DroidSans.ttf";
			string fallback = "";
			string locale = app_locale.GetVal();
			if( locale.find( "ar_" ) != string::npos ) {
				fallback = fontName;
				fontName = "DroidSansArabic.ttf";
			} else if( locale.find( "he_" ) != string::npos || locale.find( "iw_" ) != string::npos ) {
				fallback = fontName;
				fontName = "DroidSansHebrew.ttf";
			} else if( locale.find( "th_" ) != string::npos ) {
				fallback = fontName;
				fontName = "DroidSansThai.ttf";
			} else if( locale.find( "zh_" ) != string::npos ||
                      locale.find( "ko_" ) != string::npos ||
                      locale.find( "ja_" ) != string::npos ||
                      0 // locale.find( "ru_" ) != string::npos
                      ) {
                fallback = fontName;
                fontName = "DroidSansFallback.ttf";
            }
			
			font = r3::CreateStbFont( fontName, fallback, (float)app_fontSize.GetVal() ); 			
		}
		if ( fov != r_fov.GetVal() ) {
			fov = r_fov.GetVal();
			fovFontScale = app_fontScale.GetVal() * sin( ToRadians( fov ) );
		}		
		static int localTransformVersion = 0;
		if ( transformVersion != localTransformVersion ) {
			
		}
		renderInitialized = true;
	}

	void InitializeRender() {
		//Output( "InitializeRender()" );
		InitAndUpdate();
	}

	bool IsRenderInitialized() {
		return renderInitialized;
	}
	
	OrientedBounds2f StringBounds( const std::string & s, const Vec3f & direction ) {
		InitAndUpdate();
		Bounds2f b = font->GetStringDimensions( s, fovFontScale );
		
		Matrix4f m = RotateTo( direction );
		Matrix4f mt;
		mt.SetScale( app_scale.GetVal() );
		mt.SetTranslate( Vec3f( 0, 0, -1 ) );
        
		Matrix4f xf = GetTransform() * m * mt;
		
		Vec4f pts[4];
		Vec2f bias( -b.Width() / 2.f, -1.5f * b.Height() );
		pts[0] = xf * Vec4f( b.Min().x + bias.x, b.Min().y + bias.y );
		pts[1] = xf * Vec4f( b.Max().x + bias.x, b.Min().y + bias.y );
		pts[2] = xf * Vec4f( b.Max().x + bias.x, b.Max().y + bias.y );
		pts[3] = xf * Vec4f( b.Min().x + bias.x, b.Max().y + bias.y );
		OrientedBounds2f ob;
		for ( int i = 0; i < 4; i++ ) {
			float w = pts[i].w;
			if ( w > 0 ) {
				ob.vert[ i ] = Vec2f( pts[ i ].x / w, pts[ i ].y / w );
			} else {
				return OrientedBounds2f(); // just send an empty bounds if we have negative w
			}
		}
		ob.empty = false;
		return ob;
	}

	void DrawLocalizedString2D( const std::string & s, const Bounds2f & b, Font *f, TextAlignmentEnum hAlign, TextAlignmentEnum vAlign ) {
		InitAndUpdate();
		if ( f == NULL ) {
			f = font;
		}
		Bounds2f u = f->GetStringDimensions( s, 1 );
		u.ll.y = f->GetDescent();
		u.ur.y = f->GetAscent();
		
		float hs = b.Height() / u.Height();
		float sc = min<float>( b.Width() / u.Width(), hs );
        float tx = b.ll.x - AlignAdjust( b.Width(), hAlign );
        float ty = b.ll.y - AlignAdjust( b.Height(), vAlign );
		glMatrixPushEXT( GL_MODELVIEW );
		glMatrixTranslatefEXT( GL_MODELVIEW, tx, ty, 0 );
		glMatrixScalefEXT( GL_MODELVIEW, sc, sc, 1.f );
		glMatrixTranslatefEXT( GL_MODELVIEW, 0, -u.ll.y, 0 );
		f->Print( s, 0, 0, 1, hAlign, vAlign );
		glMatrixPopEXT( GL_MODELVIEW );		
	}
    
    void DrawString2D( const std::string & nls, const Bounds2f & b, Font *f, TextAlignmentEnum hAlign, TextAlignmentEnum vAlign ) {
		string s = Localize( nls );
        DrawLocalizedString2D( s, b, f, hAlign, vAlign );
    }
	
	void DrawString( const std::string & nls, const Vec3f & direction ) {
		string s = Localize( nls );
		InitAndUpdate();
		Bounds2f b = font->GetStringDimensions( s, fovFontScale );
		
		Matrix4f m = RotateTo( direction );
		glMatrixPushEXT( GL_MODELVIEW );
		glMatrixMultfEXT( GL_MODELVIEW, m.Ptr() );
		glMatrixScalefEXT( GL_MODELVIEW, app_scale.GetVal(), app_scale.GetVal(), 1.f );
		glMatrixTranslatefEXT( GL_MODELVIEW, 0, 0, -1 );
		font->Print( s, -b.Width() / 2.f, -1.5f * b.Height(), (float)fovFontScale );
		glMatrixPopEXT( GL_MODELVIEW );
	}
	
	void DrawSpriteAtLocation( Texture2D *tex, const Vec3f & position, const Matrix4f & rotation ) {
		InitAndUpdate();
        float s = 150 * app_scale.GetVal();
        ScopedPushMatrix mvp( GL_MODELVIEW );
		glMatrixTranslatefEXT( GL_MODELVIEW, position.x, position.y, position.z );
		glMatrixMultfEXT( GL_MODELVIEW, rotation.Ptr() );
        {
            Matrix4f mv, p;
            glGetFloatv( GL_MODELVIEW_MATRIX, mv.Ptr() );
            glGetFloatv( GL_PROJECTION_MATRIX, p.Ptr() );
            Matrix4f mvp = p * mv;
            Vec3f p0( 0, 0, 0 );
            Vec3f p1( 1, 1, 0 );
            mvp.MultMatrixVec( p0 );
            mvp.MultMatrixVec( p1 );
            float h = ( p1.y - p0.y ) * r_windowHeight.GetVal();
            if( h > 1000 ) {
                float cap = 1000 / h;
                glMatrixScalefEXT( GL_MODELVIEW_MATRIX, cap, cap, 1.0f);
            }
        }
		tex->Bind( 0 );
		tex->Enable( 0 );
		r3::DrawSprite( -s, -s, s, s );
		tex->Disable( 0 );
	}

	void DrawStringAtLocation( const std::string & nls, const Vec3f & position, const Matrix4f & rotation ) {
		string s = Localize( nls );
		InitAndUpdate();
		float fontScale = 14.f;
		Bounds2f b = font->GetStringDimensions( s, fontScale );
        float bh = b.Height();
		
		
        ScopedPushMatrix mvpush( GL_MODELVIEW );
		glMatrixTranslatefEXT( GL_MODELVIEW, position.x, position.y, position.z );
		glMatrixMultfEXT( GL_MODELVIEW, rotation.Ptr() );
		float sc = 0.75 * app_scale.GetVal();
		glMatrixScalefEXT( GL_MODELVIEW, sc, sc, 1.f );
        {
            Matrix4f mv, p;
            glGetFloatv( GL_MODELVIEW_MATRIX, mv.Ptr() );
            glGetFloatv( GL_PROJECTION_MATRIX, p.Ptr() );
            Matrix4f mvp = p * mv;
            Vec3f p0( 0, 0, 0 );
            Vec3f p1( 480 * fontScale, 480 * fontScale, 0 );
            mvp.MultMatrixVec( p0 );
            mvp.MultMatrixVec( p1 );
            float h = ( p1.y - p0.y ) * r_windowHeight.GetVal();
            if( h > bh ) {
                float cap = bh / h;
                glMatrixScalefEXT( GL_MODELVIEW_MATRIX, cap, cap, 1.0f);
            }
        }
		font->Print( s, -b.Width() / 2.f, -1.5f * b.Height(), fontScale );
	}
	
	void DrawDebugGrid() {
		
		for ( int i = 0; i < 360; i+= 15 ) {
			glBegin( GL_LINE_STRIP );
			for ( int j = -90; j <= 90; j+= 15 ) {
				Vec3f v = SphericalToCartesian( 1.0f, ToRadians(float(j)), ToRadians(float(i)) );
				Vec3f c = v * 0.5f + Vec3f(0.5f, 0.5f, 0.5f);
				glColor3f( c.x, c.y, c.z );
				glVertex3f( v.x, v.y, v.z );
			}
			glEnd();
		}
		for ( int j = -90; j < 90; j+= 15 ) {
			glBegin( GL_LINE_STRIP );
			for ( int i = 0; i <= 360; i+= 15 ) {
				Vec3f v = SphericalToCartesian( 1.0f, ToRadians(float(j)), ToRadians(float(i)) );
				Vec3f c = v * 0.5f + Vec3f(0.5f, 0.5f, 0.5f);
				glColor3f( c.x, c.y, c.z );
				glVertex3f( v.x, v.y, v.z );
			}
			glEnd();
		}
		
	}

	Vec3f SphericalToCartesian( float rho, float phi, float theta ) {
		Vec3f p( cos( phi ) * cos( theta ), cos( phi ) * sin( theta ), sin( phi ) );
		p *= rho;
		return p;	
	}
	
	Vec3f CartesianToSpherical( const Vec3f & c ) {
		Vec3f s;
		// rho
		s.x = c.Length();
		// phi
		s.y = atan2( c.z, sqrt( c.x * c.x + c.y * c.y ) );
		// theta
		s.z = atan2( c.y, c.x );
		return s;
	}
	
}

