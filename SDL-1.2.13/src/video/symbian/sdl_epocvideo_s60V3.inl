TInt i240StartPosition[240];
#define KVirtualKeyboardSpace 240
#include "SDL_epocvideo_vga.inl"
#include "sdl_epocvideo_s60v3_ugly.inl"

void EPOC_CalcStretchFactors(_THIS, TSize /*aTargetSize*/)
{	
	TReal xStretch=1;
	TReal yStretch=1;  
	TInt yMultiplier = 1;
	TInt xMultiplier = 1;
	Private->iKeyboardRect = TRect(0,0,0,0);
	TInt vkbWidthSpace = (current_video->hidden->iVKBState != EDisplayMiniControls)?KVirtualKeyboardSpace:current_video->hidden->iActivateButtonSize;	
	if((Private->iSX0Mode & ESX0Portrait))
	{
		xStretch = (TReal)Private->EPOC_DisplaySize.iWidth/(TReal)Private->iModeSize.iWidth;
		yStretch = (TReal)Private->EPOC_DisplaySize.iHeight/(TReal)Private->iModeSize.iHeight;		
		yMultiplier = Private->EPOC_ScreenSize.iWidth;
		if(Private->iHasMouseOrTouch && Private->iVirtualKeyBoardActive)
		{
			if((Private->EPOC_DisplaySize.iWidth)>=(Private->EPOC_DisplaySize.iHeight))
			{
				xStretch = (TReal)(Private->EPOC_DisplaySize.iWidth-vkbWidthSpace)/(TReal)Private->iModeSize.iWidth;				
				Private->iKeyboardRect = TRect(Private->EPOC_DisplaySize.iWidth-vkbWidthSpace,0, Private->EPOC_DisplaySize.iWidth,Private->EPOC_DisplaySize.iHeight);				
				if(xStretch > 2)
				{
					Private->iKeyboardRect.iTl.iX-=(xStretch-2)*(TReal)(Private->iModeSize.iWidth);			
				}			
			}
			else 
			{
				yStretch = (TReal)(Private->EPOC_DisplaySize.iHeight-vkbWidthSpace)/(TReal)Private->iModeSize.iHeight;
				Private->iKeyboardRect = TRect(0, Private->EPOC_DisplaySize.iHeight-vkbWidthSpace, Private->EPOC_DisplaySize.iWidth,Private->EPOC_DisplaySize.iHeight);
				if(yStretch > 2)
				{
					Private->iKeyboardRect.iTl.iY-=(yStretch-2)*(TReal)(Private->iModeSize.iHeight);
				}
			}
		}
		
	}
	else // Landscape
	{
		xMultiplier = Private->EPOC_ScreenSize.iWidth;
		xStretch = (TReal)Private->EPOC_DisplaySize.iHeight/(TReal)Private->iModeSize.iWidth;
		yStretch = (TReal)Private->EPOC_DisplaySize.iWidth/(TReal)Private->iModeSize.iHeight;

		if(Private->iHasMouseOrTouch && Private->iVirtualKeyBoardActive)
		{
			if((Private->EPOC_DisplaySize.iHeight)>=(Private->EPOC_DisplaySize.iWidth))
			{
				xStretch = (TReal)(Private->EPOC_DisplaySize.iHeight-vkbWidthSpace)/(TReal)Private->iModeSize.iWidth;
				Private->iKeyboardRect = TRect(0,Private->EPOC_DisplaySize.iHeight-vkbWidthSpace, Private->EPOC_DisplaySize.iWidth,Private->EPOC_DisplaySize.iHeight);
				if(xStretch > 2)
				{
					Private->iKeyboardRect.iTl.iY-=(xStretch-2)*(TReal)(Private->iModeSize.iWidth);
				}
			}
			else 
			{
				yStretch = (TReal)(Private->EPOC_DisplaySize.iWidth-vkbWidthSpace)/(TReal)Private->iModeSize.iHeight;
				Private->iKeyboardRect = TRect(Private->EPOC_DisplaySize.iWidth-vkbWidthSpace, 0, Private->EPOC_DisplaySize.iWidth,Private->EPOC_DisplaySize.iHeight);
				if(yStretch > 2)
				{
					Private->iKeyboardRect.iTl.iX-=(yStretch-2)*(TReal)(Private->iModeSize.iHeight);				
				}
			}
		}
		if(Private->iSX0Mode & ESX0Flipped)
		{
			yMultiplier = -yMultiplier;
		}
		else
		{
			xMultiplier = -xMultiplier;
		}
	}	
	
	if(xStretch>2)
	{
		xStretch = 2;
	}
	
	if(yStretch>2)
	{
		yStretch = 2;
	}
	
	// VGA mode use 1-1 always
	if (Private->EPOC_ShrinkedHeight || Private->EPOC_ShrinkedWidth) {
		
		if(xStretch>1)
		{
			xStretch = 1;
		}
		
		if(yStretch>1)
		{
			yStretch = 1;
		}
	}
	
	if(Private->iSX0Mode & ESX0KeepAspect)
		{
		// Keep aspect.. keep the lowest value
		if(yStretch != xStretch)
			{
			if(yStretch<xStretch)
				{
				xStretch = yStretch;
				}
			else
				{
				yStretch = xStretch;
				}
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
	
	Private->EPOC_ScreenOffset = 0;
	Private->iXScale = xStretch;
	Private->iYScale = yStretch;
	Private->iNoStretch = ((xStretch == 1) && (yStretch == 1));
	Private->iStretchSize = TSize(xStretch*Private->iModeSize.iWidth,yStretch*Private->iModeSize.iHeight);
}

extern "C" void EPOC_CalcScaleFactors(_THIS)
{
	TInt loop; // This is the startoffset for a 320 wide line
	TInt lastx=-1;
	if(Private->iModeSize != TSize(0,0))
	{
		EPOC_CalcStretchFactors(_this,TSize());
		if((_this->hidden->iSX0Mode & ESX0Portrait))// Portrait mode
		{
			
			for(loop=0;loop<320;loop++)
			{		
				if(_this->hidden->iSX0Mode & ESX0Stretched)
				{
					TInt pos = (loop*Private->iXScale);
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
					if(pos>loop)
						pos = loop;
					i320StartTable[loop]=pos;
				}
				else
				{
					i320StepTable[loop]=1;
					i320StartTable[loop]=loop;
				}
			}
			
			
			for(loop=0;loop<240;loop++)
			{		
				if(_this->hidden->iSX0Mode & ESX0Stretched)
				{
					if(!_this->hidden->iIs240Mode || _this->hidden->EPOC_DisplaySize.iHeight>=240)
					{		
						i240StartTable[loop]=loop* _this->hidden->EPOC_ScreenSize.iWidth;
						i240StartPosition[loop]=loop;
					}
					else
					{
						TInt pos = (loop*Private->iYScale);				
						i240StartTable[loop]=pos * _this->hidden->EPOC_ScreenSize.iWidth;
						i240StartPosition[loop]=pos;
					}
				}
				else
				{
					i240StartTable[loop]=loop* _this->hidden->EPOC_ScreenSize.iWidth;
					i240StartPosition[loop]=loop;
				}
			}	
		}
		// Compressed
		else if(_this->hidden->iSX0Mode & ESX0Stretched)
		{
			
			for(loop=0;loop<320;loop++)
			{
				TInt pos = ((loop*Private->iXScale));
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
					TInt pos = ((loop*Private->iYScale));				
					if(_this->hidden->iSX0Mode&ESX0Flipped)
					{
						i240StartTable[loop]=(Private->EPOC_DisplaySize.iWidth-1)-pos;				
					}
					else
						i240StartTable[loop]=pos;				
				}
			}
			else
			{
				for(loop=0;loop<200;loop++)
				{
					TInt pos=((loop*Private->iYScale));			
					if(_this->hidden->iSX0Mode  &ESX0Flipped)
					{
						i240StartTable[loop]=(Private->EPOC_DisplaySize.iWidth-1)-pos;
					}
					else
						i240StartTable[loop]=pos;
				}
			}
		}
		else // Full landscape.. i.e 1-1 cropped or not
		{
			for(loop=0;loop<320;loop++)
			{		
				i320StepTable[loop]=1;
				i320StartTable[loop]=loop;
			}
			
			
			for(loop=0;loop<240;loop++)
			{		
				if(!_this->hidden->iIs240Mode || _this->hidden->EPOC_DisplaySize.iWidth>=240)
				{		
					if(_this->hidden->iSX0Mode&ESX0Flipped)
					{
						i240StartTable[loop]=239-loop;
					}
					else
						i240StartTable[loop]=loop;
				}
				else
				{
					TReal factor = (TReal)_this->hidden->EPOC_DisplaySize.iWidth/(TReal)_this->hidden->iModeSize.iHeight;
					TInt pos = ((loop*factor));
					
					if(_this->hidden->iSX0Mode&ESX0Flipped)
					{
						i240StartTable[loop]=199-loop;
					}
					else
						i240StartTable[loop]=pos;
				}
			}	
			
		}
}

}

inline void EPOC_S60PortraitStretchUpdate(_THIS, int numrects, SDL_Rect *rects)
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
			TUint16* polateMem2= NULL;
			TUint16* polateTrg = NULL;
			TUint16* screenPtr = NULL;
			TUint16* bitmapPtr = NULL;
			
            for(TInt y = rect2.y ; y < sourceRectHeight ; y++) {            
				if(interpolate)
				{
					screenPtr = screenMemory;
					bitmapPtr = bitmapLine;
					for(TInt index =0;index<sourceRectWidth;index++){						
						*screenPtr = CalcAverage16(*screenPtr,*bitmapPtr);
						bitmapPtr++;
						screenPtr+=gScaleStep[index+rect2.x];
					}
				}
				else
				{
					screenPtr = screenMemory;
					bitmapPtr = bitmapLine;
					TInt step = 0;
					for(TInt index =0;index<sourceRectWidth;index++){																
						// Last step is 0
						if(step==0 && index>0)
						{						
							*(screenPtr) = CalcAverage16(*screenPtr,*(bitmapPtr));
						}
						else
						{
							*screenPtr = *bitmapPtr;
						}
						
						step =gScaleStep[index+rect2.x];
						
						if(step>1)
						{
							*(screenPtr+1) = CalcAverage16(*bitmapPtr,*(bitmapPtr+1));
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
							polateMem2 = screenMemory-(Private->EPOC_ScreenSize.iWidth*2);
							TInt scaledSourceRectWidth = gScaleXPos[sourceRectWidth];
							for(TInt index =0;index<scaledSourceRectWidth;index++){
								*polateTrg =  CalcAverage16(*polateMem1,*polateMem2);
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
					screenMemory += gScaleYStep[y];
				}
			}
        }
        // !! 256 color paletted mode: 8 bpp  --> xx bpp
        else { 
			TUint8* bitmapLine = (TUint8*)_this->screen->pixels + sourceStartOffset;
            TUint16* screenMemory = screenBuffer + targetStartOffset;
			TUint16* polateMem1= NULL;
			TUint16* polateMem2= NULL;
			TUint16* polateTrg = NULL;
			TUint16* screenPtr = NULL;
			TUint8* bitmapPtr  = NULL;
            for(TInt y = rect2.y ; y < sourceRectHeight ; y++) {                
                /* Convert each pixel from 256 palette to 4k color values */
				// for(TInt x = 0 ; x < sourceRectWidth ; x++) {
				screenPtr = screenMemory;
		            	bitmapPtr = bitmapLine;
				if(interpolate)
				{
					
					
					for(TInt index =0;index<sourceRectWidth;index++){						
						*screenPtr = CalcAverage16(*screenPtr,EPOC_HWPalette_256_to_DisplayMode[*bitmapPtr]);
						bitmapPtr++;
						screenPtr+=gScaleStep[index+rect2.x];
					}
				}
				else
				{				
					TInt step = 0;
					for(TInt index =0;index<sourceRectWidth;index++){										
						
						// Last step is 0
						if(step==0 && index>0)
						{							
							*(screenPtr) = CalcAverage16(*screenPtr,EPOC_HWPalette_256_to_DisplayMode[*(bitmapPtr)]);
						}
						else
						{
							*screenPtr = EPOC_HWPalette_256_to_DisplayMode[*bitmapPtr];
						}
						
						step =gScaleStep[index+rect2.x];
						if(step>1)
						{
							*(screenPtr+1) = CalcAverage16(EPOC_HWPalette_256_to_DisplayMode[*bitmapPtr],EPOC_HWPalette_256_to_DisplayMode[*(bitmapPtr+1)]);
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
							polateMem2 = screenMemory-(Private->EPOC_ScreenSize.iWidth*2);
							TInt scaledSourceRectWidth = gScaleXPos[sourceRectWidth];
							for(TInt index =0;index<scaledSourceRectWidth;index++){
								*polateTrg =  CalcAverage16(*polateMem1,*polateMem2);
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

inline void EPOC_S60LandscapeStretchUpdate(_THIS, int numrects, SDL_Rect *rects)
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
            	TUint16* screenPtr = screenMemory;
            	TUint16* bitmapPtr = bitmapLine;
                
            	if(interpolate)
            	{				
					for(TInt index =0;index<sourceRectWidth;index++)
					{					
							*screenPtr = CalcAverage16(*screenPtr,*bitmapPtr);
						bitmapPtr++;
						screenPtr++;
					}
				}
				else
				{				
					TInt step = 0;
					for(TInt index =0;index<sourceRectWidth;index++)
					{
						
						// Last step is 0
						if(step==0 && index>0)
						{						
							*(screenPtr) = CalcAverage16(*screenPtr,*(bitmapPtr));
						}
						else
							*screenPtr = *bitmapPtr;

						step =gScaleStep[index+rect2.x];
					
						if(step> Private->EPOC_ScreenSize.iWidth || (-step)> Private->EPOC_ScreenSize.iWidth)
						{
							*(screenPtr+yInterpolatePixel) = CalcAverage16(*bitmapPtr,*(bitmapPtr+1));
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
							polateMem2 = screenMemory-2*xInterpolatePixel;
							TInt scaledSourceRectWidth = gScaleXPos[sourceRectWidth];
							for(TInt index =0;index<scaledSourceRectWidth;index++)
							{
								*polateTrg =  CalcAverage16(*polateMem1,*polateMem2);
								polateTrg+=yInterpolatePixel;
								polateMem1+=yInterpolatePixel;
								polateMem2+=yInterpolatePixel;
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
			TUint16* polateMem2= NULL;
			TUint16* polateTrg = NULL;

           for(TInt y = rect2.y ; y < sourceRectHeight ; y++) {
				TUint16* screenPtr = screenMemory;
           		TUint8* bitmapPtr = bitmapLine;
				if(interpolate){					
					for(TInt index =0;index<sourceRectWidth;index++){					
						*screenPtr = CalcAverage16(*screenPtr,EPOC_HWPalette_256_to_DisplayMode[*bitmapPtr]);
						bitmapPtr++;
						screenPtr++;
					}
				}
				else{					
					TInt step = 0;
					for(TInt index =0;index<sourceRectWidth;index++){
						// Last step is 0
						if(step==0 && index>0)
						{							
							*(screenPtr) = CalcAverage16(*screenPtr,EPOC_HWPalette_256_to_DisplayMode[*(bitmapPtr)]);
						}
						else					
							*screenPtr = EPOC_HWPalette_256_to_DisplayMode[*bitmapPtr];

							step =gScaleStep[index+rect2.x];
						if(step> Private->EPOC_ScreenSize.iWidth || (-step)> Private->EPOC_ScreenSize.iWidth)
						{
							*(screenPtr+yInterpolatePixel) = CalcAverage16(EPOC_HWPalette_256_to_DisplayMode[*bitmapPtr],EPOC_HWPalette_256_to_DisplayMode[*(bitmapPtr+1)]);
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
							polateMem2 = screenMemory-2*xInterpolatePixel;
							TInt scaledSourceRectWidth = gScaleXPos[sourceRectWidth];
							for(TInt index =0;index<scaledSourceRectWidth;index++)
							{
								*polateTrg =  CalcAverage16(*polateMem1,*polateMem2);
								polateTrg+=yInterpolatePixel;
								polateMem1+=yInterpolatePixel;
								polateMem2+=yInterpolatePixel;
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
				rect.x = Private->iStretchSize/*EPOC_DisplaySize*/.iWidth-gScaleXPos[rect.x];
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

inline void EPOC_S60PortraitUpdate(_THIS, int numrects, SDL_Rect *rects)
{
	TInt i=0;
	TInt screenW = _this->screen->w;
    TInt screenH = _this->screen->h;
	TBool lockedHeap=EFalse;
	
    TInt sourceScanlineLength = screenW;
	
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
		
		TUint16* xStartPos=screenBuffer+i320StartTable[rect2.x];
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
				for(TInt x = rect2.x ; x < sourceRectWidth ; x+=1) 
				{	
					
					if(isNotSameCurrentLine && steppedUplast)
					{
						*(xStart) = *bitmapPos;
					}
					else
					{							
						*xStart = CalcAverage16(*xStart,*bitmapPos);						
					}
					
					xStart=xStart+i320StepTable[x];
					steppedUplast = i320StepTable[x];
					bitmapPos+=1;
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
				for(TInt x = rect2.x ; x < sourceRectWidth ; x+=1) 
				{
					
					if(isNotSameCurrentLine && steppedUplast)
					{
						*(xStart) = EPOC_HWPalette_256_to_DisplayMode[*bitmapPos];
					}
					else
					{					
						*xStart = CalcAverage16(*xStart,EPOC_HWPalette_256_to_DisplayMode[*bitmapPos]);					
					}
					
					steppedUplast = i320StepTable[x];
					xStart=xStart+steppedUplast;
					
					bitmapPos+=1;
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
		TRect realRect(i320StartTable[0],i240StartPosition[0] , i320StartTable[319]+1,(i240StartPosition[199])+1 );
		
		Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,TPoint(0,0) ,realRect);
		
	}
	else
		for(TInt loop=0;loop<numrects;loop++)
		{
			SDL_Rect rect =rects[loop];
			TRect realRect(i320StartTable[rect.x],i240StartPosition[rect.y] , i320StartTable[rect.x+rect.w-1]+1,(i240StartPosition[rect.y+rect.h-1])+1 );
			
			Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,realRect.iTl,realRect);
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
		if(numrects<=20 && !Private->iNeedUpdate)
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
	
	if(_this->hidden->iSX0Mode & ESX0Portrait)
	{
		if((_this->hidden->iSX0Mode&ESX0Stretched && !Private->EPOC_ShrinkedHeight && !Private->iNoStretch))
		{
			if(!(_this->hidden->iSX0Mode&ESX0DontInterpolate))
				{
				EPOC_S60PortraitStretchUpdate(_this,numrects,rects);
				}
			else
				{
				EPOC_S60PortraitStretchUglyUpdate(_this,numrects,rects);
				}
			return;
		}
		EPOC_S60PortraitUpdate(_this,numrects,rects);
		return;
	}
	
	if(((_this->hidden->iSX0Mode&ESX0Stretched) && !Private->EPOC_ShrinkedHeight && !Private->iNoStretch))
		{
		if(!(_this->hidden->iSX0Mode&ESX0DontInterpolate))
			{
			EPOC_S60LandscapeStretchUpdate(_this,numrects,rects);
			}
		else
			{
			EPOC_S60LandscapeStretchUglyUpdate(_this,numrects,rects);
			}
		return;
		}
	
	TInt i;
    
    TInt screenW = _this->screen->w;
    TInt screenH = _this->screen->h;
	TBool lockedHeap=EFalse;
	
    TInt sourceScanlineLength = screenW;
	
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
		
		TUint16* xStartPos=NULL;
		
		if(!(_this->hidden->iSX0Mode & ESX0Flipped))
		{
			xStartPos = screenBuffer+(319 - i320StartTable[rect2.x])* scrWidth;
		}
		else
		{
			// Invert his value tif we are going from right to left instead of lft to right
			xStartPos = screenBuffer+(i320StartTable[rect2.x])* (-scrWidth);
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
				for(TInt x = rect2.x ; x < sourceRectWidth ; x++) 
				{
					if(isNotSameCurrentLine && steppedUplast)
					{
						*(xStart) = *bitmapPos;
					}
					else
					{						
						*xStart = CalcAverage16(*xStart,*bitmapPos);						
					}
					
					xStart=xStart-scrWidth*i320StepTable[x];
					steppedUplast = i320StepTable[x];
					bitmapPos+=1;
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
				for(TInt x = rect2.x ; x < sourceRectWidth ; x++) 
				{								
					if(isNotSameCurrentLine && steppedUplast)
					{
						*(xStart) = EPOC_HWPalette_256_to_DisplayMode[*bitmapPos];
					}
					else
					{					
						*xStart = CalcAverage16(*xStart,EPOC_HWPalette_256_to_DisplayMode[*bitmapPos]);						
					}				
					xStart=xStart-scrWidth*i320StepTable[x];
					steppedUplast = i320StepTable[x];
					
					bitmapPos+=1;
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
		if(Private->iSX0Mode & ESX0Flipped)
		{
			TRect realRect(i240StartTable[199],i320StartTable[0] , i240StartTable[0]+1,i320StartTable[319]+1 );
			
			Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,realRect.iTl,realRect);
		}
		else
		{
			TRect realRect(i240StartTable[0],i320StartTable[0] , i240StartTable[199]+1,i320StartTable[319]+1 );
			
			Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,TPoint(0,0),realRect);
		}
	}
	else
		for(TInt loop=0;loop<numrects;loop++)
		{
			SDL_Rect rect =rects[loop];
			if(Private->iSX0Mode & ESX0Flipped)
			{
				TRect realRect(i240StartTable[rect.y+rect.h-1],i320StartTable[rect.x] , i240StartTable[rect.y]+1,i320StartTable[rect.x+rect.w-1]+1 );
				Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,realRect.iTl,realRect);
			}
			else
			{
				TRect realRect(i240StartTable[rect.y],i320StartTable[(319-(rect.x+rect.w-1))] , i240StartTable[rect.y+rect.h-1]+1,i320StartTable[319-rect.x]+1 );
				
				Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,realRect.iTl,realRect);
			}
		}
		
		Private->iWindowCreator->UpdateScreen();
#ifdef __WINS__
		Private->iEikEnv->WsSession().Flush();
#endif
}

