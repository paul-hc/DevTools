#ifndef ThumbPreviewCtrl_h
#define ThumbPreviewCtrl_h
#pragma once

#include "utl/FlexPath.h"
#include "ObjectCtrlBase.h"


class CThumbnailer;
class CWicDibSection;


class CThumbPreviewCtrl : public CStatic
	, public CObjectCtrlBase
{
public:
	CThumbPreviewCtrl( CThumbnailer* pThumbnailer );

	const fs::CFlexPath& GetImagePath( void ) const { return m_imageFilePath; }
	void SetImagePath( const fs::CFlexPath& imageFilePath, bool doRedraw = true );
private:
	void DrawThumbnail( CDC* pDC, const CRect& clientRect, CWicDibSection* pThumb );
	bool DrawDualThumbnailFlow( CDC* pDC, const CRect& clientRect, CWicDibSection* pThumb );		// for tiny icon like thumbnails
private:
	CThumbnailer* m_pThumbnailer;
	fs::CFlexPath m_imageFilePath;

	// generated stuff
public:
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint screenPos );
	afx_msg BOOL OnEraseBkgnd( CDC* pDC );
	afx_msg void OnPaint( void );

	DECLARE_MESSAGE_MAP()
};


#endif // ThumbPreviewCtrl_h
