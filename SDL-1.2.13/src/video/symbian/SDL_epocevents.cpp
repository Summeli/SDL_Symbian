/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002  Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

/*
    SDL_epocevents.cpp
    Handle the event stream, converting Epoc events into SDL events

    Epoc version (9210,KeyConfig,S80,S90, basic port etc) by Hannu Viitala (hannu.j.viitala@mbnet.fi)
	Epoc version (UIQ/S60) by Lars Persson
*/


#include <stdio.h>
#undef NULL
#include "SDLApp.h"

extern "C" {
#include "SDL_error.h"
#include "SDL_video.h"
#include "SDL_keysym.h"
#include "SDL_keyboard.h"
#include "SDL_events_c.h"
#include "SDL_timer.h"
} /* extern "C" */

#include "SDL_epocvideo.h"
#include "SDL_epocevents_c.h"
#ifndef UIQ3
#include <hal.h>
#endif
#include <bautils.h>
_LIT(KSdlKeyMapFileName, "SdlKeymap.cfg");

#ifdef UIQ
#include <QuartzKeys.h>
#endif
#ifdef UIQ3
#include <devicekeys.h>
void SetKeyboardMapMode(TBool aNumPadMode);
#endif
extern "C" {
/* The translation tables from a console scancode to a SDL keysym */
static SDLKey keymap[MAX_SCANCODE];
static SDL_keysym *TranslateKey(_THIS,int scancode, SDL_keysym *keysym);
/* Function prototypes */
void ReadKeyMapFile(const TDesC&, RFs&);
extern "C" void SetTextInputState(TBool aInputOn);
extern "C" void SetJoystickState(TBool aJoystickOn);

TBool isCursorVisible = EFalse;
RArray<TKeyMapItem> iKeyMapping;
TLinearOrder<TKeyMapItem>* iKeyMapOrderFunc; 

} /* extern "C" */

TInt iLastChar=-1;
TInt iLetterOffset=0;
extern bool gViewVisible;

void EPOC_GenerateKeyEvent(_THIS,int aScanCode,int aIsDown, TBool aShiftOn)
{	
	TInt scancode = aScanCode;
	SDL_keysym keysym;
	TChar key(scancode);
	SDLMod modState = SDL_GetModState();
	
	if(aShiftOn)
	{
		SDL_SetModState((SDLMod)(modState|KMOD_LSHIFT|KMOD_RSHIFT));	
	}
	
	(void*)TranslateKey(_this,scancode, &keysym);							
	SDL_PrivateKeyboard(aIsDown?SDL_PRESSED:SDL_RELEASED, &keysym);
	
	if(aShiftOn)
	{
		SDL_SetModState(modState);	
	}			
}

#ifdef S60V3
#include "bthidclient.h"

class CHIDEventMonitor : public CActive
    {
    public:
        virtual void RunL();
        virtual void DoCancel();
        static CHIDEventMonitor* NewL();
        ~CHIDEventMonitor();

    protected: 
        CHIDEventMonitor();
        void ConstructL();       
    private:         
        MHIDSrvClient* iHIDClient;
        RLibrary iHidLibrary;
		TBool shift_down;
    };

static CHIDEventMonitor* gBTEventMonitor = NULL;

TBool IsScreenKeysL(_THIS,const TPointerEvent& aPointerEvent)
{
	CArrayFixFlat<TKeyboardData>* touchKeys = _this->hidden->iWindowCreator->iTouchKeys;
	switch(aPointerEvent.iType)
	{
	case TPointerEvent::EButton1Down:
		{					
			if( _this->hidden->iWindowCreator->iToggleVKBStateRect.Contains(aPointerEvent.iPosition))
			{
				return ETrue;
			}

			TInt cnt = touchKeys->Count();
			for(TInt loop=0;loop<cnt;loop++)
			{
				if((*touchKeys)[loop].iKeyRect.Contains(aPointerEvent.iPosition))
				{
					iLastChar = (*touchKeys)[loop].iScanCode1;																	
					break;
				}
			}
			
			switch(iLastChar)
			{
			case EStdKeyDictaphonePlay: // Cursor/num mode
				current_video->hidden->iVKBState = EDisplayControls; // Mini
				UpdateScaleFactors();
				RedrawWindowL(current_video);
				break;
			case EStdKeyDictaphoneStop: // ABC mode
				current_video->hidden->iVKBState = EDisplayKeys;
				UpdateScaleFactors();
				RedrawWindowL(current_video);
				break;
			case EStdKeyDictaphoneRecord: // mini control
				current_video->hidden->iVKBState = EDisplayMiniControls; // Mini
				UpdateScaleFactors();
				RedrawWindowL(current_video);
				break;
			case EStdKeyNo:
				{
					((TInt&)_this->hidden->iMouseButtonSet)++;
					if(_this->hidden->iMouseButtonSet == ELastMouseButton )
						_this->hidden->iMouseButtonSet = ELeftMouseButton;
					RedrawWindowL(current_video);  
					iLastChar = -1;
					return ETrue;
				}
			case EStdKeyYes:
				{
					current_video->hidden->iControlKeyDown = !current_video->hidden->iControlKeyDown;
					RedrawWindowL(current_video);  
					iLastChar = -1;
					return ETrue;
				}				
			case EStdKeyNumLock:
				current_video->hidden->iNumLockOn = !current_video->hidden->iNumLockOn;
				current_video->hidden->iWindowCreator->UpdateVirtualKeyboard();
				iLastChar = -1;	
				return ETrue;				
			
			case EStdKeyRightFunc:
				current_video->hidden->iFNModeOn = !current_video->hidden->iFNModeOn;
				RedrawWindowL(current_video);  
				iLastChar = -1;	
				return ETrue;
			case EStdKeyDevice3:
				{
					if(Private->iMouseButtonSet == ENoMouseButton)
					{					
						SDL_PrivateMouseButton(SDL_PRESSED, SDL_BUTTON_LEFT, 0, 0);
						iLastChar = EStdKeyDevice3;
						return ETrue;
					}
				}
				break;
			case '#':
			case EStdKeyHash:
				{
				if(Private->iMouseButtonSet == ENoMouseButton)
					{					
					SDL_PrivateMouseButton(SDL_PRESSED, SDL_BUTTON_RIGHT, 0, 0);
					iLastChar = EStdKeyHash; 
					return ETrue;
					}
				}
				break;				
			case EStdKeyCapsLock:
				{
					current_video->hidden->iShiftOn = !current_video->hidden->iShiftOn ;
					RedrawWindowL(current_video);  
					iLastChar = -1;
					return ETrue;
				}
			case EStdKeyKeyboardExtend:
				{				
				return ETrue;
				}
			default:
				{
					if(current_video->hidden->iControlKeyDown)
					{
						TKeyEvent event;
						event.iScanCode = iLastChar;
						event.iCode = 0;
						static_cast<CEBasicAppUi*>(current_video->hidden->iEikEnv->EikAppUi())->HandleControlKeyKeysL(event, EEventKeyDown);
						iLastChar = -1;
						return ETrue;
					}
					else
					{
						TChar key(iLastChar);
						if(key.IsDigit() && current_video->hidden->iFNModeOn)
						{
							if(key == '0')
								{
								iLastChar = EStdKeyF10;
								}
							else
								{
								iLastChar = key.GetNumericValue()+EStdKeyF1-1;
								}
						}
					}
					
				}								
			}
			
			if(iLastChar!=-1)
			{
				SDLKey oldBackSpace = keymap[EStdKeyBackspace];
				keymap[EStdKeyBackspace]    = SDLK_BACKSPACE;
				EPOC_GenerateKeyEvent(_this, iLastChar, ETrue, current_video->hidden->iShiftOn);
				keymap[EStdKeyBackspace]    = oldBackSpace;
				return ETrue;
			}						
		}									
		break;
	case TPointerEvent::EButton1Up:
		{
								
			if( _this->hidden->iWindowCreator->iToggleVKBStateRect.Contains(aPointerEvent.iPosition))
			{
				current_video->hidden->iVirtualKeyBoardActive = !current_video->hidden->iVirtualKeyBoardActive;								
				UpdateScaleFactors();
				RedrawWindowL(current_video);
				return ETrue;
			}
			else if(iLastChar ==  EStdKeyDevice3 && Private->iMouseButtonSet == ENoMouseButton)
				{										
						SDL_PrivateMouseButton(SDL_RELEASED, SDL_BUTTON_LEFT, 0, 0);	
						iLastChar=-1;
						return ETrue;				
				}
			else if(iLastChar ==  EStdKeyHash && Private->iMouseButtonSet == ENoMouseButton)
				{										
						SDL_PrivateMouseButton(SDL_RELEASED, SDL_BUTTON_RIGHT, 0, 0);	
						iLastChar=-1;
						return ETrue;				
				}
			else if(iLastChar == EStdKeyKeyboardExtend)
				{				
					current_video->hidden->iVirtualKeyBoardActive = !current_video->hidden->iVirtualKeyBoardActive;								
					UpdateScaleFactors();
					RedrawWindowL(current_video);		
					iLastChar=-1;
					return ETrue; 
				}
			else if(iLastChar!=-1)
			{
				SDLKey oldBackSpace = keymap[EStdKeyBackspace];
				keymap[EStdKeyBackspace]    = SDLK_BACKSPACE;
				EPOC_GenerateKeyEvent(_this, iLastChar, EFalse, current_video->hidden->iShiftOn);
				iLastChar=-1;
				keymap[EStdKeyBackspace]    = oldBackSpace;
				return ETrue;
			}

			if(current_video->hidden->iKeyboardRect.Contains(aPointerEvent.iPosition))
			{
				return ETrue;
			}
		}
		break;
	case TPointerEvent::EDrag:
		{
			if(iLastChar!=-1)
			{
				return ETrue;
				
			}			
		}
		break;
	default:
	case TPointerEvent::EButton2Down:
	case TPointerEvent::EButton3Down:
	case TPointerEvent::EButton3Up:
		break;
		
	}	
	return EFalse;
}
#endif
#if defined (UIQ) || defined (S90) || defined (UIQ3)
TBool IsScreenKeysL(_THIS,const TPointerEvent& aPointerEvent)
{
	switch(aPointerEvent.iType)
				{
				case TPointerEvent::EButton1Down:
					{
						if(KOnScreenOffsetStartVar>=KOnScreenOffsetStartVal)
						{
							TRect rect;
							for(TInt loop=0;loop<20;loop++)
							{
#ifdef UIQ
								rect=TRect(TPoint(KOnScreenOffsetStartVal,loop*16),TSize(KScreenKeySize,KScreenKeySize));
#elif defined (UIQ3)
								rect=TRect(TPoint(KOnScreenOffsetStartVal,loop*16),TSize(KScreenKeySize,KScreenKeySize));
#else
								rect=TRect(TPoint(loop*32,KOnScreenOffsetStartVal),TSize(32,32));
#endif
								if(rect.Contains(aPointerEvent.iPosition))
								{
									switch(loop)
									{
									case 3:
										_this->hidden->iVirtualKeyBoardActive = !_this->hidden->iVirtualKeyBoardActive;
#ifdef UIQ3
										static_cast<CEBasicAppUi*>(CEikonEnv::Static()->EikAppUi())->View()->UpdateClipRect();
#endif
										static_cast<CEBasicAppUi*>(CEikonEnv::Static()->EikAppUi())->View()->UpdateVKeyBoard();									
										break;
									case 1: //rightclick set
										((TInt&)_this->hidden->iMouseButtonSet)++;
										if(_this->hidden->iMouseButtonSet == ELastMouseButton )
											_this->hidden->iMouseButtonSet = ELeftMouseButton;
										// need to repaint lower bottom
										static_cast<CEBasicAppUi*>(CEikonEnv::Static()->EikAppUi())->View()->UpdateVKeyBoard();										
										break;
									case 2: // Escape
										iLastChar = EStdKeyEscape;
										break;
									case 0:
										current_video->hidden->iControlKeyDown = !current_video->hidden->iControlKeyDown;
										static_cast<CEBasicAppUi*>(CEikonEnv::Static()->EikAppUi())->View()->UpdateVKeyBoard();
										break;
									case 4: // enter
										if(!_this->hidden->iVirtualKeyBoardActive)
											return EFalse;
										iLastChar=EStdKeyEnter;
										break;
									case 5: // Delete char
										if(!_this->hidden->iVirtualKeyBoardActive)
											return EFalse;
										iLastChar = EStdKeyBackspace;
										break; 
										
									case 6: // prev
										if(!_this->hidden->iVirtualKeyBoardActive)
											return EFalse;
										if(iLetterOffset>0)
										{
											iLetterOffset-=8;
											if(iLetterOffset<0)
												iLetterOffset=0;
											static_cast<CEBasicAppUi*>(CEikonEnv::Static()->EikAppUi())->View()->UpdateCharSelector(iLetterOffset);
										}
										break;
									case 19:// next
										if(!_this->hidden->iVirtualKeyBoardActive)
											return EFalse;
										if(iLetterOffset<KOnScreenChars().Length()-1)
										{
											iLetterOffset+=8;
											if(iLetterOffset>KOnScreenChars().Length()-1)
												iLetterOffset=KOnScreenChars().Length()-1;
											
											static_cast<CEBasicAppUi*>(CEikonEnv::Static()->EikAppUi())->View()->UpdateCharSelector(iLetterOffset);
										}
										break;
										
									default: // chars in array
										{
											if(!_this->hidden->iVirtualKeyBoardActive)
												return EFalse;
											TInt charIndex=(loop-7)+iLetterOffset;
											if(charIndex<KOnScreenChars().Length())
											{
												iLastChar=KOnScreenChars()[charIndex];
											}
										}
										break;
									}
									break;
								}
							}
							if(iLastChar!=-1)
							{
								SDLKey oldBackSpace = keymap[EStdKeyBackspace];
								keymap[EStdKeyBackspace]    = SDLK_BACKSPACE;
								TChar mychar(iLastChar);
								TBool shift = mychar.IsUpper();								
								EPOC_GenerateKeyEvent(_this, mychar.GetUpperCase(), ETrue, shift);
								keymap[EStdKeyBackspace]    = oldBackSpace;
								return ETrue;
							}
							return ETrue;
						}
						
					}
					break;
				case TPointerEvent::EButton1Up:
					{
						if(iLastChar!=-1)
						{
							SDLKey oldBackSpace = keymap[EStdKeyBackspace];
							keymap[EStdKeyBackspace]    = SDLK_BACKSPACE;
							TChar mychar(iLastChar);
							TBool shift = mychar.IsUpper();		
							EPOC_GenerateKeyEvent(_this, mychar.GetUpperCase(), EFalse, shift);
							iLastChar=-1;
							keymap[EStdKeyBackspace]    = oldBackSpace;
							return ETrue;
						}
						if(KOnScreenOffsetStartVar>KOnScreenOffsetStartVal)
						{
							return ETrue;
						}
					}
					break;
				case TPointerEvent::EDrag:
					{
						if(iLastChar!=-1)
						{
							return ETrue;
							
						}			
					}
					break;
				default:
				case TPointerEvent::EButton2Down:
				case TPointerEvent::EButton3Down:
				case TPointerEvent::EButton3Up:
					break;
					
				}	
				return EFalse;
}
#endif


