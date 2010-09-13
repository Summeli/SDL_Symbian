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

#ifdef SDL_TIMER_SYMBIAN

/*
    SDL_systimer.cpp

    Epoc version by Hannu Viitala (hannu.j.viitala@mbnet.fi)
    Markus Mertama
*/

#include <e32std.h>
#include <e32hal.h>

extern "C" {
#include "SDL_error.h"
#include "SDL_thread.h"
#include "SDL_timer.h"
#include "SDL_timer_c.h"

#if defined (UIQ3) || defined(S60V3)
static TTime start;
#else
static TTime start;
#endif

void SDL_StartTicks(void)
{
	/* Set first ticks value */
#if defined (UIQ3) || defined(S60V3)
	start.HomeTime();
#else
    start.HomeTime();
#endif
}

#if defined (UIQ3) || defined(S60V3)
const TUint32 KTickDelay = 5000;
#else
const TUint32 KTickDelay = 15624;
#endif
Uint32 SDL_GetTicks(void)
{
#if defined (UIQ3) || defined(S60V3)
	TTime now;
	now.HomeTime();
	Uint32 ms = (now.MicroSecondsFrom(start).Int64())/1000;
	return ms;
#else
	TTime now;
	now.HomeTime();
	Uint32 ms = ((now.MicroSecondsFrom(start).Int64())/1000).GetTInt();
	return ms; 
#endif
}

void SDL_Delay(Uint32 ms)
{
#if defined (UIQ3) || defined(S60V3)
    User::AfterHighRes(ms*1000);
#else
    // Lets round down to the nearest 1/64 second available.. i.e anything less than 1/64 is a zero delay
	TInt ticks = ms>>4; // Shift down by three. 1/64 is 15.625 millisecs, so 16 millisecs is the closest
						// SymbianOS built in ticker can ONLY delay to the closest tick, that means 
						// That even for a delay of 1 microsecond a delay of one tick (1/64) is made
	if(ticks>0)
	{
    User::After(TTimeIntervalMicroSeconds32((ticks*15625)-1));
	}
#endif
}

/* Data to handle a single periodic alarm */
static int timer_alive = 0;
static SDL_Thread *timer = NULL;

static int RunTimer(void * /*unused*/)
{
	while ( timer_alive ) {
		if ( SDL_timer_running ) {
			SDL_ThreadedTimerCheck();
		}
#if defined (UIQ3) || defined(S60V3)
    User::AfterHighRes(KTickDelay);
#else
		// Wait for one tick before checking again
		User::After(KTickDelay);
#endif
	}
	return(0);
}

/* This is only called if the event thread is not running */
int SDL_SYS_TimerInit(void)
{
	timer_alive = 1;
	timer = SDL_CreateThread(RunTimer, NULL);
	if ( timer == NULL )
		return(-1);
	return(SDL_SetTimerThreaded(1));
}

void SDL_SYS_TimerQuit(void)
{
	timer_alive = 0;
	if ( timer ) {
		SDL_WaitThread(timer, NULL);
		timer = NULL;
	}
}

int SDL_SYS_StartTimer(void)
{
	SDL_SetError("Internal logic error: Epoc uses threaded timer");
	return(-1);
}

void SDL_SYS_StopTimer(void)
{
	return;
}

} // extern "C"
#endif /* SDL_TIMER_EPOC */
