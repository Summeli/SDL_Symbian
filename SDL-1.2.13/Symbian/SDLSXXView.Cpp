#include <stdlib.h>
#include <stdio.h>
#include <eikenv.h>
#include <apgcli.h>
#include <e32keys.h>
#include <eikapp.h>
#include <apgtask.h>
#include <bautils.h>
#include <eikdoc.h>
#include <s32file.h>
#include <string.h>
#include <ctype.h>

#include "SDLSXXView.h"
#include "sdlapp.h"
#include "sdl_main.h"
#include "SDL_epocvideo.h"
#include "SDL_epocevents_c.h"
#include "ebasicapp.h"
#include "SDL_Keyboard.h"
#ifdef S60V3
#include <coefont.h>

const TInt16 KKeyboardConstantsMiniCtrl[18] =
{
		EStdKeyDevice3, EStdKeyDevice3,		 
		EStdKeyEnter, -1,
		EStdKeySpace, -1, 		
		EStdKeyEscape, -1,
		EStdKeyNo, EStdKeyNo,
		EStdKeyDictaphonePlay, EStdKeyDictaphonePlay,
		EStdKeyDictaphoneStop,EStdKeyDictaphoneStop,		
		EStdKeyHash, '#',
		EStdKeyKeyboardExtend, -1
};

const char* KKeyboardSettings[] = 
		{
		"FN", "POR", "FLP",
		"<->", "RND", "1:1",
		"KEY", "CUR", "JOY",
		"*", "MOU", "#"
		};

// Two TInt16 for each virtual key to get those diagonals
const TInt16 KKeyboardConstantsCtrl[51+6] =
{
		EStdKeySpace, -1, EStdKeyUpArrow, -1, EStdKeyEnter, -1, //1
		EStdKeyLeftArrow, -1, EStdKeyDevice3, -1, EStdKeyRightArrow, -1, //2
		EStdKeyEscape, -1, EStdKeyDownArrow, -1, EStdKeyTab, -1, //3
		'1', -1, '2', -1, '3', -1, //4
		'4', -1, '5', -1, '6', -1, //5
		'7', -1, '8', -1, '9', -1, //6
        '*', -1, '0', -1 , EStdKeyHash, -1, //7	        
         EStdKeyNumLock, -1,EStdKeyLeftFunc, EStdKeyRightFunc ,EStdKeyNo, EStdKeyNo,
		 EStdKeyYes, EStdKeyYes,  EStdKeyDictaphoneRecord, -1,EStdKeyDictaphoneStop, -1 //8
};

const TInt16 KKeyboardConstantsKeyb[84] = // 3x8 array
{		
		 'Q', -1, 'W', -1, 'E', -1, 'R', -1, 'T', -1, 'Y', -1, //1
		 'U', -1, 'I', -1, 'O', -1, 'P', -1, '[', -1, ']', -1, //2
		 'A', -1, 'S', -1, 'D', -1, 'F', -1, 'G', -1, 'H', -1, //3
		 'J', -1, 'K', -1, 'L', -1, ',', -1, '.', -1, '*', -1, //4
		 'Z', -1, 'X', -1, 'C', -1, 'V', -1, 'B', -1, 'N', -1, //5
		 'M', -1, EStdKeySpace, -1, EStdKeyEnter, -1, '-', -1, '+', -1,'/', -1, //6
		 EStdKeyCapsLock, -1, '\\', -1, EStdKeyBackspace, -1, //7 
		 EStdKeyKeyboardExtend, -1,  EStdKeyNo, EStdKeyNo, EStdKeyDictaphonePlay, EStdKeyDictaphonePlay //8
};

                                                     
#endif
CEBasicView::~CEBasicView()
{
	iDsa->Cancel();
	delete iDsa;
#ifdef S60V3
	delete iTouchKeys;
#endif
}


void CEBasicView::Restart(RDirectScreenAccess::TTerminationReasons /*aReason*/)
{
	if(iForeground)
	{
	iDsa->Cancel();
	iDsa->StartL();
	UpdateClipRect();
	iDrawingOn=ETrue;
#if defined (S60) || defined (S60V3)
	static_cast<CEBasicAppUi*>(iEikonEnv->EikAppUi())->SetKeyBlockMode(ENoKeyBlock);
#endif
	}
}

