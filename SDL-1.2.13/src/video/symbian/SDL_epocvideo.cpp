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
    SDL_epocvideo.cpp
    Epoc based SDL video driver implementation

    Epoc version by Hannu Viitala (hannu.j.viitala@mbnet.fi)
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <apgwgnam.h>

extern "C" {
#include "SDL_error.h"
#include "SDL_video.h"
#include "SDL_keysym.h"
#include "SDL_keyboard.h"
#include "SDL_events_c.h"
#include "SDL_timer.h"
#include "SDL_sysjoystick.h"
#include "SDL_joystick_c.h"
#include "SDL_error.h"
#include "SDL_timer.h"
#include "SDL_video.h"
#undef NULL
#include "../SDL_pixels_c.h"
}

#include "SDL_epocvideo.h"
#include "SDL_epocevents_c.h"
#include "EBasicApp.h"
#ifndef UIQ3
#include <hal.h>
#endif

#include <coedef.h>
extern bool gViewVisible;

TInt32 i240StartTable[240];
TUint16 i320StartTable[320];
TUint8 i320StepTable[320];
TInt gScaleXPos[641];
TInt gScaleYPos[641];
TInt gScaleStep[641];
TInt gScaleYStep[641];
TBool gHeapIsLocked=EFalse;

extern "C" void EPOC_CalcStretchFactors(_THIS,TSize aTargetSize);
	
#if defined (S60) || defined (S60V3)
void EPOC_SetS60Mode(_THIS,TInt aS60Mode);
#endif

void UpdateScaleFactors()
	{
	if(current_video != NULL)
		{
#if defined (S60V3) || defined (S60)						
		EPOC_SetS60Mode(current_video,current_video->hidden->iSX0Mode);
#ifdef S60V3
		current_video->hidden->iWindowCreator->UpdateVirtualKeyboard();
#endif
#else
		EPOC_CalcStretchFactors(current_video, TSize( current_video->hidden->iModeSize.iWidth,current_video->hidden->iModeSize.iHeight) );
#endif
		current_video->hidden->iCenterOffset = TPoint(0,0);
		if( !(current_video->hidden->iSX0Mode & ESX0Stretched) )
			{
			if(current_video->hidden->iSX0Mode & ESX0Portrait)
				{
				current_video->hidden->iCenterOffset.iX = current_video->hidden->EPOC_DisplaySize.iWidth/2 - current_video->hidden->iModeSize.iWidth/2;
				current_video->hidden->iCenterOffset.iY = current_video->hidden->EPOC_DisplaySize.iHeight/2- current_video->hidden->iModeSize.iHeight/2;
				}
			else
				{
				current_video->hidden->iCenterOffset.iX = current_video->hidden->EPOC_DisplaySize.iWidth/2-current_video->hidden->iModeSize.iHeight/2;
				current_video->hidden->iCenterOffset.iY = current_video->hidden->EPOC_DisplaySize.iHeight/2 - current_video->hidden->iModeSize.iWidth/2;
				}
			}		
		}

	}

void ClearBackBuffer(_THIS)
{
	/* Clear backbuffer */
#if defined (__WINS__)|| defined (S60) || defined (S80) || defined(S90)||defined(UIQ3)||defined(S60V3)
	TBool lockedHeap=EFalse;
	TBitmapUtil lock(Private->EPOC_Bitmap);	
	if(!gHeapIsLocked)
	{
		lock.Begin(TPoint(0,0)); // Lock bitmap heap
		gHeapIsLocked=ETrue;
		lockedHeap=ETrue;
	}
	TUint16* screenBuffer = (TUint16*)Private->EPOC_Bitmap->DataAddress();
#else
	TUint16* screenBuffer = (TUint16*)Private->EPOC_FrameBuffer;
#endif
	memset(screenBuffer, 0, Private->EPOC_ScreenSize.iHeight*Private->EPOC_ScreenSize.iWidth*Private->EPOC_BytesPerPixel);
#if defined (__WINS__) || defined (S60) || defined (S80) || defined(S90) || defined(UIQ3) || defined(S60V3)
	if(lockedHeap)
	{
		lock.End();
		gHeapIsLocked=EFalse;
	}
#endif
}

/* For debugging */
void RDebug_Print_b(char* error_str, void* param)
    {
    TBuf8<128> error8((TUint8*)error_str);
    TBuf<128> error;
    error.Copy(error8);
    if (param) //!! Do not work if the parameter is really 0!!
        RDebug::Print(error, param);
    else 
        RDebug::Print(error);
    }

extern "C" void RDebug_Print(char* error_str, void* param)
    {
    RDebug_Print_b(error_str, param);
    }


int Debug_AvailMem2()
    {
    //User::CompressAllHeaps();
    TMemoryInfoV1Buf membuf; 
    User::LeaveIfError(UserHal::MemoryInfo(membuf));
    TMemoryInfoV1 minfo = membuf();
	return(minfo.iFreeRamInBytes);
    }

