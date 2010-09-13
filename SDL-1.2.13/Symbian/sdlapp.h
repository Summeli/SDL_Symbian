#ifndef SDLAPPH
#define SDLAPPH
#if defined (UIQ2) || defined (UIQ3) || defined (S60V3)
#define EPOC_AS_APP
#endif
#if defined (__AVKON_ELAF__)
#include <aknapp.h>
class CSDLApp:public CAknApplication
#elif defined (UIQ) || defined(UIQ3)
#include <qikapplication.h>
class CSDLApp:public CQikApplication
#else
#include <eikapp.h>
class CSDLApp:public CEikApplication
#endif
{
public:
	IMPORT_C CSDLApp();
	IMPORT_C  ~CSDLApp();
	IMPORT_C CApaDocument*		CreateDocumentL();
	IMPORT_C static TFileName	GetExecutablePath();
	IMPORT_C  static char*		GetExecutablePathCStr();

	/**
	 * This has a default empty implementation.
	 * Is called just before SDL_Main is called to allow init of system vars
	 */
	IMPORT_C virtual	void PreInitializeAppL();
	
	/**
	 * This has a default empty implementation.
	 * This should call SDL Main and this is not located in the SDL main
	 * param argc
	 * param argv
	 */
	IMPORT_C virtual void LaunchAppL(int argc, char** argv);
	
	/**
	 * This has a default empty implementation.
	 * Is called to get the name of the SDL data folder for the app 
	 * The name is appended to either c:\data or c:\shared
	 * If a zero length string is returned then application name 
	 * will be used for default datapath
	 * Max length is 64 chars
	 */
	IMPORT_C virtual	void GetDataFolder(TDes& aDataFolder);			
#if defined(UIQ3)
	/**
			 * Returns the resource id to be used to declare the views supported by this UIQ3 app
			 * @return TInt, resource id
			 */
	virtual TInt		ViewResourceId() = 0;
#endif			
};
#endif

