static void EPOC_DirectLandscapeVGAUpdate(_THIS, int numrects, SDL_Rect *rects)
{
	TBool lockedHeap=EFalse;
	TBitmapUtil lock(Private->EPOC_Bitmap);	
	if(!gHeapIsLocked)
	{
		lock.Begin(TPoint(0,0)); // Lock bitmap heap
		lockedHeap = ETrue;
	}
	TInt screenW = _this->screen->w;
	TInt screenH = _this->screen->h;
	TInt sourceScanlineLength = screenW;
	TInt scrWidth=Private->EPOC_DisplaySize.iHeight;
	TInt scrHeight = Private->EPOC_DisplaySize.iWidth;
	TInt bufWidth = Private->EPOC_ScreenSize.iWidth;
	TInt sourceRectWidth = 0;
    TInt sourceRectHeight = 0;
    TInt sourceStartOffset = 0;
	TInt maxX = 0; // This is the ending coordinate
    TInt maxY = 0; // This is the ending coordinate
    TUint16* screenBuffer = (TUint16*)Private->EPOC_Bitmap->DataAddress();
    TUint16* xStart=NULL;
    		
    TUint16* xStartPos=NULL;
	for (TInt i=0; i < numrects; ++i ) {
		
		SDL_Rect rect2;
        const SDL_Rect& currentRect = rects[i];
        rect2.x = currentRect.x;
        rect2.y = currentRect.y;
        rect2.w = currentRect.w;
        rect2.h = currentRect.h;
		
        if (rect2.w <= 0 || rect2.h <= 0) /* sanity check */
            continue;
		
        /* All variables are measured in pixels */      
        maxX = Min(screenW - 1, rect2.x + rect2.w - 1);
        maxY = Min(screenH - 1, rect2.y + rect2.h - 1);
        if (maxX < 0 || maxY < 0) /* sanity check */
            continue;

        sourceRectWidth = (maxX - rect2.x + 1);
        sourceRectHeight = (maxY - rect2.y + 1);
      
        sourceStartOffset = rect2.x + (rect2.y * sourceScanlineLength);
        xStartPos=screenBuffer+(gScaleXPos[rect2.x]*bufWidth);
        xStartPos+=(Private->EPOC_DisplaySize.iWidth-gScaleYPos[rect2.y]);
        
        sourceRectHeight += rect2.y;
        sourceRectWidth  += rect2.x;
        
        if (_this->screen->format->BitsPerPixel == GetBpp(Private->EPOC_DisplayMode)) 
        	{ 
        	TUint16* bitmapLine = (TUint16*)_this->screen->pixels + sourceStartOffset;

        	for(TInt y = rect2.y ; y < sourceRectHeight ; y++) 
        		{        		
        		TUint16* bitmapPos = bitmapLine; /* 1 byte per pixel */
        		xStart = xStartPos;
        		if(gScaleYStep[y])
        			{
        			for(TInt x = rect2.x ; x < sourceRectWidth ; x++) 
        				{
        				*(xStart) = *bitmapPos;
        				bitmapPos++;
        				xStart+=gScaleStep[x];
        				}
        			xStartPos+=gScaleYStep[y];
        			}
        		bitmapLine += sourceScanlineLength;
        		}
        	}
        else
        	{
        	TUint8* bitmapLine = (TUint8*)_this->screen->pixels + sourceStartOffset;

        	for(TInt y = rect2.y ; y < sourceRectHeight ; y++) 
        		{	       		
        		TUint8* bitmapPos = bitmapLine; /* 1 byte per pixel */
        		xStart = xStartPos;
        		if(gScaleYStep[y])
        			{
        			for(TInt x = rect2.x ; x < sourceRectWidth ; x++) 
        				{		
        				*(xStart) = EPOC_HWPalette_256_to_DisplayMode[*bitmapPos];
        				bitmapPos++;
        				xStart+=gScaleStep[x];
        				}
        			xStartPos+=gScaleYStep[y];
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
	
	if( Private->iNeedFullRedraw )
		{
		Private->iNeedFullRedraw=EFalse;		
		TRect realRect(0 ,0 , scrHeight, scrWidth );

		Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,TPoint(0,0),realRect);
		}
	else for(TInt loop=0;loop<numrects;loop++)
		{
		SDL_Rect rect =rects[loop];
		rect.x = gScaleXPos[rect.x];
		rect.y = Private->EPOC_DisplaySize.iWidth-gScaleYPos[rect.y];
		rect.w = (gScaleXPos[rect.w])+1;
		rect.h = (gScaleYPos[rect.h])+1;
		
		TRect realRect(rect.y-rect.h,rect.x, rect.y+1, rect.x+rect.w+1 );

		Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,realRect.iTl,realRect);
		}

	Private->iWindowCreator->UpdateScreen();
}

static void EPOC_DirectFullLandscapeVGAUpdate(_THIS, int numrects, SDL_Rect *rects)
{
	TBool lockedHeap=EFalse;
	TBitmapUtil lock(Private->EPOC_Bitmap);	
	if(!gHeapIsLocked)
	{
		lock.Begin(TPoint(0,0)); // Lock bitmap heap
		lockedHeap = ETrue;
	}
	TInt screenW = _this->screen->w;
	TInt screenH = _this->screen->h;
	TInt sourceScanlineLength = screenW;
	TInt scrWidth=Private->EPOC_DisplaySize.iHeight;
	TInt scrHeight = Private->EPOC_DisplaySize.iWidth;
	TInt bufWidth = Private->EPOC_ScreenSize.iWidth;
	TInt sourceRectWidth = 0;
    TInt sourceRectHeight = 0;
    TInt sourceStartOffset = 0;
	TInt maxX = 0; // This is the ending coordinate
    TInt maxY = 0; // This is the ending coordinate
    TInt startX = 0;
    TInt startY = 0;
    TUint16* screenBuffer = (TUint16*)Private->EPOC_Bitmap->DataAddress();
    TUint16* xStart=NULL;
    		
    TUint16* xStartPos=NULL;
	for (TInt i=0; i < numrects; ++i ) {
		
		SDL_Rect rect2;
        const SDL_Rect& currentRect = rects[i];
        rect2.x = currentRect.x;
        rect2.y = currentRect.y;
        rect2.w = currentRect.w;
        rect2.h = currentRect.h;
		
        if (rect2.w <= 0 || rect2.h <= 0) /* sanity check */
            continue;
		
        /* All variables are measured in pixels */
        // Time to clip the output
        // Calculate position
        /* Check rects validity, i.e. upper and lower bounds */
        // First check if coordinates is within the display window
        startX = Max(rect2.x, Private->iPutOffset.iX);
        startY = Max(rect2.y, Private->iPutOffset.iY);

        if(startX > rect2.x) {
        	if(rect2.w > (startX-rect2.x))	
        		rect2.w -=(startX-rect2.x);
        	else
        		continue;
        	
        	rect2.x = startX;
        }
        	
        if(startY > rect2.y) {
        	if(rect2.h > (startY-rect2.y))
        		rect2.h -=(startY-rect2.y);
        	else
        		continue;
        	
        	rect2.y = startY;
        }
        
        if( (rect2.x + rect2.w)>(scrWidth+Private->iPutOffset.iX))
        	{
        	TInt diff = (rect2.x+rect2.w)-(scrWidth+Private->iPutOffset.iX);
        	if(rect2.w > diff)
        		rect2.w-=diff;
        	else 
        		continue;
        	}

        if( (rect2.y + rect2.h)>(scrHeight+Private->iPutOffset.iY))
        	{
        	TInt diff = (rect2.y + rect2.h)-(scrHeight+Private->iPutOffset.iY);
        	if(rect2.h > diff)
        		rect2.h-=diff;
        	else 
        		continue;
        	}        
                
        maxX = Min(screenW - 1, rect2.x + rect2.w - 1);
        maxY = Min(screenH - 1, rect2.y + rect2.h - 1);
        if (maxX < 0 || maxY < 0) /* sanity check */
            continue;

        sourceRectWidth = (maxX - rect2.x + 1);
        sourceRectHeight = (maxY - rect2.y + 1);
      
        sourceStartOffset = rect2.x + (rect2.y * sourceScanlineLength);
        xStartPos=screenBuffer+((rect2.x-Private->iPutOffset.iX)*bufWidth);
        xStartPos+=(Private->EPOC_DisplaySize.iWidth-(rect2.y-Private->iPutOffset.iY));
        
        sourceRectHeight += rect2.y;
        sourceRectWidth  += rect2.x;
        
        if (_this->screen->format->BitsPerPixel == GetBpp(Private->EPOC_DisplayMode)) 
        	{ 
        	TUint16* bitmapLine = (TUint16*)_this->screen->pixels + sourceStartOffset;

        	for(TInt y = rect2.y ; y < sourceRectHeight ; y++) 
        		{        		
        		TUint16* bitmapPos = bitmapLine; /* 1 byte per pixel */
        		xStart = xStartPos;
        		for(TInt x = rect2.x ; x < sourceRectWidth ; x++) 
        			{
        			*(xStart) = *bitmapPos;
        			bitmapPos++;
        			xStart+=bufWidth;
        			}
        		xStartPos--;
        		bitmapLine += sourceScanlineLength;
        		}
        	}
        else
        	{
        	TUint8* bitmapLine = (TUint8*)_this->screen->pixels + sourceStartOffset;

        	for(TInt y = rect2.y ; y < sourceRectHeight ; y++) 
        		{	       		
        		TUint8* bitmapPos = bitmapLine; /* 1 byte per pixel */
        		xStart = xStartPos;
        		for(TInt x = rect2.x ; x < sourceRectWidth ; x++) 
        			{		
        			*(xStart) = EPOC_HWPalette_256_to_DisplayMode[*bitmapPos];
        			bitmapPos++;
        			xStart+=bufWidth;
        			}
        		xStartPos--;
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
		TRect realRect(0 ,0 , scrHeight, scrWidth );

		Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,TPoint(0,0),realRect);
		}
	else
		for(TInt loop=0;loop<numrects;loop++)
			{
			SDL_Rect rect =rects[loop];

			startX = Max(rect.x, Private->iPutOffset.iX);
			startY = Max(rect.y, Private->iPutOffset.iY);

			if(startX > rect.x) {
			if(rect.w > (startX-rect.x))	
				rect.w -=(startX-rect.x);
			else
				continue;

			rect.x = startX;
			}

			if(startY > rect.y) {
			if(rect.h > (startY-rect.y))
				rect.h -=(startY-rect.y);
			else
				continue;

			rect.y = startY;
			}

			if( (rect.x + rect.w)>(scrWidth+Private->iPutOffset.iX))
				{
				TInt diff = (rect.x+rect.w)-(scrWidth+Private->iPutOffset.iX);
				if(rect.w > diff)
					rect.w-=diff;
				else 
					continue;
				}

			if( (rect.y + rect.h)>(scrHeight+Private->iPutOffset.iY))
				{
				TInt diff = (rect.y + rect.h)-(scrHeight+Private->iPutOffset.iY);
				if(rect.h > diff)
					rect.h-=diff;
				else 
					continue;
				}        
			rect.x-=Private->iPutOffset.iX;
			rect.y-=Private->iPutOffset.iY;
			rect.y = Private->EPOC_DisplaySize.iWidth-rect.y;
			TRect realRect(rect.y-rect.h,rect.x, rect.y+1, rect.x+rect.w+1 );

			Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,realRect.iTl,realRect);
			}
		Private->iWindowCreator->UpdateScreen();
}


static void EPOC_DirectPortraitVGAUpdate(_THIS, int numrects, SDL_Rect *rects)
{
	TBool lockedHeap=EFalse;
	TBitmapUtil lock(Private->EPOC_Bitmap);	
	if(!gHeapIsLocked)
	{
		lock.Begin(TPoint(0,0)); // Lock bitmap heap
		lockedHeap = ETrue;
	}
	TInt screenW = _this->screen->w;
	TInt screenH = _this->screen->h;
	TInt sourceScanlineLength = screenW;
	TInt scrWidth=Private->EPOC_DisplaySize.iWidth;
	TInt scrHeight = Private->EPOC_DisplaySize.iHeight;
	TInt bufWidth = Private->EPOC_ScreenSize.iWidth;
	TInt sourceRectWidth = 0;
    TInt sourceRectHeight = 0;
    TInt sourceStartOffset = 0;
	TInt maxX = 0; // This is the ending coordinate
    TInt maxY = 0; // This is the ending coordinate  
    TUint16* screenBuffer = (TUint16*)Private->EPOC_Bitmap->DataAddress();
    TUint16* xStart=NULL;
    		
    TUint16* xStartPos=NULL;
	for (TInt i=0; i < numrects; ++i ) {
		
		SDL_Rect rect2;
        const SDL_Rect& currentRect = rects[i];
        rect2.x = currentRect.x;
        rect2.y = currentRect.y;
        rect2.w = currentRect.w;
        rect2.h = currentRect.h;
		
        if (rect2.w <= 0 || rect2.h <= 0) /* sanity check */
            continue;
		
        /* All variables are measured in pixels */
      
        // First check if coordinates is within the display window
        maxX = Min(screenW - 1, rect2.x + rect2.w - 1);
        maxY = Min(screenH - 1, rect2.y + rect2.h - 1);
        if (maxX < 0 || maxY < 0) /* sanity check */
            continue;

        sourceRectWidth = (maxX - rect2.x + 1);
        sourceRectHeight = (maxY - rect2.y + 1);
      
        sourceStartOffset = rect2.x + (rect2.y * sourceScanlineLength);
        xStartPos=screenBuffer+gScaleXPos[rect2.x];
        xStartPos+=(gScaleYPos[rect2.y]*bufWidth);
        
        sourceRectHeight += rect2.y;
        sourceRectWidth  += rect2.x;
        if (_this->screen->format->BitsPerPixel == GetBpp(Private->EPOC_DisplayMode)) 
        	{ 
        	TUint16* bitmapLine = (TUint16*)_this->screen->pixels + sourceStartOffset;

        	for(TInt y = rect2.y ; y < sourceRectHeight ; y++) 
        		{        		
        		TUint16* bitmapPos = bitmapLine; /* 1 byte per pixel */
        		xStart = xStartPos;
        		
        		if(gScaleYStep[y])
        			{
        			for(TInt x = rect2.x ; x < sourceRectWidth ; x++) 
        				{
        				*(xStart) = *bitmapPos;
        				bitmapPos++;
        				xStart+=gScaleStep[x];
        				}
        				xStartPos+=gScaleYStep[y];  
        			}        		        		      		        	
        			
        		bitmapLine += sourceScanlineLength;
        		}
        	}
        else
        	{
        	TUint8* bitmapLine = (TUint8*)_this->screen->pixels + sourceStartOffset;

        	for(TInt y = rect2.y ; y < sourceRectHeight ; y++) 
        		{	       		
        		TUint8* bitmapPos = bitmapLine; /* 1 byte per pixel */
        		xStart = xStartPos;

        		if(gScaleYStep[y])
        			{
        			for(TInt x = rect2.x ; x < sourceRectWidth ; x++) 
        				{		
        				*(xStart) = EPOC_HWPalette_256_to_DisplayMode[*bitmapPos];
        				bitmapPos++;
        				xStart+=gScaleStep[x];
        				}
        			xStartPos+=gScaleYStep[y];
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
	
	if(Private->iNeedFullRedraw )
		{
		Private->iNeedFullRedraw=EFalse;		
		TRect realRect(0 ,0 , scrWidth, scrHeight );

		Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,TPoint(0,0),realRect);
		}
	else for(TInt loop=0;loop<numrects;loop++)
		{
		SDL_Rect rect =rects[loop];
		rect.w = gScaleXPos[rect.w];
		rect.h = gScaleYPos[rect.h];
		rect.x = gScaleXPos[rect.x];
		rect.y = gScaleYPos[rect.y];

		TRect realRect(rect.x,rect.y ,rect.x+rect.w,rect.y+rect.h);

		Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,realRect.iTl,realRect);
		}

	Private->iWindowCreator->UpdateScreen();
}


static void EPOC_DirectFullPortraitVGAUpdate(_THIS, int numrects, SDL_Rect *rects)
{
	TBool lockedHeap=EFalse;
	TBitmapUtil lock(Private->EPOC_Bitmap);	
	if(!gHeapIsLocked)
	{
		lock.Begin(TPoint(0,0)); // Lock bitmap heap
		lockedHeap = ETrue;
	}
	TInt screenW = _this->screen->w;
	TInt screenH = _this->screen->h;
	TInt sourceScanlineLength = screenW;
	TInt scrWidth=Private->EPOC_DisplaySize.iWidth;
	TInt scrHeight = Private->EPOC_DisplaySize.iHeight;
	TInt bufWidth = Private->EPOC_ScreenSize.iWidth;
	TInt sourceRectWidth = 0;
    TInt sourceRectHeight = 0;
    TInt sourceStartOffset = 0;
	TInt maxX = 0; // This is the ending coordinate
    TInt maxY = 0; // This is the ending coordinate
    TInt startX = 0;
    TInt startY = 0;
    TUint16* screenBuffer = (TUint16*)Private->EPOC_Bitmap->DataAddress();
    TUint16* xStart=NULL;
    		
    TUint16* xStartPos=NULL;
	for (TInt i=0; i < numrects; ++i ) {
		
		SDL_Rect rect2;
        const SDL_Rect& currentRect = rects[i];
        rect2.x = currentRect.x;
        rect2.y = currentRect.y;
        rect2.w = currentRect.w;
        rect2.h = currentRect.h;
		
        if (rect2.w <= 0 || rect2.h <= 0) /* sanity check */
            continue;
		
        /* All variables are measured in pixels */
        // Time to clip the output
        // Calculate position
        /* Check rects validity, i.e. upper and lower bounds */
        // First check if coordinates is within the display window
        startX = Max(rect2.x, Private->iPutOffset.iX);
        startY = Max(rect2.y, Private->iPutOffset.iY);

        if(startX > rect2.x) {
        	if(rect2.w > (startX-rect2.x))	
        		rect2.w -=(startX-rect2.x);
        	else
        		continue;
        	
        	rect2.x = startX;
        }
        	
        if(startY > rect2.y) {
        	if(rect2.h > (startY-rect2.y))
        		rect2.h -=(startY-rect2.y);
        	else
        		continue;
        	
        	rect2.y = startY;
        }
        
        if( (rect2.x + rect2.w)>(scrWidth+Private->iPutOffset.iX))
        	{
        	TInt diff = (rect2.x+rect2.w)-(scrWidth+Private->iPutOffset.iX);
        	if(rect2.w > diff)
        		rect2.w-=diff;
        	else 
        		continue;
        	}

        if( (rect2.y + rect2.h)>(scrHeight+Private->iPutOffset.iY))
        	{
        	TInt diff = (rect2.y + rect2.h)-(scrHeight+Private->iPutOffset.iY);
        	if(rect2.h > diff)
        		rect2.h-=diff;
        	else 
        		continue;
        	}        
                
        maxX = Min(screenW - 1, rect2.x + rect2.w - 1);
        maxY = Min(screenH - 1, rect2.y + rect2.h - 1);
        if (maxX < 0 || maxY < 0) /* sanity check */
            continue;

        sourceRectWidth = (maxX - rect2.x + 1);
        sourceRectHeight = (maxY - rect2.y + 1);
      
        sourceStartOffset = rect2.x + (rect2.y * sourceScanlineLength);
        xStartPos=screenBuffer+(rect2.x-Private->iPutOffset.iX);
        xStartPos+=((rect2.y-Private->iPutOffset.iY)*bufWidth);
        
        sourceRectHeight += rect2.y;
        sourceRectWidth  += rect2.x;
        
        if (_this->screen->format->BitsPerPixel == GetBpp(Private->EPOC_DisplayMode)) 
        	{ 
        	TUint16* bitmapLine = (TUint16*)_this->screen->pixels + sourceStartOffset;

        	for(TInt y = rect2.y ; y < sourceRectHeight ; y++) 
        		{        		
        		TUint16* bitmapPos = bitmapLine; /* 1 byte per pixel */
        		xStart = xStartPos;
        		for(TInt x = rect2.x ; x < sourceRectWidth ; x++) 
        			{
        			*(xStart) = *bitmapPos;
        			bitmapPos++;
        			xStart++;
        			}
        		xStartPos+=bufWidth;
        		bitmapLine += sourceScanlineLength;
        		}
        	}
        else
        	{
        	TUint8* bitmapLine = (TUint8*)_this->screen->pixels + sourceStartOffset;

        	for(TInt y = rect2.y ; y < sourceRectHeight ; y++) 
        		{	       		
        		TUint8* bitmapPos = bitmapLine; /* 1 byte per pixel */
        		xStart = xStartPos;
        		for(TInt x = rect2.x ; x < sourceRectWidth ; x++) 
        			{		
        			*(xStart) = EPOC_HWPalette_256_to_DisplayMode[*bitmapPos];
        			bitmapPos++;
        			xStart++;
        			}
        		xStartPos+=bufWidth;
        		bitmapLine += sourceScanlineLength;
        		}
        	}
	}	

	if(lockedHeap)
		{
		lock.End();
		gHeapIsLocked=EFalse;
		}
	
	if( Private->iNeedFullRedraw )
		{
		Private->iNeedFullRedraw=EFalse;		
		TRect realRect(0 ,0 , scrWidth, scrHeight );

		Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,TPoint(0,0),realRect);
		}
	else
		for(TInt loop=0;loop<numrects;loop++)
			{
			SDL_Rect rect =rects[loop];

			startX = Max(rect.x, Private->iPutOffset.iX);
			startY = Max(rect.y, Private->iPutOffset.iY);

			if(startX > rect.x) {
			if(rect.w > (startX-rect.x))	
				rect.w -=(startX-rect.x);
			else
				continue;

			rect.x = startX;
			}

			if(startY > rect.y) {
			if(rect.h > (startY-rect.y))
				rect.h -=(startY-rect.y);
			else
				continue;

			rect.y = startY;
			}

			if( (rect.x + rect.w)>(scrWidth+Private->iPutOffset.iX))
				{
				TInt diff = (rect.x+rect.w)-(scrWidth+Private->iPutOffset.iX);
				if(rect.w > diff)
					rect.w-=diff;
				else 
					continue;
				}

			if( (rect.y + rect.h)>(scrHeight+Private->iPutOffset.iY))
				{
				TInt diff = (rect.y + rect.h)-(scrHeight+Private->iPutOffset.iY);
				if(rect.h > diff)
					rect.h-=diff;
				else 
					continue;
				}        
			rect.x-=Private->iPutOffset.iX;
			rect.y-=Private->iPutOffset.iY;
			TRect realRect(rect.x, rect.y , rect.x+rect.w+1, rect.y+rect.h+1);
			Private->iWindowCreator->PutBitmap(Private->EPOC_Bitmap,realRect.iTl,realRect);
			}
	
	Private->iWindowCreator->UpdateScreen();
}