extern "C" int Debug_AvailMem()
    {
    return(Debug_AvailMem2());
    }


extern "C" {

/* Initialization/Query functions */

static int EPOC_VideoInit(_THIS, SDL_PixelFormat *vformat);
static void EPOC_SetCaption(_THIS, const char *title, const char * /*icon*/);
static SDL_Rect **EPOC_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags);
void DrawBlueStripes(_THIS);
static SDL_Surface *EPOC_SetVideoMode(_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags);
static int EPOC_SetColors(_THIS, int firstcolor, int ncolors,
			  SDL_Color *colors);
static void EPOC_VideoQuit(_THIS);

/* Hardware surface functions */

static int EPOC_AllocHWSurface(_THIS, SDL_Surface *surface);
static int EPOC_LockHWSurface(_THIS, SDL_Surface *surface);
static int EPOC_FlipHWSurface(_THIS, SDL_Surface *surface);
static void EPOC_UnlockHWSurface(_THIS, SDL_Surface *surface);
static void EPOC_FreeHWSurface(_THIS, SDL_Surface *surface);
static void EPOC_DirectUpdate(_THIS, int numrects, SDL_Rect *rects);

static int EPOC_Available(void);
static SDL_VideoDevice *EPOC_CreateDevice(int devindex);

/* Mouse functions */
static void EPOC_FreeWMCursor(_THIS, WMcursor *cursor);
static int EPOC_ShowWMCursor(_THIS, WMcursor *cursor);

static WMcursor *EPOC_CreateWMCursor(_THIS, Uint8* /*data*/, Uint8* /*mask*/, int /*w*/, int /*h*/, int /*hot_x*/, int /*hot_y*/);
/* !! Table for fast conversion from 8 bit to 12/16 bit */
static TUint16 EPOC_HWPalette_256_to_DisplayMode[256];

VideoBootStrap EPOC_bootstrap = {
	"epoc", "EPOC system",
    EPOC_Available, EPOC_CreateDevice
};

const TUint32 WindowClientHandle = 9210; //!!

/* Epoc video driver bootstrap functions */

static int EPOC_Available(void)
{
    return 1; /* Always available */
}
extern void EPOC_CloseOSKeyMap();
static void EPOC_DeleteDevice(SDL_VideoDevice *device)
{
	EPOC_CloseOSKeyMap();
	free(device->hidden);
	free(device);
}

static SDL_VideoDevice *EPOC_CreateDevice(int /*devindex*/)
{
	SDL_VideoDevice *device;

	/* Allocate all variables that we free on delete */
	device = (SDL_VideoDevice *)malloc(sizeof(SDL_VideoDevice));
	if ( device ) {
		memset(device, 0, (sizeof *device));
		device->hidden = (struct SDL_PrivateVideoData *)
				malloc((sizeof *device->hidden));
	}
	if ( (device == NULL) || (device->hidden == NULL) ) {
		SDL_OutOfMemory();
		if ( device ) {
			free(device);
		}
		return(0);
	}
	memset(device->hidden, 0, (sizeof *device->hidden));

	/* Set the function pointers */
	device->VideoInit = EPOC_VideoInit;
	device->ListModes = EPOC_ListModes;
	device->SetVideoMode = EPOC_SetVideoMode;
	device->SetColors = EPOC_SetColors;
	device->UpdateRects = NULL;
	device->VideoQuit = EPOC_VideoQuit;
	device->AllocHWSurface = EPOC_AllocHWSurface;
	device->CheckHWBlit = NULL;
	device->FillHWRect = NULL;
	device->SetHWColorKey = NULL;
	device->SetHWAlpha = NULL;
	device->LockHWSurface = EPOC_LockHWSurface;
	device->UnlockHWSurface = EPOC_UnlockHWSurface;
	device->FlipHWSurface = EPOC_FlipHWSurface;
	device->FreeHWSurface = EPOC_FreeHWSurface;
	device->SetIcon = NULL;
	device->SetCaption = EPOC_SetCaption;
	device->GetWMInfo = NULL;
	device->FreeWMCursor = EPOC_FreeWMCursor;
	device->CreateWMCursor = EPOC_CreateWMCursor;
	device->ShowWMCursor = EPOC_ShowWMCursor;
	device->WarpWMCursor = NULL;

	device->InitOSKeymap = EPOC_InitOSKeymap;
	device->PumpEvents = EPOC_PumpEvents;
	device->free = EPOC_DeleteDevice;
	device->hidden->iEikEnv=CEikonEnv::Static();

	return device;
}

TUint8 MsbToLsb(TUint8 aVal)
    {
    TUint lsbValue = ((aVal&1)<<7)|((aVal&2)<<5)|((aVal&4)<<3)|((aVal&8)<<1)|((aVal&16)>>1)|((aVal&32)>>3)|((aVal&64)>>5)|((aVal&128)>>7);
    return lsbValue;
    }