#ifdef S60V3
void CEBasicView::UpdateVirtualKeyboard()
	{
	if(current_video != NULL)
		{
		TUint32 pos = 0;		
		iTouchKeys->Reset();
		TKeyboardData keyData;
		if(current_video->hidden->iVKBState == EDisplayControls)
		{
		    if(current_video->hidden->iNumLockOn || current_video->hidden->iControlKeyDown)
		    	{
		    	pos+=2*3*3;
		    	}
		    
			TSize keySize = TSize(current_video->hidden->iKeyboardRect.Size().iWidth/3, current_video->hidden->iKeyboardRect.Size().iHeight/6);
			
			for(TInt y= 0;y<3;y++)
				for(TInt x = 0;x<3;x++)
					{
					keyData.iScanCode1 = KKeyboardConstantsCtrl[pos];
					keyData.iScanCode2 = KKeyboardConstantsCtrl[pos+1];

					keyData.iKeyRect = TRect(TPoint(current_video->hidden->iKeyboardRect.iTl)+TSize(x*keySize.iWidth, y*keySize.iHeight), keySize);
					iTouchKeys->AppendL(keyData);
					pos+=2;
					}
			
		    if(!current_video->hidden->iNumLockOn && !current_video->hidden->iControlKeyDown)
		    	{
				   pos+=2*3*3;
		    	}
		    
			for(TInt y= 3;y<6;y++)
				for(TInt x = 0;x<3;x++)
				{
					keyData.iScanCode1 = KKeyboardConstantsCtrl[pos];
					keyData.iScanCode2 = KKeyboardConstantsCtrl[pos+1];
					
					keyData.iKeyRect = TRect(TPoint(current_video->hidden->iKeyboardRect.iTl)+TSize(x*keySize.iWidth, y*keySize.iHeight), keySize);
					iTouchKeys->AppendL(keyData);
					pos+=2;
				}
		}
		else if (current_video->hidden->iVKBState == EDisplayKeys)
		{
			TSize keySize = TSize(current_video->hidden->iKeyboardRect.Size().iWidth/3, current_video->hidden->iKeyboardRect.Size().iHeight/8);
			for(TInt y= 0;y<6;y++)
				for(TInt x = 0;x<3;x++)
				{
					keyData.iScanCode1 = KKeyboardConstantsKeyb[pos];
					keyData.iScanCode2 = KKeyboardConstantsKeyb[pos+1];
					keyData.iKeyRect = TRect(TPoint(current_video->hidden->iKeyboardRect.iTl)+TSize(x*keySize.iWidth, y*keySize.iHeight), TSize(keySize.iWidth/2, keySize.iHeight));
					iTouchKeys->AppendL(keyData);
					pos+=2;
					keyData.iScanCode1 = KKeyboardConstantsKeyb[pos];
					keyData.iScanCode2 = KKeyboardConstantsKeyb[pos+1];
					keyData.iKeyRect =  TRect(TPoint(current_video->hidden->iKeyboardRect.iTl)+TSize(x*keySize.iWidth+keySize.iWidth/2, y*keySize.iHeight), TSize(keySize.iWidth/2, keySize.iHeight));
					iTouchKeys->AppendL(keyData);
					pos+=2;
				}	

			for(TInt y= 6;y<8;y++)
				for(TInt x = 0;x<3;x++)
				{
					keyData.iScanCode1 = KKeyboardConstantsKeyb[pos];
					keyData.iScanCode2 = KKeyboardConstantsKeyb[pos+1];
					
					keyData.iKeyRect = TRect(TPoint(current_video->hidden->iKeyboardRect.iTl)+TSize(x*keySize.iWidth, y*keySize.iHeight), keySize);
					iTouchKeys->AppendL(keyData);
					pos+=2;
				}
		}
		else // EDisplayMiniControls
			{
			for(TInt key = 0;key<9;key++)
				{
				keyData.iScanCode1 = KKeyboardConstantsMiniCtrl[pos];
				keyData.iScanCode2 = KKeyboardConstantsMiniCtrl[pos+1];

				if(current_video->hidden->iKeyboardRect.Size().iWidth<current_video->hidden->iKeyboardRect.Size().iHeight)
					{
					TSize keySize = TSize(current_video->hidden->iKeyboardRect.Size().iWidth, current_video->hidden->iKeyboardRect.Size().iHeight/9);					        						
					keyData.iKeyRect = TRect(TPoint(current_video->hidden->iKeyboardRect.iTl)+TSize(0, key*keySize.iHeight), keySize);							
					}
				else
					{
					TSize keySize = TSize(current_video->hidden->iKeyboardRect.Size().iWidth/9, current_video->hidden->iKeyboardRect.Size().iHeight);					        
					keyData.iKeyRect = TRect(TPoint(current_video->hidden->iKeyboardRect.iTl)+TSize(key*keySize.iWidth, 0), keySize);									
					}

				iTouchKeys->AppendL(keyData);
				pos+=2;			
				}
			}
			UpdateClipRect();
			ActivateGc();
			CWindowGc& gc = SystemGc();
			DrawVKeyBoard(gc);
			DeactivateGc();
			}
		}
