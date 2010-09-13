/*
    SDL_Main.cpp
    Symbian OS services for SDL

    Markus Mertama
*/


#include "epoc_sdl.h"

#include"sdlepocapi.h"
#include <e32base.h>
#include <estlib.h>
#include <stdio.h>
#include <badesca.h>

#include "vectorbuffer.h"
#include <w32std.h>
#include <aknappui.h>
#include <aknapp.h>
#include "SDL_epocevents_c.h"
#include "SDL_keysym.h"
#include "dsa.h"
/*
    SDL - Simple DirectMedia Layer

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


*/

#include "sdlenvdata.h"
#include "appsrv.h"

#ifdef SYMBIANC
#include <reent.h>
#endif

//Markus Mertama


extern SDLKey* KeyMap();
extern void ResetKeyMap();


extern EpocSdlEnvData* gEpocEnv;    


void CEventQueue::ConstructL()
    {
    if(EnvUtils::IsOwnThreaded())
    	User::LeaveIfError(iCS.CreateLocal());
    }
    
CEventQueue::~CEventQueue()
    {
    iCS.Close();
    }
    
TInt CEventQueue::Append(const TWsEvent& aEvent)
    {
    Lock();
   	const TInt err = iVector.Append(aEvent);
    Unlock();
    return err;
    }
    
    
TBool CEventQueue::HasData()
    {
    EnvUtils::RunSingleThread();
    return iVector.Size() > 0;
    }



void CEventQueue::Lock()
	{
	if(EnvUtils::IsOwnThreaded())
    	iCS.Wait();
	}
	
void CEventQueue::Unlock()
	{
	if(EnvUtils::IsOwnThreaded())
		iCS.Signal();
	}

const TWsEvent& CEventQueue::Shift()
    {
    const TWsEvent& event =  iVector.Shift();
    return event;
    }


#define MAINFUNC(x) EXPORT_C TMainFunc::TMainFunc(mainfunc##x aFunc){Mem::FillZ(iMainFunc, sizeof(iMainFunc)); iMainFunc[x - 1] = (void*) aFunc;}
    
MAINFUNC(1)
MAINFUNC(2)
MAINFUNC(3)
MAINFUNC(4)
MAINFUNC(5)
MAINFUNC(6)

EXPORT_C TMainFunc::TMainFunc() 
	{
	Mem::FillZ(iMainFunc, sizeof(iMainFunc));
	}
	

const void* TMainFunc::operator[](TInt aIndex) const
	{
	return iMainFunc[aIndex];
	}



_LIT(KSDLMain, "SDLMain");

LOCAL_C int MainL()
    {
    gEpocEnv->iCleanupItems = new (ELeave) CArrayFixFlat<TSdlCleanupItem>(8);
    
    char** envp=0;
     /* !! process exits here if there is "exit()" in main! */
    int ret = 0;
    for(TInt i = 0; i  < 6; i++)
        {
        void* f = (void*) gEpocEnv->iMain[i];
        if(f != NULL)
            {
            switch(i)
                {
                case 0:
                    ret = ((mainfunc1)f)(); 
                    return ret;
                case 3:
                    ((mainfunc1)f)(); 
                    return ret;
                case 1:
                    ret = ((mainfunc2)f)(EpocSdlEnv::Argc(), EpocSdlEnv::Argv());
                    return ret;
                case 4:
                    ((mainfunc2)f)(EpocSdlEnv::Argc(), EpocSdlEnv::Argv());
                    return ret;
                case 2:
                    ret = ((mainfunc3)f)(EpocSdlEnv::Argc(), EpocSdlEnv::Argv(), envp);
                    return ret;
                case 5:
                    ((mainfunc3)f)(EpocSdlEnv::Argc(), EpocSdlEnv::Argv(), envp);
                    return ret;
                }
            }
        }
    PANIC(KErrNotFound);
    return 0;
    }