void MsbToLsbVert(TUint8 aVal, TUint8* aValues, TUint8 aBitToSet)
	{
	aValues[0] = aValues[0]|((aVal&1)?aBitToSet:0);
	aValues[1] = aValues[1]|(aVal&2)?aBitToSet:0;
	aValues[2] = aValues[2]|(aVal&4)?aBitToSet:0;
	aValues[3] = aValues[3]|(aVal&8)?aBitToSet:0;
	aValues[4] = aValues[4]|(aVal&16)?aBitToSet:0;
	aValues[5] = aValues[5]|(aVal&32)?aBitToSet:0;
	aValues[6] = aValues[6]|(aVal&64)?aBitToSet:0;
	aValues[7] = aValues[7]|(aVal&128)?aBitToSet:0;
	}

void EPOC_CreateCursorBitmap(_THIS, WMcursor* aCursor, TUint8* data, TUint8* mask)
	{
	TInt w = aCursor->iWidth;
	TInt h = aCursor->iHeight;
	
	aCursor->iCursorPBitmap = new CWsBitmap(_this->hidden->iEikEnv->WsSession());
	aCursor->iCursorPMask = new CWsBitmap(_this->hidden->iEikEnv->WsSession());
	aCursor->iCursorLBitmap = new CWsBitmap(_this->hidden->iEikEnv->WsSession());
	aCursor->iCursorLMask = new CWsBitmap(_this->hidden->iEikEnv->WsSession());;
	aCursor->iCursorLFBitmap = new CWsBitmap(_this->hidden->iEikEnv->WsSession());
	aCursor->iCursorLFMask = new CWsBitmap(_this->hidden->iEikEnv->WsSession());;
	aCursor->iCursorPBitmap->Create(TSize(w,h), EGray2);
	aCursor->iCursorPMask->Create(TSize(w,h), EGray2);
	
	aCursor->iCursorLFBitmap->Create(TSize(h,w), EGray2);
	aCursor->iCursorLFMask->Create(TSize(h,w), EGray2);
		
	aCursor->iCursorLBitmap->Create(TSize(h,w), EGray2);
	aCursor->iCursorLMask->Create(TSize(h,w), EGray2);
	
	aCursor->iCursorPBitmap->LockHeap();
	TUint8* addr =(TUint8*) aCursor->iCursorPBitmap->DataAddress();
	TUint8* maskaddr =(TUint8*) aCursor->iCursorPMask->DataAddress();

	TUint8 bytes = (w/8);	

	TUint8 padding = ((w/8)%4); // four bytes aligned
	for(TInt hh = 0;hh<h;hh++)
		{		
			for(TInt ww = 0;ww<bytes;ww++)
			{
			*addr = MsbToLsb(*data);
			*maskaddr = MsbToLsb(*mask);
		
			mask++;
			data++;
		
			addr++;
			maskaddr++;
			}	
	
			addr+=padding;
			maskaddr+=padding;
		}


	aCursor->iCursorPBitmap->UnlockHeap();
	TBitmapUtil bmUtilA(aCursor->iCursorPBitmap);
	TBitmapUtil bmUtilB(aCursor->iCursorPMask);
	TBitmapUtil bmUtilC(aCursor->iCursorLBitmap);
	TBitmapUtil bmUtilD(aCursor->iCursorLMask);
	TBitmapUtil bmUtilE(aCursor->iCursorLFBitmap);
	TBitmapUtil bmUtilF(aCursor->iCursorLFMask);
	bmUtilA.Begin(TPoint());
	bmUtilB.Begin(TPoint());
	bmUtilC.Begin(TPoint());
	bmUtilD.Begin(TPoint());
	bmUtilE.Begin(TPoint());
	bmUtilF.Begin(TPoint());
	TInt width = aCursor->iCursorPBitmap->SizeInPixels().iWidth;
	TInt height = aCursor->iCursorPBitmap->SizeInPixels().iHeight;
	TUint32 dat = 0;
	TUint32 msk = 0;
	for(TInt hh = 0;hh<height;hh++)
		{
		bmUtilA.SetPos(TPoint(0, hh));
		bmUtilB.SetPos(TPoint(0, hh));
				
		bmUtilC.SetPos(TPoint(hh, width-1));
		bmUtilD.SetPos(TPoint(hh, width-1));
		
		bmUtilE.SetPos(TPoint((height-1)-hh, 0));
		bmUtilF.SetPos(TPoint((height-1)-hh, 0));
		for(TInt ww = 0;ww<width;ww++)
			{
			dat = bmUtilA.GetPixel();
			msk = bmUtilB.GetPixel();
			
			bmUtilC.SetPixel(dat);
			bmUtilD.SetPixel(msk);
			
			bmUtilE.SetPixel(dat);
			bmUtilF.SetPixel(msk);
			
			bmUtilA.IncXPos();
			bmUtilB.IncXPos();
			
			bmUtilC.DecYPos();
			bmUtilD.DecYPos();
			
			bmUtilE.IncYPos();
			bmUtilF.IncYPos();
			}		
		}
	bmUtilF.End();
	bmUtilE.End();
	bmUtilD.End();
	bmUtilC.End();
	bmUtilB.End();
	bmUtilA.End();
	
	}
