#ifndef WicDibSection_h
#define WicDibSection_h
#pragma once

#include "WicBitmap.h"
#include "ui_fwd.h"


// a DIB section generated from a WIC bitmap;
// it supports transparent drawing automatically if alpha channel is present.

class CWicDibSection : public CBitmap, public CWicBitmap
{
public:
	CWicDibSection( void ) {}
	CWicDibSection( IWICBitmapSource* pWicBitmap ) { SetWicBitmap( pWicBitmap ); }		// build after base is constructed
	virtual ~CWicDibSection();

	void Clear( void );
	bool SetWicBitmap( IWICBitmapSource* pWicBitmap );

	HBITMAP CloneBitmap( void ) const;

	// GDI drawing (draws transparently if bitmap has alpha channel and rop is SRCCOPY)
	CRect Draw( CDC* pDC, const CRect& boundsRect, ui::StretchMode stretchMode = ui::OriginalSize, CDC* pSrcDC = NULL, DWORD rop = SRCCOPY );		// returns the image rect
	CRect DrawAtPos( CDC* pDC, const CPoint& pos, CDC* pSrcDC = NULL );
protected:
	bool BlitNatural( CDC* pDC, const CRect& destRect, const CRect& srcRect, CDC* pSrcDC = NULL, DWORD rop = SRCCOPY ) const;		// alpha blends (transparently) if alpha channel present, otherwise it blits.
};


#endif // WicDibSection_h
