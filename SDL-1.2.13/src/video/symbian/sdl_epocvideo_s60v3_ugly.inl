inline void EPOC_S60PortraitStretchUglyUpdate(_THIS, int numrects, SDL_Rect *rects)
{
	TBool lockedHeap=EFalse;
	
	TInt i;  
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
        TInt targetStartOffset = gScaleXPos[rect2.x] + (gScaleYPos[rect2.y] * targetScanlineLength);   
        
		sourceRectHeight += rect2.y;
		TBool interpolate = EFalse;
		
        if (_this->screen->format->BitsPerPixel == GetBpp(Private->EPOC_DisplayMode)) { 
			
			TUint16* bitmapLine = (TUint16*)_this->screen->pixels + sourceStartOffset;
            TUint16* screenMemory = screenBuffer + targetStartOffset;
			TUint16* polateMem1= NULL;			
			TUint16* polateTrg = NULL;
			TUint16* screenPtr = NULL;
			TUint16* bitmapPtr = NULL;
			
            for(TInt y = rect2.y ; y < sourceRectHeight ; y++) {            
				if(!interpolate)									
				{
					screenPtr = screenMemory;
					bitmapPtr = bitmapLine;
					TInt step = 0;
					for(TInt index =0;index<sourceRectWidth;index++)
					{																											
						*screenPtr = *bitmapPtr;						
						step =gScaleStep[index+rect2.x];
						
						if(step>1)
						{
							*(screenPtr+1) = *bitmapPtr;
						}
						
						bitmapPtr++;
						screenPtr+=step;
					}
				}
				
				bitmapLine += sourceScanlineLength;
				
				if(y>rect2.y)
					{
					if(i240StartTable[y] != i240StartTable[y-1]){
						
						if(gScaleYStep[y-1]>Private->
							EPOC_ScreenSize.iWidth) 
							// more than one line of screen 
							// last line  skip was bigger than one line
							// Fill the gap with the privous line
						{
							polateTrg = screenMemory-Private->EPOC_ScreenSize.iWidth;
							polateMem1 = screenMemory;							
							TInt scaledSourceRectWidth = gScaleXPos[sourceRectWidth];
							Mem::Copy(polateTrg, polateMem1, scaledSourceRectWidth*2);							
						}
						
						screenMemory += gScaleYStep[y];
						interpolate = EFalse;
					}
					else
						{
						interpolate = ETrue; // We need to interpolate the next line with the current line.. or plainly skipit
						}
				}
				else
				{					
					screenMemory += gScaleYStep[y];
				}
			}
        }
        // !! 256 color paletted mode: 8 bpp  --> xx bpp
        else { 
			TUint8* bitmapLine = (TUint8*)_this->screen->pixels + sourceStartOffset;
            TUint16* screenMemory = screenBuffer + targetStartOffset;
			TUint16* polateMem1= NULL;		
			TUint16* polateTrg = NULL;
			TUint16* screenPtr = NULL;
			TUint8* bitmapPtr  = NULL;
            for(TInt y = rect2.y ; y < sourceRectHeight ; y++) {                
                /* Convert each pixel from 256 palette to 4k color values */
				// for(TInt x = 0 ; x < sourceRectWidth ; x++) {                    
				if(!interpolate)				
				{
					screenPtr = screenMemory;
					bitmapPtr = bitmapLine;
					TInt step = 0;
					for(TInt index =0;index<sourceRectWidth;index++){																											
						*screenPtr = EPOC_HWPalette_256_to_DisplayMode[*bitmapPtr];
						step =gScaleStep[index+rect2.x];
						if(step>1)
						{
							*(screenPtr+1) = EPOC_HWPalette_256_to_DisplayMode[*bitmapPtr];
						}
						
						bitmapPtr++;
						screenPtr+=step;
					}
				}                 
				bitmapLine += sourceScanlineLength;
				if(y>rect2.y){
					if(i240StartTable[y] != i240StartTable[y-1]){
						
						if(gScaleYStep[y-1]>Private->
							EPOC_ScreenSize.iWidth) // more than one line of screen // last line  skip was bigger than one line
						{
							polateTrg = screenMemory-Private->EPOC_ScreenSize.iWidth;
							polateMem1 = screenMemory;							
							TInt scaledSourceRectWidth = gScaleXPos[sourceRectWidth];
							Mem::Copy(polateTrg, polateMem1, scaledSourceRectWidth*2);							
						}
						
						screenMemory += gScaleYStep[y];
						interpolate = EFalse;
					}
					else
						{
						interpolate = ETrue;;
						}
				}
				else
				{					
					screenMemory += gScaleYStep[y];;
				}								
			}
		}
		
		}    
		
		if(lockedHeap)
		{
			lock.End();
			gHeapIsLocked=EFalse;
		}
		if(Private->iNeedFullRedraw )
		{
			Private->iNeedFullRedraw=EFalse;
			Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,TPoint(0,0),TRect(0,0,Private->iStretchSize.iWidth,Private->iStretchSize.iHeight));
		}
		else
			for(TInt loop=0;loop<numrects;loop++)
			{
				SDL_Rect rect =rects[loop];
				rect.w = gScaleXPos[rect.w];
				rect.h = gScaleYPos[rect.h];
				rect.x = gScaleXPos[rect.x];
				rect.y = gScaleYPos[rect.y];
				TRect realRect(rect.x,rect.y , rect.x+rect.w,rect.y+rect.h);
				
				Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,realRect.iTl,realRect);
			}
	Private->iWindowCreator->UpdateScreen();
