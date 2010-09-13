#ifndef EBasicAPP
#define EBasicAPP

#include <coecntrl.h>
#include <coeview.h>
#include <eikapp.h>
#include <e32base.h>
#include "IniFile.h"


_LIT(KOnScreenChars," .ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz");
const TInt KNoOnScreenKeys = 12;
const TInt KScreenKeysOffset = 112;
const TInt KScreenKeySize = 16;

// so we can access the config from anywhere
#define Configuration	static_cast<CEBasicAppUi*>(CEikonEnv::Static()->EikAppUi())->Config()
#define TheBasicAppUi	static_cast<CEBasicAppUi*>(CEikonEnv::Static()->EikAppUi())
struct SDL_VideoDevice;
struct SDL_AudioDevice;

#if defined  (UIQ) || defined(UIQ3)
#include <qikdocument.h>
class CEBasicDoc:public CQikDocument
#elif defined (S60) || defined (S60V3)
#include <akndoc.h>
class CEBasicDoc:public CAknDocument
#else
#include <eikdoc.h>
class CEBasicDoc:public CEikDocument
#endif
{
public:	
	~CEBasicDoc();
	virtual CEikAppUi* CreateAppUiL();
	CEBasicDoc(CEikApplication& aApp);
	void SaveL(MSaveObserver::TSaveType aSaveType);

private:
	void ConstructL();
};

class CEBasicView;
#if defined  (UIQ) || defined(UIQ3)
#include <qikappui.h>
class CEBasicAppUi:public CQikAppUi
#elif defined (S60) || defined (S60V3)
#include <aknappUI.h>
class CEBasicAppUi:public CAknAppUi
#else
#include <eikappui.h>
class CEBasicAppUi:public CEikAppUi
#endif
{
	public:
		CEBasicAppUi();
		~CEBasicAppUi();
		void ConstructL();
		void DoExit();
		void HandleCommandL(TInt aCommand);
		CEBasicView* View(){return iView;};
		void DoExitNext(){iExitNext=ETrue;};
		void HandleWsEventL(const TWsEvent& aEvent, CCoeControl* aDestination);
		static TInt StaticSDLStartL(TAny* aAppUi);
		static void symbian_at_exit();
		void SDLStartL();
		void DrawView();
		virtual TKeyResponse HandleKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);
		void HandleForegroundEventL(TBool aForeground);
		void SetConfig();
		void GetConfig(SDL_VideoDevice* aDevice);
		CIniFile* Config() { return iConfig; }
		TFileName GetExecutablePath() { return iExecutablePath; }
		char* GetExecutablePathCStr() { return (char*)&iExecutablePathCStr; }
#ifdef UIQ3
		void LaunchSettingsDialogL();
#endif
#if defined (S60) || defined (S60V3) || defined (UIQ3)
		TKeyResponse HandleControlKeyKeysL(const TKeyEvent& aKeyEvent,TEventCode aType);
		void HandleScreenDeviceChangedL();
		void UpdateInputState();
#if defined (S60) || defined (S60V3)
		void SetKeyBlockMode(TAknKeyBlockMode aMode);
#endif
#endif
		void ReleaseMultitapKeysL();
		void UpdateAndRedrawScreenL();
	protected:
		void SetExecutablePathL();
		TKeyResponse HandleMultiTapInput(const TKeyEvent& aKeyEvent,TEventCode aType);
		void UpdateScreenOffset(TInt aKeyCode);
#ifdef UIQ	
		/** 
		 * Capture special Application keys
		 */
		void CaptureKeysL();
	
		/**
		 *Cancels any captured Keys
		 */
		void CancelCaptureKeys();
		TInt32 iCapKey1;
		TInt32 iCapKey2;
		TInt32 iCapKey3;
		TInt32 iCapKey4;
		TInt32 iCapKey5;
	
		TInt32 iCapKey1b;
		TInt32 iCapKey2b;
		TInt32 iCapKey3b;
		TInt32 iCapKey4b;
		TInt32 iCapKey5b;
#endif
	private:
		TBool iExitNext;
		CEBasicView* iView;
		CAsyncCallBack iSDLstart;
		TInt iLastChar;
		TBool iWasControlEvent;
		
		CIniFile* iConfig;
		TFileName iExecutablePath;
		char iExecutablePathCStr[KMaxFileName];
		char* iExecutableParams[1];
		static TInt StaticMultitapTimeoutL(TAny* aAppUi);
		void MultitapTimeoutL();
	
		CPeriodic* iMultitapTimer;	
		RArray<TChar>		iReleaseKeys;
		TBool iTapTimerReleased;
		TBool iIntialTap;
};



#if defined (UIQ) || defined (UIQ3)
#ifdef UIQ
class CEBasicView:public CCoeControl,public MCoeView
#else
#include <qikviewbase.h>
class CEBasicView:public CQikViewBase,public MDirectScreenAccess
#endif
{
public:
#ifdef UIQ3
	CEBasicView(CQikAppUi& aAppUi);
#else
	CEBasicView();
#endif
	~CEBasicView();
	void SetAppUi(TUid aUid){iAppUid=aUid;};
	void ActGc(){ActivateGc();};
	void DeGc(){DeactivateGc();};
	RWindow& Win() const {return Window();};
	void ConstructL(const TRect& aRect);
	void Draw(const TRect& aRect) const;
	TCoeInputCapabilities InputCapabilities() const;
	void SetInputState(TBool aOnOff);
	TVwsViewId ViewId() const;
	virtual void ViewActivatedL(const TVwsViewId& aPrevViewId,TUid aCustomMessageId,const TDesC8& aCustomMessage);
	virtual void ViewDeactivated();
	void UpdateCharSelector(TInt aLetterOffset);
	void UpdateVKeyBoard();
	void DrawVKeyBoard(CWindowGc& aGc) const;
	void DrawCharSelector(CWindowGc& aGc) const;
	void SetCurrentMultiTapKey(TInt aKey);
	TInt iXOffSet;
	TBool iInputState;
	TDisplayMode iDispMode;         ///< Display mode for screen.
	TUid iAppUid;
	TInt iLetterOffset;
#ifdef UIQ3
	void Restart(RDirectScreenAccess::TTerminationReasons aReason);
	void UpdateClipRect();
	void AbortNow(RDirectScreenAccess::TTerminationReasons aReason);
	void ClearScreen();
	void PutBitmap(CFbsBitmap* aBitmap,TPoint aPoint,TRect aRect);
	void UpdateScreen();
	void UpdateMouseCursor();

	TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);
	/**
	Inherited from CQikViewBase and called upon by the UI Framework. 
	It creates the views with command from resource.
	*/
	void ViewConstructL();

	CDirectScreenAccess* iDsa;
	TBool iDrawingOn;
#endif
	void SizeChanged();
	TRect iWantedRect;
	TBool iForeground;
	TPoint iBitOffset;
};
#endif

#endif

