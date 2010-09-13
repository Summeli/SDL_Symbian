/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000  Sam Lantinga

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

/*
    SDL_sysmutex.cpp

    Epoc version by Markus Mertama (w@iki.fi)
*/


#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id: SDL_sysmutex.c,v 1.1.2.3 2000/06/22 15:25:23 hercules Exp $";
#endif

/* Mutex functions using the Win32 API */

#include <e32std.h>

#include "SDL_error.h"
#include "SDL_mutex.h"



struct SDL_mutex
    {
	TThreadId owner;
	TInt recursive;
	TInt handle;
    };

extern TInt CreateUnique(TInt (*aFunc)(const TDesC& aName, TAny*, TAny*), TAny*, TAny*);

TInt NewMutex(const TDesC& aName, TAny* aPtr1, TAny*)
    {
    return ((RMutex*)aPtr1)->CreateGlobal(aName);
    }

/* Create a mutex */
SDL_mutex *SDL_CreateMutex(void)
{
    RMutex rmutex;

    TInt status = CreateUnique(NewMutex, &rmutex, NULL);
	if(status != KErrNone)
	    {
			SDL_SetError("Couldn't create mutex");
		}
    SDL_mutex* mutex = new /*(ELeave)*/ SDL_mutex;
	mutex->owner = 0;
	mutex->recursive = 0;
    mutex->handle = rmutex.Handle();
	return(mutex);
}

/* Free the mutex */
void SDL_DestroyMutex(SDL_mutex *mutex)
{
	if ( mutex ) 
	{
    RMutex rmutex;
    rmutex.SetHandle(mutex->handle);
#if !defined (UIQ3) && !defined (S60V3)
	rmutex.Signal();
#endif
	rmutex.Close();
	delete(mutex);
    mutex = NULL;
	}
}

/* Lock the mutex */
int SDL_mutexP(SDL_mutex *mutex)
{
	if ( mutex == NULL ) {
		SDL_SetError("Passed a NULL mutex");
		return -1;
	}

	RThread this_thread;//  = pthread_self();
	if ( mutex->owner == this_thread.Id() ) {
		++mutex->recursive;
	} else {
	    RMutex rmutex;
		rmutex.SetHandle(mutex->handle);
		rmutex.Wait(); 
		mutex->owner = this_thread.Id();
		mutex->recursive = 0;
	}
	this_thread.Close();
	return(0);
}

/* Unlock the mutex */
int SDL_mutexV(SDL_mutex *mutex)
{
	if ( mutex == NULL ) {
		SDL_SetError("Passed a NULL mutex");
		return -1;
	}
	RThread this_thread;

	if ( this_thread.Id() == mutex->owner ) {
		if ( mutex->recursive ) {
			--mutex->recursive;
		} else {
			/* The order of operations is important.
			   First reset the owner so another thread doesn't lock
			   the mutex and set the ownership before we reset it,
			   then release the lock semaphore.
			 */
			mutex->owner = 0;
			RMutex rmutex;
			rmutex.SetHandle(mutex->handle);
			rmutex.Signal();
		}
	}


	this_thread.Close();
	return(0);
}