#if defined (__WINS__)
	Private->iEikEnv->WsSession().Flush();
#endif
}

inline void EPOC_S60LandscapeStretchUglyUpdate(_THIS, int numrects, SDL_Rect *rects)
{
	TBool lockedHeap=EFalse;
	
	TInt i;
    TInt screenW = _this->screen->w;
    TInt screenH = _this->screen->h;
    TInt sourceScanlineLength = screenW;
	
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
        TInt targetStartOffset = 0;
		TInt yInterpolatePixel=Private->EPOC_ScreenSize.iWidth;
		TInt xInterpolatePixel = 1;
		
		if(!(_this->hidden->iSX0Mode&ESX0Flipped))
		{
			yInterpolatePixel=-yInterpolatePixel;
		}
		else
		{
			xInterpolatePixel = -xInterpolatePixel;
		}
		
		if(_this->hidden->iSX0Mode&ESX0Flipped)
		{
			targetStartOffset = gScaleXPos[rect2.x]* Private->EPOC_ScreenSize.iWidth + gScaleYPos[(Private->iModeSize.iHeight-1)-rect2.y];   
		}
		else
		{
			targetStartOffset = gScaleXPos[(Private->iModeSize.iWidth-1)-rect2.x]*Private->EPOC_ScreenSize.iWidth + gScaleYPos[rect2.y];   
		}
        
		sourceRectHeight += rect2.y;
		TBool interpolate = EFalse;
        if (_this->screen->format->BitsPerPixel == GetBpp(Private->EPOC_DisplayMode)) { 
			
			TUint16* bitmapLine = (TUint16*)_this->screen->pixels + sourceStartOffset;
            TUint16* screenMemory = screenBuffer + targetStartOffset;
			TUint16* polateMem1= NULL;			
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
							*screenPtr = *bitmapPtr;
						bitmapPtr++;
						screenPtr++;
					}
				}
				else{
					TUint16* screenPtr = screenMemory;
					TUint16* bitmapPtr = bitmapLine;
					TInt step = 0;
					for(TInt index =0;index<sourceRectWidth;index++){
						
						*screenPtr = *bitmapPtr;

						step =gScaleStep[index+rect2.x];
					
						if(step> Private->EPOC_ScreenSize.iWidth || (-step)> Private->EPOC_ScreenSize.iWidth)
						{
							*(screenPtr+yInterpolatePixel) = *bitmapPtr;
						}
						
						bitmapPtr++;
						screenPtr+=step;
					}
				}
				
				bitmapLine += sourceScanlineLength;
				if(y>rect2.y)
				{
					if(i240StartTable[y] != i240StartTable[y-1])
					{
						// Next bit of functions interpolates between two lines
						if(gScaleYStep[y-1]>1 || gScaleYStep[y-1]<-1) // more than one line of screen // last line  skip was bigger than one line
						{
							polateTrg = screenMemory-xInterpolatePixel;
							polateMem1 = screenMemory;							
							TInt scaledSourceRectWidth = gScaleXPos[sourceRectWidth];
							for(TInt index =0;index<scaledSourceRectWidth;index++)
							{
								*polateTrg =  *polateMem1;
								polateTrg+=yInterpolatePixel;
								polateMem1+=yInterpolatePixel;								
							}
						}
						
						screenMemory += gScaleYStep[y];
						interpolate = EFalse;
					}
					else
						interpolate = ETrue;
				}
				else
				{					
					screenMemory += gScaleYStep[y];
				}
			}
        }
        // !! 256 color paletted mode: 8 bpp  --> xx bpp
        else { 
			TUint8* bitmapLine = (TUint8*)_this->screen->pixels + sourceStartOffset;
            TUint16* screenMemory = screenBuffer + targetStartOffset;
			TUint16* polateMem1= NULL;			
			TUint16* polateTrg = NULL;

           for(TInt y = rect2.y ; y < sourceRectHeight ; y++) {
			    TUint16* screenPtr = screenMemory;
           		TUint8* bitmapPtr = bitmapLine;
				if(!interpolate)				
				{					
					TInt step = 0;
					for(TInt index =0;index<sourceRectWidth;index++){																			
						*(screenPtr) = EPOC_HWPalette_256_to_DisplayMode[*(bitmapPtr)];
						
						step =gScaleStep[index+rect2.x];
						if(step> Private->EPOC_ScreenSize.iWidth || (-step)> Private->EPOC_ScreenSize.iWidth)
						{
							*(screenPtr+yInterpolatePixel) = EPOC_HWPalette_256_to_DisplayMode[*bitmapPtr];
						}
						
						bitmapPtr++;
						screenPtr+=step;
					}
				}
				
				bitmapLine += sourceScanlineLength;
				if(y>rect2.y)
				{
					if(i240StartTable[y] != i240StartTable[y-1])
					{
						
						// Next bit of functions interpolates between two lines
						if(gScaleYStep[y-1]>1 || gScaleYStep[y-1]<-1) // more than one line of screen // last line  skip was bigger than one line
						{
							polateTrg = screenMemory-xInterpolatePixel;
							polateMem1 = screenMemory;							
							TInt scaledSourceRectWidth = gScaleXPos[sourceRectWidth];
							for(TInt index =0;index<scaledSourceRectWidth;index++)
							{
								*polateTrg =  *polateMem1;
								polateTrg+=yInterpolatePixel;
								polateMem1+=yInterpolatePixel;								
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
					screenMemory += gScaleYStep[y];
				}
			}
		}
		
    }    
	
	if(lockedHeap)
	{
		lock.End();
		gHeapIsLocked=EFalse;
	}
	if(Private->iNeedFullRedraw )
	{
		Private->iNeedFullRedraw=EFalse;
		Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,TPoint(0,0),TRect(0,0,Private->iStretchSize.iHeight,Private->iStretchSize.iWidth));
	}
	else
		for(TInt loop=0;loop<numrects;loop++)
		{
			SDL_Rect rect =rects[loop];
			
			
			if(_this->hidden->iSX0Mode&ESX0Flipped)
			{		
				rect.x = gScaleXPos[rect.x];
				rect.y = Private->iStretchSize/*EPOC_DisplaySize*/.iHeight-gScaleYPos[rect.y];
				rect.w = (gScaleXPos[rect.w])+1;
				rect.h = (gScaleYPos[rect.h])+1;
				
				TRect realRect(rect.y-rect.h,rect.x, rect.y+1, rect.x+rect.w+1 );
				
				Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,realRect.iTl,realRect);				
			}
			else
			{
				rect.x = Private->/*EPOC_DisplaySize*/iStretchSize.iWidth-gScaleXPos[rect.x];
				rect.y = gScaleYPos[rect.y];
				rect.w = (gScaleXPos[rect.w])+1;
				rect.h = (gScaleYPos[rect.h])+1;
				
				TRect realRect(rect.y,rect.x-rect.w, rect.y+rect.h+1, rect.x+1 );
				Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,realRect.iTl,realRect);
			}
			
		}
		Private->iWindowCreator->UpdateScreen();
#if defined (__WINS__)
		Private->iEikEnv->WsSession().Flush();
#endif
}
