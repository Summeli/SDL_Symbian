/****
* SDL_EpocAudio.h
* 
* Copyright (C) 2003 Andreas Karlsson, 2005 Lars Persson
* All rights reserved
*/ 

#include <string.h> //memset

#include <MdaAudioOutputStream.h>
#include <mda\common\audio.h>

#include "SDL_SymbianAudio.h"
#include "SDL_timer.h"
const TInt KMaxVolume = 10;

extern "C" void SDL_CalculateAudioSpec(SDL_AudioSpec *spec);

static int EPOC_OpenAudio(_THIS, SDL_AudioSpec *spec);
static void EPOC_WaitAudio(_THIS);
static void EPOC_ThreadInit(_THIS);
static Uint8 *EPOC_GetAudioBuf(_THIS);
static void EPOC_PlayAudio(_THIS);
static void EPOC_WaitDone(_THIS);
static void EPOC_CloseAudio(_THIS);

SDL_AudioSpec gSpec;
class CStreamEngine;
class CSoundThreadWatcher:public CActive
{
public:
	CSoundThreadWatcher(TInt aThreadHandle,_THIS);
	~CSoundThreadWatcher();
	void RunL();
	void DoCancel();
private:
	RThread iSndThread;
	_THIS;
};

class CStreamEngine : public MMdaAudioOutputStreamCallback, public CActive
{
public:
	
	enum TState
    {
        /** No operation */
		EIdle,
			/** Preparing to play a stream */
			EStreamPrepare,
			/** Stream initiated */
			EStreamStarted,
			/** Requires new stream data */
			ENextStreamBuf,
			/** Error in playing stream */
			EStreamError,
			/** Stream closing down */
			EStreamStopping
	};
	
	static CStreamEngine *NewL();
	
	~CStreamEngine();
	
	void OpenL(TInt aSampleRate, TInt aChannels);
	
	void Close();
	
	void PlayStreamL();
	
	TUint8 *Buffer();
	
	TState State();
	
	void WaitForOpenToComplete();
	void ThreadDied();
#if defined (UIQ3) || defined (S60V3)
	void WaitForAudio();
#endif		
	
	TThreadId					 iSndThreadId;
	
protected:
	CStreamEngine();
	void ConstructL();
	
	// Implement MMdaAudioOutputStreamCallback
	void MaoscPlayComplete(TInt aError);
	
	void MaoscBufferCopied(TInt aError, const TDesC8& aBuffer);
	
	void MaoscOpenComplete( TInt aError );
	
	void DoCancel();
	
	void RunL();

	CMdaAudioOutputStream		*iAudioStreamPlayer;
	TMdaAudioDataSettings		 iSettings;
	
	TInt8						 iLastVolume;
	TInt						 iMaxVolume;
	
	TState						 iState;
	
	HBufC8*						iBuffer;
	
	TBool						 iIsOpen;
	TBool					  	 iIsStarted;
	// The streamengine is created in the sound thread and must be destroyed from it too
	TBool						iDestroyEngine;
	
	// If an error occurs during output then we need to reset the output settings for things to work
	TBool						iSetSettings;
	
	/** Set when anything but underflow happens */
	TBool						iReOpenStream;
	
	/** Protects agaisnt failed writes */
	RTimer						iProtectionTimer;
	
	TBool						iThreadDied;
#if defined (UIQ3) || defined (S60V3)
	TInt32						iBytesWritten;
	TInt32						iLastWritten;
#endif
	TInt32						iBytesPerSecond;
	CIniFile*					iConfig;
};

void IncreaseVolume()
{
	if(current_audio != NULL)
	{
		if(current_audio->hidden->iVolume < KMaxVolume)
		{
			current_audio->hidden->iVolume = current_audio->hidden->iVolume+1;		
			Configuration->WriteInt("Audio", "Volume", current_audio->hidden->iVolume);
		}
	
	}
}

void DecreaseVolume()
{
	if(current_audio != NULL)
	{
		if(current_audio->hidden->iVolume>0)
		{
			current_audio->hidden->iVolume = current_audio->hidden->iVolume-1;
			Configuration->WriteInt("Audio", "Volume", current_audio->hidden->iVolume);
		}		
	}
}

void CStreamEngine::ThreadDied()
{
	iThreadDied = ETrue;
	iIsOpen = EFalse;
	iLastVolume = KErrNotFound;
}

