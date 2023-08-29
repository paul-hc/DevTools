#ifndef IImageView_h
#define IImageView_h
#pragma once

#include "utl/UI/ImagePathKey.h"


class CWicImage;


interface IImageView
{
	virtual fs::TImagePathKey GetImagePathKey( void ) const = 0;
	virtual CWicImage* GetImage( void ) const = 0;
	virtual CScrollView* GetScrollView( void ) = 0;

	// events
	virtual void EventChildFrameActivated( void ) = 0;
};


#endif // IImageView_h
