#ifndef Image_fwd_h
#define Image_fwd_h
#pragma once

#include "ISubject.h"
#include "ScopedGdi.h"


class CEnumTags;


namespace ui
{
	// image display

	enum AutoImageSize
	{
		AutoFitLargeOnly,
		AutoFitAll,
		FitWidth,
		FitHeight,
		ActualSize,
		UseZoomPct
	};

	const CEnumTags& GetTags_AutoImageSize( void );
}


namespace ui
{
	enum ImagingApi { WicApi, GpApi };

	const CEnumTags& GetTags_ImagingApi( void );


	// implemented by clients that can draw custom images (such as thumbnails) over a transparent image entry in control's image list

	interface ICustomImageDraw
	{
		enum ImageType { SmallImage, LargeImage };

		virtual CSize GetItemImageSize( ImageType imageType = SmallImage ) const = 0;
		virtual bool DrawItemImage( CDC* pDC, const utl::ISubject* pSubject, const CRect& itemImageRect ) = 0;		// pSubject->GetCode() must refer to a fs::CPath or fs::CFlexPath

		bool CustomDrawItemImage( const NMCUSTOMDRAW* pDraw, const CRect& itemImageRect )		// for CListCtrl, CTreeCtrl
		{
			// pDraw could point to NMTVCUSTOMDRAW, NMLVCUSTOMDRAW, etc
			ASSERT_PTR( pDraw );
			return DrawItemImage( CDC::FromHandle( pDraw->hdc ), checked_static_cast< const utl::ISubject* >( (const utl::ISubject*)pDraw->lItemlParam ), itemImageRect );
		}
	};
}


enum IconStdSize
{
	DefaultSize,
	SmallIcon,		// 16x16
	MediumIcon,		// 24x24
	LargeIcon,		// 32x32
	HugeIcon,		// 48x48
	EnormousIcon	// 256x256
};


struct CIconId
{
	CIconId( UINT id = 0, IconStdSize stdSize = SmallIcon ) : m_id( id ), m_stdSize( stdSize ) {}

	bool IsValid( void ) const { return m_id != 0; }

	CSize GetStdSize( void ) const { return GetStdSize( m_stdSize ); }
	static CSize GetStdSize( IconStdSize iconStdSize );
	static IconStdSize FindStdSize( const CSize& iconSize );
public:
	UINT m_id;
	IconStdSize m_stdSize;
};


// changes the module instance returned by AfxGetResourceHandle(); use default contructor if the resource is a system resource (hInstance is NULL)

class CScopedResInst
{
public:
	CScopedResInst( void ) : m_hOldResInst( AfxGetResourceHandle() ) { AfxSetResourceHandle( (HINSTANCE)WinResource ); }	// resource located in Windows - LoadIcon( IDI_WINLOGO, NULL )
	CScopedResInst( HINSTANCE hResInst ) : m_hOldResInst( AfxGetResourceHandle() ) { AfxSetResourceHandle( hResInst ); }	// resource located in an existing module
	~CScopedResInst() { AfxSetResourceHandle( m_hOldResInst ); }

	static HINSTANCE Get( void ) { return IsSysResource() ? NULL : AfxGetResourceHandle(); }

	static HINSTANCE Find( UINT resId, const TCHAR* pResType ) { return Find( MAKEINTRESOURCE( resId ), pResType ); }
	static HINSTANCE Find( const TCHAR* pResName, const TCHAR* pResType )
	{
		pResName, pResType;			// prevent compiler warnings
		return IsSysResource() ? NULL : AfxFindResourceHandle( pResName, pResType );
	}

	static bool IsSysResource( void ) { return AfxGetResourceHandle() == (HINSTANCE)WinResource; }
private:
	HINSTANCE m_hOldResInst;
	enum { WinResource = -1 };
};


namespace res
{
	HICON LoadIcon( const CIconId& iconId );

	// icons image-list
	void LoadImageList( CImageList& rOutImageList, const UINT* pIconIds, size_t iconCount, IconStdSize iconStdSize = SmallIcon,
						UINT ilFlags = ILC_COLOR32 | ILC_MASK );

	// bitmap image-list: works with 32bpp PNG or 4/8/24bpp BMP (whichever comes first)
	bool LoadImageList( CImageList& rOutImageList, UINT bitmapId, int imageCount, const CSize& imageSize,
						COLORREF transpColor = color::Auto, bool disabledEffect = false );


	typedef CIconId CStripId;

	inline bool LoadImageList( CImageList& rOutImageList, const CStripId& stripId, int imageCount, COLORREF transpColor = color::Auto )
	{
		return LoadImageList( rOutImageList, stripId.m_id, imageCount, CIconId::GetStdSize( stripId.m_stdSize ), transpColor );
	}
}


