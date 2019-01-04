#ifndef IImageView_h
#define IImageView_h
#pragma once

#include "utl/UI/ImagePathKey.h"


class CWicImage;


interface IImageView
{
	virtual const fs::ImagePathKey& GetImagePathKey( void ) const = 0;
	virtual CWicImage* GetImage( void ) const = 0;
	virtual CScrollView* GetView( void ) = 0;

	enum RegainAction { Enter, Escape };
	virtual void RegainFocus( RegainAction regainAction, int ctrlId = 0 ) = 0;

	// events
	virtual void EventChildFrameActivated( void ) = 0;
	virtual void EventNavigSliderPosChanged( bool thumbTracking ) = 0;
};


#endif // IImageView_h