LOCAL_C TInt DoMain(TAny* /*aParam*/)
    {
   	TBool fbsconnected = EFalse;
   	CTrapCleanup* cleanup = NULL;
  	if(EnvUtils::IsOwnThreaded())
		{  
    	cleanup = CTrapCleanup::New();
   
	
		if(RFbsSession::GetSession() == NULL)
	    	{
	    	PANIC_IF_ERROR(RFbsSession::Connect());
	    	fbsconnected = ETrue;
	    	}
		}
 	gEpocEnv->iAppSrv->Init();	

#ifdef SYMBIANC 
    // Create stdlib 
    _REENT;
#endif

    // Call stdlib main
    int ret = 0;
	if(EnvUtils::IsOwnThreaded())
		{      
    	//completes waiting rendesvous
    	RThread::Rendezvous(KErrNone);
		}
		
    TRAPD(err, err = MainL());
    
    EpocSdlEnv::ObserverEvent(MSDLObserver::EEventMainExit, err);
   
    // Free resources and return
    
  	EpocSdlEnv::CleanupItems();
        
    gEpocEnv->iCleanupItems->Reset();
    delete gEpocEnv->iCleanupItems;
    gEpocEnv->iCleanupItems = NULL;
    
    gEpocEnv->Free(); //free up in thread resources 
    
#ifdef SYMBIANC    
    _cleanup(); //this is normally called at exit, I call it here
#endif

    if(fbsconnected)
        RFbsSession::Disconnect();
    
#ifdef SYMBIANC     
    CloseSTDLIB();
#endif
       
 //   delete as;
   	delete cleanup;	
   	
   	if(gEpocEnv->iCallerStatus != NULL)
   		{
   		User::RequestComplete(gEpocEnv->iCallerStatus, err);
   		return 0;
   		}
   	else
   		{
    	return err == KErrNone ? ret : err;
   		}
    }
    

    
EXPORT_C CSDL::~CSDL()
    {
    RWindow* win = EpocSdlEnv::Window();
    if(win != NULL)
        win->FreePointerMoveBuffer();
    gEpocEnv->Free();
   	gEpocEnv->Delete();
   	
    User::Free(gEpocEnv);
    gEpocEnv = NULL;
    }

EXPORT_C CSDL* CSDL::NewL(TInt aFlags)
    {
    __ASSERT_ALWAYS(gEpocEnv == NULL, PANIC(KErrAlreadyExists));
    gEpocEnv = (EpocSdlEnvData*) User::AllocL(sizeof(EpocSdlEnvData));
    Mem::FillZ(gEpocEnv, sizeof(EpocSdlEnvData));
   
    gEpocEnv->iEpocEnvFlags = aFlags;
    gEpocEnv->iEventQueue = new (ELeave) CEventQueue();
    gEpocEnv->iAppSrv = new (ELeave) CSdlAppServ();
    

    CSDL* sdl = new (ELeave) CSDL();
    
    gEpocEnv->iSdl = sdl;
    
    return sdl;
    }
    

EXPORT_C void CSDL::SetContainerWindowL(RWindow& aWindow, RWsSession& aSession, CWsScreenDevice& aDevice)
    {
    if(gEpocEnv->iDsa == NULL)
    	gEpocEnv->iDsa = CDsa::CreateL(aSession);
    gEpocEnv->iDsa->ConstructL(aWindow, aDevice);
     
    aWindow.DisablePointerMoveBuffer();
    aWindow.FreePointerMoveBuffer();
    
    aWindow.PointerFilter(EPointerFilterDrag,0);
    if(!EnvUtils::IsOwnThreaded())
        {
        aWindow.AllocPointerMoveBuffer(KPointerBufferSize, 0);
        aWindow.EnablePointerMoveBuffer();
        }
    }
        
 
   

   
