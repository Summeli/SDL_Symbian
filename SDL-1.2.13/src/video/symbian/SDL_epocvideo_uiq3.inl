#include "SDL_EpocVideo_vga.inl"
void EPOC_CalcStretchFactors(_THIS,TSize /*aTargetSize*/)
{
	TInt loop;
	
	Private->iYScale = 1;
	Private->iXScale = 1;
	
	if (Private->EPOC_ShrinkedHeight || Private->EPOC_ShrinkedWidth)   /* simulate 400 pixel height in 200 pixel screen */  
	{
		TReal xStretch=1;
		TReal yStretch=1;  
		TInt yMultiplier = 1;
		TInt xMultiplier = 1;
		if(Private->iSX0Mode & ESX0Portrait)
		{
			xStretch = (TReal)240/(TReal)Private->iModeSize.iWidth;
			yStretch = (TReal)320/(TReal)Private->iModeSize.iHeight;
			yMultiplier = Private->EPOC_ScreenSize.iWidth;
		}
		else
		{
			xMultiplier = Private->EPOC_ScreenSize.iWidth;
			xStretch = (TReal)320/(TReal)Private->iModeSize.iWidth;
			yStretch = (TReal)240/(TReal)Private->iModeSize.iHeight;
			
			if(Private->iSX0Mode & ESX0Flipped)
			{
				yMultiplier = -yMultiplier;
			}
			else
			{
				xMultiplier = -xMultiplier;
			}
		}
		
		for(TInt y=0;y<=Private->iModeSize.iHeight;y++)
		{
			gScaleYPos[y]=(y*yStretch);
			if(y>0)
			{
				gScaleYStep[y-1]=(gScaleYPos[y]-gScaleYPos[y-1])* yMultiplier;
			}
		}
		
		for(TInt x=0;x<=Private->iModeSize.iWidth;x++)
		{
			gScaleXPos[x]=(x*xStretch);
			if(x>0)
				gScaleStep[x-1]=(gScaleXPos[x]-gScaleXPos[x-1])*xMultiplier;
		}
		
		Private->iYScale = yStretch;
		Private->iXScale = xStretch;
	}
	else
	{
		if(Private->iSX0Mode & ESX0Portrait  && Private->iSX0Mode & ESX0Stretched)
		{
			Private->iYScale = (TReal)Private->EPOC_ScreenSize.iHeight/(TReal)(TReal)Private->iModeSize.iHeight;
			for(loop=0;loop<240;loop++)
			{		
				i240StartTable[loop] = (0.5+loop*Private->iYScale);
				if(loop>0)
					gScaleYStep[loop-1]=(i240StartTable[loop]-i240StartTable[loop-1]);
			}
		}
		else if(Private->iSX0Mode & ESX0Portrait)
		{
			for(loop=0;loop<240;loop++)
			{
				i240StartTable[loop]=loop;
				gScaleYStep[loop] = 1;
			}
		}
		else
		{
			if(Private->iModeSize.iHeight==240 || Private->iModeSize.iHeight == 480 )
			{
				for(loop=0;loop<240;loop++)
				{
					
					if(_this->hidden->iSX0Mode & ESX0Flipped)
					{
						i240StartTable[loop]=(Private->EPOC_ScreenSize.iWidth-1)-loop;	
						gScaleYStep[loop] = -1;
					}
					else
					{
						i240StartTable[loop]=loop;
						gScaleYStep[loop] = 1;
					}
				}
			}
			else
			{
				Private->iYScale = (TReal)Private->EPOC_ScreenSize.iWidth/(TReal)Private->iModeSize.iHeight;
				for(loop=0;loop<240;loop++)
				{		
					i240StartTable[loop] = (0.5+loop*Private->iYScale);
					if(loop>0)
						gScaleYStep[loop-1]=(i240StartTable[loop]-i240StartTable[loop-1]);
				}
			}
		}
		TInt lastx=-1;
		
		for(loop=0;loop<320;loop++)
		{
			TInt pos = (0.5+(loop*3)/4);
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
		Private->iXScale = (0.5+(loop*3)/4);
		Private->iStretchSize = TSize(320, 240);
	}
	
}

inline void EPOC_PortraitUpdate(_THIS, int numrects, SDL_Rect *rects)
{
	TInt i;
    
    TInt screenW = _this->screen->w;
    TInt screenH = _this->screen->h;
    
    TInt sourceScanlineLength = screenW;
    
	TBool lockedHeap=EFalse;
	
	TBitmapUtil lock(Private->EPOC_Bitmap);	
	if(!gHeapIsLocked)
	{
		lock.Begin(TPoint(0,0)); // Lock bitmap heap
		gHeapIsLocked=ETrue;
		lockedHeap=ETrue;
	}

    TUint16* destBuffer = (TUint16*)Private->EPOC_Bitmap->DataAddress();
	
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
		
		TUint16* xStart=NULL;
		TUint16* targetLine = NULL;
		TUint16*  srcLine = NULL;
		TUint16* xStartPos=screenBuffer+i320StartTable[rect2.x]+scrWidth*i240StartTable[rect2.y];//(319 - (rect2.x>>(xStep-1))) * scrWidth;
		sourceRectHeight += rect2.y;
		sourceRectWidth  += rect2.x;
		
		TInt currentY=0;		
        TBool steppedUplast =true;
		if (_this->screen->format->BitsPerPixel == GetBpp(Private->EPOC_DisplayMode)) 
		{ 
			TUint16* bitmapLine = (TUint16*)_this->screen->pixels + sourceStartOffset;
			
            for(TInt y = rect2.y ; y < sourceRectHeight ; y++) 
			{				
				currentY= i240StartTable[y];
				
				TUint16* bitmapPos = bitmapLine; /* 1 byte per pixel */
				xStart=xStartPos;
				TBool stretch = EFalse;
				
				if(y>rect2.y && i240StartTable[y] != i240StartTable[y-1] && (gScaleYStep[y-1]>1 || gScaleYStep[y-1]<-1))
					stretch = ETrue;
				
				for(TInt x = rect2.x ; x < sourceRectWidth ; x+=1) 
				{	
					
					if(steppedUplast)
					{
						*(xStart) = *bitmapPos;													
					}
					else
					{
						*xStart = CalcAverage16(*xStart,*bitmapPos);
					}
					
					
					steppedUplast = i320StepTable[x];
					xStart=xStart+steppedUplast;
					
					bitmapPos+=1;
				}																	
				
				bitmapLine += sourceScanlineLength;
				
				if(stretch)
				{
					targetLine = (xStartPos-scrWidth);
					xStart = xStartPos;
					srcLine = targetLine-scrWidth;
					for(TInt x = 0;x <= i320StartTable[rect2.w-1];x++)
					{
						*targetLine = CalcAverage16(*xStart,*srcLine);
						
						targetLine++;
						xStart++;
						srcLine++;
					}
				}
				
				xStartPos +=scrWidth*(i240StartTable[y+1]-currentY);			
            }
		}
		// !! 256 color paletted mode: 8 bpp  --> 12/16 bpp
        else 
		{ 
			TUint8* bitmapLine = (TUint8*)_this->screen->pixels + sourceStartOffset;
			for(TInt y = rect2.y ; y < sourceRectHeight ; y++) 
			{
				currentY= i240StartTable[y];
				TUint8* bitmapPos = bitmapLine; /* 1 byte per pixel */								
				xStart=xStartPos;
				TBool stretch = EFalse;
				
				if(y>rect2.y && i240StartTable[y] != i240StartTable[y-1] && (gScaleYStep[y-1]>1 || gScaleYStep[y-1]<-1))
					stretch = ETrue;
				/* Convert each pixel from 256 palette to 4k color values */
				TUint16 pixel1 = 0;					
				for(TInt x = rect2.x ; x < sourceRectWidth ; x+=1) 
				{
					pixel1 = EPOC_HWPalette_256_to_DisplayMode[*bitmapPos];
					
					if(steppedUplast)
					{
						*(xStart) = pixel1;		
					}
					else
					{
						*xStart = CalcAverage16(*xStart, pixel1);
					}
					
					steppedUplast = i320StepTable[x];
					xStart=xStart+steppedUplast;
					bitmapPos+=1;               
				}					
				bitmapLine += sourceScanlineLength;	
				if(stretch)
				{
					targetLine = (xStartPos-scrWidth);
					xStart = xStartPos;
					srcLine = targetLine-scrWidth;
					for(TInt x = 0;x <= i320StartTable[rect2.w-1];x++)
					{
						*targetLine = CalcAverage16(*xStart,*srcLine);
						
						targetLine++;
						xStart++;
						srcLine++;
					}
				}
				xStartPos +=scrWidth*(i240StartTable[y+1]-currentY);		
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
		TRect realRect( 0,0,240,240 );
		
		if((Private->iSX0Mode & ESX0Stretched))
		{
			realRect = TRect( 0,0,240,320 );
		}
		
		Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,TPoint(0,0),realRect);
		
	}
	else for(TInt loop=0;loop<numrects;loop++)
	{
		SDL_Rect rect =rects[loop];
		rect.h = i240StartTable[rect.h-1];
		rect.y = i240StartTable[rect.y];
		rect.x = i320StartTable[rect.x];
		rect.w = i320StartTable[rect.w-1];
		TRect realRect(rect.x, rect.y , 1+(rect.x+rect.w), (rect.y+rect.h)+1);
		Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,realRect.iTl,realRect);
	}
		
	Private->iWindowCreator->UpdateScreen();
}