// DIB & DDB bitmap info


namespace gdi
{
	// Bitmap orientation: known at creation time in BITMAPINFO, must be passed from creation (can't be retrofitted).
	//	if BITMAPINFO::biHeight < 0 is a top-down DIB and its origin is the top-left corner
	//	if positive is bottom-up (default) and its origin is the bottom-left corner
	//
	enum Orientation { BottomUp, TopDown };


	bool IsDibSection( HBITMAP hBitmap );

	CSize GetBitmapSize( HBITMAP hBitmap );
	CSize GetImageSize( const CImageList& imageList );

	WORD GetBitsPerPixel( HBITMAP hBitmap, bool* pIsDibSection = NULL );
	bool Is32BitBitmap( HBITMAP hBitmap );					// DIB/DDB

	// stride: number of bytes per scan line (aka pitch)
	inline UINT GetDibStride( UINT width, UINT bitsPerPixel ) { return ( ( ( width * bitsPerPixel ) + 31 ) / 32 ) * 4; }

	// total size of the RGB DIB
	inline UINT GetDibBufferSize( UINT height, UINT stride ) { return height * stride; }

	bool HasAlphaTransparency( HBITMAP hBitmap );			// DIB section with alpha channel?
	bool HasAlphaTransparency( const CImageList& imageList, int imagePos = 0 );
	bool HasMask( const CImageList& imageList, int imagePos = 0 );

	bool CreateBitmapMask( CBitmap& rMaskBitmap, HBITMAP hSrcBitmap, COLORREF transpColor );

	// icon from bitmap(s)
	HICON CreateIcon( HBITMAP hImageBitmap, HBITMAP hMaskBitmap );
	HICON CreateIcon( HBITMAP hImageBitmap, COLORREF transpColor );


	inline RGBQUAD ToRgbQuad( const PALETTEENTRY& palEntry )
	{
		RGBQUAD rgb = { palEntry.peBlue, palEntry.peGreen, palEntry.peRed, 0 };
		return rgb;
	}

	inline PALETTEENTRY ToPaletteEntry( const RGBQUAD& rgb )
	{
		PALETTEENTRY palEntry = { rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue, 0 };
		return palEntry;
	}
}


struct CDibMeta		// contains information that must be passed from creation
{
	CDibMeta( HBITMAP hDib = NULL )
		: m_hDib( hDib ), m_orientation( gdi::BottomUp ), m_bitsPerPixel( 0 ), m_channelCount( 0 ) {}

	bool IsValid( void ) const { return m_hDib != NULL; }
	bool HasAlpha( void ) const { return 32 == m_bitsPerPixel && 4 == m_channelCount; }

	bool StorePixelFormat( void );
	void StorePixelFormat( const BITMAPINFO& dibInfo );
	void CopyPixelFormat( const CDibMeta& right );
private:
	void StoreChannelCount( bool isDibSection );		// based on m_bitsPerPixel
public:
	HBITMAP m_hDib;
	gdi::Orientation m_orientation;		// known at creation time in BITMAPINFO, must be passed from creation (can't be retrofitted)
	UINT m_bitsPerPixel;
	UINT m_channelCount;
};


struct CBitmapInfo : public tagDIBSECTION		// DIB & DDB bitmap info
{
	CBitmapInfo( HBITMAP hBitmap ) : m_structSize( 0 ) { Build( hBitmap ); }

	void Build( HBITMAP hBitmap ) { m_structSize = ::GetObject( hBitmap, sizeof( DIBSECTION ), this ); }

	bool IsValid( void ) const { return m_structSize != 0; }
	bool IsDibSection( void ) const { return sizeof( DIBSECTION ) == m_structSize; }
	bool IsDDB( void ) const { return sizeof( BITMAP ) == m_structSize; }				// tagDIBSECTION::dsBm (BITMAP)

	// DIB
	bool IsIndexed( void ) const { ASSERT( IsDibSection() ); return dsBmih.biBitCount <= 8; }
	bool HasColorTable( void ) const { return IsIndexed(); }
	bool HasAlphaChannel( void ) const { return IsDibSection() && dsBmih.biBitCount >= 32; }

	// DIB/DDB
	bool IsMonochrome( void ) const { ASSERT( IsValid() ); return 1 == dsBm.bmBitsPixel; }
	bool IsTrueColor( void ) const { ASSERT( IsValid() ); return dsBm.bmBitsPixel >= 24; }
	bool Is32Bit( void ) const { ASSERT( IsValid() ); return dsBm.bmBitsPixel >= 32; }