#endif

void CEBasicView::UpdateClipRect()
{
#if defined (S60V3)    
	iDsa->Gc()->CancelClippingRect();
	
	if(current_video != NULL && current_video->hidden != NULL && current_video->hidden->iStretchSize != TSize(0,0))
	{	
		TInt correctForButton = current_video->hidden->iHasMouseOrTouch? current_video->hidden->iActivateButtonSize:0;
		TRect rect = (current_video->hidden->iSX0Mode & ESX0Portrait)?
			         TRect(0,0,current_video->hidden->iStretchSize.iWidth,current_video->hidden->iStretchSize.iHeight):
		             TRect(0,0,current_video->hidden->iStretchSize.iHeight,current_video->hidden->iStretchSize.iWidth);

		if(correctForButton && !current_video->hidden->iVirtualKeyBoardActive && Rect().Height()-rect.Height()<  correctForButton)
		{
			//rect.iBr.iY-=correctForButton;
		}		
		RRegion clipRegion(rect);		

		if(correctForButton != 0 && !current_video->hidden->iVirtualKeyBoardActive)
		{
			iToggleVKBStateRect = TRect(Size().iWidth-correctForButton, Size().iHeight-correctForButton, Size().iWidth, Size().iHeight);

			if(!current_video->hidden->iVirtualKeyBoardActive)
			{
				rect = TRect(0, rect.Height(), rect.Width() - correctForButton, rect.Height()+correctForButton);
				//clipRegion.SubRect(rect);
			}
		}
		else
		{
			iToggleVKBStateRect = TRect();
		}

		iDsa->Gc()->SetClippingRegion(clipRegion);
		clipRegion.Close();
	}   
#endif	

}

void CEBasicView::AbortNow(RDirectScreenAccess::TTerminationReasons /*aReason*/)
{
	iDsa->Cancel();
	iDrawingOn=EFalse;
}
#ifdef S90
const TInt KEscape[48]={0,0,0,2,0,2,4,2,4,2,4,-1,0,4,0,6,0,6,2,6,2,6,2,4,2,4,4,4,4,4,4,7,0,8,0,10,0,10,4,10,2,10,2,8,4,10,4,7};
#endif
void CEBasicView::Draw(const TRect& aRect) const
{

	CWindowGc & gc=SystemGc();
#if defined  (S80) || defined (S90)
	gc.SetBrushColor(KRgbBlack);
	gc.Clear(aRect);
#endif
	if(current_video != NULL)
	{
		current_video->hidden->iNeedFullRedraw=ETrue;
		TRect realRect( TRect(TPoint(0,0), current_video->hidden->EPOC_DisplaySize));
		gc.BitBlt(TPoint(0,0), current_video->hidden->EPOC_Bitmap,realRect);
	}
#if defined (S60) || defined (S60V3)
		DrawScreenStatus(gc);
#ifdef S60V3
		DrawVKeyBoard(gc);
#endif

#elif defined (S90)
	DrawVKeyBoard(gc);
	DrawCharSelector(gc);
#endif // S60, S60V3
	if(current_video != NULL)
		current_video->hidden->iNeedFullRedraw=ETrue;
}

