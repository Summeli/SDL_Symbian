#include <e32base.h>
#include <eikapp.h>
#include <sdlapp.h>
#include "..\SDLLauncher.hrh"
#if defined (UIQ3)
#include <sdldemo.rsg>
#endif
#if defined (EPOC_AS_APP) && !defined (UIQ3) && !defined (S60V3)
#include "ECompXL.h"
#endif

class CSDLDemoApp : public CSDLApp {
public:
	CSDLDemoApp();
	~CSDLDemoApp();
#if defined (UIQ3)
	/**
	 * Returns the resource id to be used to declare the views supported by this UIQ3 app
	 * @return TInt, resource id
	 */
	TInt ViewResourceId()
		{
		return R_SDL_VIEW_UI_CONFIGURATIONS;
		}
#endif
	TUid AppDllUid() const;
#if defined (EPOC_AS_APP) && !defined (UIQ3) && !defined (S60V3)
	TECompXL    iECompXL;
#endif
};

#ifdef EPOC_AS_APP

// this function is called automatically by the SymbianOS to deliver the new CApaApplication object
#if !defined (UIQ3) && !defined (S60V3)
EXPORT_C 
#endif
CApaApplication* NewApplication() {
	// Return pointer to newly created CQMApp
	return new CSDLDemoApp;
}

#if defined (UIQ3) || defined (S60V3)
#include <eikstart.h>
// E32Main() contains the program's start up code, the entry point for an EXE.
GLDEF_C TInt E32Main() {
 	return EikStart::RunApplication(NewApplication);
}
#endif

#endif // EPOC_AS_APP

#if !defined (UIQ3) && !defined (S60V3)
GLDEF_C  TInt E32Dll(TDllReason) {
	return KErrNone;
}
#endif
#include <stdio.h>
CSDLDemoApp::CSDLDemoApp() {	
}

CSDLDemoApp::~CSDLDemoApp() {
}

TUid CSDLDemoApp::AppDllUid() const
{
	return  TUid::Uid(KSDLAppLauncherUid);
}


