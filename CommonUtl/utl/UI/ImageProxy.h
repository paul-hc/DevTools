#ifndef ImageProxy_h
#define ImageProxy_h
#pragma once


enum
{
	AutoImage = -2,		// lookup the image
	NoImage = -1,		// no image specified

	NoOverlayMask = 0	// no overlay image specified
};


interface IImageProxy : public utl::IMemoryManaged
{
	virtual bool IsEmpty( void ) const = 0;
	virtual const CSize& GetSize( void ) const = 0;
	virtual void SizeToText( CDC* /*pDC*/ ) {}

	virtual void Draw( CDC* pDC, const CPoint& pos, UINT style = ILD_TRANSPARENT ) const = 0;
	virtual void DrawDisabled( CDC* pDC, const CPoint& pos, UINT style = ILD_TRANSPARENT ) const = 0;
};


struct CImageProxy : public IImageProxy
{
	CImageProxy( CImageList* pImageList = NULL, int index = NoImage, int overlayMask = NoOverlayMask );

	void Set( CImageList* pImageList, int index );

	// overlay
	bool HasOverlayMask( void ) const { return m_overlayMask > NoOverlayMask; }
	void SetOverlayMask( int overlayMask ) { ASSERT( overlayMask >= NoOverlayMask && overlayMask <= 4 ); m_overlayMask = overlayMask; }

	bool HasExternalOverlay( void ) const { ASSERT( NULL == m_pExternalOverlay || !m_pExternalOverlay->IsEmpty() ); return m_pExternalOverlay != NULL; }
	const CImageProxy* GetExternalOverlay( void ) const;
	void SetExternalOverlay( const CImageProxy* pExternalOverlay ) { m_pExternalOverlay = pExternalOverlay; }

	// IImageProxy interface
	virtual bool IsEmpty( void ) const;
	virtual const CSize& GetSize( void ) const;
	virtual void Draw( CDC* pDC, const CPoint& pos, UINT style = ILD_TRANSPARENT ) const;
	virtual void DrawDisabled( CDC* pDC, const CPoint& pos, UINT style = ILD_TRANSPARENT ) const;
private:
	void DrawDisabledImpl( CDC* pDC, const CPoint& pos, UINT style ) const;
	void DrawDisabledImpl_old( CDC* pDC, const CPoint& pos, UINT style ) const;
public:
	CImageList* m_pImageList;
	int m_index;
private:
	int m_overlayMask;							// one-based index of the overlay mask (1->4 in the same image list)
	const CImageProxy* m_pExternalOverlay;		// external overlay, could belong to a different image list
	mutable CSize m_size;						// lazy self-encapsulated
};


class CBitmapProxy : public IImageProxy
{
public:
	CBitmapProxy( void );
	CBitmapProxy( HBITMAP hBitmapList, int bitmapIndex, const CSize& size );

	// IImageProxy interface
	virtual bool IsEmpty( void ) const;
	virtual const CSize& GetSize( void ) const;
	virtual void Draw( CDC* pDC, const CPoint& pos, UINT style = ILD_TRANSPARENT ) const;
	virtual void DrawDisabled( CDC* pDC, const CPoint& pos, UINT style = ILD_TRANSPARENT ) const;
private:
	void CreateMonochromeMask( CDC* pMemoryDC, CDC* pMonoDC ) const;
private:
	HBITMAP m_hBitmapList; // no ownership
	int m_index;
	CSize m_size;
};


class CColorBoxImage : public IImageProxy
{
public:
	enum { AutoTextSize };

	CColorBoxImage( COLORREF color, const CSize& size = CSize( AutoTextSize, AutoTextSize ) );
	virtual ~CColorBoxImage();

	// IImageProxy interface
	virtual bool IsEmpty( void ) const;
	virtual const CSize& GetSize( void ) const;
	virtual void SizeToText( CDC* pDC );
	virtual void Draw( CDC* pDC, const CPoint& pos, UINT style = ILD_TRANSPARENT ) const;
	virtual void DrawDisabled( CDC* pDC, const CPoint& pos, UINT style = ILD_TRANSPARENT ) const;
private:
	void DrawImpl( CDC* pDC, const CPoint& pos, bool enabled ) const;
private:
	COLORREF m_color;
	CSize m_size;
};


#endif // ImageProxy_h