#if defined (S60) || defined(S60V3)

extern "C" void EPOC_CalcScaleFactors(_THIS);
void EPOC_SetS60Mode(_THIS,TInt aS60Mode)
{
	Private->iSX0Mode = aS60Mode;
	if(Private->iSX0Mode & ESX0Stretched)
	{
		Private->iPutOffset =TPoint(0,0);		
	}
	
	EPOC_CalcScaleFactors(_this);
	if(Private->iWindowCreator != NULL)
		Private->iWindowCreator->ClearScreen();
}
#endif
int EPOC_HandleWsEvent(_THIS, const TWsEvent& aWsEvent)
{
    int posted = 0;
    SDL_keysym keysym;

	int scancode=0;

    switch (aWsEvent.Type()) {
    
    case EEventPointer: /* Mouse pointer events */
    {
        const TPointerEvent* pointerEvent = aWsEvent.Pointer();
#ifdef S60V3
		if(!Private->iHasMouseOrTouch				
				&& ((Private->EPOC_ScreenSize.iWidth > 320 && Private->EPOC_ScreenSize.iHeight>240)|| (Private->EPOC_ScreenSize.iWidth>240 && Private->EPOC_ScreenSize.iHeight>320))
		   )
		{
			Private->iHasMouseOrTouch = ETrue;
			UpdateScaleFactors();
			RedrawWindowL(current_video);
		}
#endif
#if defined (UIQ) || defined (S90) || defined (UIQ3) || defined (S60V3)
		if(!IsScreenKeysL(_this,*pointerEvent))
#endif
		{
			TPoint mousePos = pointerEvent->iPosition;
			
#if defined (UIQ) || defined (UIQ3) || defined (S60V3)
		
		
#if defined (UIQ3)			
			if(Private->iSX0Mode & ESX0Portrait)
			{												
				if (Private->iSX0Mode & ESX0Stretched && !(Private->EPOC_ShrinkedHeight || Private->EPOC_ShrinkedWidth))
				{
					mousePos.iX = (pointerEvent->iPosition.iY*Private->iModeSize.iHeight);
					mousePos.iX = (mousePos.iX/Private->EPOC_ScreenSize.iHeight);				
				}
				else 
				{
					mousePos.iX = pointerEvent->iPosition.iY;
				}					

				if(!(Private->EPOC_ShrinkedHeight || Private->EPOC_ShrinkedWidth))
				{
					mousePos.iY= ((pointerEvent->iPosition.iX*Private->iModeSize.iWidth)/Private->EPOC_ScreenSize.iWidth);
				}
				else
				{
					mousePos.iY= pointerEvent->iPosition.iX;
				}
				
			}
			else
			{											
				if((Private->iSX0Mode & ESX0Flipped))
				{
					mousePos.iX = ( (_this->hidden->EPOC_DisplaySize.iWidth-1) - mousePos.iX);				
				}
				else
				{
					mousePos.iY = (_this->hidden->EPOC_DisplaySize.iHeight-1)-mousePos.iY;
				}

				if (Private->iSX0Mode & ESX0Stretched && !Private->EPOC_ShrinkedHeight)
				{
					mousePos.iX = mousePos.iX/Private->iYScale;
				}

			}

			if (Private->EPOC_ShrinkedHeight || Private->EPOC_ShrinkedWidth) 
			{
				if (Private->iSX0Mode & ESX0Stretched)
				{
					mousePos.iX = mousePos.iX/Private->iYScale;
					mousePos.iY = mousePos.iY/Private->iXScale;
				}
				else
				{
					mousePos.iX+=Private->iPutOffset.iY;
					mousePos.iY+=Private->iPutOffset.iX;
				}
			}
#elif defined (UIQ)// UIQ
			//AK flip mouse x and y on UIQ
			if (Private->EPOC_ShrinkedHeight) {
				mousePos.iY <<= 1; /* Scale y coordinate to shrinked screen height */
			}
			TInt scrWidth=320;
			if (Private->EPOC_ShrinkedWidth) {
				scrWidth=640;
				mousePos.iX <<= 1; /* Scale y coordinate to shrinked screen height */
			}
			
			if((Private->iSX0Mode & ESX0Flipped))
				{
				mousePos.iX = (207 - mousePos.iX);				
				}
			else
				{
				mousePos.iY = scrWidth-mousePos.iY;
				}

			if(_this->screen->h== 480 || _this->screen->h==240)
				{
				mousePos.iX=(mousePos.iX*1.2);
				}				
#elif defined (S60V3)
			if(Private->iSX0Mode & ESX0Portrait)
				{												
				if (Private->iSX0Mode & ESX0Stretched && !(Private->EPOC_ShrinkedHeight || Private->EPOC_ShrinkedWidth))
					{
					mousePos.iX = (pointerEvent->iPosition.iY/Private->iYScale);										
					}
				else 
					{
					mousePos.iX = pointerEvent->iPosition.iY;
					}					

				if(Private->iSX0Mode & ESX0Stretched && !(Private->EPOC_ShrinkedHeight || Private->EPOC_ShrinkedWidth))
					{
					mousePos.iY = pointerEvent->iPosition.iX / Private->iXScale;
					}
				else
					{
					mousePos.iY= pointerEvent->iPosition.iX;
					}

				}
			else
				{		
				if (Private->iSX0Mode & ESX0Stretched)
					{
					if((Private->iSX0Mode & ESX0Flipped))
						{
						mousePos.iX =  ( (_this->hidden->/*EPOC_DisplaySize.iWidth*/iStretchSize.iHeight-1) - mousePos.iX);				
						}
					else
						{
						mousePos.iY = (_this->hidden->/*EPOC_DisplaySize.iHeight*/iStretchSize.iWidth-1)-mousePos.iY;
						}


					if(!Private->EPOC_ShrinkedHeight)
						{
						mousePos.iX = mousePos.iX/Private->iYScale;
						}
					if(!Private->EPOC_ShrinkedWidth)
						{
						mousePos.iY = mousePos.iY/Private->iXScale;
						}
					}			
				else
					{
					if((Private->iSX0Mode & ESX0Flipped))
						{
						mousePos.iX =  ( (_this->hidden->iModeSize.iHeight-1) - mousePos.iX);				
						}
					else
						{
						mousePos.iY = (_this->hidden->iModeSize.iWidth-1)-mousePos.iY;
						}
					}

				}

			if (Private->EPOC_ShrinkedHeight || Private->EPOC_ShrinkedWidth) 
				{
				if (Private->iSX0Mode & ESX0Stretched)
				{
					mousePos.iX = mousePos.iX/Private->iYScale;
					mousePos.iY = mousePos.iY/Private->iXScale;
				}
				else
				{
					mousePos.iX+=Private->iPutOffset.iY;
					mousePos.iY+=Private->iPutOffset.iX;
				}
			}
#endif
	// If if motion should be normalized first.
	if(pointerEvent->iType==TPointerEvent::EButton1Down && _this->hidden->iMouseButtonSet == ENoMouseButton)
	{
		SDL_PrivateMouseReset(mousePos.iY, mousePos.iX+1);
	}

	posted += SDL_PrivateMouseMotion(0, 0, mousePos.iY, mousePos.iX); /* Absolute position on screen */
#elif defined (S80) || defined (S90)
	#ifdef S80
				if(_this->screen->h== 480 || _this->screen->h==240)
				{
					mousePos.iY=(mousePos.iY*1.2);
				}
	#endif
	#ifdef S90
				if(Private->iSX0Mode & ESX0Stretched)
				{
					mousePos.iX=((mousePos.iX-Private->EPOC_ScreenOffset)/Private->iXScale)+Private->EPOC_ScreenOffset;
					mousePos.iY=((mousePos.iY)/Private->iYScale);
				}
	#endif //S90
			if (Private->EPOC_ShrinkedHeight) {
				mousePos.iY <<= 1; /* Scale y coordinate to shrinked screen height */
			}
	posted += SDL_PrivateMouseMotion(0, 0, mousePos.iX-Private->EPOC_ScreenOffset, mousePos.iY); /* Absolute position on screen */
#endif
			
			if (pointerEvent->iType==TPointerEvent::EButton1Down && _this->hidden->iMouseButtonSet != ENoMouseButton) 
			{			
				posted += SDL_PrivateMouseButton(SDL_PRESSED, _this->hidden->iMouseButtonSet == ERightMouseButton?SDL_BUTTON_RIGHT:SDL_BUTTON_LEFT, 0, 0);
			}
			else if (pointerEvent->iType==TPointerEvent::EButton1Up && _this->hidden->iMouseButtonSet != ENoMouseButton) 
			{
				posted += SDL_PrivateMouseButton(SDL_RELEASED,  _this->hidden->iMouseButtonSet == ERightMouseButton?SDL_BUTTON_RIGHT:SDL_BUTTON_LEFT, 0, 0);
			}
			else if (pointerEvent->iType==TPointerEvent::EButton2Down)
			{
				posted += SDL_PrivateMouseButton(SDL_PRESSED, SDL_BUTTON_RIGHT, 0, 0);
			}
			else if (pointerEvent->iType==TPointerEvent::EButton2Up) 
			{
				posted += SDL_PrivateMouseButton(SDL_RELEASED, SDL_BUTTON_RIGHT, 0, 0);
			}
			//!!posted += SDL_PrivateKeyboard(SDL_PRESSED, TranslateKey(aWsEvent.Key()->iScanCode, &keysym));
		}
        break;
    }
    
    case EEventKeyDown: /* Key events */
    {
		scancode = aWsEvent.Key()->iScanCode;
		
       (void*)TranslateKey(_this,scancode, &keysym);
        
        /* Special handling */
        switch((int)keysym.sym) {
        case SDLK_CAPSLOCK:
        	/*
            if (!isCursorVisible) {
#ifndef UIQ3
                // Enable virtual cursor 
	            HAL::Set(HAL::EMouseState, HAL::EMouseState_Visible);
#endif
            }
            else {
#ifndef UIQ3
                // Disable virtual cursor 
                HAL::Set(HAL::EMouseState, HAL::EMouseState_Invisible);
#endif
            }*/
            isCursorVisible = !isCursorVisible;
            break;
#ifdef UIQ3
        case SDLK_SPACE:
        case SDLK_7:
        	if(Private->iMouseButtonSet == ENoMouseButton)
        		{
        		keysym.sym = SDLK_UNKNOWN;
        		posted += SDL_PrivateMouseButton(SDL_PRESSED, SDL_BUTTON_LEFT, 0, 0);
        		}
        	break;
        case SDLK_RETURN:
        case SDLK_9:
        	if(Private->iMouseButtonSet == ENoMouseButton)
        		{
        		keysym.sym = SDLK_UNKNOWN;
        		posted += SDL_PrivateMouseButton(SDL_PRESSED, SDL_BUTTON_RIGHT, 0, 0);
        	   	}
        	break;
		case SDLK_RMETA:
			switch(Private->iModeState)
			{
			case 0:
				Private->iModeState++;
				Private->iModeKeys = 0;
				Private->iModeKeyReleased = EFalse;
				break;		
			case 1:
				Private->iModeState = 0;
				posted += SDL_PrivateKeyboard(SDL_RELEASED, &keysym);
				return EKeyWasConsumed;
			}
			
			break;
	   case SDLK_RIGHT:
	   case SDLK_LEFT:
	   case SDLK_UP:
	   case SDLK_DOWN:
		   {
			   Uint8 *  keys = SDL_GetKeyState (NULL);
			   
			   if((keysym.sym == SDLK_RIGHT && keys[SDLK_LEFT]) 
				   || (keysym.sym == SDLK_LEFT && keys[SDLK_RIGHT])
				   || (keysym.sym == SDLK_UP && keys[SDLK_DOWN])
				   || (keys[SDLK_UP] && keysym.sym == SDLK_DOWN)
				   )
			   {
				   _this->hidden->iNumPadMode = !_this->hidden->iNumPadMode;
				   SetKeyboardMapMode(_this->hidden->iNumPadMode);
				   static_cast<CEBasicAppUi*>(CEikonEnv::Static()->EikAppUi())->View()->UpdateVKeyBoard();
			   }			   
		   }
		   break;
#endif
        }
        
        if(keysym.sym != SDLK_UNKNOWN)
		{
			posted += SDL_PrivateKeyboard(SDL_PRESSED, &keysym);
		}
	
        break;
	} 

    case EEventKeyUp: /* Key events */
    {
		scancode = aWsEvent.Key()->iScanCode;

#ifdef UIQ3
		switch(scancode)
		{
		case EStdDeviceKeyTwoWayDown:
				return posted;
		case EStdDeviceKeyTwoWayUp:
				return posted;
		default:
			break;
		}
#endif

		TranslateKey( _this,scancode, &keysym);
#ifdef UIQ3
		switch(keysym.sym)
			{
			case SDLK_SPACE:
			case SDLK_7:
				if(Private->iMouseButtonSet == ENoMouseButton)
					{
					keysym.sym = SDLK_UNKNOWN;
					posted += SDL_PrivateMouseButton(SDL_RELEASED, SDL_BUTTON_LEFT, 0, 0);
					}
				break;
			case SDLK_RETURN:
			case SDLK_9:
				if(Private->iMouseButtonSet == ENoMouseButton)
					{
					keysym.sym = SDLK_UNKNOWN;
					posted += SDL_PrivateMouseButton(SDL_RELEASED, SDL_BUTTON_RIGHT, 0, 0);
					}
				break;
				// Check if we should release META
			case SDLK_RMETA:
				{
				if(Private->iModeState == 2)
					return EKeyWasConsumed;
				// No key still pressed, stay in keysym mode
				if(Private->iModeState == 1 && Private->iModeKeys == 0) 
					{
					Private->iModeKeyReleased = ETrue;
					return EKeyWasConsumed;
					}
				Private->iModeKeys = 0;
				Private->iModeState = 0;
				}break;
			default:
				break;
			}
	
#endif
		if(keysym.sym != SDLK_UNKNOWN)
		{
			posted += SDL_PrivateKeyboard(SDL_RELEASED, &keysym);
		}

#ifdef UIQ3
		if(Private->iModeState == 1 && Private->iModeKeyReleased && Private->iModeKeys >0)
		{
			Private->iModeState = 0;
			Private->iModeKeys = 0;
			Private->iModeKeyReleased = EFalse;
			SDLMod modState = (SDLMod)(SDL_GetModState()&~KMOD_META);
			SDL_SetModState(modState);
		}
#endif
        break;
	} 
    
    case EEventFocusGained: /* SDL window got focus */
    {
        //Private->EPOC_IsWindowFocused = ETrue;
        /* Draw window background and screen buffer */
		gViewVisible=true;
        RedrawWindowL(_this);  
        break;
    }

    case EEventFocusLost: /* SDL window lost focus */
    {
        //Private->EPOC_IsWindowFocused = EFalse;

        // Wait and eat events until focus is gained again
        /*
	    while (ETrue) {
            Private->EPOC_WsSession.EventReady(&Private->EPOC_WsEventStatus);
            User::WaitForRequest(Private->EPOC_WsEventStatus);
		    Private->EPOC_WsSession.GetEvent(Private->EPOC_WsEvent);
            TInt eventType = Private->EPOC_WsEvent.Type();
		    Private->EPOC_WsEventStatus = KRequestPending;
		    //Private->EPOC_WsSession.EventReady(&Private->EPOC_WsEventStatus);
            if (eventType == EEventFocusGained) {
                RedrawWindowL(_this);
                break;
            }
	    }
        */
        break;
    }

    case EEventModifiersChanged: 
    {
	    TModifiersChangedEvent* modEvent = aWsEvent.ModifiersChanged();
        TUint modstate = KMOD_NONE;
        if (modEvent->iModifiers == EModifierLeftShift)
            modstate |= KMOD_LSHIFT;
        if (modEvent->iModifiers == EModifierRightShift)
            modstate |= KMOD_RSHIFT;
        if (modEvent->iModifiers == EModifierLeftCtrl)
            modstate |= KMOD_LCTRL;
        if (modEvent->iModifiers == EModifierRightCtrl)
            modstate |= KMOD_RCTRL;
        if (modEvent->iModifiers == EModifierLeftAlt)
            modstate |= KMOD_LALT;
        if (modEvent->iModifiers == EModifierRightAlt)
            modstate |= KMOD_RALT;
        if (modEvent->iModifiers == EModifierLeftFunc)
            modstate |= KMOD_LMETA;
        if (modEvent->iModifiers == EModifierRightFunc)
            modstate |= KMOD_RMETA;
        if (modEvent->iModifiers == EModifierCapsLock)
            modstate |= KMOD_CAPS;
        SDL_SetModState(STATIC_CAST(SDLMod,(modstate | KMOD_LSHIFT)));
        break;
    }
    default:            
        break;
	} 
	
    return posted;
}