WMcursor *EPOC_CreateWMCursor(_THIS, Uint8* data, Uint8* mask, int w, int h, int hot_x, int hot_y)
    {
	WMcursor* cursor = new WMcursor;
	TInt cursorSize = (w*h)/8;	
	cursor->iWidth = w;
	cursor->iHeight = h;
	cursor->iHotX = hot_x;
	cursor->iHotY = hot_y;
	EPOC_CreateCursorBitmap(_this, cursor, data, mask);
	return (WMcursor*) cursor; //hii! prevents SDL to view a std cursor
    }

void EPOC_FreeWMCursor(_THIS, WMcursor* cursor)
{
	if(cursor != NULL)
	{
		delete cursor->iCursorPBitmap;
		cursor->iCursorPBitmap = NULL;
		delete cursor->iCursorPMask;
		cursor->iCursorPMask = NULL;	
		
		delete cursor->iCursorLBitmap;
		cursor->iCursorLBitmap = NULL;
		delete cursor->iCursorLMask;
		cursor->iCursorLMask = NULL;
		
		delete cursor->iCursorLFBitmap;
		cursor->iCursorLFBitmap = NULL;
		delete cursor->iCursorLFMask;
		cursor->iCursorLFMask = NULL;				
	}
}

int EPOC_ShowWMCursor(_THIS, WMcursor *cursor)
    {
	/* Set the window cursor to our cursor, if applicable */
	_this->hidden->iCursor = cursor;
    return true;  
    }

int GetBpp(TDisplayMode displaymode)
{
    TInt numColors = TDisplayModeUtils::NumDisplayModeColors(displaymode);
    TInt bitsPerPixel = 1;
    for (TInt32 i = 2; i < numColors; i <<= 1, bitsPerPixel++);
    return bitsPerPixel;    
}

void EPOC_SetCaption(_THIS, const char * title, const char * /*icon*/)
{
	if(title != NULL)
	{
		TInt titleLen = strlen(title);
		HBufC* titleBuffer = HBufC::NewLC(titleLen);
		TPtrC8 titlePtr((const unsigned char*)title, titleLen);
		titleBuffer->Des().Copy(titlePtr);
		CApaWindowGroupName* groupName = CApaWindowGroupName::NewL(Private->iEikEnv->WsSession(), Private->iEikEnv->RootWin().Identifier());
		CleanupStack::PushL(groupName);	
		groupName->SetCaptionL(*titleBuffer);
		groupName->SetWindowGroupName(Private->iEikEnv->RootWin());
		CleanupStack::PopAndDestroy(2, titleBuffer);
	}
}

