#ifndef ImageState_h
#define ImageState_h
#pragma once

#include "utl/UI/Image_fwd.h"
#include "utl/UI/WindowPlacement.h"


class CImageState
{
public:
	CImageState( void );
	~CImageState();

	void Stream( CArchive& archive );

	const std::tstring& GetDocFilePath( void ) const { return m_docFilePath; }
	void SetDocFilePath( const std::tstring& docFilePath ) { m_docFilePath = docFilePath; }

	CWindowPlacement& RefFramePlacement( void ) { return m_framePlacement; }

	bool HasScrollPos( void ) const { return !( m_scrollPos == s_noScrollPos ); }
private:
	static const CPoint s_noScrollPos;

	persist std::tstring m_docFilePath;
	persist CWindowPlacement m_framePlacement;
public:
	persist DWORD m_polyFlags;					// view specific flags (can be polymorphic for CImageView and CAlbumImageView)
	persist ui::ImageScalingMode m_scalingMode;	// app::Slider_v4_0+
	persist int m_zoomPct;
	persist COLORREF m_bkColor;
	persist CPoint m_scrollPos;

	enum MorePolyFlags
	{
		IgnorePlacement	= BIT_FLAG( 16 )		// shifted to HI-WORD so that it doesn't overlap with view's own flags
	};
};


#endif // ImageState_h
