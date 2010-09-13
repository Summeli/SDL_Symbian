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

    based on win32/SDL_mmjoystick.c

    Sam Lantinga
    slouken@libsdl.org
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id: SDL_sysjoystick.cpp,v 1.9 2007-02-17 23:04:02 anotherguest Exp $";
#endif

/* Win32 MultiMedia Joystick driver, contributed by Andrei de A. Formiga */

#include <stdlib.h>
#include <stdio.h>		/* For the definition of NULL */
#include <e32base.h>
#include "SDL_epocvideo.h"
extern  SDL_VideoDevice * current_video;

extern "C" 
{
#include "SDL_error.h"
#include "SDL_joystick.h"
#include "SDL_sysjoystick.h"
#include "SDL_joystick_c.h"
#include "SDL_sysvideo.h"
#if defined (S60) || defined (S80) || defined (S90) || defined (S60V3) || defined (UIQ3)
extern void SetJoystickState(TBool aJoystickOn);
#endif

/* Function to scan the system for joysticks.
 * Joystick 0 should be the system default joystick.
 * This function should return the number of available joysticks, or -1
 * on an unrecoverable fatal error.
 */
int SDL_SYS_JoystickInit(void)
{
	return 1;
}

const char KEpocJoyName[10]="Joystick";
/* Function to get the device-dependent name of a joystick */
const char *SDL_SYS_JoystickName(int /*index*/)
{
	return  KEpocJoyName;
}

/* Function to open a joystick for use.
   The joystick to open is specified by the index field of the joystick.
   This should fill the nbuttons and naxes fields of the joystick structure.
   It returns 0, or -1 if there is an error.
 */
int SDL_SYS_JoystickOpen(SDL_Joystick * joystick)
{
#if defined (UIQ) 
	joystick->nbuttons=1;
#elif defined (S60) || defined (S60V3) 
	joystick->nbuttons=3;
	if(current_video && current_video->hidden)
	{
	SetJoystickState((current_video->hidden->iInputMode==EJoystick));// Joystick is opened set it as default and disable cursor keys
	}
#elif defined (S80)
	joystick->nbuttons=3;
	if(current_video && current_video->hidden)
	{
	SetJoystickState((current_video->hidden->iInputMode==EJoystick));// Joystick is opened set it as default and disable cursor keys
	}
#elif defined (S90) || defined (UIQ3)
	joystick->nbuttons=2;
	if(current_video && current_video->hidden)
	{
	SetJoystickState((current_video->hidden->iInputMode==EJoystick));// Joystick is opened set it as default and disable cursor keys
	}
#endif
	joystick->naxes=2;
//	current_video->hidden->->iTheJoystick=joystick;
	return 0;
}

#define JOY_DEADZONE 6400 // From scumm

/* Function to update the state of a joystick - called as a device poll.
 * This function shouldn't update the joystick structure directly,
 * but instead should call SDL_PrivateJoystick*() to deliver events
 * and update joystick device state.
 */
void SDL_SYS_JoystickUpdate(SDL_Joystick * joystick)
{
	if(current_video->hidden->iJoystickStatus[EJoyLEFT]!= current_video->hidden->iLastJoystickStatus[EJoyLEFT]|| current_video->hidden->iJoystickStatus[EJoyRIGHT]!= current_video->hidden->iLastJoystickStatus[EJoyRIGHT])
	{
		if(!current_video->hidden->iJoystickStatus[EJoyLEFT] && !current_video->hidden->iJoystickStatus[EJoyRIGHT])
		SDL_PrivateJoystickAxis(joystick,0,0);
			else
		SDL_PrivateJoystickAxis(joystick,0,current_video->hidden->iJoystickStatus[EJoyLEFT]?-JOY_DEADZONE:JOY_DEADZONE);
		current_video->hidden->iLastJoystickStatus[EJoyLEFT] = current_video->hidden->iJoystickStatus[EJoyLEFT];
		current_video->hidden->iLastJoystickStatus[EJoyRIGHT] = current_video->hidden->iJoystickStatus[EJoyRIGHT];
	}

	if(current_video->hidden->iJoystickStatus[EJoyUP]!= current_video->hidden->iLastJoystickStatus[EJoyUP]|| current_video->hidden->iJoystickStatus[EJoyDOWN]!= current_video->hidden->iLastJoystickStatus[EJoyDOWN])
	{
		if(!current_video->hidden->iJoystickStatus[EJoyUP] && !current_video->hidden->iJoystickStatus[EJoyDOWN])
		SDL_PrivateJoystickAxis(joystick,1,0);
			else
		SDL_PrivateJoystickAxis(joystick,1,current_video->hidden->iJoystickStatus[EJoyUP]?-JOY_DEADZONE:JOY_DEADZONE);
		current_video->hidden->iLastJoystickStatus[EJoyUP] = current_video->hidden->iJoystickStatus[EJoyUP];
		current_video->hidden->iLastJoystickStatus[EJoyDOWN] = current_video->hidden->iJoystickStatus[EJoyDOWN];
	}


	if(current_video->hidden->iJoystickStatus[EJoyBUT1]!= current_video->hidden->iLastJoystickStatus[EJoyBUT1])
	{
		SDL_PrivateJoystickButton(joystick,0,current_video->hidden->iJoystickStatus[EJoyBUT1]);
		current_video->hidden->iLastJoystickStatus[EJoyBUT1] = current_video->hidden->iJoystickStatus[EJoyBUT1];
	}

	if(current_video->hidden->iJoystickStatus[EJoyBUT2]!= current_video->hidden->iLastJoystickStatus[EJoyBUT2])
	{
		SDL_PrivateJoystickButton(joystick,1,current_video->hidden->iJoystickStatus[EJoyBUT2]);
		current_video->hidden->iLastJoystickStatus[EJoyBUT2] = current_video->hidden->iJoystickStatus[EJoyBUT2];
	}
	
	if(current_video->hidden->iJoystickStatus[EJoyBUT3]!= current_video->hidden->iLastJoystickStatus[EJoyBUT3])
	{
		SDL_PrivateJoystickButton(joystick,2,current_video->hidden->iJoystickStatus[6]);
		current_video->hidden->iLastJoystickStatus[EJoyBUT3] = current_video->hidden->iJoystickStatus[EJoyBUT3];
	}
}

/* Function to close a joystick after use */
void SDL_SYS_JoystickClose(SDL_Joystick * /*joystick*/)
{
#if defined (S60) || defined (S80) || defined (S60V3) || defined (UIQ3)
	SetJoystickState(EFalse);
#endif
}

/* Function to perform any system-specific joystick related cleanup */
void SDL_SYS_JoystickQuit(void)
{
}
}