	int GetWidth( void ) const { return dsBm.bmWidth; }
	int GetHeight( void ) const { return dsBm.bmHeight; }
	CSize GetBitmapSize( void ) const { return CSize( dsBm.bmWidth, dsBm.bmHeight ); }

	WORD GetBitsPerPixel( void ) const { return dsBm.bmBitsPixel; }
	BYTE* GetPixelBuffer( void ) const { return static_cast< BYTE* >( dsBm.bmBits ); }

	std::tstring FormatDbg( void ) const;
private:
	int m_structSize;
};


namespace bmp { class CSharedAccess; }


class CDibSectionInfo : public CBitmapInfo		// DIB only
{
public:
	CDibSectionInfo( HBITMAP hDib );
	~CDibSectionInfo();

	void Build( HBITMAP hDib );

	bool IsValid( void ) const { return IsDibSection(); }

	UINT GetStride( void ) const { return gdi::GetDibStride( GetWidth(), GetBitsPerPixel() ); }
	UINT GetBufferSize( void ) const { return gdi::GetDibBufferSize( GetHeight(), GetStride() ); }

	UINT GetColorTableMaxSize( void ) const { return IsIndexed() ? ( 1 << GetBitsPerPixel() ) : 0; }
	UINT GetUsedColorTableSize( void ) const { return dsBmih.biClrUsed != 0 ? dsBmih.biClrUsed : GetColorTableMaxSize(); }

	// the DIB must be selected into pDC
	COLORREF GetColorAt( const CDC* pDC, int index ) const;
	const std::vector< RGBQUAD >& GetColorTable( const CDC* pDC );
	CPalette* MakeColorPalette( const CDC* pDC );		// palette is owned

	template< typename PixelFunc >
	bool ForEachInColorTable( const bmp::CSharedAccess& dib, PixelFunc func );
private:
	// hide DDB base methods
	using CBitmapInfo::IsDibSection;
	using CBitmapInfo::IsDDB;
private:
	HBITMAP m_hDib;								// no ownership
	std::vector< RGBQUAD > m_colorTable;		// self-encapsulated
	std::auto_ptr< CPalette > m_pPalette;		// self-encapsulated, owned
};


template< typename PixelFunc >
bool CDibSectionInfo::ForEachInColorTable( const bmp::CSharedAccess& dib, PixelFunc func )
{
	ASSERT( IsIndexed() && m_hDib == dib.GetHandle() );
	CDC* pDC = dib.GetBitmapMemDC();
	GetColorTable( pDC );

	for ( std::vector< RGBQUAD >::iterator itRgb = m_colorTable.begin(); itRgb != m_colorTable.end(); ++itRgb )
	{
		CPixelBGR pixel( *itRgb );
		func( pixel );
		itRgb->rgbBlue = pixel.m_blue;
		itRgb->rgbGreen = pixel.m_green;
		itRgb->rgbRed = pixel.m_red;
	}
	// modify the color table of the DIB selected in pDC
	return ::SetDIBColorTable( *pDC, 0, (UINT)m_colorTable.size(), &m_colorTable.front() ) == m_colorTable.size();
}


// initialize a BITMAPINFO with proper color table according to bpp

class CBitmapInfoBuffer
{
public:
	CBitmapInfoBuffer( void ) {}

	bool IsValid( void ) const { return !m_buffer.empty(); }
	const BITMAPINFO* GetBitmapInfo( void ) const { ASSERT( IsValid() ); return reinterpret_cast< const BITMAPINFO* >( &m_buffer.front() ); }

	BITMAPINFO* CreateDibInfo( int width, int height, UINT bitsPerPixel, const bmp::CSharedAccess* pSrcDib = NULL );
	BITMAPINFO* CreateDibInfo( UINT bitsPerPixel, const bmp::CSharedAccess& sourceDib );
private:
	std::vector< BYTE > m_buffer;
};


class CSysColorTable
{
public:
	static const std::vector< RGBQUAD >& GetSysRgbTable( void );

	static void MakeRgbTable( RGBQUAD* pRgbTable, size_t size );		// use (1 << bpp) as size; works for 1/4/8 bit

	static inline void MakeRgbTable( std::vector< RGBQUAD >& rRgbTable, size_t size )
	{
		rRgbTable.resize( size );
		MakeRgbTable( &rRgbTable.front(), size );						// use (1 << bpp) as size; works for 1/4/8 bit
	}

	static void MakeColorTable( std::vector< COLORREF >& rColorTable, size_t size );
};


#endif // Image_fwd_h
