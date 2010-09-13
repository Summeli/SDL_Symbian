/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2004 Sam Lantinga

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
    SDL_epocvideo.h
    Epoc based SDL video driver implementation

    Epoc version by Hannu Viitala (hannu.j.viitala@mbnet.fi)
*/

#ifndef _SDL_epocvideo_h
#define _SDL_epocvideo_h

extern "C" {
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
}

#include <e32std.h>
#include <bitdev.h> 
#include <w32std.h>
#include <eikenv.h>
#include "ebasicapp.h"
#if defined (S60) || defined (S80) || defined (S90) || defined(S60V3)
#include "SDLSXXView.h"
#endif
#define KDefaultActivateButtonSize 48
enum TInputModes 
	{
		EJoystick,
		EKeyboard,
		ECursorKeys,
		EMouseMode,
		ELastInputMode
	};

enum TVKBState
	{
		EDisplayMiniControls,
		EDisplayControls,		
		EDisplayKeys
	};

	struct KbdMouse {
		TInt16  x_vel, y_vel, x_down_count, y_down_count;
		TUint32 last_time, delay_time, x_down_time, y_down_time;
	};

/* The implementation dependent data for the window manager cursor */
struct WMcursor {	
	CWsBitmap*			iCursorPBitmap;
	CWsBitmap*			iCursorPMask;
	CWsBitmap*			iCursorLBitmap;
	CWsBitmap*			iCursorLMask;
	CWsBitmap*			iCursorLFBitmap;
	CWsBitmap*			iCursorLFMask;	
	TInt				iWidth;
	TInt				iHeight;
	TInt				iHotX;
	TInt 				iHotY;		
};

enum TSX0ScreenMode
{
	ESX0Stretched = 1,
	ESX0Portrait = 2,
	ESX0Flipped = 4,
	ESX0DontInterpolate = 8,
	ESX0KeepAspect = 16
};

enum TEPoCJoyStates
{
	EJoyLEFT,
	EJoyRIGHT,
	EJoyUP,
	EJoyDOWN,
	EJoyBUT1,
	EJoyBUT2,
	EJoyBUT3,
	ENoJoySTATES
};

enum TSDLMouseStates
	{
	ELeftMouseButton,
	ERightMouseButton,
	ENoMouseButton,
	ELastMouseButton
	};

#ifdef UIQ
#define KOnScreenOffsetStartVar aPointerEvent.iPosition.iX
#define KOnScreenOffsetStartVal 200
#define KOnScreenBoxWidth 8
#elif defined (UIQ3)
#define KOnScreenOffsetStartVar aPointerEvent.iPosition.iX
#define KOnScreenOffsetStartVal 228
#define KOnScreenBoxWidth 8
#elif defined (S90)
#define KOnScreenOffsetStartVar aPointerEvent.iPosition.iY
#define KOnScreenOffsetStartVal 304
#define KOnScreenBoxWidth 16
#endif

/* Hidden "this" pointer for the video functions */
#define _THIS	SDL_VideoDevice *_this
#define Private	_this->hidden

#define SDL_NUMMODES	9

/* Private display data */
struct SDL_PrivateVideoData {

    SDL_Rect            *SDL_modelist[SDL_NUMMODES+1];
	CEikonEnv*			iEikEnv;
	CEBasicView*		iWindowCreator;
	CAsyncCallBack*		iOnecallback;
	TInputModes			iInputMode;
	TSDLMouseStates		iMouseButtonSet;
	TBool				iVirtualKeyBoardActive;
	TBool				iShiftOn;
	TChar				iCurrentChar;
	TInt				iLastKey;
	TInt				iLastPos;
	TInt				iSX0Mode;
	TReal				iXScale;
	TReal				iYScale;
	TInt				iInputModeTimer;

#if defined (__WINS__ ) || defined(S60) || defined (S80) || defined (S90) || defined (UIQ3) || defined (S60V3)
    CWsBitmap*          EPOC_Bitmap;
	TInt				iThreadId;
	TBool				iNeedUpdate;
	TInt				iNumRects;
	SDL_Rect			iDirtyRects[20];
	TBool				iNeedFullRedraw;
	TPoint				iPutOffset;
	// mouse
	KbdMouse 			_km;
#if defined (S60) || defined(S60V3)
	TBool				iIs240Mode;
	TBool				iKeyboardModifier;
	TRect				iKeyboardRect;
	TVKBState			iVKBState;
	TBool				iVKBActive;	
	TBool				iNumLockOn;
	TInt				iActivateButtonSize;
	TBool				iNoStretch;
#endif //S60 S80 S90 

#if defined (UIQ3)
	TInt				iModeState;
	TInt				iModeKeys;
	TBool				iModeKeyReleased;
	TBool				iNumPadMode;
#endif
	TBool				iFNModeOn;
#endif // WINS S60 S80 S90 UIQ3 S60V3
	TBool				iJoystickStatus[ENoJoySTATES]; // 4 directons + 3 button state
	TBool				iLastJoystickStatus[ENoJoySTATES]; // 4 directons + 2 button state
    TBool               EPOC_IsWindowFocused;
    /* Screen hardware frame buffer info */
   	TBool				EPOC_HasFrameBuffer;
	TInt				EPOC_BytesPerPixel;
	TInt				EPOC_BytesPerScanLine;
	TDisplayMode		EPOC_DisplayMode;
	TSize				EPOC_ScreenSize;
	TSize				EPOC_DisplaySize;
//AK: Points to offscreen bitmap
	TUint8*				EPOC_FrameBuffer;		/* if NULL in HW we can't do direct screen access */
    TInt                EPOC_ScreenOffset;
    /* Simulate double screen height */
    TBool               EPOC_ShrinkedHeight;
	/*might also shrink the width for UIQ and S60 */
    TBool               EPOC_ShrinkedWidth; 
	/* Flip the screen this is because on the P800 the screen has a 208 pixels wide and 320 pixels high screen  */
	TBool				EPOC_IsFlipped;
	TBool				iControlKeyDown;
	TSize				iModeSize;
	TSize				iStretchSize;
	TPoint				iCenterOffset;
	TBool				iHasMouseOrTouch;
	struct WMcursor*	iCursor;
	TPoint				iCursorPos;
	TBool				iWasUpdated;
	TInt				iLeftButtonCode1;
	TInt				iLeftButtonCode2;
	TInt				iRightButtonCode;
};

void ClearBackBuffer(_THIS);
void UpdateScaleFactors();
extern "C" {
extern void RedrawWindowL(_THIS);
extern void EPOC_ReconfigureVideo(_THIS);
#ifdef S60V3
extern void EPOC_CalcScaleFactors(_THIS);
#endif

}

#endif /* _SDL_epocvideo_h */