#if defined  (__WINS__) ||  defined (S60) ||  defined (S80) ||  defined (S90) ||  defined (UIQ3) || defined(S60V3)
void EPOC_ReconfigureVideo(_THIS)
{
    /* Initialise Epoc frame buffer */
    TDisplayMode displayMode =Private->iEikEnv->ScreenDevice()->DisplayMode();	    
    /* Create bitmap, device and context for screen drawing */
	delete Private->EPOC_Bitmap;
	Private->EPOC_Bitmap = NULL;
	Private->EPOC_Bitmap = new (ELeave) CWsBitmap(Private->iEikEnv->WsSession());
	// Not 64K or 4K color mode, try to set 64K
	if(displayMode != EColor64K && displayMode != EColor4K)
	{
		displayMode=EColor64K;; // Also tried to switch to by the view.
	}
#if defined (S60)||defined (S60V3) // this needs to be atleast 320x200 to cope	
#ifdef S60V3
    Private->EPOC_ScreenSize   =Private->iEikEnv->ScreenDevice()->SizeInPixels();// TSize(256,320);
	Private->EPOC_DisplaySize = Private->EPOC_ScreenSize;
	// Must ensure that the height is at least 320 pixels height and 240 width
	TSize createSize = Private->EPOC_ScreenSize;
	
	if(createSize.iHeight<320)
		createSize.iHeight = 320;
	
	if(createSize.iWidth<320)
	{
		Private->EPOC_ScreenSize.iWidth = 320;
		createSize.iWidth = 320;
	}

	Private->EPOC_Bitmap->Create(createSize, displayMode);
#else // S60
    Private->EPOC_ScreenSize=TSize(256,320);// TSize(256,320);
	Private->EPOC_Bitmap->Create(TSize(256,320), displayMode);
#endif
#elif defined (S80) || defined (S90)	
    Private->EPOC_ScreenSize        = Private->iEikEnv->ScreenDevice()->SizeInPixels();
	Private->EPOC_Bitmap->Create(Private->EPOC_ScreenSize, displayMode);
#else //UIQ
    Private->EPOC_ScreenSize  = Private->iEikEnv->ScreenDevice()->SizeInPixels();
    Private->EPOC_DisplaySize = Private->EPOC_ScreenSize;
	TSize createSize = Private->EPOC_ScreenSize;
	if(createSize.iHeight<320)
		createSize.iHeight = 320;
#ifndef UIQ3
	Private->EPOC_ScreenSize.iWidth=Private->EPOC_ScreenSize.iWidth-8;// Reserve lower 8 lines	
	createSize.iWidth = Private->EPOC_ScreenSize.iWidth;
#endif
	Private->EPOC_Bitmap->Create(createSize, displayMode);
#endif
	Private->EPOC_DisplayMode	    = displayMode;
	Private->EPOC_BytesPerPixel	    = ((GetBpp(displayMode)-1) / 8) + 1;
	Private->EPOC_BytesPerScanLine  = (Private->EPOC_ScreenSize.iWidth) * Private->EPOC_BytesPerPixel;
	Private->EPOC_ScreenOffset = 0;
	/* Tell the system that something has been drawn */
	TRect  rect = TRect(Private->iWindowCreator->Size());
  	Private->iWindowCreator->Win().Invalidate(rect);
  	UpdateScaleFactors();

	if(current_video && current_video->screen != NULL) {
		RedrawWindowL(current_video);
	}
}
#endif

int EPOC_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
    // !!TODO:handle leave functions!
#if defined UIQ || defined UIQ3 || defined S90
	Private->iHasMouseOrTouch = ETrue;
#else
	Private->iHasMouseOrTouch = EFalse;
#endif
	Private->iOnecallback = new CAsyncCallBack (CActive::EPriorityLow); //Service all the others first and then this	
	Private->iWindowCreator=static_cast<CEBasicAppUi*>(Private->iEikEnv->EikAppUi())->View();

    int i;

	/* Initialize all variables that we clean on shutdown */   

	for ( i=0; i<SDL_NUMMODES; ++i ) {
		Private->SDL_modelist[i] = (SDL_Rect *)malloc(sizeof(SDL_Rect));
		Private->SDL_modelist[i]->x = Private->SDL_modelist[i]->y = 0;
	}

    /* Initialise Epoc frame buffer */
    TDisplayMode displayMode =Private->iEikEnv->ScreenDevice()->DisplayMode();
 #if !defined  (__WINS__) && !defined (S60) && !defined (S80) && !defined (S90) && !defined (UIQ3) && !defined(S60V3)

	TInt value;
	HAL::Get(HALData::EMachineUid,value);
	TInt offset=0;
	switch(value)
	{
	case 0x101f6b26: //A9xx
	case 0x101fd279://P3x
	 offset=32;
	 break;
	case 0x101f408b://P800
	case 0x101fb2ae://P900
	case 0x10200ac6://P910
		break;
	default:
		TBuf<16>valuebuf;
		valuebuf.Format(_L("0x%x"),value);
		 offset=0; // Default to Sony Pxxx platform
		CEikonEnv::Static()->InfoWinL(_L("Unknown HAL value!"),valuebuf);
		break;
	}

    TScreenInfoV01 screenInfo;
	TPckg<TScreenInfoV01> sInfo(screenInfo);
	UserSvr::ScreenInfo(sInfo);
	
	Private->EPOC_ScreenSize		=screenInfo.iScreenSize; 
	Private->EPOC_DisplayMode		= displayMode;
    Private->EPOC_HasFrameBuffer	= screenInfo.iScreenAddressValid;
	Private->EPOC_FrameBuffer		= Private->EPOC_HasFrameBuffer ? (TUint8*) screenInfo.iScreenAddress+offset : NULL;
	Private->EPOC_BytesPerPixel	    = ((GetBpp(displayMode)-1) / 8) + 1;
	Private->EPOC_BytesPerScanLine	= screenInfo.iScreenSize.iWidth * Private->EPOC_BytesPerPixel;

    /* It seems that in SA1100 machines for 8bpp displays there is a 512 palette table at the 
     * beginning of the frame buffer. E.g. Series 7 and Netbook.
     * In 12 bpp machines the table has 16 entries.
	 */
	if (Private->EPOC_HasFrameBuffer && GetBpp(displayMode) == 8)
		Private->EPOC_FrameBuffer += 512;
	// this is NOT valid for a P800. It has NO palette entry