class CWaitForActivate:public CActive
{
public:
	CWaitForActivate();
	~CWaitForActivate();
	void RunL();
	void DoCancel();
	RTimer iTimer;
};

CWaitForActivate* gWaitForActive = NULL;

CWaitForActivate::CWaitForActivate():CActive(EPriorityStandard)
{
	CActiveScheduler::Add(this);
	iTimer.CreateLocal();
	iStatus=KRequestPending;
	iTimer.After(iStatus,250000);
	SetActive();
}

CWaitForActivate::~CWaitForActivate()
{
	iTimer.Close();
}


void CWaitForActivate::RunL()
{
	if(gViewVisible)
	{
        RedrawWindowL(current_video);  
		CActiveScheduler::Stop(); 
		delete gWaitForActive;
		gWaitForActive = NULL;
	}
	else
	{
		iStatus=KRequestPending;
		iTimer.After(iStatus,250000);
		SetActive();
	}
}

void CWaitForActivate::DoCancel()
{
	iTimer.Cancel();
}

static TInt StopSchedulerTimeOut(TAny* /*any*/)
{
#if !defined (UIQ3) && !defined(S60V3)
	TRawEvent	event;
	event.Set(TRawEvent::EActive);// keep us happy even if no events has been posted
	UserSvr::AddEvent(event); 
#else
	User::ResetInactivityTime();
#endif
	if(!gViewVisible)
	{
		delete gWaitForActive;
		gWaitForActive = NULL;
		gWaitForActive = new CWaitForActivate;
	}
	else
		CActiveScheduler::Stop(); 
	return 0;
}

