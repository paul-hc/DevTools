#ifndef WicDibSection_h
#define WicDibSection_h
#pragma once

#include "WicBitmap.h"
#include "ui_fwd.h"


// a DIB section generated from a WIC bitmap

class CWicDibSection : public CBitmap, public CWicBitmap
{
public:
	CWicDibSection( void ) {}
	CWicDibSection( IWICBitmapSource* pWicBitmap ) { SetWicBitmap( pWicBitmap ); }		// build after base is constructed
	virtual ~CWicDibSection();

	void Clear( void );
	bool SetWicBitmap( IWICBitmapSource* pWicBitmap );

	HBITMAP CloneBitmap( void ) const;

	// GDI drawing
	CRect Draw( CDC* pDC, const CRect& boundsRect, ui::StretchMode stretchMode = ui::OriginalSize, CDC* pSrcDC = NULL, DWORD rop = SRCCOPY );			// returns the image rect
	CRect DrawAtPos( CDC* pDC, const CPoint& pos, CDC* pSrcDC = NULL );
};


#endif // WicDibSection_h
