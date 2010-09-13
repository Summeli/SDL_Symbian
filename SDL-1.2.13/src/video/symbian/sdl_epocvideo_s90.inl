
// This is the max size
void EPOC_CalcStretchFactors(_THIS,TSize aTargetSize)
{
  TReal xStretch=1;
  TReal yStretch=1;
  if(aTargetSize.iWidth<=320)
  {
  xStretch = 1.5;
  }

  if(aTargetSize.iHeight<=240)
  {
  yStretch = 1.26;
  }

  if(aTargetSize.iHeight<=200)
  {
  yStretch = 1.5;
  }

  for(TInt y=0;y<=aTargetSize.iHeight;y++)
  {
	  gScaleYPos[y]=(0.5+y*yStretch);
	  if(y>0)
	  {
		  gScaleYStep[y-1]=(gScaleYPos[y]-gScaleYPos[y-1])* Private->EPOC_ScreenSize.iWidth;
	  }
  }

   for(TInt x=0;x<=aTargetSize.iWidth;x++)
	  {
		  gScaleXPos[x]=(0.5+x*xStretch);
		  if(x>0)
			gScaleStep[x-1]=(gScaleXPos[x]-gScaleXPos[x-1]);
	  }

   Private->EPOC_ScreenOffset = (Private->EPOC_ScreenSize.iWidth-( (TInt)(xStretch*aTargetSize.iWidth)))/2;
   Private->iXScale = xStretch;
   Private->iYScale = yStretch;
   Private->iStretchSize = TSize(xStretch*aTargetSize.iWidth,yStretch*aTargetSize.iHeight);
}