EXPORT_C TThreadId CSDL::CallMainL(const TMainFunc& aFunc, TRequestStatus* const aStatus, const CDesC8Array* const aArg, TInt aFlags, TInt aStackSize)
    {
    ASSERT(gEpocEnv != NULL);
    gEpocEnv->iMain = aFunc;
    const TBool args = aArg != NULL;
    
    if(gEpocEnv->iArgv != NULL)
    	User::Leave(KErrAlreadyExists);
    
    gEpocEnv->iArgc = args ? aArg->Count() + 1 : 0;
    gEpocEnv->iArgv = (char**) User::AllocL(sizeof(char*) * (gEpocEnv->iArgc + 2));  
      
    TInt k = 0;
    const TFileName processName = RProcess().FileName();
    const TInt len = processName.Length();
    gEpocEnv->iArgv[k] = (char*) User::AllocL(len + 1);
    Mem::Copy(gEpocEnv->iArgv[k], processName.Ptr(), len);
    gEpocEnv->iArgv[k][len] = 0;
      
    for(TInt i =  0; args && (i < aArg->Count()); i++)
        {
        k++;
        const TInt len = aArg->MdcaPoint(i).Length();
        gEpocEnv->iArgv[k] = (char*) User::AllocL(len + 1);
        Mem::Copy(gEpocEnv->iArgv[k], aArg->MdcaPoint(i).Ptr(), len);
        gEpocEnv->iArgv[k][len] = 0;
        }
        
    gEpocEnv->iArgv[k + 1] = NULL;
    
    //For legacy, set to be threaded if resume is requested
    if(aFlags & CSDL::ERequestResume)
    	{ //unless explicitly told not to
    	if(!(gEpocEnv->iEpocEnvFlags & EMainThread))
    		{ //this has a flaw, some functions may ask this function
    		  //before it is set here...bad desing, sorry.
    		  //...maintaining BC is easy, but I think this is minor
    		  //please use CSDL::EOwnTread explicitly in NewL function
    		  //that is so simple fix...
    		  //for me this occured only with mouse move event problems...
    		gEpocEnv->iEpocEnvFlags |= EOwnThread;
    		}
    	}
    
    gEpocEnv->iEventQueue->ConstructL();
    gEpocEnv->iAppSrv->ConstructL();
    
    gEpocEnv->iStackSize = aStackSize;
    
    if(EnvUtils::IsOwnThreaded())
		{  
    	RThread thread;
    	User::LeaveIfError(thread.Create(KSDLMain, DoMain, aStackSize, NULL, NULL));
    
   	 	if(aStatus != NULL)
    		{
    		thread.Logon(*aStatus);
    		}
 
    	gEpocEnv->iId = thread.Id();
    	thread.SetPriority(EPriorityLess);
    	if((aFlags & CSDL::ERequestResume) == 0)
        	{
        	thread.Resume();
       	 	}
    	thread.Close();
		}
	else
		{
		gEpocEnv->iCaller = CIdle::NewL(CActive::EPriorityIdle);
		gEpocEnv->iCaller->Start(TCallBack(DoMain));
		gEpocEnv->iCallerStatus = aStatus;
		if(aStatus != NULL)
			*aStatus = KRequestPending;
		gEpocEnv->iId = RThread().Id();
		RThread().SetPriority(EPriorityLess);
		
		
		}
    return gEpocEnv->iId;
    }
    
EXPORT_C TInt CSDL::AppendWsEvent(const TWsEvent& aEvent)
    {
    return EpocSdlEnv::EventQueue().Append(aEvent);
    }
    
EXPORT_C void CSDL::SDLPanic(const TDesC& aInfo, TInt aErr)
    {
    EpocSdlEnv::PanicMain(aInfo, aErr);
    }
    
EXPORT_C TInt CSDL::GetSDLCode(TInt aScanCode)
    {
    if(aScanCode < 0)
        return MAX_SCANCODE;
    if(aScanCode >= MAX_SCANCODE)
        return -1;
    return KeyMap()[aScanCode];
    }
    
EXPORT_C TInt CSDL::SDLCodesCount() const
	{
	return MAX_SCANCODE;
	}
	
EXPORT_C void CSDL::ResetSDLCodes()
	{
	ResetKeyMap();
	}
    
EXPORT_C void CSDL::SetOrientation(TOrientationMode aMode)
	{
	EpocSdlEnv::SetOrientation(aMode);
	}
    
