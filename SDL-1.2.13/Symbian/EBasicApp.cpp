#include "EBasicApp.h"
#ifdef UIQ
#include <quartzkeys.h>
#endif
#ifdef UIQ3
#include <devicekeys.h>
#include <coefont.h>
#include <qikviewswitcher.h>
#include <qikcommand.h>
#endif

#include <uikon.hrh>
#include <reent.h>
#include <eikenv.h>
#include <e32keys.h>
#include <eikenv.h>
#include <eikapp.h>
#include <bautils.h>
#include <unistd.h>
#include "sdl.h"
#include "SDL_epocvideo.h"
extern "C"
{
#include "SDL_epocevents_c.h"
#include "sdl_audio.h"
#include "sdlapp.h"
#include "sdl_main.h"
#include "sdl_events_c.h"
}
const TInt KMultitapTimeoutValue = 1500000;

/////////////////////////////////////////////////////////////////////////

bool gViewVisible=false;
_LIT(KLitIniFileName,      "sdl.ini");

const char KMultiTapKeys[8][6]=
{
	"ABC2",
		"DEF3",
		"GHI4",
		"JKL5",
		"MNO6",
		"PQRS7",
		"TUV8",
		"WXYZ9"
};

#ifdef __WINS__
static RHeap* gHeap = NULL;
void*	symbian_malloc	(size_t _size)
{
	void * p = malloc(_size);
	if(_size>64000)
	{
		int i = 0;
	}
	return p;
}

void	symbian_free	(void * p)
{
	if(p != NULL)
	{
		unsigned char* pp = (unsigned char*)p;
		pp--;
		if(pp[0]==0xFF)
			gHeap->Free(pp);
		//else
		//	free(pp);
	}
}

void*	symbian_calloc	(size_t _nmemb, size_t _size) {
	
	void *p = symbian_malloc(_nmemb * _size);	//gpcalloc doesnt clear?
	
	return p;
}

#endif

////////// CSDLApp //////////////////////////////////////////////////////

EXPORT_C CApaDocument* CSDLApp::CreateDocumentL()
{
	CEBasicDoc* doc = NULL;
	doc = new (ELeave) CEBasicDoc(*this);
	return doc;
}

EXPORT_C CSDLApp::CSDLApp()
{
}

EXPORT_C CSDLApp::~CSDLApp()
{
}

// this is a static function calleable as CSDLApp::GetExecutablePath() from the SDL host app
EXPORT_C TFileName CSDLApp::GetExecutablePath()
{
	return TheBasicAppUi->GetExecutablePath();
}

EXPORT_C char* CSDLApp::GetExecutablePathCStr()
{
	return TheBasicAppUi->GetExecutablePathCStr();
}

/**
 * This has a default empty implementation.
 * Is called just before SDL_Main is called to allow init of system vars
 */
EXPORT_C void CSDLApp::PreInitializeAppL()
{
}

/**
 * This has a default empty implementation.
 * This should call SDL Main and this is not located in the SDL main
 */
EXPORT_C void CSDLApp::LaunchAppL(int , char** params)
{
#ifndef __DLL__
	main(1, params);
#endif
}

/**
 * This has a default empty implementation.
 * Is called to get the name of the SDL data folder for the app 
 * The name is appended to either c:\data or c:\shared
 * If a zero length string is returned then application name 
 * will be used for default datapath
 * Max length is 64 chars
 */
EXPORT_C void CSDLApp::GetDataFolder(TDes& /*aDataFolder*/)
	{
	
	}

////////// CEBasicDoc ///////////////////////////////////////////////////
extern "C" void SetTextInputState(TBool aInputOn);
extern "C" void SetJoystickState(TBool aJoystickOn);

#if defined (UIQ) || defined(UIQ3)
CEBasicDoc::CEBasicDoc(CEikApplication& aApp):CQikDocument(aApp)
#elif defined(S60) || defined (S80) || defined (S90) || defined(S60V3)
#if defined (S60) || defined (S60V3)
CEBasicDoc::CEBasicDoc(CEikApplication& aApp):CAknDocument(aApp)
#else
CEBasicDoc::CEBasicDoc(CEikApplication& aApp):CEikDocument(aApp)
#endif // S60
#endif
{
}

CEBasicDoc::~CEBasicDoc()
{
}

void CEBasicDoc::ConstructL()
{
}

CEikAppUi* CEBasicDoc::CreateAppUiL()
{
	return new (ELeave) CEBasicAppUi;
}

void CEBasicDoc::SaveL(MSaveObserver::TSaveType aSaveType)
{
	switch(aSaveType)
	{
	case MSaveObserver::EReleaseRAM:
		{
			//	CEikonEnv::Static()->InfoWinL(_L("CEBasicDoc::SaveL"),_L("Release RAM"));
		}break;
		
	case MSaveObserver::EReleaseDisk:
		{
			//CEikonEnv::Static()->InfoWinL(_L("CEBasicDoc::SaveL"),_L("Release disk"));
		}break;
	default:
		{
			//CEikonEnv::Static()->InfoWinL(_L("CEBasicDoc::SaveL"),_L("Save before turnoff"));
		}break;
	}
}
// 
////////// CEBasicAppUi /////////////////////////////////////////////////

CEBasicAppUi::CEBasicAppUi():iSDLstart(CActive::EPriorityLow)
{
	iLastChar=-1;
#ifdef UIQ
	iCapKey1=-1;
#else
#endif
}

CEBasicAppUi::~CEBasicAppUi()
{
#ifndef UIQ3
	if(iView)
	{
		RemoveFromStack(iView);
		delete iView;
	}
#endif
#ifdef UIQ
	CancelCaptureKeys();

#endif
	if(iMultitapTimer)
	{
	iMultitapTimer->Cancel();
	delete iMultitapTimer;
	}

	iReleaseKeys.Close();

	if (iConfig) {
		delete iConfig;
	}	
}

void CEBasicAppUi::HandleCommandL(TInt aCommand)
{
	switch(aCommand)
	{
	case EEikCmdExit:
		SDL_Quit();
		Exit();
		// Exit here wont work since sdl is running at its own pace..
		break;
	default:
#ifdef UIQ3
		CQikAppUi::HandleCommandL(aCommand);
#endif
		break;
	}
	
}
int EPOC_HandleWsEvent(_THIS,const TWsEvent& aWsEvent);
void EPOC_GenerateKeyEvent(_THIS,int aScanCode,int aIsDown, TBool aShiftOn);

extern "C" SDL_VideoDevice * current_video;

void IncreaseVolume();
void DecreaseVolume();

TInt CEBasicAppUi::StaticMultitapTimeoutL(TAny* aAppUi)
{
	static_cast<CEBasicAppUi*>(aAppUi)->MultitapTimeoutL();
	return KErrNone;
}

void CEBasicAppUi::MultitapTimeoutL()
{
	iTapTimerReleased = ETrue;
	iMultitapTimer->Cancel();
	iReleaseKeys.Append(current_video->hidden->iCurrentChar.GetUpperCase());
	SetTextInputState(EFalse); // Allow numbers
	EPOC_GenerateKeyEvent(current_video,current_video->hidden->iCurrentChar.GetUpperCase(), ETrue, current_video->hidden->iShiftOn);
	SetTextInputState(ETrue);
}

void CEBasicAppUi::ReleaseMultitapKeysL()
{
	while(iReleaseKeys.Count())
	{
		SetTextInputState(EFalse); // Allow numbers
		EPOC_GenerateKeyEvent(current_video, iReleaseKeys[0], EFalse, current_video->hidden->iShiftOn);
		SetTextInputState(ETrue);
		iReleaseKeys.Remove(0);
	}
}

