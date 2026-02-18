#ifndef ImageProxies_h
#define ImageProxies_h
#pragma once

#include "IImageProxy.h"


class CDibSectionTraits;


class CBitmapProxy : public ui::IImageProxy
{
public:
	CBitmapProxy( HBITMAP hBitmap = nullptr );
	CBitmapProxy( HBITMAP hBitmapList, int bitmapIndex, const CSize& size );

	const CDibSectionTraits* GetDibSectionTraits( void ) const { return m_pDibTraits.get(); }

	// ui::IImageProxy interface
	virtual bool IsEmpty( void ) const override { return nullptr == m_hBitmapList || m_index < 0; }
	virtual const CSize& GetSize( void ) const override { return m_size; }
	virtual bool HasTransparency( void ) const override;
	virtual void Draw( CDC* pDC, const CPoint& pos, COLORREF transpColor = CLR_NONE ) const override;
	virtual void DrawDisabled( CDC* pDC, const CPoint& pos, COLORREF transpColor = CLR_NONE ) const override;
private:
	void Init( void );
	void CreateMonochromeMask( CDC* pMemoryDC, CDC* pMonoDC ) const;
private:
	HBITMAP m_hBitmapList;		// no ownership
	int m_index;
	CSize m_size;
	std::auto_ptr<CDibSectionTraits> m_pDibTraits;
};


class CIcon;


class CIconProxy : public ui::IImageProxy
	, private utl::noncopyable
{
public:
	CIconProxy( const CIcon* pIcon = nullptr );		// no ownership
	CIconProxy( HICON hIcon );						// with ownership
	virtual ~CIconProxy();

	// ui::IImageProxy interface
	virtual bool IsEmpty( void ) const override { return nullptr == m_pIcon; }
	virtual const CSize& GetSize( void ) const override;
	virtual bool HasTransparency( void ) const override { return true; }
	virtual void Draw( CDC* pDC, const CPoint& pos, COLORREF transpColor = CLR_NONE ) const override;
	virtual void DrawDisabled( CDC* pDC, const CPoint& pos, COLORREF transpColor = CLR_NONE ) const override;
private:
	void CreateMonochromeMask( CDC* pMemoryDC, CDC* pMonoDC ) const;
private:
	CIcon* m_pIcon;
	bool m_hasOwnership;
};


// Proxy for drawing 1 image in the image list.
//
struct CImageListProxy : public ui::IImageProxy
{
	CImageListProxy( CImageList* pImageList = nullptr, int imageIndex = NoImage, int overlayMask = NoOverlayMask );

	void Reset( CImageList* pImageList, int imageIndex );

	CImageList* GetImageList( void ) const { return m_pImageList; }
	void SetImageList( CImageList* pImageList );

	int GetImageIndex( void ) const { return m_imageIndex; }
	bool SetImageIndex( int imageIndex ) { return utl::ModifyValue( m_imageIndex, imageIndex ); }

	// ui::IImageProxy interface
	virtual bool IsEmpty( void ) const  override { return nullptr == m_pImageList || m_imageIndex < 0; }
	virtual const CSize& GetSize( void ) const override { return m_imageSize; }
	virtual bool HasTransparency( void ) const override { return true; }
	virtual void Draw( CDC* pDC, const CPoint& pos, COLORREF transpColor = CLR_NONE ) const override;
	virtual void DrawDisabled( CDC* pDC, const CPoint& pos, COLORREF transpColor = CLR_NONE ) const override;

	// overlay
	bool HasOverlayMask( void ) const { return m_overlayMask > NoOverlayMask; }
	void SetOverlayMask( int overlayMask ) { ASSERT( overlayMask >= NoOverlayMask && overlayMask <= 4 ); m_overlayMask = overlayMask; }

	bool HasExternalOverlay( void ) const { ASSERT( nullptr == m_pExternalOverlay || !m_pExternalOverlay->IsEmpty() ); return m_pExternalOverlay != nullptr; }
	const CImageListProxy* GetExternalOverlay( void ) const { return m_pExternalOverlay; }
	void SetExternalOverlay( const CImageListProxy* pExternalOverlay ) { m_pExternalOverlay = pExternalOverlay; }
private:
	void DrawDisabledImpl( CDC* pDC, const CPoint& pos, UINT style ) const;
	void DrawDisabledImpl_old( CDC* pDC, const CPoint& pos, UINT style ) const;
private:
	CImageList* m_pImageList;					// no ownership
	int m_imageIndex;

	int m_overlayMask;							// one-based index of the overlay mask (1->4 in the same image list)
	const CImageListProxy* m_pExternalOverlay;	// external overlay, could belong to a different image list
	CSize m_imageSize;
};


// Proxy for drawing all images in the image list.
//
class CImageListStripProxy : public ui::IImageProxy
{
public:
	CImageListStripProxy( const CImageList* pImageList );

	const CSize& GetImageSize( void ) const { return m_imageSize; }

	// ui::IImageProxy interface
	virtual bool IsEmpty( void ) const  override { return nullptr == m_pImageList; }
	virtual const CSize& GetSize( void ) const override { return m_stripSize; }
	virtual bool HasTransparency( void ) const override { return true; }
	virtual void Draw( CDC* pDC, const CPoint& pos, COLORREF transpColor = CLR_NONE ) const override;
	virtual void DrawDisabled( CDC* pDC, const CPoint& pos, COLORREF transpColor = CLR_NONE ) const override;
private:
	CImageList* m_pImageList;					// no ownership
	int m_imageCount;
	CSize m_imageSize;
	CSize m_stripSize;
};


class CColorBoxImage : public ui::IImageProxy
{
public:
	enum { AutoTextSize };

	CColorBoxImage( COLORREF color, const CSize& size = CSize( AutoTextSize, AutoTextSize ) );
	virtual ~CColorBoxImage();

	// ui::IImageProxy interface
	virtual bool IsEmpty( void ) const override { return false; }
	virtual const CSize& GetSize( void ) const override { return m_size; }
	virtual void SizeToText( CDC* pDC ) override;
	virtual void Draw( CDC* pDC, const CPoint& pos, COLORREF transpColor = CLR_NONE ) const;
	virtual void DrawDisabled( CDC* pDC, const CPoint& pos, COLORREF transpColor = CLR_NONE ) const;
private:
	void DrawImpl( CDC* pDC, const CPoint& pos, bool enabled ) const;
private:
	COLORREF m_color;
	CSize m_size;
};


#endif // ImageProxies_h