inline void EPOC_StretchedUpdate(_THIS, int numrects, SDL_Rect *rects)
{
	TInt i;
    
    TInt screenW = _this->screen->w;
    TInt screenH = _this->screen->h;
    
    TInt sourceScanlineLength = screenW;  
	TBool lockedHeap=EFalse;
	
	TBitmapUtil lock(Private->EPOC_Bitmap);	
	if(!gHeapIsLocked)
	{
		lock.Begin(TPoint(0,0)); // Lock bitmap heap
		gHeapIsLocked=ETrue;
		lockedHeap=ETrue;
	}
    TUint16* destBuffer = (TUint16*)Private->EPOC_Bitmap->DataAddress();
	
	
	TUint16* screenBuffer = destBuffer;

	
	/* Render the rectangles in the list */
	for ( i=0; i < numrects; ++i ) {
	    TInt scrWidth=Private->EPOC_ScreenSize.iWidth;
        
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
		
		TUint16* xStart=NULL;
		TUint16* xStartPos=screenBuffer+(319 - rect2.x) * scrWidth;
		sourceRectHeight += rect2.y;
		sourceRectWidth  += rect2.x;
		
		TInt yFlip = 0;
		if(Private->iSX0Mode & ESX0Flipped)
		{
			xStartPos = screenBuffer+ (((rect2.x+1) * scrWidth)-1);		
			scrWidth  = -scrWidth;
			yFlip = -1;
		}
		else
		{
			xStartPos = screenBuffer+((319 - rect2.x) * scrWidth);
			yFlip = 1;
		}

		TInt currentY=0;	
        
		if (_this->screen->format->BitsPerPixel == GetBpp(Private->EPOC_DisplayMode)) 
		{ 
			TUint16* bitmapLine = (TUint16*)_this->screen->pixels + sourceStartOffset;
			
            for(TInt y = rect2.y ; y < sourceRectHeight ; y++) 
			{				
				currentY= i240StartTable[y];
				TUint16* bitmapPos = bitmapLine; /* 1 byte per pixel */
				xStart=xStartPos+(yFlip*currentY);
				TBool stretch = EFalse;
				
				if(y>rect2.y && i240StartTable[y] != i240StartTable[y-1] && (gScaleYStep[y-1]>1 || gScaleYStep[y-1]<-1))
					stretch = ETrue;
				
				for(TInt x = rect2.x ; x < sourceRectWidth ; x+=1) 
				{									
					*(xStart) = *bitmapPos;													
					
					if(stretch)
						*(xStart-(yFlip*1)) = CalcAverage16(*(xStart),*(xStart-(yFlip*2)));
					xStart=xStart-scrWidth;
					bitmapPos+=1;
				}																	
				
				bitmapLine += sourceScanlineLength;
            }
		}
		// !! 256 color paletted mode: 8 bpp  --> 12/16 bpp
        else 
		{ 
			TUint8* bitmapLine = (TUint8*)_this->screen->pixels + sourceStartOffset;
			for(TInt y = rect2.y ; y < sourceRectHeight ; y++) 
			{
				currentY= i240StartTable[y];
				TUint8* bitmapPos = bitmapLine; /* 1 byte per pixel */				
				xStart=xStartPos+ (yFlip*currentY);
				
				TBool stretch = EFalse;
				
				if(y>rect2.y && i240StartTable[y] != i240StartTable[y-1] && (gScaleYStep[y-1]>1 || gScaleYStep[y-1]<-1))
					stretch = ETrue;
				
				for(TInt x = rect2.x ; x < sourceRectWidth ; x+=1) 
				{
					*(xStart)= EPOC_HWPalette_256_to_DisplayMode[*bitmapPos];
					
					if(stretch)
						*(xStart-(yFlip*1)) = CalcAverage16(*(xStart),*(xStart-(yFlip*2)));
					
					xStart=xStart-scrWidth;
					bitmapPos+=1;               
				}					
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
		TRect realRect( 0,0,240,320 );
		
		Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,TPoint(0,0),realRect);
		
	}
	else for(TInt loop=0;loop<numrects;loop++)
	{
		SDL_Rect rect =rects[loop];
		rect.h = i240StartTable[rect.h];
		rect.y = i240StartTable[rect.y];
		if((Private->iSX0Mode & ESX0Flipped))
		{
			TRect realRect(240-(rect.y+rect.h), rect.x , 240-(rect.y), rect.x+rect.w);
			Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,realRect.iTl,realRect);
		}
		else
		{		
			TRect realRect(rect.y,320-(rect.x+rect.w) , rect.y+rect.h,320-rect.x );
			Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,realRect.iTl,realRect);
		}
	}
		
	Private->iWindowCreator->UpdateScreen();
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
	
	if (Private->EPOC_ShrinkedHeight || Private->EPOC_ShrinkedWidth) {  /* simulate 400 pixel height in 200 pixel screen */  
		if(_this->hidden->iSX0Mode & ESX0Portrait)
		{
			if(_this->hidden->iSX0Mode & ESX0Stretched)
			{
				EPOC_DirectPortraitVGAUpdate(_this, numrects, rects);
			}
			else
			{
				EPOC_DirectFullPortraitVGAUpdate(_this, numrects, rects);
			}
		}
		else
		{
			if(_this->hidden->iSX0Mode & ESX0Stretched)
			{
				if(_this->hidden->iSX0Mode & ESX0Flipped)
				{
					EPOC_DirectFlippedLandscapeVGAUpdate(_this, numrects, rects);
				}
				else
				{
					EPOC_DirectLandscapeVGAUpdate(_this, numrects, rects);
				}
			}		
			else
			{
				if(_this->hidden->iSX0Mode & ESX0Flipped)
				{
					EPOC_DirectFullLandscapeVGAUpdate(_this, numrects, rects);
				}
				else
				{
					EPOC_DirectFlippedFullLandscapeVGAUpdate(_this, numrects, rects);
				}
			}
		}
		
		return; // Handle higher res in separate code
	}
	
	if(Private->iSX0Mode & ESX0Portrait)
	{
		EPOC_PortraitUpdate(_this,numrects,rects);
		return;
	}
	
	if((Private->iSX0Mode & ESX0Stretched && !Private->EPOC_ShrinkedHeight && _this->screen->h<240))
	{
		EPOC_StretchedUpdate(_this,numrects,rects);
		return;
	}
	
	TInt screenW = _this->screen->w;
    TInt screenH = _this->screen->h;
	
	TInt i;
	
    TInt sourceScanlineLength = screenW;
	
	TBool lockedHeap=EFalse;
	
	TBitmapUtil lock(Private->EPOC_Bitmap);	
	if(!gHeapIsLocked)
	{
		lock.Begin(TPoint(0,0)); // Lock bitmap heap
		gHeapIsLocked=ETrue;
		lockedHeap=ETrue;
	}
    TUint16* destBuffer = (TUint16*)Private->EPOC_Bitmap->DataAddress();
	
	TUint16* screenBuffer = destBuffer;
	TInt scrWidth;

	/* Render the rectangles in the list */
	for ( i=0; i < numrects; ++i ) {
        SDL_Rect rect2;
        const SDL_Rect& currentRect = rects[i];
		scrWidth= Private->EPOC_ScreenSize.iWidth;

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
		
		TUint16* xStart=NULL;
		TUint16* xStartPos= NULL;
		TInt yFlip = 0;

		if(Private->iSX0Mode & ESX0Flipped)
		{
			xStartPos = screenBuffer+ (((rect2.x+1) * scrWidth)-1);		
			scrWidth  = -scrWidth;
			yFlip = -1;
		}
		else
		{
			xStartPos = screenBuffer+((319 - rect2.x) * scrWidth);
			yFlip = 1;
		}
		
		sourceRectHeight += rect2.y;
		sourceRectWidth  += rect2.x;
		
        if (_this->screen->format->BitsPerPixel == GetBpp(Private->EPOC_DisplayMode)) 
		{ 
			TUint16* bitmapLine = (TUint16*)_this->screen->pixels + sourceStartOffset;
			
            for(TInt y = rect2.y ; y < sourceRectHeight ; y++) 
			{				
				TUint16* bitmapPos = bitmapLine; /* 1 byte per pixel */
				xStart=xStartPos+(yFlip*y);//currentY;

				for(TInt x = rect2.x ; x < sourceRectWidth ; x+=1) 
				{							
					*(xStart) = *bitmapPos;		
					
					xStart=xStart-scrWidth;
					bitmapPos+=1;
				}
				
				bitmapLine += sourceScanlineLength;
            }
		}
		// !! 256 color paletted mode: 8 bpp  --> 12/16 bpp
        else 
		{ 
			TUint8* bitmapLine = (TUint8*)_this->screen->pixels + sourceStartOffset;
			for(TInt y = rect2.y ; y < sourceRectHeight ; y++) 
			{
                TUint8* bitmapPos = bitmapLine; /* 1 byte per pixel */				
				xStart=xStartPos+ (yFlip*y);//currentY;
				/* Convert each pixel from 256 palette to 4k color values */						
				for(TInt x = rect2.x ; x < sourceRectWidth ; x+=1) 
				{
					*(xStart)  = EPOC_HWPalette_256_to_DisplayMode[*bitmapPos];																										
					
					xStart=xStart-scrWidth;
					bitmapPos+=1;               
				}
				
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
		TRect realRect( 0,0,240,320 );
		
		Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,TPoint(0,0),realRect);
		
	}
	else for(TInt loop=0;loop<numrects;loop++)
	{
		SDL_Rect rect =rects[loop];
		if((Private->iSX0Mode & ESX0Flipped))
		{
			TRect realRect(240-(rect.y+rect.h), rect.x , 240-(rect.y), rect.x+rect.w);
			Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,realRect.iTl,realRect);
		}
		else
		{
			TRect realRect(rect.y,320-(rect.x+rect.w) , rect.y+rect.h,320-rect.x );
			Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,realRect.iTl,realRect);
		}
	}
		
	Private->iWindowCreator->UpdateScreen();
 }