TKeyResponse CEBasicAppUi::HandleMultiTapInput(const TKeyEvent& aKeyEvnt,TEventCode aType)
{
	TBool on=(aType ==EEventKeyDown);
	TKeyEvent keyEvent = aKeyEvnt;
	switch(keyEvent.iScanCode)
	{
	case EStdKeyHash:
	case EStdKeyLeftShift:
	case EStdKeyRightShift:
		if(aType == EEventKeyDown)
		{
			current_video->hidden->iShiftOn = !current_video->hidden->iShiftOn;
			if(current_video->hidden->iShiftOn)
				current_video->hidden->iCurrentChar.UpperCase();
			else
				current_video->hidden->iCurrentChar.LowerCase();
#if defined (UIQ3)
		iView->SetCurrentMultiTapKey(current_video->hidden->iCurrentChar);
#endif
		}
		return EKeyWasConsumed;	
	case EStdKeyNkp0:
	case '0':
		{
			iMultitapTimer->Cancel();
			SetTextInputState(EFalse); // Allow numbers
			EPOC_GenerateKeyEvent(current_video,' ',on, EFalse);
			SetTextInputState(ETrue);		
			return EKeyWasConsumed;
		}
	case EStdKeyNkp1:
	case '1':
		{
			iMultitapTimer->Cancel();
			SetTextInputState(EFalse); // Allow numbers
			EPOC_GenerateKeyEvent(current_video,current_video->hidden->iCurrentChar.GetUpperCase(),on, current_video->hidden->iShiftOn);
			SetTextInputState(ETrue);		
		}		
		return EKeyWasConsumed;		
	case EStdKeyNkp2:
	case EStdKeyNkp3:
	case EStdKeyNkp4:
	case EStdKeyNkp5:
	case EStdKeyNkp6:
	case EStdKeyNkp7:
	case EStdKeyNkp8:
	case EStdKeyNkp9:		
		keyEvent.iScanCode = (keyEvent.iScanCode-EStdKeyNkp2)+'2';
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		{
		// Restart at 0 if new key is pressed
		if(on)
		{
			if(current_video->hidden->iLastKey!= keyEvent.iScanCode)
			{
				current_video->hidden->iLastPos = -1;
				current_video->hidden->iLastKey = keyEvent.iScanCode;
				if(!iTapTimerReleased && !iIntialTap)
				{
					MultitapTimeoutL();
				}
				iTapTimerReleased = EFalse;
			}

			iIntialTap = EFalse;
			current_video->hidden->iLastPos++;
			if(KMultiTapKeys[keyEvent.iScanCode-'2'][current_video->hidden->iLastPos]==0)
				current_video->hidden->iLastPos=0;
			current_video->hidden->iCurrentChar = KMultiTapKeys[keyEvent.iScanCode-'2'][current_video->hidden->iLastPos];
			if(current_video->hidden->iShiftOn)
				current_video->hidden->iCurrentChar.UpperCase();
			else
				current_video->hidden->iCurrentChar.LowerCase();

			iMultitapTimer->Cancel();
			TCallBack callback(StaticMultitapTimeoutL, this);
			iMultitapTimer->Start(KMultitapTimeoutValue, KMultitapTimeoutValue, callback);
		}
#if defined (UIQ3)
		iView->SetCurrentMultiTapKey(current_video->hidden->iCurrentChar);
#endif
		return EKeyWasConsumed;		
		}
	default:
		break;
	}
	iMultitapTimer->Cancel();
	iIntialTap = ETrue;
	return EKeyWasNotConsumed;
}

#if defined (S60) || defined (S60V3) || defined (UIQ3) // Special multitap handling for non touch devices
void CEBasicAppUi::HandleScreenDeviceChangedL()
{
#ifdef UIQ3

#else
	CAknAppUi::HandleScreenDeviceChangedL();
#endif

	if(iView != NULL)
	{
		iView->SetRect(TRect(TPoint(0,0),iEikonEnv->ScreenDevice()->SizeInPixels()));
	}

	if(current_video != NULL)
	{
		EPOC_ReconfigureVideo(current_video);  
	}
}

void CEBasicAppUi::UpdateInputState()
	{
	SetTextInputState(current_video->hidden->iInputMode==EKeyboard);
	SetJoystickState((current_video->hidden->iInputMode==EJoystick));
#if defined (UIQ3) || defined (S60) || defined (S60V3)
	iView->UpdateClipRect();
#if defined (S60) || defined (S60V3)				
	iView->UpdateScreen();
#else
	iView->UpdateVKeyBoard();	
#endif
#endif // UIQ3 , S60 S60V3
	}
void CEBasicAppUi::UpdateScreenOffset(TInt aKeyCode)
	{
	TInt xAdd=0;
	TInt yAdd=0;
	TSize dspSize;
	if(current_video->hidden->iSX0Mode & ESX0Portrait)
		{
		dspSize = current_video->hidden->EPOC_DisplaySize;
		}
	else
		{
		dspSize.iWidth = current_video->hidden->EPOC_DisplaySize.iHeight;
		dspSize.iHeight = current_video->hidden->EPOC_DisplaySize.iWidth;
		}
	switch (aKeyCode)
		{
#if defined (UIQ3)
		case EStdDeviceKeyTwoWayUp:
		case EStdDeviceKeyFourWayUp:
#endif
		case EStdKeyUpArrow:
			yAdd = -(dspSize.iHeight/2); 
			break;
#if defined (UIQ3)
		case EStdDeviceKeyTwoWayDown:
		case EStdDeviceKeyFourWayDown:
#endif			
		case EStdKeyDownArrow:
			yAdd = (dspSize.iHeight/2);
			break;
#if defined (UIQ3)
		case EStdDeviceKeyFourWayLeft:
#endif		
		case EStdKeyLeftArrow:
			xAdd = -(dspSize.iWidth/2);
			break;
#if defined (UIQ3)
		case EStdDeviceKeyFourWayRight:
#endif			
		case EStdKeyRightArrow:
			xAdd = (dspSize.iWidth/2);
			break;
#if defined (UIQ3)
		case EStdDeviceKeyAction:
		case EStdDeviceKeyDone:
#endif
		case EStdKeyDevice3: // Used for center
			{			
				current_video->hidden->iPutOffset = TPoint(current_video->screen->w/2-dspSize.iWidth/2 ,
														   current_video->screen->h/2-dspSize.iHeight/2);				
			}
			break;	
		}

	if(current_video->hidden->iSX0Mode & ESX0Portrait)
		{
		current_video->hidden->iPutOffset+=TSize(xAdd,yAdd);
		}
	else
		{
		current_video->hidden->iPutOffset+=TSize(yAdd,xAdd);
		}
	
	if(current_video->hidden->iPutOffset.iX < 0)
		current_video->hidden->iPutOffset.iX = 0;
	if(current_video->hidden->iPutOffset.iY < 0)
		current_video->hidden->iPutOffset.iY = 0;

	if(current_video->hidden->iPutOffset.iX+
			dspSize.iWidth> current_video->screen->w)
		current_video->hidden->iPutOffset.iX = current_video->screen->w-dspSize.iWidth;

	if(current_video->hidden->iPutOffset.iY+
			dspSize.iHeight> current_video->screen->h)
		current_video->hidden->iPutOffset.iY = current_video->screen->h-dspSize.iHeight;

	RedrawWindowL(current_video);  
	}


void CEBasicAppUi::UpdateAndRedrawScreenL()
	{
	UpdateScaleFactors();		
	iView->ClearScreen();
	ClearBackBuffer(current_video);
	RedrawWindowL(current_video);  
	}