void UpdateKbdMouse() {
TUint32 curTime = SDL_GetTicks();
if (curTime >= current_video->hidden->_km.last_time + current_video->hidden->_km.delay_time) 
	{
	current_video->hidden->_km.last_time = curTime;
	if (current_video->hidden->_km.x_down_count == 1) 
		{
		current_video->hidden->_km.x_down_time = curTime;
		current_video->hidden->_km.x_down_count = 2;
		}
	if (current_video->hidden->_km.y_down_count == 1) 
		{
		current_video->hidden->_km.y_down_time = curTime;
		current_video->hidden->_km.y_down_count = 2;
		}

	if (current_video->hidden->_km.x_vel || current_video->hidden->_km.y_vel) 
		{
		if (current_video->hidden->_km.x_down_count) 
			{
			if (curTime > current_video->hidden->_km.x_down_time + current_video->hidden->_km.delay_time * 12) 
				{
				if (current_video->hidden->_km.x_vel > 0)
					current_video->hidden->_km.x_vel++;
				else
					current_video->hidden->_km.x_vel--;
				} else if (curTime > current_video->hidden->_km.x_down_time + current_video->hidden->_km.delay_time * 8) 
					{
					if (current_video->hidden->_km.x_vel > 0)
						current_video->hidden->_km.x_vel = 5;
					else
						current_video->hidden->_km.x_vel = -5;
					}
			}
		if (current_video->hidden->_km.y_down_count) 
			{
			if (curTime > current_video->hidden->_km.y_down_time + current_video->hidden->_km.delay_time * 12) 
				{
				if (current_video->hidden->_km.y_vel > 0)
					current_video->hidden->_km.y_vel++;
				else
					current_video->hidden->_km.y_vel--;
				} else if (curTime > current_video->hidden->_km.y_down_time + current_video->hidden->_km.delay_time * 8) 
					{
					if (current_video->hidden->_km.y_vel > 0)
						current_video->hidden->_km.y_vel = 5;
					else
						current_video->hidden->_km.y_vel = -5;
					}
			}
		SDL_PrivateMouseMotion(0 , 1, current_video->hidden->_km.x_vel, current_video->hidden->_km.y_vel);
		}
	}
}

