#ifndef ThumbPreviewCtrl_h
#define ThumbPreviewCtrl_h
#pragma once


#include "utl/FlexPath.h"


class CThumbnailer;
class CWicDibSection;


class CThumbPreviewCtrl : public CStatic
{
public:
	CThumbPreviewCtrl( CThumbnailer* pThumbnailer ) : m_pThumbnailer( pThumbnailer ) { ASSERT_PTR( m_pThumbnailer ); }

	const fs::CFlexPath& GetImagePath( void ) const { return m_imageFilePath; }
	void SetImagePath( const fs::CFlexPath& imageFilePath, bool doRedraw = true );
private:
	void DrawThumbnail( CDC* pDC, const CRect& clientRect, CWicDibSection* pThumb );
	bool DrawDualThumbnailFlow( CDC* pDC, const CRect& clientRect, CWicDibSection* pThumb );		// for tiny icon like thumbnails
private:
	CThumbnailer* m_pThumbnailer;
	fs::CFlexPath m_imageFilePath;
public:
	// generated stuff
protected:
	afx_msg BOOL OnEraseBkgnd( CDC* pDC );
	afx_msg void OnPaint();

	DECLARE_MESSAGE_MAP()
};


#endif // ThumbPreviewCtrl_h