TKeyResponse CEBasicAppUi::HandleControlKeyKeysL(const TKeyEvent& aKeyEvent,TEventCode aType)
{
	if(aType == EEventKeyDown)
	{
		switch(aKeyEvent.iScanCode)
		{
#if defined (S60)
		case EStdKeyUpArrow:
		case EStdKeyDownArrow:
		case EStdKeyLeftArrow:
		case EStdKeyRightArrow:
			if(!(current_video->hidden->iSX0Mode & ESX0Stretched))
			{
				TInt xAdd=0;
				TInt yAdd=0;
				if(aKeyEvent.iScanCode == EStdKeyUpArrow || aKeyEvent.iScanCode == EStdKeyDownArrow)
				{
					yAdd = aKeyEvent.iScanCode == EStdKeyUpArrow?-10:10; 
				}
				
				if(aKeyEvent.iScanCode == EStdKeyLeftArrow || aKeyEvent.iScanCode == EStdKeyRightArrow)
				{
					xAdd = aKeyEvent.iScanCode == EStdKeyLeftArrow?-10:10; 
				}
				
				current_video->hidden->iPutOffset+=TSize(xAdd,yAdd);	
				RedrawWindowL(current_video);  
			}
			break;
		case EStdKeyDevice3: 
			{
				if(!(current_video->hidden->iSX0Mode & ESX0Stretched))
				{
					if( current_video->hidden->iSX0Mode & ESX0Flipped)
					{
						current_video->hidden->iPutOffset = TPoint(-12,-56);
					}
					else // Portrait
					{
						current_video->hidden->iPutOffset = TPoint(-11,0);
					}
					RedrawWindowL(current_video);  
				}
			}
			break;	
#else
#if defined (UIQ3)
		case EStdDeviceKeyTwoWayDown:
		case EStdDeviceKeyFourWayDown:
		case EStdDeviceKeyFourWayLeft:
#endif
#if defined (UIQ3)
		case EStdDeviceKeyAction:
		case EStdDeviceKeyDone:
#endif
		case EStdKeyDevice3: // Used for center
			UpdateScreenOffset(aKeyEvent.iScanCode);
		break;
		case EStdKeyDownArrow:
		case EStdKeyLeftArrow:
			{
			if(!(current_video->hidden->iSX0Mode & ESX0Stretched))
				{
				UpdateScreenOffset(aKeyEvent.iScanCode);
				}			
			else
				DecreaseVolume();
			}
			break;
#if defined (UIQ3)
		case EStdDeviceKeyTwoWayUp:
		case EStdDeviceKeyFourWayUp:
		case EStdDeviceKeyFourWayRight:
#endif
		case EStdKeyUpArrow:
		case EStdKeyRightArrow:
			{	
			if(!(current_video->hidden->iSX0Mode & ESX0Stretched))
				{
				UpdateScreenOffset(aKeyEvent.iScanCode);
				}
			else
				IncreaseVolume();
			}
			break;
#endif

		case 'k':
		case 'K':
		case EStdKeyNkp7:
		case '7':
			{
			if(current_video->hidden->iInputMode!=EKeyboard)
				{
				current_video->hidden->iInputMode=EKeyboard;
				iIntialTap = ETrue;
				current_video->hidden->iInputModeTimer=0;
#if defined (UIQ3)
				current_video->hidden->iVirtualKeyBoardActive = ETrue;				
#endif
				UpdateInputState();
				}
			}break;
		case 'C':
		case 'c':
		case EStdKeyNkp8:
		case '8':
			{
			if(current_video->hidden->iInputMode!=ECursorKeys)
				{
				current_video->hidden->iInputMode=ECursorKeys;
				current_video->hidden->iInputModeTimer=80;
#if defined (UIQ3)
				current_video->hidden->iVirtualKeyBoardActive = EFalse;
#else
				iView->iStatusChar = _L("C");
#endif			
				UpdateInputState();
				}
			}break;
		case 'J':
		case 'j':
		case EStdKeyNkp9:
		case '9':
			{
			if(current_video->hidden->iInputMode!=EJoystick)
				{
				current_video->hidden->iInputMode=EJoystick;
				current_video->hidden->iInputModeTimer=80;		
#if defined (UIQ3)
				current_video->hidden->iVirtualKeyBoardActive = EFalse;
#else
				iView->iStatusChar = _L("J");
#endif			
				UpdateInputState();					
				}
			}break;		
		case 'M':
		case 'm':
		case '0':
		case EStdKeyNkp0:
			if(current_video->hidden->iInputMode!=EMouseMode)
				{
				current_video->hidden->iInputMode=EMouseMode;
				current_video->hidden->iInputModeTimer=80;		
#if defined (UIQ3)
				current_video->hidden->iVirtualKeyBoardActive = EFalse;
#else
				iView->iStatusChar = _L("M");
#endif
				UpdateInputState();					
				}			
			break;
		case 'N':
		case 'n':
		case EStdKeyNkp1:
		case '1':
		case EStdKeyLeftFunc:
		case EStdKeyRightFunc:
			current_video->hidden->iFNModeOn = !current_video->hidden->iFNModeOn;
#if defined (UIQ3)
#else
			current_video->hidden->iInputModeTimer=80;
			if(current_video->hidden->iFNModeOn)
				{
				iView->iStatusChar = _L("FN");
				}
			else
				{
				switch(current_video->hidden->iInputMode)
					{
					case EJoystick:
						iView->iStatusChar = _L("K");
						break;
					case ECursorKeys:
						iView->iStatusChar = _L("J");				
						break;
					case EKeyboard:									
						iView->iStatusChar = _L("C");
						break;
					case EMouseMode:									
						iView->iStatusChar = _L("M");
						break;
					}
				}
#endif			
			RedrawWindowL(current_video);
			break;
		case 'I':
		case 'i':							
			{
				switch(current_video->hidden->iInputMode)
				{
				case EJoystick:
					current_video->hidden->iInputMode=EKeyboard;
					iIntialTap = ETrue;
					current_video->hidden->iInputModeTimer=0; 
#if defined (UIQ3)
					current_video->hidden->iVirtualKeyBoardActive = ETrue;	
#else
					iView->iStatusChar = _L("K");
#endif
					break;
				case ECursorKeys:
					current_video->hidden->iInputMode=EJoystick;
					current_video->hidden->iInputModeTimer=80;
#if defined (UIQ3)
					current_video->hidden->iVirtualKeyBoardActive = EFalse;	
#else
					iView->iStatusChar = _L("J");
#endif

					break;
				case EKeyboard:
					current_video->hidden->iInputMode = ECursorKeys;
					current_video->hidden->iInputModeTimer=80;
#if defined (UIQ3)
					current_video->hidden->iVirtualKeyBoardActive = EFalse;
#else
					iView->iStatusChar = _L("C");
#endif					
					break;
				default:
					break;
				}
				
				UpdateInputState();
			}
			break;
		case 'P':
		case 'p':
		case EStdKeyNkp2:
		case '2':
			{
			current_video->hidden->iSX0Mode=current_video->hidden->iSX0Mode^ESX0Portrait;
			UpdateScaleFactors();
			ClearBackBuffer(current_video);
			UpdateScreenOffset(KErrNotFound);  						
			}break;	
		case 'F':
		case 'f':
		case EStdKeyNkp3:
		case '3':
			{
				if(!(current_video->hidden->iSX0Mode & ESX0Portrait))
				{
					current_video->hidden->iSX0Mode=current_video->hidden->iSX0Mode^ESX0Flipped;						
					UpdateAndRedrawScreenL();
				}
			}
			break;
		case 's':
		case 'S':
		case EStdKeyNkp4:
		case '4':
			{
				current_video->hidden->iSX0Mode=current_video->hidden->iSX0Mode^ESX0Stretched;
				UpdateAndRedrawScreenL();
			}break;
		case 'r':
		case 'R':
		case EStdKeyNkp5:
		case '5':
			{
			current_video->hidden->iSX0Mode=current_video->hidden->iSX0Mode^ESX0DontInterpolate;
			UpdateAndRedrawScreenL();
			}break;
		case 'a':
		case 'A':
		case EStdKeyNkp6:
		case '6':
			{
			current_video->hidden->iSX0Mode=current_video->hidden->iSX0Mode^ESX0KeepAspect;
			UpdateAndRedrawScreenL();
			}break;	
		default:
			{
			
			}break;
			
		}
	}

	return EKeyWasConsumed;
}
#endif


