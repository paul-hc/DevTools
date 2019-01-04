#ifndef ThemeCustomDraw_h
#define ThemeCustomDraw_h
#pragma once

#include "utl/UI/Image_fwd.h"
#include "utl/UI/GpUtilities.h"


class COptions;


namespace hlp
{
	void DrawError( CDC* pDC, const CRect& coreRect );
	void DrawFrameGuides( CDC* pDC, CRect coreRect, Color guideColor );
	void DrawGuides( CDC* pDC, CRect coreRect, Color guideColor );
}


class CThemeCustomDraw : public ui::ICustomImageDraw
{
public:
	CThemeCustomDraw( const COptions* pOptions, const std::tstring& itemCaption = _T("text") );

	static CThemeCustomDraw* MakeListCustomDraw( const COptions* pOptions );
	static CThemeCustomDraw* MakeTreeCustomDraw( const COptions* pOptions );
private:
	// ui::ICustomImageDraw interface
	virtual CSize GetItemImageSize( ui::GlyphGauge glyphGauge = ui::SmallGlyph ) const;
	virtual bool SetItemImageSize( const CSize& boundsSize );					// call when UI control drives image bounds size
	virtual bool DrawItemImage( CDC* pDC, const utl::ISubject* pSubject, const CRect& itemRect );
private:
	enum Metrics { TextMargin = 5 };

	const COptions* m_pOptions;
	mutable CSize m_boundsSize;
public:
	CSize m_imageSize[ ui::_GlyphGaugeCount ];		// size of the core image
	CSize m_imageMargin;			// extra spacing of the image
	int m_textMargin;				// extra spacing to the right between image and item text

	std::tstring m_itemCaption;

	static const CSize s_themePreviewSize;
	static const CSize s_themePreviewSizeLarge;
};


#endif // ThemeCustomDraw_h