inline void EPOC_S90StretchedUpdate(_THIS, int numrects, SDL_Rect *rects)
{
	TBool lockedHeap=EFalse;
	TUint16 (*CalcAverage)( TUint16, TUint16 ) = NULL;
	if(Private->EPOC_DisplayMode == EColor4K) 
		CalcAverage = &CalcAverage12;
	else
		CalcAverage = &CalcAverage16;

	TInt i;
    TInt fixedOffset = Private->EPOC_ScreenOffset;   
    TInt screenW = _this->screen->w;
    TInt screenH = _this->screen->h;
    TInt sourceScanlineLength = screenW;

 TInt targetScanlineLength = Private->EPOC_ScreenSize.iWidth;
	TBitmapUtil lock(Private->EPOC_Bitmap);	
	if(!gHeapIsLocked)
	{
    lock.Begin(TPoint(0,0)); // Lock bitmap heap
	lockedHeap = ETrue;
	}
    TUint16* screenBuffer = (TUint16*)Private->EPOC_Bitmap->DataAddress();

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
        maxY = Min(maxY, 239); 

        TInt sourceRectWidth = maxX - rect2.x + 1;
        TInt sourceRectHeight = maxY - rect2.y + 1;
        TInt sourceStartOffset = rect2.x + rect2.y * sourceScanlineLength;
        TInt targetStartOffset = fixedOffset + gScaleXPos[rect2.x] + gScaleYPos[i240StartTable[rect2.y]] * targetScanlineLength;   
        
        // !! Nokia9210 native mode: native bpp --> native bpp
		sourceRectHeight += rect2.y;
		TBool interpolate = EFalse;
        if (_this->screen->format->BitsPerPixel == GetBpp(Private->EPOC_DisplayMode)) { 

	        TUint16* bitmapLine = (TUint16*)_this->screen->pixels + sourceStartOffset;
            TUint16* screenMemory = screenBuffer + targetStartOffset;
		    TUint16* polateMem1= NULL;
			TUint16* polateMem2= NULL;
			TUint16* polateTrg = NULL;
            for(TInt y = rect2.y ; y < sourceRectHeight ; y++) {
                __ASSERT_DEBUG(screenMemory < (screenBuffer 
                    + Private->EPOC_ScreenSize.iWidth * Private->EPOC_ScreenSize.iHeight), 
                    User::Panic(_L("SDL"), KErrCorrupt));
                __ASSERT_DEBUG(screenMemory >= screenBuffer, 
                    User::Panic(_L("SDL"), KErrCorrupt));
                __ASSERT_DEBUG(bitmapLine < ((TUint16*)_this->screen->pixels + 
                    + (_this->screen->w * _this->screen->h)), 
                    User::Panic(_L("SDL"), KErrCorrupt));
                __ASSERT_DEBUG(bitmapLine >=  (TUint16*)_this->screen->pixels, 
                    User::Panic(_L("SDL"), KErrCorrupt));
				if(interpolate){
					TUint16* screenPtr = screenMemory;
					TUint16* bitmapPtr = bitmapLine;
					for(TInt index =0;index<sourceRectWidth;index++){
					if(*screenPtr == 0)
						*screenPtr = *bitmapPtr;
					else
						*screenPtr = CalcAverage(*screenPtr,*bitmapPtr);
					bitmapPtr++;
					screenPtr++;
					}
				}
				else{
					TUint16* screenPtr = screenMemory;
    				TUint16* bitmapPtr = bitmapLine;
					TInt step = 0;
					for(TInt index =0;index<sourceRectWidth;index++){
					step =gScaleStep[index+rect2.x];
					*screenPtr = *bitmapPtr;
					if(step>1)
					{
					 *(screenPtr+1) = CalcAverage(*bitmapPtr,*(bitmapPtr+1));
					}

					bitmapPtr++;
					screenPtr+=step;
					}
		//		Mem::Copy(screenMemory, bitmapLine, sourceRectWidthInBytes);
				}
		        bitmapLine += sourceScanlineLength;
				if(y>rect2.y){
				if(i240StartTable[y] != i240StartTable[y-1]){

					if(gScaleYStep[y-1]>Private->EPOC_ScreenSize.iWidth) // more than one line of screen // last line  skip was bigger than one line
						{
						polateTrg = screenMemory-Private->EPOC_ScreenSize.iWidth;
						polateMem1 = screenMemory;
						polateMem2 = screenMemory-(Private->EPOC_ScreenSize.iWidth*2);
						TInt scaledSourceRectWidth = gScaleXPos[sourceRectWidth];
						for(TInt index =0;index<scaledSourceRectWidth;index++){
								*polateTrg =  CalcAverage(*polateMem1,*polateMem2);
								polateTrg++;
								polateMem1++;
								polateMem2++;
							}
						}

				       	screenMemory += gScaleYStep[y];
						interpolate = EFalse;
					}
					else
						interpolate = ETrue;;
				}
				else
				{					
					screenMemory += gScaleYStep[y];;
				}
			}
        }
        // !! 256 color paletted mode: 8 bpp  --> xx bpp
        else { 
	        TUint8* bitmapLine = (TUint8*)_this->screen->pixels + sourceStartOffset;
            TUint16* screenMemory = screenBuffer + targetStartOffset;
            for(TInt y = rect2.y ; y < sourceRectHeight ; y++) {
                TUint8* bitmapPos = bitmapLine; /* 1 byte per pixel */
                TUint16* screenMemoryLinePos = screenMemory; /* 2 bytes per pixel */
                /* Convert each pixel from 256 palette to 4k color values */
                for(TInt x = 0 ; x < sourceRectWidth ; x++) {
                    __ASSERT_DEBUG(screenMemoryLinePos < (screenBuffer 
                        + (Private->EPOC_ScreenSize.iWidth * Private->EPOC_ScreenSize.iHeight)), 
                        User::Panic(_L("SDL"), KErrCorrupt));
                    __ASSERT_DEBUG(screenMemoryLinePos >= screenBuffer, 
                        User::Panic(_L("SDL"), KErrCorrupt));
                    __ASSERT_DEBUG(bitmapPos < ((TUint8*)_this->screen->pixels + 
                        + (_this->screen->w * _this->screen->h)), 
                        User::Panic(_L("SDL"), KErrCorrupt));
                    __ASSERT_DEBUG(bitmapPos >= (TUint8*)_this->screen->pixels, 
                        User::Panic(_L("SDL"), KErrCorrupt));
					if(interpolate)
					{
					if(*screenMemoryLinePos == 0)
						*screenMemoryLinePos =  EPOC_HWPalette_256_to_DisplayMode[*bitmapPos];
					else
						*screenMemoryLinePos = CalcAverage(*screenMemoryLinePos,EPOC_HWPalette_256_to_DisplayMode[*bitmapPos]);
					}
					else
						*screenMemoryLinePos = EPOC_HWPalette_256_to_DisplayMode[*bitmapPos];

                    bitmapPos++;
                    screenMemoryLinePos++;
                }
				
				if(y>rect2.y){
					if(i240StartTable[y] != i240StartTable[y-1]){
						screenMemory += gScaleYStep[y];
						interpolate = EFalse;
					}
					else
						interpolate = ETrue;
					
				}
				else
					screenMemory += gScaleYStep[y];

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
		Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,TPoint(Private->EPOC_ScreenOffset,0),TRect(Private->EPOC_ScreenOffset,0,Private->EPOC_ScreenOffset+Private->iStretchSize.iWidth,Private->iStretchSize.iHeight));
	}
	else
		for(TInt loop=0;loop<numrects;loop++)
		{
			SDL_Rect rect =rects[loop];
			rect.w = gScaleXPos[rect.w];
			rect.h = gScaleYPos[rect.h];
			rect.x = gScaleXPos[rect.x];
			rect.y = gScaleYPos[i240StartTable[rect.y]];
			rect.w = rect.w;
			rect.h = rect.h;
			TRect realRect(rect.x+Private->EPOC_ScreenOffset,rect.y , Private->EPOC_ScreenOffset+rect.x+rect.w,rect.y+rect.h);

			Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,realRect.iTl,realRect);
		}
	Private->iWindowCreator->UpdateScreen();