TKeyResponse CEBasicAppUi::HandleKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType)
{
	if(current_video != NULL)
	{
		if(aType == EEventKeyDown || aType == EEventKeyUp)
		{
	/*	if(aType == EEventKeyDown)
		{
		//TBuf<128> msg;
		//msg.Format(_L("Key 0x%x,0x%x"),aKeyEvent.iScanCode,aKeyEvent.iCode);
		//iEikonEnv->InfoMsg(msg);
		}*/
			TBool on=aType ==EEventKeyDown;
			TInt switchCode = aKeyEvent.iScanCode;
#if defined (S60) || defined (S60V3) || defined (UIQ3)// Special multitap handling for S60 and UIQ3
			if(current_video->hidden->iInputMode == EKeyboard && !current_video->hidden->iControlKeyDown)
			{			
				TKeyResponse response = HandleMultiTapInput(aKeyEvent, aType);
				if(response == EKeyWasConsumed)
				{
#if defined (S60) || defined (S60V3)
					iView->UpdateScreen();
#endif
					iWasControlEvent = ETrue;
					return response;
				}
			}
			else if (current_video->hidden->iControlKeyDown)
			{
				iWasControlEvent = ETrue;
				
				return HandleControlKeyKeysL(aKeyEvent, aType);
			}
#endif			
			if(aKeyEvent.iScanCode == current_video->hidden->iLeftButtonCode2 || aKeyEvent.iScanCode == current_video->hidden->iLeftButtonCode1)
				{
				switchCode = KLeftButtonCodeCase;
				}
			
			if(aKeyEvent.iScanCode == current_video->hidden->iRightButtonCode)
				{
				switchCode = KRightButtonCodeCase;
				}
			
			switch(switchCode)
			{

#ifdef S60V3			  
			  case EStdKeyRightFunc:
				  {
				  if(on)
					  {
						current_video->hidden->iKeyboardModifier = !current_video->hidden->iKeyboardModifier;						
						current_video->hidden->iInputModeTimer=80;	
						iView->iStatusChar = _L("F");						
					  }
				  }break;
#endif

#if defined (UIQ)
			case EStdQuartzKeyFourWayUp:
					current_video->hidden->iJoystickStatus[EJoyRIGHT]=on;
				break;
#endif
#if defined (UIQ3)
			case EStdDeviceKeyFourWayUp:
			case EStdKeyNkp2:
#endif
			case EStdKeyUpArrow:
#if defined(S60) || defined (S80) || defined (S90) || defined (S60V3) || defined (UIQ3)
#if defined (S60) || defined (S60V3) || defined (UIQ3)
				if(current_video->hidden->iInputMode == EMouseMode)													
					{
					if(!(current_video->hidden->iSX0Mode & ESX0Portrait))
						{
						if(current_video->hidden->iSX0Mode & ESX0Flipped)
							{
							current_video->hidden->_km.x_vel = on?-1:0; 
							}
						else
							{
							current_video->hidden->_km.x_vel = on?1:0; 
							}

						current_video->hidden->_km.x_down_count = on;
						}
					else
						{
						current_video->hidden->_km.y_down_count = on;
						current_video->hidden->_km.y_vel = on?-1:0;
						}																		
					}
				else
#endif
				if(current_video->hidden->iInputMode == EJoystick)
#if defined (S60) || defined (S60V3) || defined (UIQ3)
					if(!(current_video->hidden->iSX0Mode & ESX0Portrait))
					{
						if(current_video->hidden->iSX0Mode & ESX0Flipped)
						{
							current_video->hidden->iJoystickStatus[EJoyLEFT]=on;
						}
						else
							current_video->hidden->iJoystickStatus[EJoyRIGHT]=on;
					}
					else
#endif
#endif
						current_video->hidden->iJoystickStatus[EJoyUP]=on;
					break;
#if defined (UIQ)
			case EStdQuartzKeyFourWayDown:
#endif
#if defined (UIQ3)
			case EStdDeviceKeyFourWayDown:
			case EStdKeyNkp8:
#endif
			case EStdKeyDownArrow:
#if defined(S60) || defined (S80) || defined (S90) || defined (S60V3) || defined (UIQ3)
#if defined (S60) || defined (S60V3) || defined (UIQ3)
				if(current_video->hidden->iInputMode == EMouseMode)												
					{
					if(!(current_video->hidden->iSX0Mode & ESX0Portrait))
						{
						if(current_video->hidden->iSX0Mode & ESX0Flipped)
							{
							current_video->hidden->_km.x_vel = on?1:0; 
							}
						else
							{
							current_video->hidden->_km.x_vel = on?-1:0; 
							}

						current_video->hidden->_km.x_down_count = on;
						}
					else
						{
						current_video->hidden->_km.y_down_count = on;
						current_video->hidden->_km.y_vel = on?1:0;
						}																
					}
				else
#endif
				if(current_video->hidden->iInputMode == EJoystick)
#if defined (S60) || defined (S60V3) || defined (UIQ3)
					if(!(current_video->hidden->iSX0Mode & ESX0Portrait))
					{
						if(current_video->hidden->iSX0Mode & ESX0Flipped)
						{
							current_video->hidden->iJoystickStatus[EJoyRIGHT]=on;
						}
						else
							current_video->hidden->iJoystickStatus[EJoyLEFT]=on;
					}
					else
#endif
#endif
						current_video->hidden->iJoystickStatus[EJoyDOWN]=on;
					break;
#if defined (UIQ)
			case EStdQuartzKeyFourWayLeft:
#endif
#if defined (UIQ3)
			case EStdDeviceKeyFourWayLeft:
			case EStdKeyNkp4:
#endif
				
			case EStdKeyLeftArrow:
#if defined(S60) || defined (S60V3) || defined (UIQ3)
				if(current_video->hidden->iInputMode == EKeyboard)
				{
					if( current_video->hidden->iCurrentChar>32 && on)
						current_video->hidden->iCurrentChar-=1;
				}
				else if(current_video->hidden->iInputMode == EMouseMode)									
					{
					if(!(current_video->hidden->iSX0Mode & ESX0Portrait))
					{
						if(current_video->hidden->iSX0Mode & ESX0Flipped)
						{
						current_video->hidden->_km.y_vel = on?1:0; 
						}
						else
						{
						current_video->hidden->_km.y_vel = on?-1:0; 
						}
						
						current_video->hidden->_km.y_down_count = on;
					}
					else
					{
						current_video->hidden->_km.x_down_count = on;
						current_video->hidden->_km.x_vel = on?-1:0;
					}									
					}
				else if(current_video->hidden->iInputMode == EJoystick)
					if(!(current_video->hidden->iSX0Mode & ESX0Portrait))
					{
						if(current_video->hidden->iSX0Mode & ESX0Flipped)
						{
							current_video->hidden->iJoystickStatus[EJoyDOWN]=on;
						}
						else
							current_video->hidden->iJoystickStatus[EJoyUP]=on;
					}
					else	
#elif defined(S80)
						if(current_video->hidden->iInputMode == EJoystick) // Cursor or joystick
#endif
							current_video->hidden->iJoystickStatus[EJoyLEFT]=on;
						
						break;
#if defined (UIQ)
			case EStdQuartzKeyFourWayRight:
#endif
#if defined (UIQ3)
			case EStdDeviceKeyFourWayRight:
			case EStdKeyNkp6:
#endif
				
			case EStdKeyRightArrow:
#if defined(S60)  || defined (S60V3) || defined (UIQ3)
				if(current_video->hidden->iInputMode == EKeyboard)
				{
					if(current_video->hidden->iCurrentChar<125 && on)
						current_video->hidden->iCurrentChar+=1;
				}
				else if(current_video->hidden->iInputMode == EMouseMode)
					{
					if(!(current_video->hidden->iSX0Mode & ESX0Portrait))
					{
						if(current_video->hidden->iSX0Mode & ESX0Flipped)
						{
						current_video->hidden->_km.y_vel = on?-1:0; 
						}
						else
						{
						current_video->hidden->_km.y_vel = on?1:0; 
						}
						
						current_video->hidden->_km.y_down_count = on;
					}
					else
					{
						current_video->hidden->_km.x_down_count = on;
						current_video->hidden->_km.x_vel = on?1:0;
					}					
					}
				else if(current_video->hidden->iInputMode == EJoystick)					
					if(!(current_video->hidden->iSX0Mode & ESX0Portrait))
					{
						if(current_video->hidden->iSX0Mode & ESX0Flipped)
						{
							current_video->hidden->iJoystickStatus[EJoyUP]=on;
						}
						else
							current_video->hidden->iJoystickStatus[EJoyDOWN]=on;
					}
				else
#elif defined(S80) 
						if(current_video->hidden->iInputMode == EJoystick) // Cursor or joystick
#endif
							current_video->hidden->iJoystickStatus[EJoyRIGHT]=on;
						
				break;												
#ifdef S80
			case EStdKeyDevice1: // Joystick button 2
				if(current_video->hidden->iInputMode == EJoystick) // Cursor or joystick
				{
					current_video->hidden->iJoystickStatus[EJoyBUT2]=on;
				}
				break;
			case EStdKeyDevice2: // Joystick button 3 
				if(current_video->hidden->iInputMode == EJoystick) // Cursor or joystick
				{
					current_video->hidden->iJoystickStatus[EJoyBUT3]=on;
				}
				else
				{
					if(aType == EEventKeyUp)
					{
						current_video->hidden->iSX0Mode = current_video->hidden->iSX0Mode^ESX0Stretched;
						current_video->hidden->EPOC_ScreenOffset = !(current_video->hidden->iSX0Mode & ESX0Stretched)?(current_video->hidden->EPOC_ScreenSize.iWidth - current_video->screen->w) / 2:(current_video->hidden->EPOC_ScreenSize.iWidth-current_video->screen->w*current_video->hidden->iXScale)/2 ;
						RedrawWindowL(current_video);  
					}
					
				}
				break;
			case EStdKeyDevice3:
				if(aType == EEventKeyDown)
				{
					switch(current_video->hidden->iInputMode)
					{
					case ECursorKeys:
						current_video->hidden->iInputMode=EJoystick;
						iEikonEnv->InfoMsg(_L("JOY"));
						break;
					case EJoystick:
						current_video->hidden->iInputMode = ECursorKeys;
						iEikonEnv->InfoMsg(_L("CURSOR"));
						break;
					default:
						{}break;
					}	
					
					SetJoystickState((current_video->hidden->iInputMode==EJoystick));
				}
				break;
			case EStdKeyDevice0: // Joystick button 1
#endif
#ifdef S90
			case EStdKeyDevice5: // Input mode
				if(current_video->hidden->iControlKeyDown)
				{
					if(aType == EEventKeyUp)
					{						
						switch(current_video->hidden->iInputMode)
						{
						case EJoystick:
							current_video->hidden->iInputMode=ECursorKeys;
							iEikonEnv->InfoMsg(_L("CURSOR"));
							break;
						case ECursorKeys:
							current_video->hidden->iInputMode=EJoystick;
							iEikonEnv->InfoMsg(_L("JOY"));
							break;			
						}
						SetJoystickState((current_video->hidden->iInputMode==EJoystick));
					}
				}else if(current_video->hidden->iInputMode==EJoystick)
				{
					current_video->hidden->iJoystickStatus[EJoyBUT3]=on;
				}
				break;
#ifdef __WINS__
			case EStdKeyForwardSlash:
#endif
			case EStdKeyDevice7:
				current_video->hidden->iControlKeyDown = on;
				break;				
			case EStdKeyDevice4: // Toggle screen mode					
				if(current_video->hidden->iControlKeyDown) // Button 1
				{
					if(aType == EEventKeyUp)
					{
						current_video->hidden->iSX0Mode = current_video->hidden->iSX0Mode^ESX0Stretched;
						current_video->hidden->EPOC_ScreenOffset = !(current_video->hidden->iSX0Mode) & ESX0Stretched?(current_video->hidden->EPOC_ScreenSize.iWidth - current_video->screen->w) / 2:(current_video->hidden->EPOC_ScreenSize.iWidth-current_video->screen->w*current_video->hidden->iXScale)/2 ;
						RedrawWindowL(current_video);  
					}
				}
				else 												
#endif			
#if defined (UIQ)
			case EStdQuartzKeyConfirm:				
#endif
#if defined (UIQ3) || defined (S60) || defined (S60V3)	
			case KRightButtonCodeCase: // Joystick button 2 or keyboard space
				if(current_video->hidden->iInputMode == EJoystick)
					current_video->hidden->iJoystickStatus[EJoyBUT2]=on;
				else if(current_video->hidden->iInputMode == EMouseMode)
					SDL_PrivateMouseButton(on?SDL_PRESSED:SDL_RELEASED, SDL_BUTTON_RIGHT, 0, 0);					
				break;				
			case KLeftButtonCodeCase:// Joystick button 2 or keyboard return					
#endif
				if(current_video->hidden->iInputMode==EJoystick)
					current_video->hidden->iJoystickStatus[EJoyBUT1]=on;
				else if(current_video->hidden->iInputMode == EMouseMode)
					SDL_PrivateMouseButton(on?SDL_PRESSED:SDL_RELEASED, SDL_BUTTON_LEFT, 0, 0);					
				break;
		}		
	  }
#if defined (S60) || defined (S60V3) || defined (UIQ3)
	  else if(aType == EEventKey)
	  {		  
		  
		  switch(aKeyEvent.iScanCode)
		  {
#ifdef UIQ3		  
		  case EStdDeviceKeyYes:
#else
		  case EStdKeyYes:
#endif
			  {
				  current_video->hidden->iControlKeyDown = !current_video->hidden->iControlKeyDown;
				  RedrawWindowL(current_video);  
			  }
			  break;
		  }					  
	  }
#endif
	}
	
	return EKeyWasNotConsumed;
}


