#ifndef ImageEdit_h
#define ImageEdit_h
#pragma once

#include "TextEdit.h"
#include "IImageProxy.h"


class CImageEdit : public CTextEdit
{
public:
	CImageEdit( void );
	virtual ~CImageEdit();

	ui::IImageProxy* GetImageProxy( void ) const { return m_pImageProxy.get(); }
	bool SetImageProxy( ui::IImageProxy* pImageProxy );

	template< typename ImageProxyT >
	ImageProxyT* GetImageProxyAs( void ) const { return checked_static_cast<ImageProxyT*>( m_pImageProxy.get() ); }

	// image list
	void SetImageList( CImageList* pImageList );

	int GetImageIndex( void ) const;
	bool SetImageIndex( int imageIndex );

	virtual bool HasValidImage( void ) const;
protected:
	virtual void DrawImage( CDC* pDC, const CRect& imageRect );

	virtual void UpdateControl( void );
private:
	void ResizeNonClient( void );
private:
	std::auto_ptr<ui::IImageProxy> m_pImageProxy;
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
