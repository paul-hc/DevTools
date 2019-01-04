#ifndef CustomDrawImager_fwd_h
#define CustomDrawImager_fwd_h
#pragma once

#include "Image_fwd.h"


class CBaseCustomDrawImager;


interface ICustomDrawControl
{
	virtual CBaseCustomDrawImager* GetCustomDrawImager( void ) const = 0;
	virtual void SetCustomFileGlyphDraw( bool showGlyphs = true ) = 0;
};


#endif // CustomDrawImager_fwd_h