void CEBasicAppUi::HandleWsEventL(const TWsEvent& aEvent, CCoeControl* aDestination)
{
	TInt type= aEvent.Type();
#ifdef UIQ3
	CQikAppUi::HandleWsEventL(aEvent,aDestination);
#else
	CEikAppUi::HandleWsEventL(aEvent,aDestination);
#endif
	if(current_video!=NULL && (type == EEventPointer || type ==EEventKeyDown || type == EEventKeyUp))
	{
		if(!iWasControlEvent) // map control event mapping to nothing
		{
			EPOC_HandleWsEvent(current_video,aEvent);
		}
		iWasControlEvent = EFalse;
	}
	else
	{
		if(current_video!=NULL && type ==EEventFocusGained)
		{
			EPOC_HandleWsEvent(current_video,aEvent);
		}
	}
	
}
TInt CEBasicAppUi::StaticSDLStartL(TAny* aAppUi)
{
	static_cast<CEBasicAppUi*>(aAppUi)->SDLStartL();
	return KErrNone;
}

void CEBasicAppUi::SDLStartL()
{
#if defined (S60) || defined (S60V3)
	SetKeyBlockMode(ENoKeyBlock);
#endif
#if defined (UIQ)
	TBuf8<256> path;
	TParsePtrC parser(Application()->AppFullName());
	iEikonEnv->FsSession().SetSessionPath(parser.DriveAndPath());
	path.Copy(parser.DriveAndPath());
	path.ZeroTerminate();
	chdir((char*)path.PtrZ());
#endif
#if defined (UIQ) || defined (S80) || defined (S90) || defined (S60)
	iEikonEnv->FsSession().Share(RSessionBase::EAutoAttach);
#else
	iEikonEnv->FsSession().ShareAuto();
#endif
	
	/**
	 * This has a default empty implementation.
	 * Is called just before SDL_Main is called to allow init of system vars
	 */
	if(Application() != NULL)
	{
		static_cast<CSDLApp*>(Application())->PreInitializeAppL();
	}
	TRAPD(exiterr, static_cast<CSDLApp*>(Application())->LaunchAppL(1, iExecutableParams));	
#if defined (UIQ) || defined (S80) || defined (S90) || defined (S60)
	iEikonEnv->FsSession().Share(RSessionBase::EExplicitAttach);
#endif
	SDL_Quit();
	CloseSTDLIB();
	Exit();
}

void CEBasicAppUi::symbian_at_exit()
{
	static_cast<CEBasicAppUi*>(CEikonEnv::Static()->EikAppUi())->DoExit();
}

void CEBasicAppUi::SetExecutablePathL()
{
#ifdef __WINS__
#if defined UIQ3 || defined S60V3
#ifdef UIQ3
	iExecutablePath.Append(_L("C:\\Shared\\"));
#else
	iExecutablePath.Append(_L("C:\\Data\\"));
#endif	
	TBuf<64> dataFolder = KNullDesC();
	static_cast<CSDLApp*>(Application())->GetDataFolder(dataFolder);
	if(dataFolder.Length() == 0)
		{
		TParse parser;		
		parser.Set(Application()->AppFullName(),NULL,NULL);
		iExecutablePath.Append(parser.Name());
		}
	else
		{
		iExecutablePath.Append(dataFolder);
		}
	
	iExecutablePath.Append(_L("\\"));
	
	if(iExecutablePath.Length()>0 && iExecutablePath[0]=='\\')
	{
		iExecutablePath.Insert(0,_L("C:"));
	}
#elif defined (UIQ)
	TParse parser;
	parser.Set(Application()->AppFullName(),NULL,NULL);
	iExecutablePath.Append(_L("C:\\Documents\\"));
	iExecutablePath.Append(parser.Name());
	iExecutablePath.Append(_L("\\"));
	iEikonEnv->FsSession().SetSessionPath(Application()->AppFullName());
#else // S80,S90,S60
	RThread thread;
	iExecutablePath.Append(_L("C:\\Documents\\"));
	iExecutablePath.Append(thread.Name());
	iExecutablePath.Append(_L("\\"));
#endif
#else // Target build
	TFileName fname;
	RProcess process;
#if defined (UIQ)  // only UIQ uses APP, process.FileName() refers to Z:\System\Programs\AppRun.exe
	fname = Application()->AppFullName();
#elif defined(UIQ3)	|| defined (S60V3)
#ifdef UIQ3
	fname = _L("C:\\Shared\\");
#else
	fname = _L("C:\\Data\\");
#endif	
	TBuf<64> dataFolder = KNullDesC();
	static_cast<CSDLApp*>(Application())->GetDataFolder(dataFolder);
	if(dataFolder.Length() == 0)
		{	
		TParse fparser;
		fparser.Set(process.FileName(),NULL,NULL);
		fname.Append(fparser.Name());
		}
	else
		{
		fname.Append(dataFolder);
		}
	fname.Append(_L("\\"));
#else // only for Sxx .EXEs that get launched by an .APP
	fname = process.FileName();
	TParsePtr fparser(fname);
	process.Rename(fparser.Name());
#endif
	TParsePtr parser(fname);
	iExecutablePath.Append(parser.DriveAndPath()); // includes trailing "\\"
#endif // END WINSCW and TARGET
	// Check that drive is present
	// create easy C string for later
	TBuf8<KMaxFileName> executablePath8;
	executablePath8.Copy(iExecutablePath);
	strcpy(iExecutablePathCStr, (char*)executablePath8.PtrZ());
	iExecutableParams[0] =&(iExecutablePathCStr[0]);
}

void CEBasicAppUi::ConstructL()
{
#if defined (EPOC_AS_APP)
#ifdef UIQ3
	//	SetAutoExitOnAppSwitch(EFalse);
#endif
	BaseConstructL();	
#else //!EPOC_AS_APP
	BaseConstructL(ENoAppResourceFile);	
#endif //EPOC_AS_APP
	
#if defined (UIQ) || defined (UIQ3)
#ifdef UIQ
	iView = new (ELeave) CEBasicView;
#else // UIQ3
	iView = new (ELeave) CEBasicView(*this);;
#endif
	TRect rect= ClientRect();
	iView->ConstructL(rect);
	
	iView->SetAppUi(Application()->AppDllUid());
#ifdef UIQ3
	AddViewL(*iView);
#else
	RegisterViewL(*iView);
	SetDefaultViewL(*iView);
	AddToStackL(*iView,iView);
	CaptureKeysL();
#endif
#else //!UIQ variant
	iView = new (ELeave) CEBasicView;
	iView->ConstructL();
	AddToStackL(iView);
#endif
	
#if defined (S60) || defined (S60V3)
	SetKeyBlockMode(ENoKeyBlock);
#endif
	SetExecutablePathL();
	
	TRAPD(err,BaflUtils::EnsurePathExistsL(iEikonEnv->FsSession(),iExecutablePath));
	
	TFileName iniFilePath = iExecutablePath;
	iniFilePath.Append(KLitIniFileName); 
	
	iConfig = new CIniFile(iniFilePath);
		
	iMultitapTimer = CPeriodic::NewL(CActive::EPriorityStandard);
	TCallBack start(StaticSDLStartL,this);
	iSDLstart.Set(start);
	iSDLstart.CallBack();
	atexit(&symbian_at_exit);
}