#ifdef S80
	if (Private->EPOC_HasFrameBuffer && GetBpp(displayMode) == 12 || Private->EPOC_HasFrameBuffer && GetBpp(displayMode) == 16)
		Private->EPOC_FrameBuffer += 16 * 2;
#endif
#else /* defined __WINS__ || S60 || S80||S90 || UIQ3*/
    EPOC_ReconfigureVideo(_this);  	
#if defined (S60)||defined (S60V3) // this needs to be atleast 320x200 to cope
	Private->iSX0Mode = ESX0Stretched|ESX0Flipped;
	Private->iCurrentChar=65; // Start of with an A
#elif defined (S80) || defined (S90)
#else //UIQ	
	Private->iSX0Mode = ESX0Stretched;
#endif
    Private->EPOC_HasFrameBuffer    = EFalse;
    Private->EPOC_FrameBuffer       = NULL; 
	RThread mainThread;
	Private->iThreadId=mainThread.Id();
#endif /* defined __WINS__ || S60 || S80||S90 || UIQ3*/

    /* The "best" video format should be returned to caller. */
    vformat->BitsPerPixel       = GetBpp(Private->EPOC_DisplayMode) ;
    vformat->BytesPerPixel      = Private->EPOC_BytesPerPixel;

	TheBasicAppUi->GetConfig(_this);
	/* Modes sorted largest to smallest !!TODO:sorting order??*/
	Private->SDL_modelist[0]->w = 320; Private->SDL_modelist[0]->h = 200;
	Private->SDL_modelist[1]->w = 640; Private->SDL_modelist[1]->h = 200; 
	Private->SDL_modelist[2]->w = 640; Private->SDL_modelist[2]->h = 400; 
	Private->SDL_modelist[3]->w = 640; Private->SDL_modelist[3]->h = 480;
	Private->SDL_modelist[4]->w = 320; Private->SDL_modelist[4]->h = 240;	
	Private->SDL_modelist[5]->w = 240; Private->SDL_modelist[5]->h = 320;
	Private->SDL_modelist[6]->w = 256; Private->SDL_modelist[6]->h = 240;
	Private->SDL_modelist[7]->w = 	Private->EPOC_ScreenSize.iWidth ; Private->SDL_modelist[7]->h = 	Private->EPOC_ScreenSize.iHeight;
	Private->SDL_modelist[8]->w = 	Private->EPOC_ScreenSize.iHeight ; Private->SDL_modelist[8]->h = 	Private->EPOC_ScreenSize.iWidth ;
	Private->SDL_modelist[9] = NULL;

    //!! TODO: error handling
#if defined (__WINS__) || defined (S60) || defined (S80)|| defined(S90)||defined(UIQ3)||defined(S60V3)
	TBool lockedHeap=EFalse;
	TBitmapUtil lock(Private->EPOC_Bitmap);	
	if(!gHeapIsLocked)
	{
		lock.Begin(TPoint(0,0)); // Lock bitmap heap
		gHeapIsLocked=ETrue;
		lockedHeap=ETrue;
	}
#endif
	DrawBlueStripes(_this);
#if defined (__WINS__) || defined (S60) || defined (S80) || defined(S90) || defined(UIQ3) || defined(S60V3)
	if(lockedHeap)
	{
		lock.End();
		gHeapIsLocked=EFalse;
	}
#endif
	  /* Tell the system that something has been drawn */
	TRect  rect = TRect(Private->iWindowCreator->Size());
  	Private->iWindowCreator->Win().Invalidate(rect);
	return(0);
}

SDL_Rect **EPOC_ListModes(_THIS, SDL_PixelFormat *format, Uint32 /*flags*/)
{
    if (format->BitsPerPixel == GetBpp(Private->EPOC_DisplayMode)  || format->BitsPerPixel == 8)
        return Private->SDL_modelist;
    return NULL;
}

int EPOC_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
	if(_this->info.vfmt->palette != NULL)
	{
		TBool palchanged = EFalse;	
	
		if(ncolors+firstcolor>256)
		{
			ncolors = 256-firstcolor;
		}

		for(int i = 0; i < ncolors; i++) {
			
			// 4k value: 000rgb
			TUint16 colornK = 0;
			if(Private->EPOC_DisplayMode==EColor4K)
			{
				colornK |= ((colors[i].r & 0x0000f0) << 4); 
				colornK |= (colors[i].g & 0x0000f0);      
				colornK |= ((colors[i].b & 0x0000f0) >> 4);
			}
			else
			{
				colornK |= (colors[i].r & 0x0000f8) << 8; 
				colornK |= (colors[i].g & 0x0000fc)<<3;      
				colornK |= (colors[i].b & 0x0000f8) >> 3;
			}

			if(EPOC_HWPalette_256_to_DisplayMode[i+firstcolor] != colornK)
			{
				EPOC_HWPalette_256_to_DisplayMode[i+firstcolor] = colornK;
				palchanged = ETrue;
			}		
		}

		if(palchanged)
		{
			SDL_Rect updaterect;
			
			updaterect.x = updaterect.y = 0;
			updaterect.w = _this->screen->w;
			updaterect.h = _this->screen->h;
			/* image needs to be redrawn */
			EPOC_DirectUpdate(_this, 1, &updaterect);
		}

		return(1);
	}
	return 0;
}