CStreamEngine *CStreamEngine::NewL()
{
	CStreamEngine *self = new(ELeave) CStreamEngine;
	CleanupStack::PushL( self );
	self->ConstructL();
	CleanupStack::Pop();
	
	return self;
}

CStreamEngine::CStreamEngine():CActive(EPriorityNormal)
{
	RThread audioThread;
	iSndThreadId = audioThread.Id();
	iLastVolume = KErrNotFound;
}

CStreamEngine::~CStreamEngine()
{
	RThread currentThread;
	if(currentThread.Id() == iSndThreadId)
	{
		if( iAudioStreamPlayer )
		{		
			delete iAudioStreamPlayer;
			iAudioStreamPlayer = NULL;
		}
		Cancel();
		iProtectionTimer.Close();
	}
	else
	{
		if(!iThreadDied)
		{
			iDestroyEngine = ETrue;
			TInt ticks = 0;
			while(iDestroyEngine && ticks<32)
			{
				User::After(31249);
				ticks++;
			}
		}
	}
	
	delete iBuffer;
	iBuffer = NULL;
}

void CStreamEngine::OpenL(TInt aSampleRate, TInt aChannels)
{
	TInt sampleRate = TMdaAudioDataSettings::ESampleRate8000Hz;
	switch( aSampleRate )
		{
		case  8000: sampleRate = TMdaAudioDataSettings::ESampleRate8000Hz;  break;
		case 11025: sampleRate = TMdaAudioDataSettings::ESampleRate11025Hz; break;
		case 16000: sampleRate = TMdaAudioDataSettings::ESampleRate16000Hz; break;
		case 22050: sampleRate = TMdaAudioDataSettings::ESampleRate22050Hz; break;
		case 32000: sampleRate = TMdaAudioDataSettings::ESampleRate32000Hz; break;
		case 44100: sampleRate = TMdaAudioDataSettings::ESampleRate44100Hz; break;
		case 48000: sampleRate = TMdaAudioDataSettings::ESampleRate48000Hz; break;
		}

	iBytesPerSecond = (aSampleRate*(aChannels == TMdaAudioDataSettings::EChannelsMono?1:2))/1000;

	iSettings.Query();
	iSettings.iSampleRate = sampleRate;
	iSettings.iChannels = aChannels;
	iSettings.iFlags = 0;
	iSettings.iVolume = 0;
	
	iAudioStreamPlayer = CMdaAudioOutputStream::NewL( *this );
	iAudioStreamPlayer->Open( &iSettings );
	
	iProtectionTimer.CreateLocal();	
	CActiveScheduler::Add(this);
	
	WaitForOpenToComplete();
}

void CStreamEngine::Close()
{
	RThread currentThread;
	if(currentThread.Id() == iSndThreadId)
	{

		if(iAudioStreamPlayer != NULL)
		{
			iAudioStreamPlayer->Stop();
		}
	}
}

void CStreamEngine::DoCancel()
{
	RThread currentThread;
	if(currentThread.Id() == iSndThreadId)
	{
		iProtectionTimer.Cancel();
	}
}

void CStreamEngine::RunL()
{
	if(	iIsStarted)
	{
		CActiveScheduler::Stop();
		iIsStarted=EFalse;
	}
}


#if defined (UIQ3) || defined (S60V3)
void CStreamEngine::WaitForAudio()
{
	TInt32 bytes = iAudioStreamPlayer?iAudioStreamPlayer->GetBytes():0;
	TInt32 diff = (iBytesWritten-bytes);
	
	if(diff > 0)
	{	
		// Bytes per second are actually half kb per second normal calculation ((1000000*diff)/2)/bytes per second
		// But bytes per second are already divided by 1000, and does not count the 16 bit number.. 
		// Diff in second is diff/iBytesPerSecond
		// This is (diff/iBytesPerSecond)*1000000 micro seconds
		
		TUint32 timeout = (125*diff)/iBytesPerSecond;
		
		if(timeout > 10000)
		{
			// If the value is to high that means that the bytes are not updated (not rendering).. so keep the engine happy.. 
			// Since we always need more data to keep rendering the audio
			timeout = 10000;
		}
		
		User::AfterHighRes(timeout);
	}
}
#endif


