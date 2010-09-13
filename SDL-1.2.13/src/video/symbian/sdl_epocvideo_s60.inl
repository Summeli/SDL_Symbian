extern "C" void EPOC_CalcScaleFactors(_THIS)
{
	TInt loop; // This is the startoffset for a 320 wide line
	TInt lastx=-1;
	TInt pos = 0;
	if(_this->hidden->iSX0Mode & ESX0Portrait)
	{
		for(loop=0;loop<320;loop++)
		{		

			if(_this->hidden->iSX0Mode & ESX0Stretched)
			{
				pos = (0.5+(loop*11)/20);
			}
			else
			{
				pos = (0.5+(loop*3)/4);
			}
			if(loop>0)
			{
				if (pos== lastx)
					i320StepTable[loop-1]=0;
				else 
				{
					i320StepTable[loop-1]=1;
					lastx=pos;
				}
			}
			else
			{
				lastx=pos;
			}
			
			i320StartTable[loop]=pos;
		}
		
		
		for(loop=0;loop<240;loop++)
		{		
			if(!_this->hidden->iIs240Mode)
			{		
				i240StartTable[loop]=loop<<8;
			}
			else
			{
				TInt pos = (0.5+(loop*13)/15);				
				i240StartTable[loop]=pos<<8;
			}
		}	
	}
	else if(_this->hidden->iSX0Mode & ESX0Stretched)
	{
		for(loop=0;loop<320;loop++)
		{
			TInt pos = (0.5+(loop*13)/20);
			if(loop>0)
			{
				if (pos== lastx)
					i320StepTable[loop-1]=0;
				else 
				{
					i320StepTable[loop-1]=1;
					lastx=pos;
				}
			}
			else
			{
				lastx=pos;
			}
			
			i320StartTable[loop]=pos;
		}
		
		if(_this->hidden->iIs240Mode)
		{
			for(loop=0;loop<240;loop++)
			{
				TInt pos = (0.5+(loop*11)/15);				
				if(_this->hidden->iSX0Mode & ESX0Flipped)
				{
					i240StartTable[loop]=175-pos;				
				}
				else
					i240StartTable[loop]=pos;				
			}
		}
		else
		{
			for(loop=0;loop<200;loop++)
			{
				TInt pos=(0.5+(loop*22)/25);			
				if(_this->hidden->iSX0Mode & ESX0Flipped)
				{
					i240StartTable[loop]=175-pos;
				}
				else
					i240StartTable[loop]=pos;
			}
		}
	}
	else 
	{
		for(loop=0;loop<320;loop++)
		{		
			i320StepTable[loop]=1;
			i320StartTable[loop]=loop;
		}
		
		
		for(loop=0;loop<240;loop++)
		{		
			if(!_this->hidden->iIs240Mode)
			{		
				if(_this->hidden->iSX0Mode & ESX0Flipped)
				{
					i240StartTable[loop]=200-loop;
				}
				else
					i240StartTable[loop]=loop;
			}
			else
			{
				TInt pos = (0.5+(loop*5)/6);
				
				if(_this->hidden->iSX0Mode & ESX0Flipped)
				{
					i240StartTable[loop]=200-loop;
				}
				else
					i240StartTable[loop]=pos;
			}
		}	
		
	}
	
}

