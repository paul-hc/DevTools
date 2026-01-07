#ifndef ImageEdit_h
#define ImageEdit_h
#pragma once

#include "TextEdit.h"


class CImageEdit : public CTextEdit
{
public:
	CImageEdit( void );
	virtual ~CImageEdit();

	CImageList* GetImageList( void ) const { return m_pImageList; }
	void SetImageList( CImageList* pImageList );

	int GetImageIndex( void ) const { return m_imageIndex; }
	bool SetImageIndex( int imageIndex );

	virtual bool HasValidImage( void ) const { return m_pImageList != nullptr && m_imageIndex >= 0; }
protected:
	virtual void DrawImage( CDC* pDC, const CRect& imageRect );

	void UpdateControl( void );
private:
	void ResizeNonClient( void );
private:
	CImageList* m_pImageList;
	int m_imageIndex;
	CSize m_imageSize;
	CRect m_imageNcRect;

	enum { ImageSpacing = 0, ImageToTextGap = 2 };

	// generated overrides
public:
	virtual void PreSubclassWindow( void );
protected:
	afx_msg void OnNcCalcSize( BOOL calcValidRects, NCCALCSIZE_PARAMS* pNcSp );
	afx_msg void OnNcPaint( void );
	afx_msg LRESULT OnNcHitTest( CPoint point );
	afx_msg void OnNcLButtonDown( UINT hitTest, CPoint point );
	afx_msg void OnMove( int x, int y );

	DECLARE_MESSAGE_MAP()
};


#endif // ImageEdit_h