void CStreamEngine::PlayStreamL()
{
	if(iIsOpen)
	{		
		iAudioStreamPlayer->WriteL( *iBuffer );

		if(iLastVolume != current_audio->hidden->iVolume)
		{
			iLastVolume = current_audio->hidden->iVolume;
			iAudioStreamPlayer->SetVolume((iLastVolume*iAudioStreamPlayer->MaxVolume())/KMaxVolume);
		}
#if defined (UIQ3) || defined (S60V3)
		iLastWritten = iBuffer->Length();
		iBytesWritten+= iLastWritten;;
#endif
		Cancel();
		
		iProtectionTimer.After(iStatus,499999);
		
		SetActive();
		
		iIsStarted=ETrue;
		
		CActiveScheduler::Start();
		
		Cancel();
		
		if(iSetSettings)
		{
			iSetSettings = EFalse;
			iAudioStreamPlayer->Stop();
			TRAPD(err,iAudioStreamPlayer->SetAudioPropertiesL(iSettings.iSampleRate,iSettings.iChannels));
			if(err != KErrNone)
			{				
				iIsOpen =EFalse;
			}
		}
		
		if(iReOpenStream)
		{
			iReOpenStream = EFalse;
			iAudioStreamPlayer->Stop();
			delete iAudioStreamPlayer;
			iAudioStreamPlayer = NULL;
			iAudioStreamPlayer = CMdaAudioOutputStream::NewL( *this );
			iAudioStreamPlayer->Open( &iSettings );
			WaitForOpenToComplete();
			iBytesWritten = 0;
		}
	}
	else
	{
		User::After(15624); // Wait for a tick.. sound is not opened properly
	}
	
	if(iDestroyEngine)
	{
		if(iAudioStreamPlayer != NULL)
		{		
			delete iAudioStreamPlayer;
			iAudioStreamPlayer = NULL;
		}
		iIsOpen = EFalse;
		iDestroyEngine = EFalse;
	}
}

TUint8 *CStreamEngine::Buffer()
{
	if(iState == EStreamStarted || iState == ENextStreamBuf)
		return (TUint8*)iBuffer->Des().Ptr();
	return 
		NULL;
}

CStreamEngine::TState CStreamEngine::State()
{
	return iState;
}



void CStreamEngine::WaitForOpenToComplete()
{
	iIsStarted=ETrue;
	
	CActiveScheduler::Start();
	if (!iIsOpen)
	{
		User::InfoPrint(_L("Audio is not open"));
	}
	else
	{
		iAudioStreamPlayer->SetPriority(EMdaPriorityNormal , EMdaPriorityPreferenceNone); // Set the priority
		TRAPD(err,iAudioStreamPlayer->SetAudioPropertiesL(iSettings.iSampleRate,iSettings.iChannels)); // Set the properties
		
		if(err == KErrNone)
		{
			iState = EStreamStarted;
		}	
	}
}

void CStreamEngine::ConstructL()
{
	iIsStarted=EFalse;
	iBuffer=HBufC8::NewL(gSpec.size);
	iBuffer->Des().SetLength(gSpec.size);
}

// Implement MMdaAudioOutputStreamCallback
void CStreamEngine::MaoscPlayComplete(TInt aError)
{	
	if(	iIsStarted)
	{
		CActiveScheduler::Stop();
		iIsStarted=EFalse;
	}
	
	if(aError != KErrNone)
	{
		
		if(aError != KErrUnderflow && aError != KErrAbort)
		{
			iReOpenStream = ETrue;
		}
		else
		{
			iSetSettings = ETrue;
		}		
	}
}

void CStreamEngine::MaoscBufferCopied(TInt aError, const TDesC8& /*aBuffer*/)
{
	// Check for error
	iState = ENextStreamBuf;
	if(	iIsStarted)
	{
		CActiveScheduler::Stop();
		iIsStarted=EFalse;
	}
	if(aError != KErrNone && aError != KErrAbort)
	{
		if(aError != KErrUnderflow)
		{
			iReOpenStream = ETrue;
		}
		else
		{
			iSetSettings = ETrue;
		}		
	}
	//Tell the SDL client that it can put more buffers :D
}