inline void EPOC_S60PortraitUpdate(_THIS, int numrects, SDL_Rect *rects)
{
	TInt i=0;
	TInt screenW = _this->screen->w;
    TInt screenH = _this->screen->h;
	TBool lockedHeap=EFalse;
	
	TUint16 (*CalcAverage)( TUint16, TUint16 ) = NULL;
	if(Private->EPOC_DisplayMode == EColor4K) 
		CalcAverage = &CalcAverage12Func;
	else
		CalcAverage = &CalcAverage16Func;
	
	
    TInt sourceScanlineLength = screenW;
    if (Private->EPOC_ShrinkedHeight) {  /* simulate 400 pixel height in 200 pixel screen */  
        sourceScanlineLength <<= 1; 
        screenH >>= 1;
    }
	
	TBitmapUtil lock(Private->EPOC_Bitmap);	
	if(!gHeapIsLocked)
	{
		lock.Begin(TPoint(0,0)); // Lock bitmap heap
		gHeapIsLocked=ETrue;
		lockedHeap=ETrue;
	}
    TUint16* screenBuffer = (TUint16*)Private->EPOC_Bitmap->DataAddress();
	
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
		
		TUint16* xStartPos=screenBuffer+i320StartTable[(rect2.x>>(xStep-1))];
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
				bool steppedUplast =true;
				for(TInt x = rect2.x ; x < sourceRectWidth ; x+=xStep) 
				{	if(xStep==1)
				{	
					if(isNotSameCurrentLine && steppedUplast)
					{
						*(xStart) = *bitmapPos;
					}
					else
					{						
						*xStart = CalcAverage(*xStart,*bitmapPos);							
					}
				}
				else
				{										
					TUint16 pixel=CalcAverage(*bitmapPos,*(bitmapPos+1));
					if(isNotSameCurrentLine && steppedUplast)
					{
						*(xStart) = pixel;
					}
					else
					{													
						*xStart = CalcAverage(*xStart,pixel);					
					}							
				}
				xStart=xStart+i320StepTable[x];
				steppedUplast = i320StepTable[x];
				bitmapPos+=xStep;
				}
				lastY = currentY;
				bitmapLine += sourceScanlineLength;
            }
		}else
		{
			TUint8* bitmapLine = (TUint8*)_this->screen->pixels + sourceStartOffset;
			
            for(TInt y = rect2.y ; y < sourceRectHeight ; y++) 
			{				
				currentY= i240StartTable[y];
				TUint8* bitmapPos = bitmapLine; /* 1 byte per pixel */
				xStart=xStartPos+currentY;//currentY;
				isNotSameCurrentLine = (lastY!=currentY);
				bool steppedUplast =true;
				for(TInt x = rect2.x ; x < sourceRectWidth ; x+=xStep) 
				{	if(xStep==1)
				{								
					if(isNotSameCurrentLine && steppedUplast)
					{
						*(xStart) = EPOC_HWPalette_256_to_DisplayMode[*bitmapPos];
					}
					else
					{					
						*xStart = CalcAverage(*xStart,EPOC_HWPalette_256_to_DisplayMode[*bitmapPos]);							
					}
				}
				else
				{										
					TUint16 pixel=CalcAverage(EPOC_HWPalette_256_to_DisplayMode[*bitmapPos],EPOC_HWPalette_256_to_DisplayMode[*bitmapPos+1]);
					if(isNotSameCurrentLine && steppedUplast)
					{
						*(xStart) = pixel;
					}
					else
					{							
						*xStart = CalcAverage(*xStart,pixel);							
					}							
				}
				xStart=xStart+i320StepTable[x];
				steppedUplast = i320StepTable[x];
				bitmapPos+=xStep;
				}
				lastY = currentY;
				bitmapLine += sourceScanlineLength;
            }
		}
	}
	if(lockedHeap)
	{
		lock.End();
		gHeapIsLocked=EFalse;
	}
	if(Private->iNeedFullRedraw)
	{
		Private->iNeedFullRedraw=EFalse;
		TRect realRect(i320StartTable[0],i240StartTable[0]>>8 , i320StartTable[319]+1,(i240StartTable[199]>>8)+1 );
		
		Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,Private->iPutOffset,realRect);
		
	}
	else
		for(TInt loop=0;loop<numrects;loop++)
		{
			SDL_Rect rect =rects[loop];
			TRect realRect(i320StartTable[rect.x],i240StartTable[rect.y]>>8 , i320StartTable[rect.x+rect.w-1]+1,(i240StartTable[rect.y+rect.h-1]>>8)+1 );
			
			Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,Private->iPutOffset+realRect.iTl,realRect);
		}
		Private->iWindowCreator->UpdateScreen();
#ifdef __WINS__
		Private->iEikEnv->WsSession().Flush();
#endif
}