SDL_Surface *EPOC_SetVideoMode(_THIS, SDL_Surface *current,
				int width, int height, int bpp, Uint32 /*flags*/)
{
    if (!(((width == 640 && height == 200 ) ||
          (width == 640 && height == 400 ) || 
          (width == 640 && height == 480 ) || 
          (width == 320 && height == 200 )||
		  (width == 320 && height == 240 ) ||
		  (width == 256 && height == 240 ) ||
		  (width == Private->EPOC_ScreenSize.iWidth && height == Private->EPOC_ScreenSize.iHeight)||
		  (width == Private->EPOC_ScreenSize.iHeight && height == Private->EPOC_ScreenSize.iWidth)
		  ) &&(bpp == 8 || bpp == GetBpp(Private->EPOC_DisplayMode) )
		  )){
		SDL_SetError("Requested video mode is not supported");
        return NULL;
    }

    if (current && current->pixels) {
        free(current->pixels);
        current->pixels = NULL;
    }

	if ( ! SDL_ReallocFormat(current, bpp, 0, 0, 0, 0) ) {
		return(NULL);
	}

    /* Set up the new mode framebuffer */
    if (bpp == 8) 
	    current->flags = (SDL_FULLSCREEN|SDL_SWSURFACE|SDL_PREALLOC|SDL_HWPALETTE); 
    else // 12,16 bpp
	    current->flags = (SDL_FULLSCREEN|SDL_SWSURFACE|SDL_PREALLOC); 
	current->w = width;
	current->h = height;
    int numBytesPerPixel = ((bpp-1)>>3) + 1;   
	current->pitch = numBytesPerPixel * width; // Number of bytes in scanline 

	current->pixels = malloc(width * height * numBytesPerPixel);
	memset(current->pixels, 0, width * height * numBytesPerPixel);

	/* Set the blit function */
	_this->UpdateRects = EPOC_DirectUpdate;

    /* Must buffer height be shrinked to screen by 2 ? */
    if (current->h >= 400)
        Private->EPOC_ShrinkedHeight = ETrue;
	else
		 Private->EPOC_ShrinkedHeight = EFalse;

	Private->iModeSize = TSize(current->w,current->h);
#if !defined (S80) && !defined(S90)
	if (current->w > Private->EPOC_ScreenSize.iWidth )
		Private->EPOC_IsFlipped = ETrue;
	else
#endif //!S80
		Private->EPOC_IsFlipped = EFalse;

#if !defined (S80) && !defined (S90)
	if(current->w > 320)
		Private->EPOC_ShrinkedWidth = ETrue;
	else
#endif
		Private->EPOC_ShrinkedWidth = EFalse;

#if defined(S80) || defined (UIQ)
	if(current->h==240 || current->h == 480)
	{
		TInt loop;
		for(loop=0;loop<240;loop++)
		{
			i240StartTable[loop]=(loop*5)/6;
		}
	}
	else
	{
		for(TInt loop=0;loop<240;loop++)
		{
			i240StartTable[loop]=loop;
		}
	}		
#elif defined (S60) || defined(S60V3)
	Private->iIs240Mode = (current->h==240 || current->h == 480);
#elif defined (S90)
		for(TInt loop=0;loop<240;loop++)
		{
			i240StartTable[loop]=loop;
		}		
#endif // End UIQ & S80
	UpdateScaleFactors();
    /* Centralize game window on device screen  */
    Private->EPOC_ScreenOffset = 0;//
#if defined (S80) || defined(S90)
	Private->EPOC_ScreenOffset = !(Private->iSX0Mode & ESX0Stretched)?(Private->EPOC_ScreenSize.iWidth - current->w) / 2:(Private->EPOC_ScreenSize.iWidth-current->w*Private->iXScale)/2 ;
#endif	
	/* We're done */
	return(current);
}

void DrawBlueStripes(_THIS)
{
	/* Draw blue stripes background */
#if defined (__WINS__)|| defined (S60) || defined (S80) || defined(S90)||defined(UIQ3)||defined(S60V3)
	TUint16* screenBuffer = (TUint16*)Private->EPOC_Bitmap->DataAddress();
#else
	TUint16* screenBuffer = (TUint16*)Private->EPOC_FrameBuffer;
#endif
	for (int y=0; y < Private->EPOC_ScreenSize.iHeight; y++) {
		for (int x=0; x < Private->EPOC_ScreenSize.iWidth; x++) {
			TUint16 color = ((x+y)>>1) & 0x0; /* Draw pattern */
			*screenBuffer++ = color;
		}
	}
}

