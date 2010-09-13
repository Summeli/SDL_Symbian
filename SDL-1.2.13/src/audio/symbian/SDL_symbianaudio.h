/****
 * SDL_EpocAudio.h
 * 
 * Copyright (C) 2003 Andreas Karlsson
 * All rights reserved
 */ 


#ifndef __SDL_AUDIO_EPOC_SDL_EPOCAUDIO_H__
#define __SDL_AUDIO_EPOC_SDL_EPOCAUDIO_H__


extern "C" {
#include "SDL_audio.h"
#include "../SDL_sysaudio.h"
}

#include "ebasicapp.h"

#define  _THIS SDL_AudioDevice *_this
#define Private	_this->hidden

class CStreamEngine;
class CSoundThreadWatcher;

struct SDL_PrivateAudioData
{
	CSoundThreadWatcher* iSoundThreadWatcher;
	CStreamEngine	*iStreamEngine;
	TInt			iVolume;
	CTrapCleanup	*iCleanup;
	CActiveScheduler *iScheduler;
};




#endif //__SDL_AUDIO_EPOC_SDL_EPOCAUDIO_H__
