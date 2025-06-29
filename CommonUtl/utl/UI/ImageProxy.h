#ifndef ImageProxy_h
#define ImageProxy_h
#pragma once


enum
{
	AutoImage = -2,		// lookup the image
	NoImage = -1,		// no image specified

	NoOverlayMask = 0	// no overlay image specified
};


namespace ui
{
	interface IImageProxy : public utl::IMemoryManaged
	{
		virtual bool IsEmpty( void ) const = 0;
		virtual const CSize& GetSize( void ) const = 0;
		virtual void SizeToText( CDC* /*pDC*/ ) {}

		virtual void Draw( CDC* pDC, const CPoint& pos, COLORREF transpColor = color::Null ) const = 0;
		virtual void DrawDisabled( CDC* pDC, const CPoint& pos, COLORREF transpColor = color::Null ) const = 0;

		// implemented methods
		virtual bool HasTransparency( void ) const { return false; }
		virtual void FillBackground( CDC* pDC, const CPoint& pos, COLORREF bkColor ) const;
	};
}


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
	virtual void Draw( CDC* pDC, const CPoint& pos, COLORREF transpColor = color::Null ) const override;
	virtual void DrawDisabled( CDC* pDC, const CPoint& pos, COLORREF transpColor = color::Null ) const override;
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
{
public:
	CIconProxy( const CIcon* pIcon = nullptr );

	// ui::IImageProxy interface
	virtual bool IsEmpty( void ) const override { return nullptr == m_pIcon; }
	virtual const CSize& GetSize( void ) const override;
	virtual bool HasTransparency( void ) const override { return true; }
	virtual void Draw( CDC* pDC, const CPoint& pos, COLORREF transpColor = color::Null ) const override;
	virtual void DrawDisabled( CDC* pDC, const CPoint& pos, COLORREF transpColor = color::Null ) const override;
private:
	void CreateMonochromeMask( CDC* pMemoryDC, CDC* pMonoDC ) const;
private:
	const CIcon* m_pIcon;		// no ownership
};


struct CImageListProxy : public ui::IImageProxy
{
	CImageListProxy( CImageList* pImageList = nullptr, int index = NoImage, int overlayMask = NoOverlayMask );

	void Set( CImageList* pImageList, int index );

	// overlay
	bool HasOverlayMask( void ) const { return m_overlayMask > NoOverlayMask; }
	void SetOverlayMask( int overlayMask ) { ASSERT( overlayMask >= NoOverlayMask && overlayMask <= 4 ); m_overlayMask = overlayMask; }

	bool HasExternalOverlay( void ) const { ASSERT( nullptr == m_pExternalOverlay || !m_pExternalOverlay->IsEmpty() ); return m_pExternalOverlay != nullptr; }
	const CImageListProxy* GetExternalOverlay( void ) const { return m_pExternalOverlay; }
	void SetExternalOverlay( const CImageListProxy* pExternalOverlay ) { m_pExternalOverlay = pExternalOverlay; }

	// ui::IImageProxy interface
	virtual bool IsEmpty( void ) const  override { return nullptr == m_pImageList || m_index < 0; }
	virtual const CSize& GetSize( void ) const override;
	virtual bool HasTransparency( void ) const override { return true; }
	virtual void Draw( CDC* pDC, const CPoint& pos, COLORREF transpColor = color::Null ) const override;
	virtual void DrawDisabled( CDC* pDC, const CPoint& pos, COLORREF transpColor = color::Null ) const override;
private:
	void DrawDisabledImpl( CDC* pDC, const CPoint& pos, UINT style ) const;
	void DrawDisabledImpl_old( CDC* pDC, const CPoint& pos, UINT style ) const;
public:
	CImageList* m_pImageList;
	int m_index;
private:
	int m_overlayMask;							// one-based index of the overlay mask (1->4 in the same image list)
	const CImageListProxy* m_pExternalOverlay;	// external overlay, could belong to a different image list
	mutable CSize m_size;						// lazy self-encapsulated
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
	virtual void Draw( CDC* pDC, const CPoint& pos, COLORREF transpColor = color::Null ) const;
	virtual void DrawDisabled( CDC* pDC, const CPoint& pos, COLORREF transpColor = color::Null ) const;
private:
	void DrawImpl( CDC* pDC, const CPoint& pos, bool enabled ) const;
private:
	COLORREF m_color;
	CSize m_size;
};


#endif // ImageProxy_h
