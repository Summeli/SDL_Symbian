/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2006 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/

/*

Symbian version Markus Mertama
and Lars Persson
*/


#ifndef _SDL_CONFIG_SYMBIAN_H
#define _SDL_CONFIG_SYMBIAN_H

#include "SDL_platform.h"

/* This is the minimal configuration that can be used to build SDL */


#include <stdarg.h>
#include <stddef.h>
#include <sys\types.h>

#ifdef __GCCE__
#define SYMBIAN32_GCCE
#endif

#if !defined _SIZE_T_DEFINED && !defined _SIZE_T_DECLARED
//typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#define _SIZE_T_DECLARED
#endif

#ifndef _INTPTR_T_DECLARED
typedef unsigned int uintptr_t;
#define _INTPTR_T_DECLARED
#endif 

#ifndef _INT8_T_DECLARED
typedef signed char int8_t;
#define _INT8_T_DECLARED
#endif 

#ifndef _UINT8_T_DECLARED
typedef unsigned char uint8_t;
#define _UINT8_T_DECLARED
#endif

#ifndef _INT16_T_DECLARED
typedef signed short int16_t;
#define _INT16_T_DECLARED
#endif

#ifndef _UINT16_T_DECLARED
typedef unsigned short uint16_t;
#define _UINT16_T_DECLARED
#endif

#ifndef _INT32_T_DECLARED
typedef signed int int32_t;
#define _INT32_T_DECLARED
#endif

#ifndef _UINT32_T_DECLARED
typedef unsigned int uint32_t;
#define _UINT32_T_DECLARED
#endif

#ifndef _INT64_T_DECLARED
	#ifndef __WINS__
	typedef signed long long int64_t;
	#define _INT64_T_DECLARED
	#endif
#endif

#ifndef _UINT64_T_DECLARED
	#ifndef __WINS__
		typedef unsigned long long uint64_t;
		#define _UINT64_T_DECLARED
	#endif
#endif


#define SDL_HAS_64BIT_TYPE	1

#ifndef __GCCE__ // Should be symbian os 9
#undef SDL_HAS_64BIT_TYPE
#endif

/* Enabled for SDL 1.2 (binary compatibility) */
#define HAVE_LIBC	1
#ifdef HAVE_LIBC
/* Useful headers */
#define HAVE_STDIO_H 1
#define STDC_HEADERS 1
#define HAVE_STRING_H 1
#define HAVE_CTYPE_H 1
#define HAVE_MATH_H 1

/* C library functions */
#define HAVE_MALLOC 1
#define HAVE_CALLOC 1
#define HAVE_REALLOC 1
#define HAVE_FREE 1
#define HAVE_ALLOCA 1
#define HAVE_QSORT 1
#define HAVE_ABS 1
#define HAVE_MEMSET 1
#define HAVE_MEMCPY 1
#define HAVE_MEMMOVE 1
#define HAVE_MEMCMP 1
#define HAVE_STRLEN 1
#define HAVE__STRUPR 1
#define HAVE_STRCHR 1
#define HAVE_STRRCHR 1
#define HAVE_STRSTR 1
#define HAVE_ITOA 1
#define HAVE_STRTOL 1
#define HAVE_STRTOUL 1
#define HAVE_STRTOD 1
#define HAVE_ATOI 1
#define HAVE_ATOF 1
#define HAVE_STRCMP 1
#define HAVE_STRNCMP 1
#define HAVE__STRNICMP 1
#define HAVE_STRNCASECMP 1
#define HAVE_SSCANF 1
#else
#define HAVE_STDARG_H	1
#define HAVE_STDDEF_H	1
#endif

/* Enable various audio drivers */
#define SDL_AUDIO_DRIVER_SYMBIAN	1

/* Enable various cdrom drivers */
#define SDL_CDROM_DUMMY      1

/* Enable various input drivers */
#define SDL_JOYSTICK_SYMBIAN	1

/* Enable various threading systems */
#define SDL_THREAD_SYMBIAN	1

/* Enable various timer systems */
#define SDL_TIMER_SYMBIAN	1

/* Enable various video drivers */
#define SDL_VIDEO_DRIVER_SYMBIAN	1

/* Enable OpenGL support */
/*#ifndef _WIN32_WCE
#define SDL_VIDEO_OPENGL	1
#define SDL_VIDEO_OPENGL_WGL	1
#endif*/ // Maybe for Symbian V9
#undef SDL_VIDEO_OPENGL
#undef SDL_VIDEO_OPENGL_WGL
/* Enable assembly routines (Win64 doesn't have inline asm) */
#define SDL_ASSEMBLY_ROUTINES	0
#endif /* _SDL_CONFIG_SYMBIAN_H */