#if defined (__WINS__)
	Private->iEikEnv->WsSession().Flush();
#endif
}

static void EPOC_DirectUpdate(_THIS, int numrects, SDL_Rect *rects)
{
	if(!gViewVisible)
		return;
	RThread currentThread;
	TInt currentThreadId = currentThread.Id();
	TBool lockedHeap=EFalse;

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

	if((Private->iSX0Mode & ESX0Stretched && !Private->EPOC_ShrinkedHeight && _this->screen->w<400))
	{
		EPOC_S90StretchedUpdate(_this,numrects,rects);
		return;
	}

	TUint16 (*CalcAverage)( TUint16, TUint16 ) = NULL;
	if(Private->EPOC_DisplayMode == EColor4K) 
		CalcAverage = &CalcAverage12;
	else
		CalcAverage = &CalcAverage16;

	TInt i;
    TInt sourceNumBytesPerPixel = ((_this->screen->format->BitsPerPixel-1)>>3) + 1;   
    TInt fixedOffset = Private->EPOC_ScreenOffset;   
    TInt screenW = _this->screen->w;
    TInt screenH = _this->screen->h;
    TInt sourceScanlineLength = screenW;
    if (Private->EPOC_ShrinkedHeight) {  /* simulate 400 pixel height in 200 pixel screen */  
        sourceScanlineLength <<= 1; 
        screenH >>= 1;
    }

    TInt targetScanlineLength = Private->EPOC_ScreenSize.iWidth;
	TBitmapUtil lock(Private->EPOC_Bitmap);	
	if(!gHeapIsLocked)
	{
    lock.Begin(TPoint(0,0)); // Lock bitmap heap
	lockedHeap = ETrue;
	}
    TUint16* screenBuffer = (TUint16*)Private->EPOC_Bitmap->DataAddress();

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
        maxY = Min(maxY, 239); 

        TInt sourceRectWidth = maxX - rect2.x + 1;
        TInt sourceRectWidthInBytes = sourceRectWidth * sourceNumBytesPerPixel;
        TInt sourceRectHeight = maxY - rect2.y + 1;
        TInt sourceStartOffset = rect2.x + rect2.y * sourceScanlineLength;
        TInt targetStartOffset = fixedOffset + rect2.x + i240StartTable[rect2.y] * targetScanlineLength;   
        
        // !! Nokia9210 native mode: native bpp --> native bpp
		sourceRectHeight += rect2.y;
		TBool interpolate = EFalse;
        if (_this->screen->format->BitsPerPixel == GetBpp(Private->EPOC_DisplayMode)) { 

	        TUint16* bitmapLine = (TUint16*)_this->screen->pixels + sourceStartOffset;
            TUint16* screenMemory = screenBuffer + targetStartOffset;

            for(TInt y = rect2.y ; y < sourceRectHeight ; y++) {
                __ASSERT_DEBUG(screenMemory < (screenBuffer 
                    + Private->EPOC_ScreenSize.iWidth * Private->EPOC_ScreenSize.iHeight), 
                    User::Panic(_L("SDL"), KErrCorrupt));
                __ASSERT_DEBUG(screenMemory >= screenBuffer, 
                    User::Panic(_L("SDL"), KErrCorrupt));
                __ASSERT_DEBUG(bitmapLine < ((TUint16*)_this->screen->pixels + 
                    + (_this->screen->w * _this->screen->h)), 
                    User::Panic(_L("SDL"), KErrCorrupt));
                __ASSERT_DEBUG(bitmapLine >=  (TUint16*)_this->screen->pixels, 
                    User::Panic(_L("SDL"), KErrCorrupt));
				if(interpolate){
					TUint16* screenPtr = screenMemory;
					TUint16* bitmapPtr = bitmapLine;
					for(TInt index =0;index<sourceRectWidth;index++){
					if(*screenPtr == 0)
						*screenPtr = *bitmapPtr;
					else
						*screenPtr = CalcAverage(*screenPtr,*bitmapPtr);
					bitmapPtr++;
					screenPtr++;
					}
				}
				else{
				Mem::Copy(screenMemory, bitmapLine, sourceRectWidthInBytes);
				}
		        bitmapLine += sourceScanlineLength;
				if(y>rect2.y){
				if(i240StartTable[y] != i240StartTable[y-1]){
				        screenMemory += targetScanlineLength;
						interpolate = EFalse;
					}
					else
						interpolate = ETrue;;
				}
				else
					screenMemory += targetScanlineLength;
			}
        }
        // !! 256 color paletted mode: 8 bpp  --> xx bpp
        else { 
	        TUint8* bitmapLine = (TUint8*)_this->screen->pixels + sourceStartOffset;
            TUint16* screenMemory = screenBuffer + targetStartOffset;
            for(TInt y = rect2.y ; y < sourceRectHeight ; y++) {
                TUint8* bitmapPos = bitmapLine; /* 1 byte per pixel */
                TUint16* screenMemoryLinePos = screenMemory; /* 2 bytes per pixel */
                /* Convert each pixel from 256 palette to 4k color values */
                for(TInt x = 0 ; x < sourceRectWidth ; x++) {
                    __ASSERT_DEBUG(screenMemoryLinePos < (screenBuffer 
                        + (Private->EPOC_ScreenSize.iWidth * Private->EPOC_ScreenSize.iHeight)), 
                        User::Panic(_L("SDL"), KErrCorrupt));
                    __ASSERT_DEBUG(screenMemoryLinePos >= screenBuffer, 
                        User::Panic(_L("SDL"), KErrCorrupt));
                    __ASSERT_DEBUG(bitmapPos < ((TUint8*)_this->screen->pixels + 
                        + (_this->screen->w * _this->screen->h)), 
                        User::Panic(_L("SDL"), KErrCorrupt));
                    __ASSERT_DEBUG(bitmapPos >= (TUint8*)_this->screen->pixels, 
                        User::Panic(_L("SDL"), KErrCorrupt));
					if(interpolate)
					{
					if(*screenMemoryLinePos == 0)
						*screenMemoryLinePos =  EPOC_HWPalette_256_to_DisplayMode[*bitmapPos];
					else
						*screenMemoryLinePos = CalcAverage(*screenMemoryLinePos,EPOC_HWPalette_256_to_DisplayMode[*bitmapPos]);
					}
					else
						*screenMemoryLinePos = EPOC_HWPalette_256_to_DisplayMode[*bitmapPos];

                    bitmapPos++;
                    screenMemoryLinePos++;
                }
				
				if(y>rect2.y){
					if(i240StartTable[y] != i240StartTable[y-1]){
						screenMemory += targetScanlineLength;
						interpolate = EFalse;
					}
					else
						interpolate = ETrue;
					
				}
				else
					screenMemory += targetScanlineLength;

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
		Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,TPoint(Private->EPOC_ScreenOffset,0),TRect(Private->EPOC_ScreenOffset,0,Private->EPOC_ScreenOffset+current_video->screen->w,current_video->screen->h));
	}
	else
		for(TInt loop=0;loop<numrects;loop++)
		{
			SDL_Rect rect =rects[loop];
			TRect realRect(rect.x+Private->EPOC_ScreenOffset,i240StartTable[rect.y] , Private->EPOC_ScreenOffset+rect.x+rect.w,i240StartTable[rect.y+rect.h-1]+1 );

			Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,realRect.iTl,realRect);
		}
	Private->iWindowCreator->UpdateScreen();
#if defined (__WINS__)
	Private->iEikEnv->WsSession().Flush();
#endif
}
