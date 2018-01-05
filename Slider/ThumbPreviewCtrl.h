#ifndef ThumbPreviewCtrl_h
#define ThumbPreviewCtrl_h
#pragma once


#include "utl/FlexPath.h"


class CThumbPreviewCtrl : public CStatic
{
public:
	CThumbPreviewCtrl( void ) {}

	const fs::CFlexPath& GetImagePath( void ) const { return m_imageFilePath; }
	void SetImagePath( const fs::CFlexPath& imageFilePath, bool doRedraw = true );
private:
	fs::CFlexPath m_imageFilePath;
public:
	// generated stuff
protected:
	afx_msg BOOL OnEraseBkgnd( CDC* pDC );
	afx_msg void OnPaint();

	DECLARE_MESSAGE_MAP()
};


#endif // ThumbPreviewCtrl_h
