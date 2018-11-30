#ifndef ThemeCustomDraw_h
#define ThemeCustomDraw_h
#pragma once

#include "utl/Image_fwd.h"
#include "utl/GpUtilities.h"


class CThemeSampleOptions;


namespace hlp
{
	void DrawError( CDC* pDC, const CRect& coreRect );
	void DrawFrameGuides( CDC* pDC, CRect coreRect, Color guideColor );
	void DrawGuides( CDC* pDC, CRect coreRect, Color guideColor );
}


class CThemeCustomDraw : public ui::ICustomImageDraw
{
public:
	CThemeCustomDraw( const CThemeSampleOptions* pOptions, const std::tstring& itemCaption )
		: m_pOptions( pOptions ), m_itemCaption( itemCaption ), m_boundsSize( 0, 0 ) {}
private:
	// ui::ICustomImageDraw interface
	virtual CSize GetItemImageSize( ui::GlyphGauge glyphGauge = ui::SmallGlyph ) const;
	virtual bool SetItemImageSize( const CSize& boundsSize );					// call when UI control drives image bounds size
	virtual bool DrawItemImage( CDC* pDC, const utl::ISubject* pSubject, const CRect& itemRect );
private:
	const CThemeSampleOptions* m_pOptions;
	std::tstring m_itemCaption;
	CSize m_boundsSize;
};


#endif // ThemeCustomDraw_h
