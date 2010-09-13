/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001  Sam Lantinga

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
    slouken@devolution.com
*/
#include "SDL_config.h"

/*
    SDL_epocevents_c.h
    Handle the event stream, converting Epoc events into SDL events

    Symbian version by Hannu Viitala (hannu.j.viitala@mbnet.fi) and Markus Mertama
	and Lars Persson    
*/


#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id: SDL_aaevents_c.h,v 1.1.2.2 2000/03/16 15:20:39 hercules Exp $";
#endif

extern "C" {
#include "SDL_sysvideo.h"
}

#define MAX_SCANCODE 255
#ifdef UIQ3
#include <devicekeys.h>
#define KDefaultLeftButtonCode1 EStdDeviceKeyAction 
#define KDefaultLeftButtonCode2 EStdDeviceKeySoftkey1
#define KDefaultRightButtonCode EStdDeviceKeySoftkey2
#else
#define KDefaultLeftButtonCode1 EStdKeyDevice0
#define KDefaultLeftButtonCode2 EStdKeyDevice3
#define KDefaultRightButtonCode EStdKeyDevice1
#endif
#define KLeftButtonCode1 Private->iLeftButtonCode1
#define KLeftButtonCode2 Private->iLeftButtonCode2
#define KRightButtonCode Private->iRightButtonCode
#define KLeftButtonCodeCase (ENonCharacterKeyBase+ENonCharacterKeyCount)
#define KRightButtonCodeCase (ENonCharacterKeyBase+ENonCharacterKeyCount+1)
/* Variables and functions exported by SDL_sysevents.c to other parts 
   of the native video subsystem (SDL_sysvideo.c)
*/
/**
 * Flags for key remapping
 */
enum TEpocKeyMapFlags
	{
	EKeyMapFlagNone     = 0,
	EKeyMapFlagShift    = 1,
	EKeyMapFlagCtrl     = 2,
	EKeyMapFlagFunc     = 4,
	//EKeyMapFlagStick    = 8,
	//EKeyMapFlagHotkey   = 16,
	//EKeyMapFlagSimulated= 32,
	EKeyMapFlagMaxValue
	};

/* Hidden "this" pointer for the video functions */
#define _THIS	SDL_VideoDevice *_this
#define Private	_this->hidden

enum TSdlKeyMapFlags
	{
	ESdlKeyMapFlagNone     = 0,
	//ESdlKeyMapFlagShift    = 1,
	};
extern "C" {
/**
 * !!
 */
class TKeyMapItem
	{
public:
	TKeyMapItem(TUint8 aOldFlag = EKeyMapFlagNone, TUint aOldCode = 0, TUint8 aNewFlag = ESdlKeyMapFlagNone, TUint8 aNewCode = 0)
		:iOldFlag(aOldFlag), iOldCode(aOldCode), iNewFlag(aNewFlag), iNewCode(aNewCode) {};
	static TInt Compare(const TKeyMapItem& aFirst, const TKeyMapItem& aSecond);
public:
	TUint8 iOldFlag;
	TUint iOldCode;
	TUint8 iNewFlag;
	TUint iNewCode;
	};
extern void EPOC_InitOSKeymap(_THIS);
extern void EPOC_PumpEvents(_THIS);
}

extern TBool isCursorVisible;