void CEBasicAppUi::HandleForegroundEventL(TBool aForeground)
{
	iView->iForeground=aForeground;
#if defined (S60) || defined (S60V3)
	CAknAppUi::HandleForegroundEventL(aForeground);
	
	if(aForeground)
	{
		SetKeyBlockMode(ENoKeyBlock);
		iEikonEnv->VirtualCursor().SetCursorStateL( TEikVirtualCursor::EOn,
		                                                *iEikonEnv);
		iEikonEnv->WsSession().SetPointerCursorMode( EPointerCursorNormal );
	}
	else
	{
		iEikonEnv->VirtualCursor().SetCursorStateL( TEikVirtualCursor::EOff,
			                                                *iEikonEnv);
		iEikonEnv->WsSession().SetPointerCursorMode( EPointerCursorNone );
	}
#elif defined(UIQ)
	CQikAppUi::HandleForegroundEventL(aForeground);
	if(aForeground)
	{
		CaptureKeysL();
	}
	else
	{
		CancelCaptureKeys();
	}
#elif defined (UIQ3)
	CQikAppUi::HandleForegroundEventL(aForeground);
#else
	CEikAppUi::HandleForegroundEventL(aForeground);
#endif

#if !defined(UIQ)	
	if(!aForeground)
	{
		iView->AbortNow(RDirectScreenAccess::ETerminateCancel);
		gViewVisible=false;
	}
	else
	{
		iView->Restart(RDirectScreenAccess::ETerminateCancel);
		gViewVisible=true;
	}
#endif

	SDL_PauseAudio(!aForeground);
}


void CEBasicAppUi::DrawView()
{
	iView->DrawNow();
}

void CEBasicAppUi::DoExit()
{
	CloseSTDLIB();
	Exit();
}

void CEBasicAppUi::SetConfig()
{
	SDL_PrivateVideoData* priv = current_video->hidden;
	iConfig->WriteInt("Video","InputMode",priv->iInputMode);
	iConfig->WriteInt("Video","LeftButtonCode1",priv->iLeftButtonCode1);
	iConfig->WriteInt("Video","LeftButtonCode2",priv->iLeftButtonCode2);
	iConfig->WriteInt("Video","RightButtonCode",priv->iRightButtonCode);
#if defined (S80) || defined(S90) || defined (UIQ3)	|| defined (UIQ)
	iConfig->WriteInt("Video","ScreenMode", priv->iSX0Mode);
#else // S60 & S60V3
	iConfig->WriteInt("Video","ScreenMode",priv->iSX0Mode);
	iConfig->WriteInt("Video","OffsetX",priv->iPutOffset.iX);
	iConfig->WriteInt("Video","OffsetY",priv->iPutOffset.iY);
	iConfig->WriteInt("Video","ActivateButtonSize",priv->iActivateButtonSize);
#endif
}

void CEBasicAppUi::GetConfig(SDL_VideoDevice* aDevice)
{
	SDL_PrivateVideoData* priv = aDevice->hidden;
	TBool fileExist = iConfig->ReadFromFile();
	priv->iInputMode =(TInputModes) iConfig->ReadInt("Video", "InputMode", ECursorKeys);
	priv->iLeftButtonCode1 =(TInputModes) iConfig->ReadInt("Video", "LeftButtonCode1", KDefaultLeftButtonCode1);
	priv->iLeftButtonCode2 =(TInputModes) iConfig->ReadInt("Video", "LeftButtonCode2", KDefaultLeftButtonCode2);
	priv->iRightButtonCode =(TInputModes) iConfig->ReadInt("Video", "RightButtonCode", KDefaultRightButtonCode);
#if defined (UIQ3)
	priv->iSX0Mode = iConfig->ReadInt("Video","ScreenMode",ESX0Stretched);
#elif defined (S80) || defined(S90)|| defined (UIQ)
	priv->iSX0Mode = iConfig->ReadInt("Video","ScreenMode",0);
#else // (S60) (S60V3)
	if(fileExist)
		{
		priv->iSX0Mode = iConfig->ReadInt("Video","ScreenMode", ESX0Stretched|ESX0Flipped);
		}
	else
		{
		priv->iSX0Mode = priv->EPOC_ScreenSize.iWidth > priv->EPOC_ScreenSize.iHeight?ESX0Stretched|ESX0Flipped|ESX0Portrait:ESX0Stretched|ESX0Flipped;
		}
	
	priv->iPutOffset.iX = iConfig->ReadInt("Video","OffsetX",0);
	priv->iPutOffset.iY = iConfig->ReadInt("Video","OffsetY",0);
	priv->iShiftOn = ETrue;
	priv->iActivateButtonSize = iConfig->ReadInt("Video","ActivateButtonSize", KDefaultActivateButtonSize);
	SetKeyBlockMode(ENoKeyBlock); // Just as a precaution
#endif // S60|| S60V3
}

#if defined (S60) || defined (S60V3)
void CEBasicAppUi::SetKeyBlockMode(TAknKeyBlockMode aMode)
{
	CAknAppUi::SetKeyBlockMode(aMode);
}
#endif

#if defined (UIQ) || defined(UIQ3)

#ifdef UIQ
void CEBasicAppUi::CaptureKeysL()
{
	if(iCapKey1==-1) // not already captured
	{
		iCapKey1= iEikonEnv->RootWin().CaptureKey(EKeyApplication0,0,0,1);
		iCapKey2= iEikonEnv->RootWin().CaptureKey(EKeyApplication1,0,0,1);
		iCapKey3= iEikonEnv->RootWin().CaptureKey(EKeyDevice0,0,0,1);
		iCapKey4= iEikonEnv->RootWin().CaptureKey(EKeyDeviceD,0,0,0);
		iCapKey5= iEikonEnv->RootWin().CaptureKey(EKeyDeviceE,0,0,0);
		
		iCapKey1b = iEikonEnv->RootWin().CaptureKeyUpAndDowns(EStdKeyApplication0,0,0,1);
		iCapKey2b = iEikonEnv->RootWin().CaptureKeyUpAndDowns(EStdKeyApplication1,0,0,1);
		iCapKey3b = iEikonEnv->RootWin().CaptureKeyUpAndDowns(EStdKeyDevice0,0,0,1);
		iCapKey4b = iEikonEnv->RootWin().CaptureKeyUpAndDowns(EStdKeyDeviceD,0,0,0);
		iCapKey5b = iEikonEnv->RootWin().CaptureKeyUpAndDowns(EStdKeyDeviceE,0,0,0);
	}
}
/**
*Cancels any captured Keys
*/
void CEBasicAppUi::CancelCaptureKeys()
{
	if(	iCapKey1 !=-1)
	{		
		iEikonEnv->RootWin().CancelCaptureKey(iCapKey1);
		iEikonEnv->RootWin().CancelCaptureKey(iCapKey2);
		iEikonEnv->RootWin().CancelCaptureKey(iCapKey3);
		iEikonEnv->RootWin().CancelCaptureKey(iCapKey4);
		iEikonEnv->RootWin().CancelCaptureKey(iCapKey5);
		
		iEikonEnv->RootWin().CancelCaptureKeyUpAndDowns(iCapKey1b);
		iEikonEnv->RootWin().CancelCaptureKeyUpAndDowns(iCapKey2b);
		iEikonEnv->RootWin().CancelCaptureKeyUpAndDowns(iCapKey3b);
		iEikonEnv->RootWin().CancelCaptureKeyUpAndDowns(iCapKey4b);
		iEikonEnv->RootWin().CancelCaptureKeyUpAndDowns(iCapKey5b);
		iCapKey1=-1;	
	}
}
#endif
////////// CEBasicView //////////////////////////////////////////////////

#ifdef UIQ
CEBasicView::CEBasicView()
#else // UIQ3
CEBasicView::CEBasicView(CQikAppUi& aAppUi):CQikViewBase(aAppUi,KNullViewId)
#endif
{
	iInputState = EFalse;
	iLetterOffset=0;
}

