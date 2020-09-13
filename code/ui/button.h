/*
 *  button
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

#ifndef __STAR3MAP_BUTTON_H__
#define __STAR3MAP_BUTTON_H__

#include "r3/bounds.h"
#include "r3/texture.h"
#include "r3/var.h"

#include <vector>

namespace star3map {
	
	class Button {
	protected:
		r3::Texture2D *tex;
        Button() : tex( NULL ), inputOver( false ), color( 1, 1, 1, 1 ) {}
		Button( r3::Texture2D * bTexture );
		Button( const std::string & bTextureFilename );
		bool inputOver;
		double timestamp;
		bool heldCalled;
	public:
		virtual ~Button();
		bool ProcessInput( bool active, int x, int y ); 
		virtual void Tick();
		virtual void Draw();
		virtual void OnPressed() {}
		virtual void OnHeld() {}
		r3::Bounds2f bounds;		
		r3::Vec4f color;
	};
	
	class PushButton : public Button {
	protected:
		std::string pressedCommand;
		std::string heldCommand;		
	public:
		PushButton( const std::string & pbTextureFilename, const std::string & pbCommand, const std::string & hbCommand = "" );		
		virtual void OnPressed();
		virtual void OnHeld();
	};
    
    class SequenceButton : public Button {
    public:
        struct Item {
            Item() {}
            Item( r3::Texture2D *t, const std::string & c ) : tex( t ), command( c ) {}
            r3::Texture2D *tex;
            std::string command;
        };
        std::vector<Item> items;
        int currentItem;

        SequenceButton() : currentItem( 0 ) {}
        
        virtual void Draw();
        virtual void OnPressed();
        
    };
	
	class ToggleButton : public Button {
	protected:
		r3::Var *var;
	public:
		ToggleButton( const std::string & tbTextureFilename, const std::string & tbVarName );
		ToggleButton( r3::Texture2D * tbTexture, const std::string & tbVarName );
		virtual void Draw();
		virtual void OnPressed();
		r3::Vec4f onColor;
		r3::Vec4f offColor;
	};
	
	
}

#endif // __STAR3MAP_BUTTON_H__