void CEBasicView::ConstructL()
{
	CreateWindowL();
	ActivateL();
	SetFocus(ETrue);
	SetRect(TRect(TPoint(0,0),iEikonEnv->ScreenDevice()->SizeInPixels()));
	iDsa=CDirectScreenAccess::NewL(iEikonEnv->WsSession(),*iEikonEnv->ScreenDevice(),Window(),*this);
	iDsa->StartL();
	iDsa->Gc()->SetClippingRegion(iDsa->DrawingRegion());
	iDrawingOn=ETrue;
	EnableDragEvents();

	Window().PointerFilter(EPointerFilterMove, 0);
	ClaimPointerGrab(ETrue);
	if(Window().DisplayMode() != EColor4K && Window().DisplayMode() != EColor64K)
	{
		Window().SetRequiredDisplayMode(EColor64K); // Try to set 64K color mode
	}
#ifdef S60V3
	iTouchKeys = new (ELeave) CArrayFixFlat<TKeyboardData>(5);	
#endif
}
void CEBasicView::ClearScreen()
{
	if(iDrawingOn)
	{
		iDsa->Gc()->SetBrushColor(KRgbBlack);
		iDsa->Gc()->Clear();
	}
}
#ifdef S60V3
void CEBasicView::DrawVKeyBoard(CWindowGc& aGc) const
{
	if(current_video &&  current_video->hidden)
	{
		if(current_video->hidden->iHasMouseOrTouch && !current_video->hidden->iVirtualKeyBoardActive)
		{
			aGc.SetPenColor(KRgbGray);
			aGc.SetBrushStyle(CGraphicsContext::ENullBrush);
			aGc.SetPenStyle(CGraphicsContext::ESolidPen);			
			aGc.DrawRect(TRect(Rect().iBr-TSize(current_video->hidden->iActivateButtonSize,current_video->hidden->iActivateButtonSize), TSize(current_video->hidden->iActivateButtonSize, current_video->hidden->iActivateButtonSize)));
		}
		else if(current_video->hidden->iKeyboardRect.Size() != TSize(0,0) && iTouchKeys->Count() > 0)
		{	
			TBuf<16> format;
			TCoeFont font(20,TCoeFont::EPlain);
			const CFont* usedFont = &ScreenFont(font);
			TInt baseLine = usedFont->BaselineOffsetInPixels();
			baseLine = usedFont->FontMaxHeight();
			aGc.UseFont(usedFont);
			aGc.SetBrushStyle(CGraphicsContext::ESolidBrush);
			aGc.SetBrushColor(KRgbBlack);   			
			aGc.Clear(current_video->hidden->iKeyboardRect);	
			aGc.SetPenStyle(CGraphicsContext::ESolidPen);
			aGc.SetBrushStyle(CGraphicsContext::ENullBrush);
			for(TInt loop = iTouchKeys->Count()-1; loop>=0;loop--)
			{		
				aGc.SetPenColor(KRgbGray);			
				aGc.DrawRect((*iTouchKeys)[loop].iKeyRect);
				aGc.SetPenColor(KRgbWhite);
				format = KNullDesC();
				if((*iTouchKeys)[loop].iScanCode1 >32 && (*iTouchKeys)[loop].iScanCode1 < 0x60)
				{
					TUint value = (*iTouchKeys)[loop].iScanCode1;
					TChar charac(value);
					
					if(charac.IsAlpha() && !(SDL_GetModState() & KMOD_CAPS) && !current_video->hidden->iShiftOn)
					{
						value = charac.GetLowerCase();
					}
					
					if(charac.IsDigit() && current_video->hidden->iFNModeOn)
					{
						if(charac == '0')
							{
							format = _L("F10");
							}
						else
							{
							format.Format(_L("F%d"), charac.GetNumericValue());
							}
					}
					else
					{
						format.Format(_L("%c"), value);					
					}
				}
				else
				{
					switch((*iTouchKeys)[loop].iScanCode1)
					{
					case EStdKeyLeftFunc:
						format = _L("FNC");		
						break;
					case EStdKeyNumLock:
						format = _L("NUM");
						break;
					case EStdKeyCapsLock:
					case EStdKeyLeftShift:					
						format = _L("CAP");					
						break;
					case EStdKeySpace:
						format = _L("SPC");
						break;
					case EStdKeyEnter:
						format = _L("<-|");
						break;
					case EStdKeyEscape:
						format = _L("ESC");
						break;
					case EStdKeyTab:
						format = _L("TAB");
						break;
					case EStdKeyDevice3:
						if(current_video->hidden->iMouseButtonSet == ENoMouseButton)
							{
							format = _L("LFT");
							}
						else
							{
							format = _L(".");
							}
						break;
					case EStdKeyKeyboardExtend:
						{
						format=_L("<o>");
						}break;
					case '#':
					case EStdKeyHash:
						{
						if(current_video->hidden->iMouseButtonSet == ENoMouseButton)
							{
							format = _L("RGT");
							}
						else
							{
							format = _L("#");
							}
						}break;
					case EStdKeyNo:
						switch(current_video->hidden->iMouseButtonSet)
						{
						case ELeftMouseButton:
							format = _L("*-");
							break;
						case ERightMouseButton:
							format = _L("-*");
							break;
						case ENoMouseButton:
							format = _L("--");
							break;
						default:
							break;
						}
						break;
						case EStdKeyYes:
							aGc.SetPenColor(KRgbGreen);
							format = _L("CTL");
							break;
						case EStdKeyDictaphonePlay:							
							format = _L("CUR");
							break;
						case EStdKeyDictaphoneStop:							
							format = _L("ABC");
							break;
						case EStdKeyDictaphoneRecord:
							format = _L("MIN");
							break;
						case EStdKeyBackspace:
							format = _L("<--");
							break;
						case EStdKeyUpArrow:
							if((*iTouchKeys)[loop].iScanCode2 == -1)
							{
								format = _L("^");
							}
							else if((*iTouchKeys)[loop].iScanCode2 == EStdKeyLeftArrow)
							{
								format = _L("\\");
							}
							else
							{
								format = _L("/");
							}
							break;
						case EStdKeyDownArrow:
							if((*iTouchKeys)[loop].iScanCode2 == -1)
							{
								format = _L("v");
							}
							else if((*iTouchKeys)[loop].iScanCode2 == EStdKeyLeftArrow)
							{
								format = _L("/");
							}
							else
							{
								format = _L("\\");
							}
							break;
						case EStdKeyLeftArrow:
							format = _L("<");
							break;
						case EStdKeyRightArrow:
							format = _L(">");
							break;
					}
				}
				
				aGc.DrawText(format, (*iTouchKeys)[loop].iKeyRect, baseLine, CGraphicsContext::ECenter);
			}	
			/*Gc.SetBrushColor(KRgbGray);
			aGc.SetBrushStyle(CGraphicsContext::ESolidBrush);
			aGc.SetPenStyle(CGraphicsContext::ENullPen);
			aGc.SetDrawMode(CGraphicsContext::EDrawModeWriteAlpha);
			aGc.DrawRect(TRect(Rect().iBr-TSize(current_video->hidden->iActivateButtonSize,current_video->hidden->iActivateButtonSize), TSize(current_video->hidden->iActivateButtonSize, current_video->hidden->iActivateButtonSize)));
			*/
			aGc.DiscardFont();
		}
	}
	
	aGc.SetBrushStyle(CGraphicsContext::ENullBrush);
}
#endif
#ifdef S90
void CEBasicView::DrawCharSelector(CWindowGc& aGc) const
{
	if(current_video && current_video->hidden->iVirtualKeyBoardActive)
	{
		aGc.SetPenColor(KRgbWhite);
		aGc.SetBrushColor(KRgbBlack);
		aGc.UseFont(iEikonEnv->DenseFont());
		TInt fntHeight= iEikonEnv->DenseFont()->HeightInPixels();
		TInt base=Size().iHeight-16;
		TInt charWidth=0;
		for(TInt chr=0;chr<13;chr++)
		{
			TRect charRect( TPoint(192+chr*32,base),TSize(28,16));
			aGc.DrawRect(charRect);
			aGc.SetClippingRect(charRect);
			if(chr+iLetterOffset<KOnScreenChars().Length())
			{
				TBuf<2>chrbuf=KOnScreenChars().Mid(chr+iLetterOffset,1);
				charWidth=iEikonEnv->AnnotationFont()->CharWidthInPixels(chrbuf[0]);
				aGc.DrawText(chrbuf,TPoint(206+chr*32-(charWidth/2),base+15));
			}
			aGc.CancelClippingRect();
		}
		aGc.DiscardFont();
	}
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

void CEBasicView::DrawVKeyBoard(CWindowGc& aGc) const
{
	aGc.SetBrushColor(KRgbBlack);
	aGc.SetBrushStyle(CGraphicsContext::ESolidBrush);
	TInt base=Size().iHeight-16;
	
	aGc.Clear(TRect(TPoint(0,base),TSize(Size().iWidth,16)));
	
	aGc.SetPenColor(KRgbWhite);
	// Extra button
	aGc.DrawRect(TRect(TPoint(0,base),TSize(28,16))); 
	
	if(current_video && current_video->hidden->iVirtualKeyBoardActive)
	{
		aGc.DrawRect(TRect(TPoint(96,base),TSize(28,16))); // outerline
		aGc.DrawLine(TPoint(10+96,base+2),TPoint(10+96,base+6)); 
		aGc.DrawLine(TPoint(10+96,base+5),TPoint(4+96,base+5)); //enter
		
		// Draw Left upper
		aGc.DrawRect(TRect(TPoint(160,base),TSize(28,16)));
		aGc.DrawLine(TPoint(9+160,base+1+3),TPoint(2+160,base+3+3)); // arrow
		aGc.DrawLine(TPoint(2+160,base+3+3),TPoint(9+160,base+6+3));
		
		// Draw right arrow
		aGc.DrawRect(TRect(TPoint(608,base),TSize(28,16)));
		aGc.DrawLine(TPoint(608+6,base+1+3),TPoint(608+13,base+3+3)); // arrow
		aGc.DrawLine(TPoint(608+13,base+3+3),TPoint(608+6,base+6+3));
		
		// Draw delete buton 
		aGc.DrawRect(TRect(TPoint(128,base),TSize(28,16)));
		aGc.DrawLine(TPoint(8+128,base+1+3),TPoint(2+128,base+3+3)); // arrow
		aGc.DrawLine(TPoint(2+128,base+3+3),TPoint(8+128,base+6+3));
		aGc.DrawLine(TPoint(2+128,base+6),TPoint(18+128,base+6));
	}
	
	// Draw cfg button
	aGc.DrawRect(TRect(TPoint(64,base),TSize(28,16)));
	for(TInt loop=0;loop<12;loop++)
	{
		aGc.DrawLine(TPoint(KEscape[loop*4+1]+66,KEscape[loop*4]+base+1),	 TPoint(KEscape[loop*4+3]+66,KEscape[loop*4+2]+base+1)); 
	}
	
	aGc.DrawRect(TRect(TPoint(32,base),TSize(28,16))); 
	
	if(current_video && current_video->hidden->iMouseButtonSet == ERightMouseButton)
	{			
		aGc.SetBrushColor(KRgbWhite);
		aGc.DrawRect(TRect(TPoint(48,base+2),TSize(4,4)));
	}
	else if(current_video && current_video->hidden->iMouseButtonSet == ELeftMouseButton)
	{
		aGc.SetBrushColor(KRgbBlack);
		aGc.DrawRect(TRect(TPoint(48,base+2),TSize(4,4)));		
	}
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
#endif

void CEBasicView::PutBitmap(CFbsBitmap* aBitmap,TPoint aPoint,TRect aRect)
{
	if(iDrawingOn)
	{
		iDsa->Gc()->BitBlt(aPoint,aBitmap,aRect);
		current_video->hidden->iCursorPos = TPoint();
#ifdef S60V3		
		if(current_video->hidden->iHasMouseOrTouch && !current_video->hidden->iVirtualKeyBoardActive && aRect.Intersects(iToggleVKBStateRect))
				{
				iDsa->Gc()->SetPenColor(KRgbGray);
				iDsa->Gc()->SetBrushStyle(CGraphicsContext::ENullBrush);
				iDsa->Gc()->SetPenStyle(CGraphicsContext::ESolidPen);			
				iDsa->Gc()->DrawRect(iToggleVKBStateRect);
				}
#endif
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
		TReal xscale = 1;
		TReal yscale = 1;
		if(current_video->hidden->iSX0Mode & ESX0Stretched)
		{		
			xscale = current_video->hidden->iXScale;
			yscale = current_video->hidden->iYScale;
		}		

		if(current_video->hidden->iSX0Mode & ESX0Portrait)
		{
			pos.iX = xscale*pos.iX;
			pos.iY= yscale*pos.iY;
			
			cursorBitmap = current_video->hidden->iCursor->iCursorPBitmap;
			cursorMask = current_video->hidden->iCursor->iCursorPMask;
		}
		else
		{
			if(current_video->hidden->iSX0Mode & ESX0Flipped)
			{
			cursorBitmap = current_video->hidden->iCursor->iCursorLFBitmap;
			cursorMask = current_video->hidden->iCursor->iCursorLFMask;
			pos.iY = xscale*orgPos.iX;
			pos.iX= current_video->hidden->iStretchSize.iHeight-((yscale*orgPos.iY)+cursorBitmap->SizeInPixels().iWidth);	
			}
			else
			{
			cursorBitmap = current_video->hidden->iCursor->iCursorLBitmap;
			cursorMask = current_video->hidden->iCursor->iCursorLMask;
			pos.iY = current_video->hidden->iStretchSize.iWidth-((xscale*orgPos.iX)+cursorBitmap->SizeInPixels().iHeight);
			pos.iX= yscale*orgPos.iY;	
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

#if defined (S60) || defined (S60V3)
void CEBasicView::DrawScreenStatus(CBitmapContext& aGc) const
{
	if(current_video != NULL)
	{	
		const CFont* fnt = iEikonEnv->DenseFont();	
#ifdef S60V3
		TInt heightInPixels = fnt->HeightInPixels()+fnt->FontMaxDescent();
#else
		TInt heightInPixels = fnt->HeightInPixels()+fnt->DescentInPixels();
#endif
		TInt maxNormalCharWidth = fnt->MaxCharWidthInPixels();
		if(maxNormalCharWidth == 0)
			maxNormalCharWidth = 18;

		TRect box;
		if(current_video->hidden->iSX0Mode  & ESX0Portrait)
		{
			box = TRect(TPoint(0,0),TSize(maxNormalCharWidth,heightInPixels));
		}
		else
		{
			box = TRect(TPoint(0,Size().iHeight-maxNormalCharWidth),TSize(heightInPixels,maxNormalCharWidth));
		}				
		
		if(current_video->hidden->iInputMode ==EKeyboard)
		{
			
			aGc.SetBrushStyle(CGraphicsContext::ESolidBrush);
			aGc.SetBrushColor(KRgbBlack);						
			aGc.Clear(box);
			aGc.UseFont(fnt);
			aGc.SetPenStyle(CGraphicsContext::ESolidPen);		
			aGc.SetPenColor(KRgbWhite);
			TBuf<16> charBuf;
			charBuf.Format(_L("%c"),current_video->hidden->iCurrentChar.operator TUint());
#ifdef S60V3
			if(current_video->hidden->iSX0Mode & ESX0Portrait)
#endif
			{
				aGc.DrawText(charBuf,box,heightInPixels-fnt->DescentInPixels(),CGraphicsContext::ECenter);
			}
#ifdef S60V3				
			else
			{			
				aGc.DrawTextVertical(charBuf,box.iTl+TPoint(heightInPixels-fnt->DescentInPixels(), maxNormalCharWidth-2), ETrue);
			}
#endif
			aGc.DiscardFont();
		}
		else if(current_video->hidden->iInputModeTimer>0)
		{
			aGc.SetBrushStyle(CGraphicsContext::ESolidBrush);
			aGc.SetBrushColor(KRgbBlack);
			aGc.BitBlt(box.iTl, current_video->hidden->EPOC_Bitmap, box);
			aGc.SetBrushStyle(CGraphicsContext::ENullBrush);
			if(--current_video->hidden->iInputModeTimer)
			{
				aGc.UseFont(fnt);
				aGc.SetPenColor(KRgbRed);
				aGc.SetPenStyle(CGraphicsContext::ESolidPen);			
				if(current_video->hidden->iKeyboardModifier)
				{			
					aGc.SetPenColor(KRgbYellow);
				}			
#ifdef S60V3				
				if(!(current_video->hidden->iSX0Mode & ESX0Portrait))
				{				
					aGc.DrawTextVertical(iStatusChar,box.iTl+TPoint(heightInPixels-fnt->DescentInPixels(), maxNormalCharWidth-2), ETrue);
				}
				else
#endif
					aGc.DrawText(iStatusChar,box,heightInPixels-fnt->DescentInPixels(),CGraphicsContext::ECenter);		
				
				aGc.DiscardFont();
			}
			else
			{
				current_video->hidden->iNeedFullRedraw=ETrue;
			}
		}
		
		if(current_video->hidden->iControlKeyDown)
		{
			aGc.UseFont(fnt);
			aGc.SetPenColor(KRgbGreen);
#ifdef S60V3				
			if(!(current_video->hidden->iSX0Mode & ESX0Portrait))
				aGc.DrawTextVertical(_L("CTRL") ,TPoint(64,current_video->hidden->iStretchSize.iWidth-3),ETrue);
			else
#endif
				aGc.DrawText(_L("CTRL") ,TPoint(3,64));
			aGc.DiscardFont();
		}
	}
}
#endif
void CEBasicView::UpdateScreen()
{
	if(iDrawingOn)
	{
#if defined (S60) || defined (S60V3)
		DrawScreenStatus(*iDsa->Gc());		
#endif
#ifdef S90
		if(current_video != NULL)
		{
	
			iDsa->Gc()->SetBrushColor(KRgbWhite);
			iDsa->Gc()->SetPenColor(KRgbBlack);
			if(current_video->hidden->iInputModeTimer>0)
			{
				current_video->hidden->iInputModeTimer--;			
				TBuf<1> inputType =current_video->hidden->iInputMode ==EJoystick?_L("J"):_L("C");
				iDsa->Gc()->UseFont(iEikonEnv->DenseFont());
				iDsa->Gc()->SetPenColor(KRgbRed);
				iDsa->Gc()->DrawText(inputType,TPoint(3,14));

				iDsa->Gc()->DiscardFont();
				if(current_video->hidden->iInputModeTimer==0)
				{
					iDsa->Gc()->SetBrushColor(KRgbBlack);
					iDsa->Gc()->Clear();
					current_video->hidden->iNeedFullRedraw=ETrue;
				}
			}
		}
#endif
		iDsa->ScreenDevice()->Update();
	}

}

#ifndef EPOC_AS_APP
void Execute()
	{
	__UHEAP_MARK;
	CTrapCleanup* cleanup = CTrapCleanup::New();

	// Create a eikenv
	CEikonEnv* eikenv = new CEikonEnv;
	if (!eikenv) 
		{
		return /*KErrNoMemory*/;
		}
	TRAPD(eikErr, eikenv->ConstructL());
	if (eikErr != KErrNone) 
		{
		delete eikenv;
		return /*eikErr*/;
		}
	CEBasicAppUi* appUi = new (ELeave) CEBasicAppUi;
	if (!appUi) 
		{
		delete eikenv;
		return /*KErrNoMemory*/;
		}

	TRAPD(constructErr,appUi->ConstructL());
	
	eikenv->SetAppUi(appUi);	// passing ownership of appUi to coe

	TInt leaveValue = KErrNone;	
	if (leaveValue != KErrNone)
		{
			delete eikenv;
		}
	else
	{
		// now accept request from clients (start the scheduler)
		eikenv->ExecuteD();
	}

	delete cleanup;
	
	__UHEAP_MARKEND;
}

//ARM build

GLDEF_C TInt E32Main()
	{
	Execute();
	return KErrNone;
	}
#if defined(__WINS__)

EXPORT_C TInt WinsMain()
	{
	E32Main();
	return KErrNone;
	}
#endif
#endif

