#ifndef SDLSXXAPPH
#define SDLSXXAPPH
#if defined (S60) || defined (S80) || defined (S90) || defined(S60V3)
	#include <e32base.h>
	#include <eikappui.h>
	#include <coecntrl.h>
#ifdef S60V3
class TKeyboardData
	{
public:
	TRect iKeyRect;
	TInt16 iScanCode1;
	TInt16 iScanCode2;	
	};

#endif
	class CEBasicView:public CCoeControl,public MDirectScreenAccess
	{
	public:
		CEBasicView() {
#ifdef S90
				iLetterOffset=0;
#endif
		};
		~CEBasicView();
		void Draw(const TRect& aRect) const;
		void ConstructL();
		void PutBitmap(CFbsBitmap* aBitmap,TPoint aPoint,TRect aRect);
		void UpdateMouseCursor();
		void UpdateScreen();
		void Restart(RDirectScreenAccess::TTerminationReasons aReason);
		void AbortNow(RDirectScreenAccess::TTerminationReasons aReason);
		void SetAppUi(TUid aUid){iAppUid=aUid;};
		void ActGc(){ActivateGc();};
		void DeGc(){DeactivateGc();};	
		void ClearScreen();
		void UpdateClipRect();
#if defined (S60) || defined (S60V3)
		void DrawScreenStatus(CBitmapContext& aGc) const;
#ifdef S60V3
		void DrawCharSelector(CWindowGc& aGc) const;
		void DrawVKeyBoard(CWindowGc& aGc) const;
		void UpdateVirtualKeyboard();
#endif
#endif
		RWindow& Win() const {return Window();};
#ifdef S90
		void UpdateCharSelector(TInt aLetterOffset);
		void DrawCharSelector(CWindowGc& aGc) const;
		TInt iLetterOffset;
		void UpdateVKeyBoard();
		void DrawVKeyBoard(CWindowGc& aGc) const;

#endif

		CDirectScreenAccess* iDsa;
		TBool iDrawingOn;
		TBool iForeground;
		TUid iAppUid;
		TBuf<2> iStatusChar;
#ifdef S60V3
		CArrayFixFlat<TKeyboardData>* iTouchKeys;
		TRect iToggleVKBStateRect;
#endif
	};
#endif
#endif