CEBasicView::~CEBasicView()
{
#ifdef UIQ3
	delete iDsa;
#endif
}

void CEBasicView::ConstructL(const TRect& aRect)
{
	CreateWindowL();
#ifdef UIQ3 // UIQ3
    CQikViewBase::BaseConstructL();
#endif
	SetFocus(ETrue);
	EnableDragEvents();
	Window().SetShadowDisabled(ETrue);
	iDispMode = iCoeEnv->ScreenDevice()->DisplayMode();
#ifdef UIQ3
	iDsa=CDirectScreenAccess::NewL(iEikonEnv->WsSession(),*iEikonEnv->ScreenDevice(),Window(),*this);
	iDsa->StartL();
	iDsa->Gc()->SetClippingRegion(iDsa->DrawingRegion());
	iDrawingOn=ETrue;
	PreemptViewConstructionL();
	TQikViewMode viewmode;
	viewmode.SetFullscreen();
	SetViewModeL(viewmode);
#endif
	if(Window().DisplayMode() != EColor4K && Window().DisplayMode() != EColor64K)
	{
		Window().SetRequiredDisplayMode(EColor64K); // Try to set 64K color mode
	}
	SetRect(TRect(TPoint(0,0),TSize(aRect.Width(),320)));
	iWantedRect = Rect();
	iBitOffset = TPoint (0,0);
	
	ActivateL();
}

void CEBasicView::SizeChanged()
	{
#ifdef UIQ3
	CQikViewBase::SizeChanged();
	if(Rect() != iWantedRect && iWantedRect.Size() != TSize(0,0))
		{
		SetRect(iWantedRect);
		}
#endif
	}

const TInt KEscape[48]={0,0,0,2,0,2,4,2,4,2,4,-1,0,4,0,6,0,6,2,6,2,6,2,4,2,4,4,4,4,4,4,7,0,8,0,10,0,10,4,10,2,10,2,8,4,10,4,7};
void CEBasicView::Draw(const TRect& /*aRect*/) const
{
	CWindowGc& gc = SystemGc();
#ifdef UIQ3
//	if(CQUiConfigClient::Static().CurrentConfig().TouchScreen() == EQikUiConfigTouchScreenYes)
#endif
	{
#ifdef UIQ3
	if(current_video != NULL)
	{
		current_video->hidden->iNeedFullRedraw=ETrue;
		TRect realRect( 0,0,240,320 );
		gc.BitBlt(TPoint (0,0),current_video->hidden->EPOC_Bitmap,realRect);
	}
#endif	

	DrawVKeyBoard(gc);
	DrawCharSelector(gc);
	}
}

void CEBasicView::DrawVKeyBoard(CWindowGc& aGc) const
{
	aGc.SetBrushColor(KRgbBlack);
	aGc.SetBrushStyle(CGraphicsContext::ESolidBrush);
	
	
	TInt base=Size().iWidth-KOnScreenBoxWidth;
	if(current_video && !current_video->hidden->iVirtualKeyBoardActive)
	{
#ifdef UIQ3
	TRect keyrect (base,0,Size().iWidth,Size().iHeight);
	aGc.BitBlt(TPoint (base,0),current_video->hidden->EPOC_Bitmap, keyrect);
#else
	aGc.Clear(TRect(TPoint(base,0),TSize(KOnScreenBoxWidth,Size().iHeight)));
#endif
	}

	aGc.SetPenColor(KRgbGray);
	aGc.DrawRect(TRect(TPoint(base,KScreenKeySize),TSize(KOnScreenBoxWidth,KScreenKeySize))); 		
	// Draw escape round
	aGc.DrawRect(TRect(TPoint(base,KScreenKeySize*2),TSize(KOnScreenBoxWidth,KScreenKeySize)));
	// Draw keyboard select out
	aGc.DrawRect(TRect(TPoint(base,KScreenKeySize*3),TSize(KOnScreenBoxWidth,KScreenKeySize))); // outerline	
	// Control key
	aGc.SetBrushColor(current_video && current_video->hidden->iControlKeyDown?KRgbGreen:KRgbDarkGray);
	aGc.DrawRect(TRect(TPoint(base, 0),TSize(KOnScreenBoxWidth,KScreenKeySize))); // outerline

#ifdef UIQ3
	if(current_video)
	{
		aGc.SetPenColor(KRgbWhite);
		TCoeFont font(9,TCoeFont::EPlain);
		TBuf<2> letter;
		switch (current_video->hidden->iInputMode)
		{
		case EKeyboard:
			letter = _L("K");
			break;
		case EJoystick:
			letter = _L("J");
			break;
		case ECursorKeys:
			letter = _L("C");
			break;
		case EMouseMode:
			letter = _L("M");
			break;
		}

		if(current_video->hidden->iNumPadMode)
		{
			letter = _L("N");
		}

		const CFont* usedFont = &ScreenFont(font);
		aGc.UseFont(usedFont);
		aGc.DrawText(letter, TPoint((base+4)-(usedFont->TextWidthInPixels(letter)/2),KScreenKeySize*4-4));
		aGc.DiscardFont();
		aGc.SetPenColor(KRgbGray);
	}
#endif
	
	aGc.SetPenColor(KRgbWhite);
	// Draw escape
	for(TInt loop=0;loop<12;loop++)
		{
		aGc.DrawLine(TPoint(KEscape[loop*4]+base+1,KEscape[loop*4+1]+KScreenKeySize*2+2),TPoint(KEscape[loop*4+2]+base+1,KEscape[loop*4+3]+KScreenKeySize*2+2)); 
		}
	
	if(current_video && current_video->hidden->iMouseButtonSet == ERightMouseButton)
		{		
			aGc.SetBrushColor(KRgbWhite);
			aGc.DrawRect(TRect(TPoint(base+2,19),TSize(4,4)));
		}
		else if(current_video && current_video->hidden->iMouseButtonSet == ELeftMouseButton)		
		{	
			aGc.SetBrushColor(KRgbWhite);
			aGc.DrawRect(TRect(TPoint(base+2,25),TSize(4,4)));
		}
	
	if(current_video && current_video->hidden->iVirtualKeyBoardActive)
		{
		aGc.SetPenColor(KRgbGray);
		aGc.SetBrushColor(KRgbBlack);
		// Delete button rect
		aGc.DrawRect(TRect(TPoint(base,KScreenKeySize*5),TSize(KOnScreenBoxWidth,KScreenKeySize)));
		// Enter button
		aGc.DrawRect(TRect(TPoint(base,KScreenKeySize*4),TSize(KOnScreenBoxWidth,KScreenKeySize))); // outerline
		aGc.SetPenColor(KRgbWhite);
		aGc.DrawLine(TPoint(base+2,KScreenKeySize*4+4),TPoint(base+6,KScreenKeySize*4+4)); 
		aGc.DrawLine(TPoint(base+5,KScreenKeySize*4+10),TPoint(base+5,KScreenKeySize*4+4)); //enter		

	
		// Delete button
		aGc.DrawLine(TPoint(base+3,KScreenKeySize*5+2),TPoint(base+3,KScreenKeySize*5+KScreenKeySize-2)); 
		aGc.DrawLine(TPoint(base+5,KScreenKeySize*5+9),TPoint(base+3,KScreenKeySize*5+KScreenKeySize-2)); 
		aGc.DrawLine(TPoint(base+1,KScreenKeySize*5+9),TPoint(base+3,KScreenKeySize*5+KScreenKeySize-2));
		
		// Draw arrow upper
		aGc.SetPenColor(KRgbGray);
		aGc.DrawRect(TRect(TPoint(base,304),TSize(KOnScreenBoxWidth,KScreenKeySize)));
		aGc.DrawRect(TRect(TPoint(base,KScreenKeySize*6),TSize(KOnScreenBoxWidth,KScreenKeySize)));
		aGc.SetPenColor(KRgbWhite);
		aGc.DrawLine(TPoint(base+1,KScreenKeySize*6+8),TPoint(base+3,KScreenKeySize*6+2)); // arrow
		aGc.DrawLine(TPoint(base+3,KScreenKeySize*6+2),TPoint(base+6,KScreenKeySize*6+8));
		//Draw lower arrow
		aGc.DrawLine(TPoint(base+1,310),TPoint(base+3,317)); // arrow
		aGc.DrawLine(TPoint(base+3,317),TPoint(base+6,310));
	}
	
}

void CEBasicView::SetCurrentMultiTapKey(TInt aKey)
{
	TInt tapIndex = KOnScreenChars().Locate(aKey);
	if(tapIndex != KErrNotFound)
	{
		UpdateCharSelector(tapIndex);
	}

}