void RedrawWindowL(_THIS)
{
    SDL_Rect fullScreen;
    fullScreen.x = 0;
    fullScreen.y = 0;
    fullScreen.w = _this->screen->w;
    fullScreen.h = _this->screen->h;

#if defined (__WINS__) || defined (S60) || defined (S80)|| defined(S90)||defined(UIQ3)||defined(S60V3)
	Private->iNeedFullRedraw = ETrue;
	TBool lockedHeap=EFalse;
	    TBitmapUtil lock(Private->EPOC_Bitmap);	
		if(!gHeapIsLocked)
		{
	        lock.Begin(TPoint(0,0)); // Lock bitmap heap
			gHeapIsLocked=ETrue;
			lockedHeap=ETrue;
		}
#endif

    if (fullScreen.w < Private->EPOC_ScreenSize.iWidth
        || fullScreen.h < Private->EPOC_ScreenSize.iHeight) {
		DrawBlueStripes(_this);
    }
#if defined (__WINS__) || defined (S60) || defined (S80) || defined(S90)||defined(UIQ3)||defined(S60V3)
	if(lockedHeap)
		{
			lock.End();
			gHeapIsLocked=EFalse;
		}
#endif

    /* Draw current buffer */
    EPOC_DirectUpdate(_this, 1, &fullScreen);

    /* Tell the system that something has been drawn */
	TRect  rect = TRect(Private->iWindowCreator->Size());
  	Private->iWindowCreator->Win().Invalidate(rect);
}


/* We don't actually allow hardware surfaces other than the main one */
static int EPOC_AllocHWSurface(_THIS, SDL_Surface * /*surface*/)
{
	return(-1);
}
static void EPOC_FreeHWSurface(_THIS, SDL_Surface * /*surface*/)
{
}

static int EPOC_LockHWSurface(_THIS, SDL_Surface * /*surface*/)
{
	return(0);
}
static void EPOC_UnlockHWSurface(_THIS, SDL_Surface * /*surface*/)
{
}

static int EPOC_FlipHWSurface(_THIS, SDL_Surface * /*surface*/)
{
	return(0);
}

#define CalcAverage16(a, b) ((((a)&(0xF7DE))>>1)+(((b)&(0xF7DE))>>1)+((a)&(~(0xF7DE))))

inline TUint16 CalcAverage16Func(TUint16 pixel1, TUint16 pixel2)
	{
	pixel1 = (pixel1 & 0xF7DE)>>1;
	pixel2 = (pixel2 & 0xF7DE)>>1;
	return pixel1+pixel2+ ((pixel1)&(~(0xF7DE)));
	}

inline TUint16 CalcAverage12Func(TUint16 pixel1, TUint16 pixel2)
	{
	pixel1 = (pixel1 & 0xEEE)>>1;
	pixel2 = (pixel2 & 0xEEE)>>1;
	return pixel1+pixel2 + ((pixel1)&(~(0xEEE)));;
	}

/* Note:  If we are terminated, this could be called in the middle of
   another SDL video routine -- notably UpdateRects.
*/
void EPOC_VideoQuit(_THIS)
{
	int i;

	/* Free video mode lists */
	for ( i=0; i<SDL_NUMMODES; ++i ) {
		if ( Private->SDL_modelist[i] != NULL ) {
			free(Private->SDL_modelist[i]);
			Private->SDL_modelist[i] = NULL;
		}
	}
	
    if ( _this->screen && (_this->screen->flags & SDL_HWSURFACE) ) {
		/* Direct screen access, no memory buffer */
		_this->screen->pixels = NULL;
	}

    if (_this->screen && _this->screen->pixels) {
        free(_this->screen->pixels);
        _this->screen->pixels = NULL;
    }

    /* Free Epoc resources */

    /* Disable events for me */

 #if defined (__WINS__) || defined(S60) || defined (S80) || defined (S90) || defined (S60V3) || defined(UIQ3)
	delete Private->EPOC_Bitmap;
	Private->EPOC_Bitmap = NULL;
  #endif
	TheBasicAppUi->SetConfig();
	Configuration->WriteToFile();
	delete Private->iOnecallback;
	Private->iOnecallback = NULL;	
}

#ifdef UIQ
#include "SDL_EpocVideo_UIQ.inl"
#elif defined (UIQ3)
#include "SDL_EpocVideo_UIQ3.inl"
#elif defined (S60)
#include "SDL_EpocVideo_S60.inl"
#elif defined (S60V3)
#include "SDL_EpocVideo_S60V3.inl"
#elif defined (S80)
#include "SDL_EpocVideo_S80.inl"
#elif defined (S90)
#include "SDL_EpocVideo_S90.inl"
#endif
} // extern "C"
