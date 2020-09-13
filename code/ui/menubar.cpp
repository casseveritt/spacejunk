/*
 *  menubar
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


#include "ui/menubar.h"
#include "r3/draw.h"

#include <GL/Regal.h>

using namespace r3;
using namespace star3map;

namespace star3map {
	
	MenuBar::MenuBar() {
		
	}
	
	void MenuBar::Render() {
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glEnable( GL_BLEND );
		Bounds2f b = bounds;
		b.Add( Vec2f( rect.ur.x - b.ll.x, b.ll.y ) );
		b.Inset( -borderWidth );
		glColor4f( backgroundColor.x, backgroundColor.y, backgroundColor.z, backgroundColor.w );
        glBegin( GL_QUADS );
		glVertex2f( b.ll.x, b.ll.y );
		glVertex2f( b.ur.x, b.ll.y );
		glVertex2f( b.ur.x, b.ur.y );
		glVertex2f( b.ll.x, b.ur.y );
        glEnd();
		for ( int i = 0; i < (int)buttons.size(); i++ ) {
			buttons[i]->Draw();
		}
        glDisable( GL_BLEND );
	}
	
	void MenuBar::Clear() {
		buttons.clear();
		bounds.Clear();
		bounds.Add( Vec2f( borderWidth, rect.ur.y - borderWidth - buttonWidth ) );
	}

	void MenuBar::AddButton( Button *b ) {
		b->bounds = Bounds2f( bounds.ur.x, bounds.ll.y, bounds.ur.x + buttonWidth, bounds.ll.y + buttonWidth );
		bounds.Add( b->bounds );
		bounds.ur.x += borderWidth;
		buttons.push_back( b );
	}
	
	
	bool MenuBar::ProcessInput( bool active, int x, int y ) {
		for ( int i = 0; i < (int)buttons.size(); i++ ) {
			if ( buttons[i]->ProcessInput( active, x, y ) ) {
				return true;
			}
		}
		return false;
	}

	void MenuBar::Tick() {
		for ( int i = 0; i < (int)buttons.size(); i++ ) {
			buttons[i]->Tick();
		}
	}
	
	
	
}

