static void EPOC_DirectUpdate(_THIS, int numrects, SDL_Rect *rects)
{
	if(!gViewVisible)
		return;
#if defined (__WINS__)
	RThread currentThread;
	TInt currentThreadId = currentThread.Id();
	if(currentThreadId != Private->iThreadId)
	{
	
		if(numrects<=20 && !	Private->iNeedUpdate)
		{
			Private->iNumRects = numrects;
			for(TInt loop=0;loop<numrects;loop++)
			{
				Private->iDirtyRects[loop]=rects[loop];
			}
		}
		else // Previous rects set, or >20 then fullscreen
		{
			SDL_Rect fullScreen;
			fullScreen.x = 0;
			fullScreen.y = 0;
			fullScreen.w = _this->screen->w;
			fullScreen.h = _this->screen->h;
			Private->iNumRects =1;
			Private->iDirtyRects[0]=fullScreen;
		}
		Private->iNeedUpdate = ETrue;
		return; // Can't blit in other than main thread
	}
#endif
#if defined (__WINS__)
	TBool lockedHeap=EFalse;

	TBitmapUtil lock(Private->EPOC_Bitmap);	
	if(!gHeapIsLocked)
		{
		    lock.Begin(TPoint(0,0)); // Lock bitmap heap
			gHeapIsLocked=ETrue;
			lockedHeap=ETrue;
		}
    TUint16* destBuffer = (TUint16*)Private->EPOC_Bitmap->DataAddress();
#else
    TUint16* destBuffer = (TUint16*)Private->EPOC_FrameBuffer;
#endif

	if(Private->EPOC_IsFlipped)
	{
		TUint16 (*CalcAverage)( TUint16, TUint16 ) = NULL;
		if(Private->EPOC_DisplayMode == EColor4K) 
			CalcAverage = &CalcAverage12Func;
		else
			CalcAverage = &CalcAverage16Func;
		
		TInt i;
		
		TInt screenW = _this->screen->w;
		TInt screenH = _this->screen->h;
		
		TInt sourceScanlineLength = screenW;
		if (Private->EPOC_ShrinkedHeight) {  /* simulate 400 pixel height in 200 pixel screen */  
			sourceScanlineLength <<= 1; 
			screenH >>= 1;
		}
		
		TUint16* screenBuffer = destBuffer;
		TInt scrWidth=Private->EPOC_ScreenSize.iWidth;
		/* Render the rectangles in the list */
		bool isNotSameCurrentLine = false;
		for ( i=0; i < numrects; ++i ) {
			SDL_Rect rect2;
			const SDL_Rect& currentRect = rects[i];
			rect2.x = currentRect.x;
			rect2.y = currentRect.y;
			rect2.w = currentRect.w;
			rect2.h = currentRect.h;
			
			if (rect2.w <= 0 || rect2.h <= 0) /* sanity check */
				continue;
			
			if (Private->EPOC_ShrinkedHeight) {  /* simulate 400 pixel height in 200 pixel screen */        
				rect2.y >>= 1;
				if (!(rect2.h >>= 1)) 
					rect2.h = 1; // always at least 1 pixel height!
			}
			
			/* All variables are measured in pixels */
			
			/* Check rects validity, i.e. upper and lower bounds */
			TInt maxX = Min(screenW - 1, rect2.x + rect2.w - 1);
			TInt maxY = Min(screenH - 1, rect2.y + rect2.h - 1);
			if (maxX < 0 || maxY < 0) /* sanity check */
				continue;
			
			TInt sourceRectWidth = maxX - rect2.x + 1;
			TInt sourceRectHeight = maxY - rect2.y + 1;
			TInt sourceStartOffset = rect2.x + rect2.y * sourceScanlineLength;
			
			TInt xStep=1;
			if(Private->EPOC_ShrinkedWidth)
			{
				xStep=2;
			}
			TUint16* xStart=NULL;
			TUint16* xStartPos=screenBuffer+(319 - (rect2.x>>(xStep-1))) * scrWidth;
			sourceRectHeight += rect2.y;
			sourceRectWidth  += rect2.x;
			TInt currentY=0;
			TInt lastY = -1;
			if (_this->screen->format->BitsPerPixel == GetBpp(Private->EPOC_DisplayMode)) 
			{ 
				TUint16* bitmapLine = (TUint16*)_this->screen->pixels + sourceStartOffset;
				
				for(TInt y = rect2.y ; y < sourceRectHeight ; y++) 
				{
					currentY= i240StartTable[y];
					TUint16* bitmapPos = bitmapLine; /* 1 byte per pixel */
					xStart=xStartPos+currentY;//currentY;
					isNotSameCurrentLine = (lastY!=currentY);
					for(TInt x = rect2.x ; x < sourceRectWidth ; x+=xStep) 
					{	if(xStep==1)
					{	
						if(isNotSameCurrentLine)
						{
							*(xStart) = *bitmapPos;
						}
						else
						{
							if(*xStart == 0)
							{
								*(xStart) = *bitmapPos;
							}
							else
							{
								*xStart = CalcAverage(*xStart,*bitmapPos);
							}
						}
					}
					else
					{										
						TUint16 pixel=CalcAverage(*bitmapPos,*(bitmapPos+1));
						if(isNotSameCurrentLine)
						{
							*(xStart) = pixel;
						}
						else
						{
							if(*xStart == 0)
							{
								*(xStart) = pixel;
							}
							else
							{
								*xStart = CalcAverage(*xStart,pixel);
							}
						}							
					}
					xStart=xStart-scrWidth;
					bitmapPos+=xStep;
					}
					lastY = currentY;
					bitmapLine += sourceScanlineLength;
				}
			}
			// !! 256 color paletted mode: 8 bpp  --> 12/16 bpp
			else { 
				TUint8* bitmapLine = (TUint8*)_this->screen->pixels + sourceStartOffset;
				for(TInt y = rect2.y ; y < sourceRectHeight ; y++) 
				{
					TUint8* bitmapPos = bitmapLine; /* 1 byte per pixel */
					currentY= i240StartTable[y];
					xStart=xStartPos+ currentY;//currentY;//i240StartTable[y];//currentY;
					/* Convert each pixel from 256 palette to 4k color values */
					TUint16 pixel1 = 0;
					TUint16 pixel2 = 0;
					TUint16 pixel = 0;
					isNotSameCurrentLine = (lastY!=currentY);
					for(TInt x = rect2.x ; x < sourceRectWidth ; x+=xStep) 
					{
						pixel1 = EPOC_HWPalette_256_to_DisplayMode[*bitmapPos];
						if(xStep==1)
						{						
							if(isNotSameCurrentLine)
							{
								*(xStart) = pixel1;
							}
							else
							{
								if(*xStart == 0)
									*(xStart) = pixel1;
								else
									*xStart = CalcAverage(*xStart,pixel1);
							}
						}
						else
						{	pixel2 = EPOC_HWPalette_256_to_DisplayMode[*(bitmapPos+1)];
						pixel  = CalcAverage(pixel1,pixel2);
						if(isNotSameCurrentLine)
						{
							*(xStart) = pixel;
						}
						else
						{
							if(*xStart == 0)
							{
								*(xStart) = pixel;
							}
							else
							{
								*xStart = CalcAverage(*xStart,pixel);
							}
						}
						}
						xStart=xStart-scrWidth;
						bitmapPos+=xStep;               
					}
					lastY = currentY;		
					bitmapLine += sourceScanlineLength;
				}
			}
			
    }    
	
	}
	else // Update in portrait mode
	{
		TUint16 (*CalcAverage)( TUint16, TUint16 ) = NULL;
		if(Private->EPOC_DisplayMode == EColor4K) 
			CalcAverage = &CalcAverage12Func;
		else
			CalcAverage = &CalcAverage16Func;
		
		TInt i;
		
		TInt screenW = _this->screen->w;
		TInt screenH = _this->screen->h;
		
		TInt sourceScanlineLength = screenW;		
		
		TUint16* screenBuffer = destBuffer;
		TInt scrWidth=Private->EPOC_ScreenSize.iWidth;
		/* Render the rectangles in the list */
		for ( i=0; i < numrects; ++i ) {
			SDL_Rect rect2;
			const SDL_Rect& currentRect = rects[i];
			rect2.x = currentRect.x;
			rect2.y = currentRect.y;
			rect2.w = currentRect.w;
			rect2.h = currentRect.h;
			
			if (rect2.w <= 0 || rect2.h <= 0) /* sanity check */
				continue;						
			
			/* All variables are measured in pixels */
			
			/* Check rects validity, i.e. upper and lower bounds */
			TInt maxX = Min(screenW - 1, rect2.x + rect2.w - 1);
			TInt maxY = Min(screenH - 1, rect2.y + rect2.h - 1);
			if (maxX < 0 || maxY < 0) /* sanity check */
				continue;
			
			TInt sourceRectWidth = maxX - rect2.x + 1;
			TInt sourceRectHeight = maxY - rect2.y + 1;
			TInt sourceStartOffset = rect2.x + rect2.y * sourceScanlineLength;
			
			TInt xStep=1;
		
			TUint16* xStart=NULL;
			TUint16* xStartPos=screenBuffer+rect2.x+rect2.y*scrWidth;
			sourceRectHeight += rect2.y;
			sourceRectWidth  += rect2.x;
			TInt currentY=0;
			TInt lastY = -1;
			if (_this->screen->format->BitsPerPixel == GetBpp(Private->EPOC_DisplayMode)) 
			{ 
				TUint16* bitmapLine = (TUint16*)_this->screen->pixels + sourceStartOffset;
				
				for(TInt y = rect2.y ; y < sourceRectHeight ; y++) 
				{
					currentY= y;
					TUint16* bitmapPos = bitmapLine; /* 1 byte per pixel */
					xStart=xStartPos;//currentY;
					for(TInt x = rect2.x ; x < sourceRectWidth ; x+=xStep) 
					{	
											
						*(xStart) = *bitmapPos;						
						xStart=xStart++;
						bitmapPos+=xStep;
					}
					lastY = currentY;
					bitmapLine += sourceScanlineLength;
					xStartPos+=scrWidth;
				}
			}
			// !! 256 color paletted mode: 8 bpp  --> 12/16 bpp
			else { 
				TUint8* bitmapLine = (TUint8*)_this->screen->pixels + sourceStartOffset;
				for(TInt y = rect2.y ; y < sourceRectHeight ; y++) 
				{
					TUint8* bitmapPos = bitmapLine; /* 1 byte per pixel */
					currentY= y;
					xStart=xStartPos;//currentY;//i240StartTable[y];//currentY;
					/* Convert each pixel from 256 palette to 4k color values */				
					for(TInt x = rect2.x ; x < sourceRectWidth ; x+=xStep) 
					{																				
						*(xStart) = EPOC_HWPalette_256_to_DisplayMode[*bitmapPos];																
						xStart++;
						bitmapPos+=xStep;               
					}
					lastY = currentY;		
					bitmapLine += sourceScanlineLength;
					xStartPos+=scrWidth;
				}
			}
			
    }   
	}
#if defined (__WINS__)
	
	if(lockedHeap)
	{
		lock.End();
		gHeapIsLocked=EFalse;
	}
	Private->iWindowCreator->ActGc();
	Private->iEikEnv->SystemGc().BitBlt(TPoint(), Private->EPOC_Bitmap);
	Private->iWindowCreator->DeGc();	
	Private->iEikEnv->WsSession().Flush();
#endif
    /* Update virtual cursor */
    //!!Private->EPOC_WsSession.SetPointerCursorPosition(Private->EPOC_WsSession.PointerCursorPosition());
}