EXPORT_C TInt CSDL::SetSDLCode(TInt aScanCode, TInt aSDLCode)
    {
    const TInt current = GetSDLCode(aScanCode);
    if(aScanCode >= 0 && aScanCode < MAX_SCANCODE)
        KeyMap()[aScanCode] = static_cast<SDLKey>(aSDLCode);
    return current;
    }


EXPORT_C MSDLObserver* CSDL::Observer()
	{
	return gEpocEnv->iAppSrv->Observer();
	}    
    
EXPORT_C void CSDL::SetObserver(MSDLObserver* aObserver)
	{
	gEpocEnv->iAppSrv->SetObserver(aObserver);
	}
	
EXPORT_C void CSDL::Resume()
	{
	EpocSdlEnv::Resume();
	}
	
EXPORT_C void CSDL::Suspend()
	{
	if(gEpocEnv->iDsa != NULL)
		gEpocEnv->iDsa->DoStop();
	}
	
EXPORT_C CSDL::CSDL()
    {
    }

EXPORT_C void CSDL::DisableKeyBlocking(CAknAppUi& aAppUi) const
	{
	gEpocEnv->iAppUi = &aAppUi;
	EnvUtils::DisableKeyBlocking();
	}

EXPORT_C TInt CSDL::SetBlitter(MBlitter* aBlitter)
	{
	if(gEpocEnv && gEpocEnv->iDsa)
		{
		gEpocEnv->iDsa->SetBlitter(aBlitter);
		return KErrNone;
		}
	return KErrNotReady;
	}
		
	
EXPORT_C TInt CSDL::AppendOverlay(MOverlay& aOverlay, TInt aPriority)
	{
	if(gEpocEnv && gEpocEnv->iDsa)
		{
		return gEpocEnv->iDsa->AppendOverlay(aOverlay, aPriority);
		}
	return KErrNotReady;
	}

EXPORT_C TInt CSDL::RemoveOverlay(MOverlay& aOverlay)	
	{
	if(gEpocEnv && gEpocEnv->iDsa)
		{
		return gEpocEnv->iDsa->RemoveOverlay(aOverlay);
		}
	return KErrNotReady;
	}

EXPORT_C TInt CSDL::RedrawRequest()
	{
	if(gEpocEnv && gEpocEnv->iDsa)
		{
		//const TInt err =  gEpocEnv->iDsa->RedrawRequest();
		TWsEvent event;
		event.SetType(EEventScreenDeviceChanged),
		event.SetTimeNow();
		AppendWsEvent(event);/*
		EpocSdlEnv::PostUpdate();
		return err; */
		}
	return KErrNotReady;
	}

EXPORT_C void CSDL::SetAppOrientation(CAknAppUi& aAppUi, TAppOrientation aAppOrientation) //Set application orientation 
    {
    gEpocEnv->iAppUi = &aAppUi;
    gEpocEnv->iAppOrientation = aAppOrientation;
    EnvUtils::SetAppOrientation(aAppOrientation);
    }

//macro in macro should be wrapped in macro :-)
#define LITERALIZE(x, y) _LIT(x, y)


LOCAL_C TInt GetBuildDate(TDes* aDes)
    {
    LITERALIZE(KDate, __DATE__);
    const TInt len = KDate().Length();
    if(aDes == NULL || aDes->MaxLength() < len)
        return KErrArgument;
    aDes->Copy(KDate);
    return KErrNone;
    }

/*
 * New functions should be added using this instead of introducing new EXPORTS.
 * Reason is that if developer A and developer B freezes new functions independently
 * The S60 library will lost its binary compatiblity. If A and B just implement new 
 * extension IDs (with unique ids) then definition files does not need to update.
 * inline functions can be used to make extensions more clean
 */
EXPORT_C TInt CSDL::Extension_(TUint aExtensionId, TAny*& /*a0*/, TAny* a1)
    {
    switch(aExtensionId)
        {
        case KBuildDate:
         return GetBuildDate(reinterpret_cast<TDes*>(a1));   
        }
    return KErrNotSupported;
    }
	
		


	