void CStreamEngine::MaoscOpenComplete( TInt aError )
{
	if( aError )
	{
		//User::Panic(_L("SDL_AUDIO_INIT"), aError);
		User::InfoPrint(_L("SDL AUDIO INIT Failed"));
	}
	else
	{
		iMaxVolume = iAudioStreamPlayer->MaxVolume();

		if(iLastVolume != current_audio->hidden->iVolume)
		{
			iLastVolume = current_audio->hidden->iVolume;
			iAudioStreamPlayer->SetVolume((iLastVolume*iAudioStreamPlayer->MaxVolume())/KMaxVolume);
		}
		iState = EStreamStarted;
		
		iIsOpen = ETrue;
	}
	
	if(	iIsStarted)
	{
		CActiveScheduler::Stop();
		iIsStarted=EFalse;
	}
}

static int Audio_Available()
{
	return 1;
}

static void Audio_DeleteDevice( _THIS )
{
	delete _this->hidden->iStreamEngine;
	_this->hidden->iStreamEngine = NULL;
	delete _this->hidden;
	_this->hidden = NULL;
	delete _this;
}

static SDL_AudioDevice *Audio_CreateDevice( int /*devindex*/ )
{
	
	_THIS;
	_this = new(ELeave) SDL_AudioDevice;
	
	memset(_this, 0, sizeof(SDL_AudioDevice));
	_this->hidden = new(ELeave) SDL_PrivateAudioData;
	memset(_this->hidden, 0, sizeof(SDL_PrivateAudioData));
	
	_this->hidden->iStreamEngine	= NULL; //Has to be allocated in player thread... 
	_this->hidden->iCleanup			= NULL; //Has to be allocated in player thread...  ??
	
	_this->OpenAudio	= EPOC_OpenAudio;
	_this->ThreadInit	= EPOC_ThreadInit;
	_this->WaitAudio	= EPOC_WaitAudio;
	_this->PlayAudio	= EPOC_PlayAudio;
	_this->GetAudioBuf	= EPOC_GetAudioBuf;
	_this->WaitDone		= EPOC_WaitDone;
	_this->CloseAudio	= EPOC_CloseAudio;
	_this->free			= Audio_DeleteDevice;

#if defined (UIQ) || defined (UIQ3)
	_this->hidden->iVolume = KMaxVolume;
#else
	_this->hidden->iVolume = KMaxVolume/2;
#endif
	_this->hidden->iVolume = Configuration->ReadInt("Audio","Volume", _this->hidden->iVolume);
	return _this;
}

AudioBootStrap	EPOC_audiobootstrap = 
{
	"EPOC_media_stream"
	, "Symbian Audio Streaming"
	, Audio_Available
	, Audio_CreateDevice
};

void EPOC_ThreadInit( _THIS )
{
	_this->hidden->iCleanup = CTrapCleanup::New();

	//Install Active Scheduler
	/* _this->hidden->iScheduler = new CActiveScheduler; 
	if(_this->hidden->iScheduler == NULL)
		User::Panic(_L("SDL Audio"), KErrNoMemory);
	CActiveScheduler::Install(_this->hidden->iScheduler);*/ 

	// Now lets increast the priority of this thread
	RThread audioThread; //this is the audiothread
	audioThread.SetPriority(EPriorityMore);

	TRAPD(err, _this->hidden->iStreamEngine = CStreamEngine::NewL());
	if(err)
		User::Panic(_L("SDL Audio"), err);

	TInt channels=0;

	switch( gSpec.channels )
	{
	case 1:	channels = TMdaAudioDataSettings::EChannelsMono; break;
	case 2: channels = TMdaAudioDataSettings::EChannelsStereo; break;
	default:
		User::Panic(_L("SDL AUDIO"), -2);
	}

	switch( gSpec.format & 0xFF )
	{
	case 8:
		User::Panic(_L("SDL_AUDIO"), -1 );
		break;
	case 16:
		gSpec.format = AUDIO_S16;
		
	
	}

	TRAP(err, _this->hidden->iStreamEngine->OpenL( gSpec.freq , channels ));
	if(err)
		User::InfoPrint(_L("SDL Audio Init error"));
	
}