static void EPOC_DirectUpdate(_THIS, int numrects, SDL_Rect *rects)
{
	if(!gViewVisible)
		return;
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
	if(Private->iSX0Mode & ESX0Portrait)
	{
		EPOC_S60PortraitUpdate(_this,numrects,rects);
		return;
	}
	TUint16 (*CalcAverage)( TUint16, TUint16 ) = NULL;
	if(Private->EPOC_DisplayMode == EColor4K) 
		CalcAverage = &CalcAverage12Func;
	else
		CalcAverage = &CalcAverage16Func;
	TInt i;
    
    TInt screenW = _this->screen->w;
    TInt screenH = _this->screen->h;
	TBool lockedHeap=EFalse;
	
    TInt sourceScanlineLength = screenW;
    if (Private->EPOC_ShrinkedHeight) {  /* simulate 400 pixel height in 200 pixel screen */  
        sourceScanlineLength <<= 1; 
        screenH >>= 1;
    }
	
	TBitmapUtil lock(Private->EPOC_Bitmap);	
	if(!gHeapIsLocked)
	{
		lock.Begin(TPoint(0,0)); // Lock bitmap heap
		gHeapIsLocked=ETrue;
		lockedHeap=ETrue;
	}
    TUint16* screenBuffer = (TUint16*)Private->EPOC_Bitmap->DataAddress();
	
	TInt scrWidth=Private->EPOC_ScreenSize.iWidth;
	if(_this->hidden->iSX0Mode & ESX0Flipped)
	{
		scrWidth=-scrWidth;
	}
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
		
		TUint16* xStartPos=NULL;
		
		if(!(_this->hidden->iSX0Mode & ESX0Flipped))
		{
			xStartPos = screenBuffer+(319 - i320StartTable[(rect2.x>>(xStep-1))])* scrWidth;
		}
		else
		{
			// Invert his value tif we are going from right to left instead of lft to right
			xStartPos = screenBuffer+(i320StartTable[(rect2.x>>(xStep-1))])* (-scrWidth);
		}
		
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
				bool steppedUplast =true;
				for(TInt x = rect2.x ; x < sourceRectWidth ; x+=xStep) 
				{	if(xStep==1)
				{	
					if(isNotSameCurrentLine && steppedUplast)
					{
						*(xStart) = *bitmapPos;
					}
					else
					{								
						*xStart = CalcAverage(*xStart,*bitmapPos);								
					}
				}	
				else
				{										
					TUint16 pixel=CalcAverage(*bitmapPos,*(bitmapPos+1));
					if(isNotSameCurrentLine && steppedUplast)
					{
						*(xStart) = pixel;
					}
					else
					{
						*xStart = CalcAverage(*xStart,pixel);
					}							
				}
				xStart=xStart-scrWidth*i320StepTable[x>>(xStep-1)];
				steppedUplast = i320StepTable[x>>(xStep-1)];
				bitmapPos+=xStep;
				}
				lastY = currentY;
				bitmapLine += sourceScanlineLength;
            }
		}else
		{
			TUint8* bitmapLine = (TUint8*)_this->screen->pixels + sourceStartOffset;
			
            for(TInt y = rect2.y ; y < sourceRectHeight ; y++) 
			{				
				currentY= i240StartTable[y];
				TUint8* bitmapPos = bitmapLine; /* 1 byte per pixel */
				xStart=xStartPos+currentY;//currentY;
				isNotSameCurrentLine = (lastY!=currentY);
				bool steppedUplast =true;
				for(TInt x = rect2.x ; x < sourceRectWidth ; x+=xStep) 
				{	if(xStep==1)
				{								
					if(isNotSameCurrentLine && steppedUplast)
					{
						*(xStart) = EPOC_HWPalette_256_to_DisplayMode[*bitmapPos];
					}
					else
					{
						if(*xStart == 0)
						{
							*(xStart) =EPOC_HWPalette_256_to_DisplayMode[*bitmapPos];
						}
						else if(EPOC_HWPalette_256_to_DisplayMode[*bitmapPos] != 0)
						{
							*xStart = CalcAverage(*xStart,EPOC_HWPalette_256_to_DisplayMode[*bitmapPos]);
						}
					}
				}
				else
				{										
					TUint16 pixel=CalcAverage(EPOC_HWPalette_256_to_DisplayMode[*bitmapPos],EPOC_HWPalette_256_to_DisplayMode[*bitmapPos+1]);
					if(isNotSameCurrentLine && steppedUplast)
					{
						*(xStart) = pixel;
					}
					else
					{
						if(*xStart == 0)
						{
							*(xStart) = pixel;
						}
						else if(pixel != 0)
						{
							*xStart = CalcAverage(*xStart,pixel);
						}
					}							
				}
				xStart=xStart-scrWidth*i320StepTable[x>>(xStep-1)];
				steppedUplast = i320StepTable[x>>(xStep-1)];
				
				bitmapPos+=xStep;
				}
				lastY = currentY;
				bitmapLine += sourceScanlineLength;
            }
		}
	}
	if(lockedHeap)
	{
		lock.End();
		gHeapIsLocked=EFalse;
	}
	if(Private->iNeedFullRedraw)
	{
		Private->iNeedFullRedraw=EFalse;
		if(_this->hidden->iSX0Mode & ESX0Flipped)
		{
			TRect realRect(i240StartTable[199],i320StartTable[0] , i240StartTable[0]+1,i320StartTable[319]+1 );
			
			Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,Private->iPutOffset+realRect.iTl,realRect);
		}
		else
		{
			TInt addition = (_this->hidden->iSX0Mode &ESX0Stretched)?48:0;
			TRect realRect(i240StartTable[0],i320StartTable[0]+addition, i240StartTable[199]+1,i320StartTable[319]+1+addition );
			
			Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,Private->iPutOffset,realRect);
		}
	}
	else
		for(TInt loop=0;loop<numrects;loop++)
		{
			SDL_Rect rect =rects[loop];
			if(_this->hidden->iSX0Mode & ESX0Flipped)
			{
				TRect realRect(i240StartTable[rect.y+rect.h-1],i320StartTable[rect.x] , i240StartTable[rect.y]+1,i320StartTable[rect.x+rect.w-1]+1 );
				Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,Private->iPutOffset+realRect.iTl,realRect);
			}
			else
			{
				TInt addition = (_this->hidden->iSX0Mode &ESX0Stretched)?48:0;

				TRect realRect(i240StartTable[rect.y],i320StartTable[(319-(rect.x+rect.w-1))]+addition , i240StartTable[rect.y+rect.h-1]+1,i320StartTable[319-rect.x]+addition+1 );
				
				Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,Private->iPutOffset+realRect.iTl-TPoint(0,addition),realRect);
			}
		}
		Private->iWindowCreator->UpdateScreen();
#ifdef __WINS__
		Private->iEikEnv->WsSession().Flush();
#endif
}