void CEBasicView::DrawCharSelector(CWindowGc& aGc) const
{
	if(current_video && current_video->hidden->iVirtualKeyBoardActive)
	{
		aGc.SetBrushColor(KRgbBlack);
		aGc.SetBrushStyle(CGraphicsContext::ESolidBrush);
		
#ifdef UIQ3
		TCoeFont font(9,TCoeFont::EPlain);
		const CFont* usedFont = &ScreenFont(font);//&ScreenFont(TCoeFont(TCoeFont::EExtraSmall,TCoeFont::EPlain));
#else
		const CFont* usedFont = iEikonEnv->AnnotationFont();
#endif
		aGc.UseFont(usedFont);
		TInt base=Size().iWidth-8;
		TInt charWidth=0;
		for(TInt chr=0;chr<KNoOnScreenKeys;chr++)
		{
			aGc.SetPenColor(KRgbGray);
			
			aGc.DrawRect(TRect(TPoint(base,KScreenKeysOffset+chr*KScreenKeySize),TSize(KOnScreenBoxWidth,KScreenKeySize)));
			aGc.SetPenColor(KRgbWhite);
			
			if(chr+iLetterOffset<KOnScreenChars().Length())
			{
				TBuf<2>chrbuf=KOnScreenChars().Mid(chr+iLetterOffset,1);
				charWidth=usedFont->CharWidthInPixels(chrbuf[0]);
				TPoint charPosition(base+4-charWidth/2,KScreenKeysOffset+KScreenKeySize);
				charPosition.iY+=(chr*KScreenKeySize);
				charPosition.iY-=2;
				aGc.DrawText(chrbuf,charPosition);
			}
		}
		aGc.DiscardFont();
	}
}

TCoeInputCapabilities CEBasicView::InputCapabilities() const
{
	TCoeInputCapabilities caps(0);
	if(iInputState)
	{
		caps.SetCapabilities(TCoeInputCapabilities::EAllText|TCoeInputCapabilities::ENavigation);
	}
	
	return caps;
}

void CEBasicView::SetInputState(TBool aOnOff)
{
	iInputState = aOnOff;
	SetFocus(ETrue);
}


TVwsViewId CEBasicView::ViewId() const
{
	TVwsViewId view(iAppUid,TUid::Uid(0x10000001));
	return view;
}

void CEBasicView::ViewActivatedL(const TVwsViewId& aPrevViewId,TUid aCustomMessageId,const TDesC8& aCustomMessage)
{
#ifdef UIQ
	SetFocus(ETrue);
	MakeVisible(ETrue);
#else
	CQikViewBase::ViewActivatedL(aPrevViewId,aCustomMessageId,aCustomMessage);
	SetFocus(ETrue);
	TQikViewMode viewmode;
	viewmode.SetFullscreen();
	SetViewModeL(viewmode);
	if(aPrevViewId.iAppUid.iUid != 0 && aPrevViewId.iAppUid != iAppUid)
	{
		SetParentView(aPrevViewId);
	}
#endif
	gViewVisible=true;
}
void CEBasicView::ViewDeactivated()
{
	gViewVisible=false;
#ifdef UIQ
	MakeVisible(EFalse);
#else
	CQikViewBase::ViewDeactivated();
#endif
}

void CEBasicView::UpdateVKeyBoard()
{
	ActivateGc();
	CWindowGc& gc = SystemGc();
	gc.SetPenColor(KRgbWhite);
	gc.SetBrushColor(KRgbBlack);
	gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
	DrawVKeyBoard(gc);
	DrawCharSelector(gc);
	DeactivateGc();
}

void CEBasicView::UpdateCharSelector(TInt aLetterOffset)
{
	iLetterOffset=aLetterOffset;
	ActivateGc();
	CWindowGc& gc = SystemGc();
	gc.SetPenColor(KRgbWhite);
	gc.SetBrushColor(KRgbBlack);
	gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
	DrawCharSelector(gc);
	DeactivateGc();
}

#ifdef UIQ3
void CEBasicView::Restart(RDirectScreenAccess::TTerminationReasons /*aReason*/)
{
	if(iForeground)
	{
		iDsa->Cancel();
		TRAPD(err,iDsa->StartL());
		if(err == KErrNone)
		{
			iDrawingOn=ETrue;
			UpdateClipRect();
		}
	}
}

void CEBasicView::UpdateClipRect()
{
	TRect rect = iDsa->ScreenDevice()->SizeInPixels();
	iDsa->Gc()->CancelClippingRegion();
	rect.iBr.iX-=8; // Decrease the clip area by 8 pixels
	RRegion clipRegion(rect);
	if(!current_video || (current_video &&  !(current_video->hidden->iVirtualKeyBoardActive || (current_video->hidden->iInputMode == EKeyboard))))
	{
		rect.iTl.iX= rect.iBr.iX;
		rect.iTl.iY=4*KScreenKeySize;
		rect.iBr.iX+=8;
		clipRegion.AddRect(rect);
	}
	
	iDsa->Gc()->SetClippingRegion(clipRegion);
	clipRegion.Close();
}


void CEBasicView::AbortNow(RDirectScreenAccess::TTerminationReasons /*aReason*/)
{
	iDrawingOn=EFalse;
	iDsa->Cancel();
	
	if(current_video != NULL)
	{
		current_video->hidden->iNeedFullRedraw=ETrue;
	}
}
void CEBasicView::ClearScreen()
{
	if(iDrawingOn)
	{
		iDsa->Gc()->SetBrushColor(KRgbBlack);
		iDsa->Gc()->Clear();
	}
}
void CEBasicView::PutBitmap(CFbsBitmap* aBitmap,TPoint aPoint,TRect aRect)
{
	if(iDrawingOn)
	{
		iDsa->Gc()->BitBlt(aPoint,aBitmap,aRect);	
		current_video->hidden->iCursorPos = TPoint();
		UpdateMouseCursor();		
		current_video->hidden->iWasUpdated = ETrue;
	}
}

void CEBasicView::UpdateMouseCursor()
{
	if(iDrawingOn && current_video->hidden->iCursor != NULL && SDL_ShowCursor(-1) == 1 )
	{		
		CWsBitmap* cursorBitmap = NULL;
		CWsBitmap* cursorMask = NULL;
	
		TPoint pos;
		TPoint orgPos;
		SDL_GetMouseState(&pos.iX, &pos.iY);
		orgPos = pos;
		if(current_video->hidden->iSX0Mode & ESX0Portrait)
		{
		pos.iX = current_video->hidden->iXScale*pos.iX;
		pos.iY= current_video->hidden->iYScale*pos.iY;
			
			cursorBitmap = current_video->hidden->iCursor->iCursorPBitmap;
			cursorMask = current_video->hidden->iCursor->iCursorPMask;
		}
		else
		{
			if(current_video->hidden->iSX0Mode & ESX0Flipped)
			{
			cursorBitmap = current_video->hidden->iCursor->iCursorLFBitmap;
			cursorMask = current_video->hidden->iCursor->iCursorLFMask;
			pos.iY = current_video->hidden->iXScale*orgPos.iX;
			pos.iX= current_video->hidden->iStretchSize.iHeight-(current_video->hidden->iYScale*orgPos.iY);	
			}
			else
			{
			cursorBitmap = current_video->hidden->iCursor->iCursorLBitmap;
			cursorMask = current_video->hidden->iCursor->iCursorLMask;
			pos.iY = current_video->hidden->iStretchSize.iWidth-(current_video->hidden->iXScale*orgPos.iX);
			pos.iX= current_video->hidden->iYScale*orgPos.iY;	
			}
		}
		
		TSize sze = cursorBitmap->SizeInPixels();
		if(!(current_video->hidden->iSX0Mode & ESX0Stretched))
			{
			pos-=current_video->hidden->iPutOffset;
			}

		if(pos != current_video->hidden->iCursorPos)
		{
		iDsa->Gc()->BitBlt(current_video->hidden->iCursorPos, current_video->hidden->EPOC_Bitmap, TRect(current_video->hidden->iCursorPos, sze));
		iDsa->Gc()->BitBltMasked(pos, cursorBitmap, TRect(sze),  cursorMask, EFalse);		
		current_video->hidden->iCursorPos = pos;
		}
	}
}


void CEBasicView::UpdateScreen()
{
	if(iDrawingOn)
	{
		iDsa->ScreenDevice()->Update();
	}
}
/**
Inherited from CQikViewBase and called upon by the UI Framework. 
It creates the views with command from resource.
*/
void CEBasicView::ViewConstructL()
{
	// Loads information about the UI configurations this view supports 
	// together with definition of each view, its layout and commands.
	ViewConstructFromResourceL(static_cast<CSDLApp*>(iEikonEnv->EikAppUi()->Application())->ViewResourceId());
}

TKeyResponse CEBasicView::OfferKeyEventL(const TKeyEvent& /*aKeyEvent*/,TEventCode /*aType*/)
{
	return EKeyWasNotConsumed;
}
#endif // UIQ3
#endif // UIQ3 || UIQ