int EPOC_OpenAudio(_THIS, SDL_AudioSpec *aSpec)
{
//Needs to open in the same thread as playing is going to be :|
#if defined (S60) 
	int channels= Configuration->ReadInt("Audio", "Channels", 1); // only used if set
	int samplerate = Configuration->ReadInt("Audio", "Samplerate", -1); // only used if set
	
	if(channels == -1) // Not set yet
	{
		channels = 1;
		if(CEikonEnv::Static()->QueryWinL(_L("SDL"),_L("Use stereo?")))
		{
			channels = 2;
		}
		Configuration->WriteInt("Audio","Channels",channels);
		Configuration->WriteToFile();
	}

	aSpec->channels = channels;
	
	if(aSpec->format == AUDIO_U8 || aSpec->format == AUDIO_S8)
	{
		aSpec->size*=2;
	}

	aSpec->format = AUDIO_S16;

	switch(aSpec->freq)
	{
	// Keep 8000 as is
	case 8000:
		break;
	// Always set other rates to 16K
	default:
		{
		if(samplerate != -1) // Sample override
		{
		aSpec->freq =samplerate; 
		}
		else
		{
		aSpec->freq =16000; // 16Khz is the only audio supported on older platforms.
		}
		}
	}
	SDL_CalculateAudioSpec(aSpec);

#else
	int channels= Configuration->ReadInt("Audio", "Channels", -1); // only used if set
	int samplerate = Configuration->ReadInt("Audio", "Samplerate", -1); // only used if set
	if(channels != -1)
	{
		aSpec->channels = channels;
	}

	if(samplerate != -1)
	{
		aSpec->freq = samplerate;
	}

	if(aSpec->format == AUDIO_U8 || aSpec->format == AUDIO_S8)
	{
		aSpec->size*=2;
	}

	aSpec->format = AUDIO_S16;

	SDL_CalculateAudioSpec(aSpec);
#endif
	gSpec = *aSpec;
	return 0;
}

void EPOC_CloseAudio( _THIS )
{
	if(_this->hidden->iStreamEngine != NULL)
		_this->hidden->iStreamEngine->Close();

	if(_this->hidden->iSoundThreadWatcher != NULL)
	{
		delete _this->hidden->iSoundThreadWatcher;
		_this->hidden->iSoundThreadWatcher = NULL;
	}
}

void EPOC_WaitDone( _THIS )
{
	if(_this->hidden->iStreamEngine != NULL)
	{
		delete 	_this->hidden->iStreamEngine;
		_this->hidden->iStreamEngine = NULL;
	}

	if(_this->hidden->iScheduler != NULL)
	{
		delete 	_this->hidden->iScheduler;
		_this->hidden->iScheduler = NULL;
	}

	if(_this->hidden->iCleanup != NULL)
	{
		delete _this->hidden->iCleanup;
		_this->hidden->iCleanup = NULL;
	}
}

void EPOC_PlayAudio( _THIS )
{	
	TRAPD(err,_this->hidden->iStreamEngine->PlayStreamL());
	int i = 0;
	if(i == 1)
		{
			User::Leave(KErrNotFound);
		}
}

Uint8 *EPOC_GetAudioBuf(_THIS)
{
	return _this->hidden->iStreamEngine->Buffer();
}

void EPOC_WaitAudio( _THIS )
{
#if defined (UIQ3) || defined (S60V3)
	_this->hidden->iStreamEngine->WaitForAudio();
#endif
}

extern "C" void CreateThreadWatcher(int aThreadhandle,_THIS)
{
	if(_this->hidden->iSoundThreadWatcher != NULL)
	{
		delete _this->hidden->iSoundThreadWatcher;
		_this->hidden->iSoundThreadWatcher = NULL;
	}

	_this->hidden->iSoundThreadWatcher = new CSoundThreadWatcher(aThreadhandle, _this);
}

CSoundThreadWatcher::CSoundThreadWatcher(TInt aThreadHandle,_THIS):CActive(CActive::EPriorityStandard),_this(_this)
{
	CActiveScheduler::Add(this);
	
	iStatus = KRequestPending;
	iSndThread.SetHandle(aThreadHandle);
	iSndThread.Logon(iStatus);

	SetActive();
}

CSoundThreadWatcher::~CSoundThreadWatcher()
{
	Cancel();
	iSndThread.Close();
}

void CSoundThreadWatcher::RunL()
{
	if(_this->hidden->iStreamEngine != NULL)
	{
	_this->hidden->iStreamEngine->ThreadDied();
	}
	iSndThread.Kill(0);
	iSndThread.Close();
}

void CSoundThreadWatcher::DoCancel()
{
	iSndThread.LogonCancel(iStatus);
}