extern "C" {

void EPOC_PumpEvents(_THIS)
{	
	static_cast<CEBasicAppUi*>(Private->iEikEnv->EikAppUi())->ReleaseMultitapKeysL();

	TCallBack callback (StopSchedulerTimeOut);
	Private->iOnecallback->Set(callback);
	Private->iOnecallback->Cancel();
	Private->iOnecallback->CallBack();
	CActiveScheduler::Start();// Run scheduler so the pointerevents and keyboard events get placed
	
#if defined (__WINS__ ) || defined(S60) || defined (S80) || defined(S90) || defined (UIQ3) || defined (S60V3)
#if  defined (UIQ3) || defined (S60V3) || defined (S60)
	if(Private->iInputMode == EMouseMode)
		{
		UpdateKbdMouse();
		}
#endif
	if(Private->iNeedUpdate) // Update is needed.. done out of this thread.
	{	
		 /* Draw current buffer */
		TInt numrects=Private->iNumRects;
		Private->iNumRects = 0;
		_this->UpdateRects(_this, numrects, Private->iDirtyRects);
		if(Private->iNumRects>0)
		{
			numrects=Private->iNumRects;
			_this->UpdateRects(_this, numrects, Private->iDirtyRects);
			Private->iNumRects = 0;
		}
		
		Private->iNeedUpdate=EFalse;
	}

	if(!Private->iWasUpdated)
	{
		Private->iWindowCreator->UpdateMouseCursor();	
		Private->iWindowCreator->UpdateScreen();
	}
	Private->iWasUpdated = EFalse;
#endif
}

void EPOC_CloseOSKeyMap()
{
	delete iKeyMapOrderFunc;
	iKeyMapOrderFunc = NULL;
	iKeyMapping.Close();
}

void EPOC_InitOSKeymap(_THIS)
{
	TUint32 i;
	current_video->hidden->_km.delay_time = 25;
	current_video->hidden->_km.last_time = 0;
	iKeyMapOrderFunc = new(ELeave) TLinearOrder<TKeyMapItem>(TKeyMapItem::Compare); //!!Not freed

	// Check if the file exists

	TFileName fileName = CSDLApp::GetExecutablePath();
	fileName.Append(KSdlKeyMapFileName);

	ReadKeyMapFile(fileName, CEikonEnv::Static()->FsSession());
	/* Initialize the key translation table */
	for ( i=0; i<SDL_TABLESIZE(keymap); ++i )
		keymap[i] = SDLK_UNKNOWN;


	/* Numbers */
	for ( i = 0; i<32; ++i ){
		keymap[' ' + i] = (SDLKey)(SDLK_SPACE+i);
	}
	/* e.g. Alphabet keys */
	for ( i = 0; i<32; ++i ){
		keymap['A' + i] = (SDLKey)(SDLK_a+i);
	}
#if defined (S60) || defined (S60V3) || defined (UIQ3)
	keymap[EStdKeyBackspace]    = SDLK_ESCAPE;
#else
	keymap[EStdKeyBackspace]    = SDLK_BACKSPACE;
#endif
	keymap[EStdKeyTab]          = SDLK_TAB;
	keymap[EStdKeyEnter]        = SDLK_RETURN;
	keymap[EStdKeyEscape]       = SDLK_ESCAPE;
   	keymap[EStdKeySpace]        = SDLK_SPACE;
   	keymap[EStdKeyPause]        = SDLK_PAUSE;
   	keymap[EStdKeyHome]         = SDLK_HOME;
   	keymap[EStdKeyEnd]          = SDLK_END;
   	keymap[EStdKeyPageUp]       = SDLK_PAGEUP;
   	keymap[EStdKeyPageDown]     = SDLK_PAGEDOWN;
	keymap[EStdKeyDelete]       = SDLK_DELETE;
	keymap[EStdKeyUpArrow]      = SDLK_UP;
	keymap[EStdKeyDownArrow]    = SDLK_DOWN;
	keymap[EStdKeyLeftArrow]    = SDLK_LEFT;
	keymap[EStdKeyRightArrow]   = SDLK_RIGHT;
	keymap[EStdKeyCapsLock]     = SDLK_CAPSLOCK;
	keymap[EStdKeyLeftShift]    = SDLK_LSHIFT;
	keymap[EStdKeyRightShift]   = SDLK_RSHIFT;
	keymap[EStdKeyLeftAlt]      = SDLK_LALT;
	keymap[EStdKeyRightAlt]     = SDLK_RALT;
	keymap[EStdKeyLeftCtrl]     = SDLK_LCTRL;
	keymap[EStdKeyRightCtrl]    = SDLK_RCTRL;
	keymap[EStdKeyLeftFunc]     = SDLK_LMETA;
	keymap[EStdKeyRightFunc]    = SDLK_UNKNOWN;
	keymap[EStdKeyInsert]       = SDLK_INSERT;
	keymap[EStdKeyComma]        = SDLK_COMMA;
	keymap[EStdKeyFullStop]     = SDLK_PERIOD;
	keymap[EStdKeyForwardSlash] = SDLK_SLASH;
	keymap[EStdKeyBackSlash]    = SDLK_BACKSLASH;
	keymap[EStdKeySemiColon]    = SDLK_SEMICOLON;
	keymap[EStdKeySingleQuote]  = SDLK_QUOTE;
	keymap[EStdKeyHash]         = SDLK_HASH;
	keymap[EStdKeySquareBracketLeft]    = SDLK_LEFTBRACKET;
	keymap[EStdKeySquareBracketRight]   = SDLK_RIGHTBRACKET;
	keymap[EStdKeyMinus]        = SDLK_MINUS;
	keymap[EStdKeyEquals]       = SDLK_EQUALS;
	keymap[EStdKeyYes]			= SDLK_UNKNOWN;
	keymap[EStdKeyNo]			= SDLK_END;

   	keymap[EStdKeyF1]          = SDLK_F1;  /* chr + q */
   	keymap[EStdKeyF2]          = SDLK_F2;  /* chr + w */
   	keymap[EStdKeyF3]          = SDLK_F3;  /* chr + e */
   	keymap[EStdKeyF4]          = SDLK_F4;  /* chr + r */
   	keymap[EStdKeyF5]          = SDLK_F5;  /* chr + t */
   	keymap[EStdKeyF6]          = SDLK_F6;  /* chr + y */
   	keymap[EStdKeyF7]          = SDLK_F7;  /* chr + i */
   	keymap[EStdKeyF8]          = SDLK_F8;  /* chr + o */

   	keymap[EStdKeyF9]          = SDLK_F9;  /* chr + a */
   	keymap[EStdKeyF10]         = SDLK_F10; /* chr + s */
   	keymap[EStdKeyF11]         = SDLK_F11; /* chr + d */
   	keymap[EStdKeyF12]         = SDLK_F12; /* chr + f */
	keymap[EStdKeyNkpAsterisk]	 = SDLK_ASTERISK;
	keymap[EStdKeyNkp1]         = SDLK_1;
	keymap[EStdKeyNkp2]         = SDLK_2;
	keymap[EStdKeyNkp3]         = SDLK_3;
	keymap[EStdKeyNkp4]         = SDLK_4;
	keymap[EStdKeyNkp5]         = SDLK_5;
	keymap[EStdKeyNkp6]         = SDLK_6;
	keymap[EStdKeyNkp7]         = SDLK_7;
	keymap[EStdKeyNkp8]         = SDLK_8;
	keymap[EStdKeyNkp9]         = SDLK_9;
	keymap[EStdKeyNkp0]         = SDLK_0;

	keymap[EStdKeyNkp2]         = SDLK_UP;
	keymap[EStdKeyNkp4]         = SDLK_LEFT;
	keymap[EStdKeyNkp6]         = SDLK_RIGHT;
	keymap[EStdKeyNkp8]         = SDLK_DOWN;
#ifdef UIQ
	keymap[EStdQuartzKeyConfirm]	 = SDLK_F5;
	keymap[EStdQuartzKeyTwoWayDown] = SDLK_PAGEDOWN;
	keymap[EStdQuartzKeyTwoWayUp] = SDLK_PAGEUP;
	keymap[EStdQuartzKeyFourWayUp] = SDLK_UP;
	keymap[EStdQuartzKeyFourWayDown] = SDLK_DOWN;
	keymap[EStdQuartzKeyFourWayLeft] = SDLK_LEFT;
	keymap[EStdQuartzKeyFourWayRight] = SDLK_RIGHT;

	keymap[EStdKeyApplication0] = SDLK_F1;
	keymap[EStdKeyApplication1] = SDLK_F2;
	keymap[EStdKeyApplicationA] = SDLK_F3; // A1000 A
	keymap[EStdKeyApplicationB] = SDLK_F4; // A1000 B
	keymap[EStdKeyApplicationC] = SDLK_F5; // A92X A
	keymap[EStdKeyApplicationD] = SDLK_F6; // A92X B
	keymap[EStdKeyDevice0] = SDLK_F3;
#endif
#ifdef UIQ3
	keymap[EStdKeyDevice1B]	 = SDLK_F5; // P990 confirm
	keymap[EStdKeyApplication1] = SDLK_F1; // 0xb5
	keymap[EStdKeyApplication5] = SDLK_F2; //Play button 0xb9
	keymap[EStdKeyF17] = SDLK_F4;
	keymap[EStdDeviceKeyAction]	 = SDLK_F5;
	keymap[EStdDeviceKeyTwoWayDown] = SDLK_PAGEDOWN;
	keymap[EStdDeviceKeyTwoWayUp] = SDLK_PAGEUP;
	keymap[EStdDeviceKeyFourWayUp] = SDLK_UP;
	keymap[EStdDeviceKeyFourWayDown] = SDLK_DOWN;
	keymap[EStdDeviceKeyFourWayLeft] = SDLK_LEFT;
	keymap[EStdDeviceKeyFourWayRight] = SDLK_RIGHT;
	keymap[EStdDeviceKeyMenu] = SDLK_MENU;
	keymap[EStdKeyDevice0] = SDLK_F3;
	keymap[EStdDeviceKeySoftkey1] = SDLK_RETURN;
	keymap[EStdDeviceKeySoftkey2] = SDLK_SPACE;
	keymap[EStdDeviceKeyClear] = SDLK_ESCAPE;
	keymap[EStdKeyHash]	 = SDLK_HASH;
	keymap['*']	 = SDLK_ASTERISK;
	keymap[EStdKeyLeftFunc]     = SDLK_LMETA;
	keymap[EStdKeyRightFunc]    = SDLK_RMETA;
#endif
#ifdef S90
	keymap[EStdKeyMenu] = SDLK_MENU;
	keymap[EStdKeyDevice4] = SDLK_UNKNOWN;
	keymap[EStdKeyDevice5] = SDLK_UNKNOWN;
	keymap[EStdKeyDevice7] = SDLK_UNKNOWN;
#endif
#ifdef S80
	keymap[EStdKeyMenu] = SDLK_MENU;
	keymap[EStdKeyDevice6]  = SDLK_LEFT;// <
	keymap[EStdKeyDevice7]  = SDLK_RIGHT;// >
	keymap[EStdKeyDevice8]  = SDLK_UP;// ^
	keymap[EStdKeyDevice9]   = SDLK_DOWN;// v
	keymap[EStdKeyDeviceA]   = SDLK_KP_ENTER;// o
#endif

#if defined (S60) || defined (S60V3) || defined (UIQ3)
	SetJoystickState( current_video->hidden->iInputMode == EJoystick);
	SetTextInputState(current_video->hidden->iInputMode==EKeyboard);
#endif
#ifdef S60V3
	TRAPD(err, gBTEventMonitor = CHIDEventMonitor::NewL());
#endif
    /* !!TODO
	EStdKeyNumLock=0x1b,
	EStdKeyScrollLock=0x1c,

	EStdKeyNkpForwardSlash=0x84,
	EStdKeyNkpAsterisk=0x85,
	EStdKeyNkpMinus=0x86,
	EStdKeyNkpPlus=0x87,
	EStdKeyNkpEnter=0x88,
	EStdKeyNkp1=0x89,
	EStdKeyNkp2=0x8a,
	EStdKeyNkp3=0x8b,
	EStdKeyNkp4=0x8c,
	EStdKeyNkp5=0x8d,
	EStdKeyNkp6=0x8e,
	EStdKeyNkp7=0x8f,
	EStdKeyNkp8=0x90,
	EStdKeyNkp9=0x91,
	EStdKeyNkp0=0x92,
	EStdKeyNkpFullStop=0x93,
    EStdKeyMenu=0x94,
    EStdKeyBacklightOn=0x95,
    EStdKeyBacklightOff=0x96,
    EStdKeyBacklightToggle=0x97,
    EStdKeyIncContrast=0x98,
    EStdKeyDecContrast=0x99,
    EStdKeySliderDown=0x9a,
    EStdKeySliderUp=0x9b,
    EStdKeyDictaphonePlay=0x9c,
    EStdKeyDictaphoneStop=0x9d,
    EStdKeyDictaphoneRecord=0x9e,
    EStdKeyHelp=0x9f,
    EStdKeyOff=0xa0,
    EStdKeyDial=0xa1,
    EStdKeyIncVolume=0xa2,
    EStdKeyDecVolume=0xa3,
    EStdKeyDevice0=0xa4,
    EStdKeyDevice1=0xa5,
    EStdKeyDevice2=0xa6,
    EStdKeyDevice3=0xa7,
    EStdKeyDevice4=0xa8,
    EStdKeyDevice5=0xa9,
    EStdKeyDevice6=0xaa,
    EStdKeyDevice7=0xab,
    EStdKeyDevice8=0xac,
    EStdKeyDevice9=0xad,
    EStdKeyDeviceA=0xae,
    EStdKeyDeviceB=0xaf,
    EStdKeyDeviceC=0xb0,
    EStdKeyDeviceD=0xb1,
    EStdKeyDeviceE=0xb2,
    EStdKeyDeviceF=0xb3,
    EStdKeyApplication0=0xb4,
    EStdKeyApplication1=0xb5,
    EStdKeyApplication2=0xb6,
    EStdKeyApplication3=0xb7,
    EStdKeyApplication4=0xb8,
    EStdKeyApplication5=0xb9,
    EStdKeyApplication6=0xba,
    EStdKeyApplication7=0xbb,
    EStdKeyApplication8=0xbc,
    EStdKeyApplication9=0xbd,
    EStdKeyApplicationA=0xbe,
    EStdKeyApplicationB=0xbf,
    EStdKeyApplicationC=0xc0,
    EStdKeyApplicationD=0xc1,
    EStdKeyApplicationE=0xc2,
    EStdKeyApplicationF=0xc3,
    EStdKeyYes=0xc4,
    EStdKeyNo=0xc5,
    EStdKeyIncBrightness=0xc6,
    EStdKeyDecBrightness=0xc7, 
    EStdKeyCaseOpen=0xc8,
    EStdKeyCaseClose=0xc9
    */

}

/**
 * Reads unicode text file where key mappings are defined. 
 * Syntax of the unicode file keymap.cfg is as follows. String ".." means any white space:
 * ..<flag>..<Epoc device key scancode>..<SDL flag>..<SDL key code>..<any characters><eol>
 * E.g.
 * 0  65   0  10	// A
 * 3  65   0   4	// chr+A = F1 	
 *
 * Supported Epoc flags are:
 * 0: None
 * 1: Shift
 * 2: Ctrl
 * 4: Func
 * Supported SDL flags are:
 * 0: None
 * 1: Shift
 */
void ReadKeyMapFile(const TDesC& aKeyMapFileName, RFs& fs)
	{
	// Open the Keymap file from the Frodo folder
	RFile file;
	TFileName keyConfFile;
	keyConfFile.Append(aKeyMapFileName);
#ifdef __WINS__
	//!!keyConfFile.Replace(0, 1, _L("C"));
#endif
	if(file.Open(fs, keyConfFile, EFileRead) != KErrNone)
		return;
	TFileText fileText;
	fileText.Set(file);
	TBuf<256> textLine;
	TKeyMapItem keyMapItem;

	// Read file line by line
	while(fileText.Read(textLine) == KErrNone)
		{
		TLex lex(textLine);
		TPtrC nextToken;

		// Read flag
		lex.SkipSpaceAndMark() ; // remember where we are
		lex.SkipCharacters() ; // move to end of character token
		if( lex.TokenLength() == 0 ) // if not valid potential token
			continue; // Support for empty lines
		nextToken.Set(lex.MarkedToken());
		TLex lexToken(nextToken);
		if (lexToken.Val(keyMapItem.iOldFlag, EDecimal) != KErrNone)
			continue; // Support for comment lines

		// Read device key scancode
		lex.SkipSpaceAndMark() ; // remember where we are
		lex.SkipCharacters() ; // move to end of character token
		if( lex.TokenLength() == 0 ) // if not valid potential token
			break;
		nextToken.Set(lex.MarkedToken());
		TLex lexToken2(nextToken);
		if (lexToken2.Val(keyMapItem.iOldCode) != KErrNone)
			break;

		// Read C64 flag
		lex.SkipSpaceAndMark() ; // remember where we are
		lex.SkipCharacters() ; // move to end of character token
		if( lex.TokenLength() == 0 ) // if not valid potential token
			break;
		nextToken.Set(lex.MarkedToken());
		TLex lexToken3(nextToken);
		if (lexToken3.Val(keyMapItem.iNewFlag, EDecimal) != KErrNone)
			break;

		// Read C64 key code
		lex.SkipSpaceAndMark() ; // remember where we are
		lex.SkipCharacters() ; // move to end of character token
		if( lex.TokenLength() == 0 ) // if not valid potential token
			break;
		nextToken.Set(lex.MarkedToken());
		TLex lexToken4(nextToken);
		if (lexToken4.Val(keyMapItem.iNewCode, EDecimal) != KErrNone)
			break;

		// Update keymap table
		// ELOG3(_L8("Read keymap file: %d -> %d\n"),keyMapItem.iOldCode, keyMapItem.iNewCode);
		User::LeaveIfError(iKeyMapping.InsertInOrderAllowRepeats(keyMapItem, *iKeyMapOrderFunc));
		}

	file.Close();
	}

/** !!
*/
TInt TKeyMapItem::Compare(const TKeyMapItem& aFirst, const TKeyMapItem& aSecond)
	{
	if (aFirst.iOldCode < aSecond.iOldCode)
		return -1;
	if (aFirst.iOldCode > aSecond.iOldCode)
		return 1;
	// Scancodes are equal, compare flags
	if (aFirst.iOldFlag < aSecond.iOldFlag)
		return -1;
	if (aFirst.iOldFlag > aSecond.iOldFlag)
		return 1;
	return 0;
	}

static const  SDLKey KNumberToCharMappings[10] = 
{SDLK_m, SDLK_r,SDLK_t, SDLK_y, SDLK_f, SDLK_g, SDLK_h, SDLK_v, SDLK_b, SDLK_n };

static SDL_keysym *TranslateKey(_THIS, int scancode, SDL_keysym *keysym)
{
    //char debug[256];
    //SDL_TRACE1("SDL: TranslateKey, scancode=%d", scancode); //!!

	/* Set the keysym information */ 
	keysym->scancode = scancode;
	keysym->sym = SDLK_UNKNOWN;
	keysym->mod = SDL_GetModState(); //!!Is this right??
	TBool isMappingFound = EFalse;

	/* Search mapping from the map file */
	if (iKeyMapping.Count() > 0) {

		TKeyMapItem keyMapItem(EKeyMapFlagNone, scancode);
		TInt index = KErrNone;
		//ELOG3(_L8("Find key mapping for: %d (flag: %d)\n"),keyMapItem.iOldCode, keyMapItem.iOldFlag);
		if (iKeyMapping.FindInOrder(keyMapItem, index, *iKeyMapOrderFunc) == KErrNone) {
			//ELOG2(_L8("Found mapped key!: index:%d "), index);
			//ELOG3(_L8(", key: %d (flag: %d)\n"),aAppUi->iKeyMapping[index].iNewCode, aAppUi->iKeyMapping[index].iNewFlag);
			keyMapItem = iKeyMapping[index];
			keysym->sym = (SDLKey)keyMapItem.iNewCode;
			isMappingFound = ETrue;
		}
	}

	/* If mapping is not found from the file, use the default map table */
	if (!isMappingFound) {
		
		if ((scancode >= MAX_SCANCODE) && 
			((scancode - ENonCharacterKeyBase + 0x0081) >= MAX_SCANCODE)) {
			SDL_SetError("Too big scancode");
			keysym->scancode = SDLK_UNKNOWN;
			keysym->mod = KMOD_NONE; 
			return keysym;
		}
		
		/* Handle function keys: F1, F2, F3 ... */
		if (keysym->mod & KMOD_META) {
#ifdef UIQ3			
			SDLKey keycode = SDLK_UNKNOWN;
			TBool translatedKey = ETrue;
			switch(scancode) {
			case 'Q': keycode = SDLK_1; break;
			case 'W': keycode = SDLK_2; break;
			case 'E': keycode = SDLK_3; break;
			case 'R': keycode = SDLK_4; break;
			case 'T': keycode = SDLK_5; break;
			case 'Y': keycode = SDLK_6; break;
			case 'U': keycode = SDLK_7; break;
			case 'I': keycode = SDLK_8; break;
			case 'O': keycode = SDLK_9; break;
			case 'P': keycode = SDLK_0; break;
			case 'A': keycode = SDLK_EXCLAIM; break;
			case 'S': keycode = SDLK_QUOTEDBL; break;
			case 'D': keycode = SDLK_ESCAPE; break;
			case 'F': keycode = SDLK_DOLLAR; break;
			case 'G': keycode = SDLK_EURO; break;
			case 'H': keycode = SDLK_PLUS; break;
			case 'J': keycode = SDLK_COLON; break;
			case 'K': keycode = SDLK_SEMICOLON; break;
			case 'L': keycode = SDLK_QUOTE; break;
			case EStdKeyBackspace: keycode = SDLK_DELETE; break;
			case EStdKeyComma: keycode = SDLK_AT; break;
			case EStdKeySpace:keycode = SDLK_TAB;break;
			case 'Z': keycode = SDLK_SLASH; break;
			case 'X': keycode = SDLK_ASTERISK; break;
			case 'C': keycode = SDLK_LEFTPAREN; break;
			case 'V': keycode = SDLK_RIGHTPAREN; break;
			case 'B': keycode = SDLK_MINUS; break;
			case 'N': keycode = SDLK_UNDERSCORE; break;
			case 'M': keycode = SDLK_QUESTION; break;
			case EStdKeyLeftShift:keycode = SDLK_MENU;break;
			case EStdKeyFullStop: keycode = SDLK_HASH; break;
			case EStdKeyLeftArrow:keycode = SDLK_UP;break;
			case EStdKeyRightArrow: keycode = SDLK_DOWN; break;
			default:
				if(scancode<MAX_SCANCODE)
					keycode = keymap[scancode];
				translatedKey = EFalse;
				break;
			}
			
			if(translatedKey)
			{
				Private->iModeKeys++;				
			}

			keysym->sym = keycode;
			keysym->mod = KMOD_NONE; 
			if( SDL_TranslateUNICODE)
			{
				keysym->scancode = keycode;
			}
#else
			if (scancode >= 'A' && scancode < ('A' + 24)) { /* first 32 alphapet keys */
				switch(scancode) {
                case 'Q': scancode = EStdKeyF1; break;
                case 'W': scancode = EStdKeyF2; break;
                case 'E': scancode = EStdKeyF3; break;
                case 'R': scancode = EStdKeyF4; break;
                case 'T': scancode = EStdKeyF5; break;
                case 'Y': scancode = EStdKeyF6; break;
                case 'U': scancode = EStdKeyF7; break;
                case 'I': scancode = EStdKeyF8; break;
                case 'A': scancode = EStdKeyF9; break;
                case 'S': scancode = EStdKeyF10; break;
                case 'D': scancode = EStdKeyF11; break;
                case 'F': scancode = EStdKeyF12; break;
				}
				keysym->sym = keymap[scancode];
			}
#endif

		}
		
		if (scancode >= ENonCharacterKeyBase && keysym->sym == SDLK_UNKNOWN) {
			// Non character keys
			keysym->sym = keymap[scancode - 
				ENonCharacterKeyBase + 0x0081]; // !!hard coded
		} else if(keysym->sym == SDLK_UNKNOWN){
			if(scancode<MAX_SCANCODE)
				keysym->sym = keymap[scancode];
			else // not mapped 
			{
				keysym->scancode = SDLK_UNKNOWN;
				keysym->mod = KMOD_NONE; 
			}
		}
#ifdef S60V3
		// If this flag is on then scancodes 0-19 + '*' and '#' should be chars
		if(Private->iKeyboardModifier)
		{
			switch(scancode)
			{
			case EStdKeyHash:
				keysym->sym = SDLK_j;
				break;
			case EStdKeyNkpAsterisk:
				keysym->sym = SDLK_u;
				break;
			case '0':		
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				keysym->sym = KNumberToCharMappings[scancode-'0'];
				break;		
			default: // Do nothing by default
				break;
			}
		}
#endif
	}

#if defined (__WINS__ ) || defined(S60) || defined (S80) || defined (S90)
	// Screenshot support!
	if (keysym->sym == SDLK_PRINT ) {
		Private->EPOC_Bitmap->Save(_L("c:\\sdl_screenshot.mbm"));
	}
#endif
	/* Remap the arrow keys if the device is rotated */
#if defined (S60) || defined (S60V3) || defined (UIQ3)
	if(((keysym->scancode>='0' && keysym->scancode<='9') || (keysym->scancode>=EStdKeyNkp1 && keysym->scancode<=EStdKeyNkp9))&& Private->iFNModeOn)
		{		
		if(keysym->scancode>='0' && keysym->scancode<='9')
			{
			
			if(keysym->scancode != '0')
				{
				keysym->scancode-='1';
				keysym->scancode+=EStdKeyF1;
				}
			else
				{
				keysym->scancode = EStdKeyF10;
				}
			}
		else
			{
			keysym->scancode-=EStdKeyNkp1;
			keysym->scancode+=EStdKeyF1;
			}
		
			keysym->sym = keymap[keysym->scancode];
			keysym->unicode = keysym->sym;
		}
	
	if((current_video->hidden->iInputMode==EJoystick || current_video->hidden->iInputMode==EMouseMode)
		&&(keysym->sym == SDLK_UP|| keysym->sym == SDLK_DOWN|| keysym->sym == SDLK_LEFT|| keysym->sym == SDLK_RIGHT 
		|| keysym->scancode == KLeftButtonCode1 || keysym->scancode == KLeftButtonCode2 || keysym->scancode == KRightButtonCode ))
		{
		keysym->sym = SDLK_UNKNOWN; 
		}	
	else if ((!(Private->iSX0Mode & ESX0Portrait)) && (Private->iSX0Mode & ESX0Flipped)) {
			switch(keysym->sym) {
			case SDLK_UP:	keysym->sym = SDLK_LEFT;  break;
			case SDLK_DOWN: keysym->sym = SDLK_RIGHT; break;
			case SDLK_LEFT: keysym->sym = SDLK_DOWN;  break;
			case SDLK_RIGHT:keysym->sym = SDLK_UP;   break;
			default:
				break;
		}
	}
	else if (!(Private->iSX0Mode & ESX0Portrait)) {
#else
	if(Private->EPOC_IsFlipped && !(Private->iSX0Mode & ESX0Portrait)){
#endif
		switch(keysym->sym) {
			case SDLK_UP:	keysym->sym = SDLK_RIGHT;  break;
			case SDLK_DOWN: keysym->sym = SDLK_LEFT; break;
			case SDLK_LEFT: keysym->sym = SDLK_UP;  break;
			case SDLK_RIGHT:keysym->sym = SDLK_DOWN;   break;
			default:
				break;
		}
	}

	/* If UNICODE is on, get the UNICODE value for the key */

	keysym->unicode = 0;
	
	if ( SDL_TranslateUNICODE ) 
    {
		/* Populate the unicode field with the ASCII value */	
		switch (keysym->scancode)
		{
			/* Esc key */
		case EStdKeyEscape: 
			keysym->unicode = 27;
			break;
			/* BackSpace key */
		case EStdKeyBackspace: 
			keysym->unicode = keysym->sym;
			break;
			/* Enter key */
		case EStdKeyEnter: 
			keysym->unicode = SDLK_RETURN;
			break;
		case EStdKeySpace:
			keysym->unicode = ' ';
			break;
		case EStdKeyComma:
			keysym->unicode = ',';
			break;
		case EStdKeyFullStop:
			keysym->unicode = '.';
			break;
		case EStdKeySingleQuote:
			keysym->unicode = '´';
			break;
		case EStdKeyHash:
			keysym->unicode = '#';
			break;
		case EStdKeyBackSlash:
			keysym->unicode = '\\';
			break;

		case EStdKeyForwardSlash:
			keysym->unicode = '/';

			break;
		default:
			TChar chr(keysym->scancode);
			if(chr.IsPrint())
			{
				if(chr.IsAlpha())
				{
					if(keysym->mod&KMOD_SHIFT)
					{
						keysym->unicode = chr.GetUpperCase();
					}
					else
					{
						keysym->unicode = chr.GetLowerCase();
					}
				}
				else
				{
					if(keysym->sym<128)
					{
					keysym->unicode = keysym->sym;
					}
					else
					{
						keysym->unicode  = 0;
					}
				}
			}
			break;
		}
		
		
	}
	
    //!!
    //sprintf(debug, "SDL: TranslateKey: keysym->scancode=%d, keysym->sym=%d, keysym->mod=%d",
    //    keysym->scancode, keysym->sym, keysym->mod);
    //SDL_TRACE(debug); //!!

	return(keysym);
}

void SetTextInputState(TBool aInputOn)
{
	if(aInputOn)
	{
		keymap[EStdKeyBackspace] = SDLK_BACKSPACE;
		keymap[EStdKeyDelete]    =	 SDLK_DELETE;
		keymap['2']    = SDLK_UNKNOWN;
		keymap['3']    = SDLK_UNKNOWN;
		keymap['4']    = SDLK_UNKNOWN;
		keymap['5']    = SDLK_UNKNOWN;
		keymap['6']    = SDLK_UNKNOWN;
		keymap['7']    = SDLK_UNKNOWN;
		keymap['8']    = SDLK_UNKNOWN;
		keymap['9']    = SDLK_UNKNOWN;	
	}
	else
	{
#if defined (S60) || defined (S60V3) || defined (UIQ3)
		keymap[EStdKeyBackspace]    = SDLK_ESCAPE;
		keymap[EStdKeyDelete]    =	 SDLK_ESCAPE;
#endif
		keymap['2']    = SDLK_2;
		keymap['3']    = SDLK_3;
		keymap['4']    = SDLK_4;
		keymap['5']    = SDLK_5;
		keymap['6']    = SDLK_6;
		keymap['7']    = SDLK_7;
		keymap['8']    = SDLK_8;
		keymap['9']    = SDLK_9;		
	}
}



#if defined (S60) || defined (S80) || defined (S90) || defined (S60V3) || defined (UIQ3)
void SetJoystickState(TBool aJoystickOn)
{
	if(aJoystickOn) // Joystick is handling this
	{
#ifdef UIQ3
	keymap[EStdKeyNkp2]         = SDLK_2;
	keymap[EStdKeyNkp4]         = SDLK_4;
	keymap[EStdKeyNkp6]         = SDLK_6;
	keymap[EStdKeyNkp8]         = SDLK_8;
	keymap[EStdDeviceKeySoftkey1] = SDLK_UNKNOWN;
	keymap[EStdDeviceKeySoftkey2] = SDLK_UNKNOWN;

	keymap['*']	 = SDLK_F5;
	keymap[EStdKeyNkpAsterisk]	 = SDLK_F5;

#elif !defined (S90)
	keymap[EStdKeyDevice0]		= SDLK_UNKNOWN;
	keymap[EStdKeyDevice1]		= SDLK_UNKNOWN;
	keymap[EStdKeyDevice3]		= SDLK_UNKNOWN;
#else
	keymap[EStdKeyDevice4] = SDLK_UNKNOWN;
	keymap[EStdKeyDevice5] = SDLK_UNKNOWN;
#endif
	}
	else // Enable cursor keys
	{
#ifdef UIQ3
	keymap[EStdKeyNkp2]         = SDLK_UP;
	keymap[EStdKeyNkp4]         = SDLK_LEFT;
	keymap[EStdKeyNkp6]         = SDLK_RIGHT;
	keymap[EStdKeyNkp8]         = SDLK_DOWN;
	keymap[EStdDeviceKeySoftkey1] = SDLK_RETURN;
	keymap[EStdDeviceKeySoftkey2] = SDLK_SPACE;

	keymap['*']					= SDLK_ASTERISK;
	keymap[EStdKeyNkpAsterisk]	 = SDLK_ASTERISK;

#elif !defined (S90)
	keymap[EStdKeyDevice0]		= SDLK_RETURN;
	keymap[EStdKeyDevice1]		= SDLK_SPACE;
	keymap[EStdKeyDevice3]		= SDLK_PERIOD;
#else
	keymap[EStdKeyDevice4] = SDLK_F5;
	keymap[EStdKeyDevice5] = SDLK_F1;
#endif
	}
}
#endif
} /* extern "C" */

#ifdef UIQ3
void SetKeyboardMapMode(TBool aNumPadMode)
{
	if(aNumPadMode)
	{
		keymap['Q']= keymap['W'] = (SDLKey) '1';
		keymap['E']= keymap['R'] = (SDLKey) '2';
		keymap['T']= keymap['Y'] = (SDLKey) SDLK_UP;
		keymap['U']= keymap['I'] = (SDLKey) '3';
		keymap['O']= keymap['P'] = (SDLKey) '4';
		keymap['A']= keymap['S'] = (SDLKey) '5';
		keymap['D']= keymap['F'] = (SDLKey) SDLK_LEFT;
		keymap['G']= keymap['H'] = (SDLKey) '6';
		keymap['J']= keymap['K'] = (SDLKey) SDLK_RIGHT;
		keymap['L']= (SDLKey) SDLK_BACKSPACE;
		keymap[',']= keymap['Z'] = (SDLKey) '7';
		keymap['X']= keymap['C'] = (SDLKey) '8';
		keymap['V']= keymap['B'] =(SDLKey)  SDLK_DOWN;
		keymap['N']= keymap['M'] = (SDLKey) '9';
		keymap['.'] = (SDLKey) SDLK_RETURN;
	}
	else
	{
		/* e.g. Alphabet keys */
		for (TInt i = 0; i<32; ++i ){
			keymap['A' + i] = (SDLKey)(SDLK_a+i);
		keymap['.'] = (SDLKey) '.';
		keymap[','] = (SDLKey) ',';
		}
	}
}
#endif


#ifdef S60V3
static const TUint32 hid_to_SDL[256] =
    {
    /*  00  */ 000,000,000,000,'a','b','c','d','e','f','g','h','i','j','k','l',
    /*  16  */ 'm','n','o','p','q','r','s','t','u','v','w','x','y','z','1','2',
    /*  32  */ '3','4','5','6','7','8','9','0',SDLK_RETURN,SDLK_ESCAPE,SDLK_BACKSPACE,SDLK_TAB,SDLK_SPACE,'-','=','[',
    /*  48  */ ']','\\',000,';', '\'', '`', ',','.','/',SDLK_CAPSLOCK,SDLK_F1,SDLK_F2,SDLK_F3,SDLK_F4,SDLK_F5,SDLK_F6,
    /*  64  */ SDLK_F7,SDLK_F8,SDLK_F9,SDLK_F10,SDLK_F11,SDLK_F12,SDLK_PRINT,SDLK_SCROLLOCK,SDLK_BREAK,SDLK_INSERT,SDLK_HOME,SDLK_PAGEUP,SDLK_DELETE,SDLK_END,SDLK_PAGEDOWN,SDLK_RIGHT,
    /*  80  */ SDLK_LEFT,SDLK_DOWN,SDLK_UP,SDLK_NUMLOCK,SDLK_KP_DIVIDE,SDLK_KP_MULTIPLY,SDLK_KP_MINUS,SDLK_KP_PLUS,SDLK_KP_ENTER,SDLK_KP1,SDLK_KP2,SDLK_KP3,SDLK_KP4,SDLK_KP5,SDLK_KP6,SDLK_KP7,
    /*  96  */ SDLK_KP8,SDLK_KP9,SDLK_KP0,SDLK_KP_PERIOD,'<',000,000,SDLK_KP_EQUALS,SDLK_F13,SDLK_F14,SDLK_F15,000,000,000,000,000,
    /*  112 */ 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
    /*  128 */ 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
    /*  144 */ 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
    /*  160 */ 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
    /*  176 */ 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
    /*  192 */ 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
    /*  208 */ 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
    /*  224 */ SDLK_LCTRL,SDLK_LSHIFT,SDLK_LALT,'~',SDLK_RCTRL,SDLK_RSHIFT,SDLK_RALT,000,000,000,000,000,000,000,000,000,
    /*  240 */ 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000
    };

static const TUint32 hid_to_SDL_shifted[256] = 
    {
    /*  00  */ 000,000,000,000,'a','b','c','d','e','f','g','h','i','j','k','l',
    /*  16  */ 'm','n','o','p','q','r','s','t','u','v','w','x','y','z','!','@',
    /*  32  */ '#','$','%','^','&','*','(',')',SDLK_RETURN,SDLK_ESCAPE,SDLK_BACKSPACE,SDLK_TAB,SDLK_SPACE,'_','+','{',
    /*  48  */ '}','|',000,':','"','~','<','>','?',SDLK_CAPSLOCK,SDLK_F1,SDLK_F2,SDLK_F3,SDLK_F4,SDLK_F5,SDLK_F6,
    /*  64  */ SDLK_F7,SDLK_F8,SDLK_F9,SDLK_F10,SDLK_F11,SDLK_F12,SDLK_PRINT,SDLK_SCROLLOCK,SDLK_BREAK,SDLK_INSERT,SDLK_HOME,SDLK_PAGEUP,SDLK_DELETE,SDLK_END,SDLK_PAGEDOWN,SDLK_RIGHT,
    /*  80  */ SDLK_LEFT,SDLK_DOWN,SDLK_UP,SDLK_NUMLOCK,SDLK_KP_DIVIDE,SDLK_KP_MULTIPLY,SDLK_KP_MINUS,SDLK_KP_PLUS,SDLK_KP_ENTER,SDLK_KP1,SDLK_KP2,SDLK_KP3,SDLK_KP4,SDLK_KP5,SDLK_KP6,SDLK_KP7,
    /*  96  */ SDLK_KP8,SDLK_KP9,SDLK_KP0,SDLK_KP_PERIOD,'<',000,000,SDLK_KP_EQUALS,SDLK_F13,SDLK_F14,SDLK_F15,000,000,000,000,000,
    /*  112 */ 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
    /*  128 */ 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
    /*  144 */ 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
    /*  160 */ 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
    /*  176 */ 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
    /*  192 */ 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
    /*  208 */ 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,
    /*  224 */ SDLK_LCTRL,SDLK_LSHIFT,SDLK_LALT,'~',SDLK_RCTRL,SDLK_RSHIFT,SDLK_RALT,000,000,000,000,000,000,000,000,000,
    /*  240 */ 000,000,000,000,000,000,000,000,000,000,000,000,000,000,000,000
    };

void HID_GenerateKeyEvent(_THIS,int aScanCode,int aIsDown, TBool aShiftOn)
{		
	SDL_keysym keysym;	
	SDLMod modState = SDL_GetModState();
	keysym.scancode = aScanCode;
	if (aShiftOn)
		{
		keysym.sym = (SDLKey) hid_to_SDL_shifted[aScanCode];
		}
	else
		{
		keysym.sym = (SDLKey) hid_to_SDL[aScanCode];
		}
	
	if(aShiftOn)
	{
		SDL_SetModState((SDLMod)(modState|KMOD_LSHIFT|KMOD_RSHIFT));	
	}
	
								
	SDL_PrivateKeyboard(aIsDown ? SDL_PRESSED:SDL_RELEASED, &keysym);
	
	if(aShiftOn)
	{
		SDL_SetModState(modState);	
	}			
}

void CHIDEventMonitor::RunL()
    {
    THIDEvent hidEvent;
    iHIDClient->GetEvent( hidEvent );
    switch (hidEvent.Type())
        {
        case THIDEvent::EMouseEvent:
            {
            TBool down = EFalse;
            TMouseEvent* mouse = hidEvent.Mouse();
            switch (mouse->Type())
                {
                case EEventRelativeXY:
                    //currentPosition += mouse->iPosition;
					SDL_PrivateMouseMotion(0 , 1, mouse->iPosition.iX, mouse->iPosition.iY);
                    break;
                case EEventButtonDown:
                    down = ETrue;
                case EEventButtonUp:
                    switch(mouse->iValue)
                        {
                        case EMouseButtonLeft:
                            SDL_PrivateMouseButton(down?SDL_PRESSED:SDL_RELEASED, SDL_BUTTON_LEFT, 0, 0);
                            break;
                        case EMouseButtonRight:
                            SDL_PrivateMouseButton(down?SDL_PRESSED:SDL_RELEASED, SDL_BUTTON_RIGHT, 0, 0);
                            break;
                        case EMouseButtonMiddle:
                            SDL_PrivateMouseButton(down?SDL_PRESSED:SDL_RELEASED, SDL_BUTTON_MIDDLE, 0, 0);
                            break;
						case EMouseButtonSide:
							EPOC_GenerateKeyEvent(current_video, SDLK_RETURN, down, EFalse);							
							break;
						case EMouseButtonForward:
							EPOC_GenerateKeyEvent(current_video, SDLK_PAGEDOWN, down, EFalse);
							break;
						case EMouseButtonBack:
							EPOC_GenerateKeyEvent(current_video, SDLK_PAGEUP, down, EFalse);
							break;
                        default:
                            break;
                        }
                    break;
                case EEventRelativeWheel:
                    if (mouse->iValue > 0)
                        {
						SDL_PrivateMouseButton(SDL_PRESSED, SDL_BUTTON_WHEELUP, 0, 0);
						SDL_PrivateMouseButton(SDL_RELEASED, SDL_BUTTON_WHEELUP, 0, 0);                       
                        }
                    else
                        {
						SDL_PrivateMouseButton(SDL_PRESSED, SDL_BUTTON_WHEELDOWN, 0, 0);
						SDL_PrivateMouseButton(SDL_RELEASED, SDL_BUTTON_WHEELDOWN, 0, 0);         
                        }
                    break;
                default:
                    break;
                }
            }
            break;
        case THIDEvent::EKeyEvent:
            {
            THIDKeyEvent* key = hidEvent.Key();
                
            switch (key->Type())
                {
                case EEventHIDKeyUp:
                    
                    if (key->ScanCode() == 0xE1 || key->ScanCode() == 0xE5)
                        {
                        shift_down = EFalse;
                        }                 
                    HID_GenerateKeyEvent(current_video, key->ScanCode(), EFalse, shift_down);
                    break;
                case EEventHIDKeyDown:
                	if (key->ScanCode() == 0xE1 || key->ScanCode() == 0xE5)
                        {
                        shift_down = ETrue;
                        }
                                      
					HID_GenerateKeyEvent(current_video, key->ScanCode(), ETrue, shift_down);
                    break;
                default:
                    break;
                }
            }
            break;
        case THIDEvent::EConsumerEvent:
            {
            THIDConsumerEvent* consumer = hidEvent.Consumer();
            //Com_Printf( "consumer, code %d\n",consumer->ButtonCode());
            break;
            }
        default:
            break;
        }
    iHIDClient->EventReady( &iStatus );
    SetActive();
    }
    
void CHIDEventMonitor::DoCancel()
    {
    iHIDClient->EventReadyCancel();
    }

CHIDEventMonitor* CHIDEventMonitor::NewL()
    {
    CHIDEventMonitor* self = new (ELeave) CHIDEventMonitor;
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
    return self;
    }

CHIDEventMonitor::~CHIDEventMonitor()
    {
    Cancel();
    delete iHIDClient;
    }

CHIDEventMonitor::CHIDEventMonitor() 
        :CActive(CActive::EPriorityUserInput)
    {
    }
        
void CHIDEventMonitor::ConstructL()
    {
    TInt error = iHidLibrary.Load(_L("hidsrv.dll"));
    User::LeaveIfError( error );
    TLibraryFunction entry = iHidLibrary.Lookup(1);
    if (entry)
        {
        iHIDClient = (MHIDSrvClient*) entry();
        if (iHIDClient)
            {
            error = iHIDClient->Connect();
            User::LeaveIfError( error );
            iHIDClient->EventReady( &iStatus );
            CActiveScheduler::Add( this );
            SetActive();
            }
        }
    else
        {
        User::Leave(KErrNotFound);
        }
    }

#endif

