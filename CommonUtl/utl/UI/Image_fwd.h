#ifndef Image_fwd_h
#define Image_fwd_h
#pragma once

#include "ISubject.h"
#include "ScopedGdi.h"


class CEnumTags;


namespace ui
{
	// image display

	enum ImageScalingMode
	{
		AutoFitLargeOnly,
		AutoFitAll,
		FitWidth,
		FitHeight,
		ActualSize,
		UseZoomPct
	};

	const CEnumTags& GetTags_ImageScalingMode( void );
}


namespace ui
{
	enum ImagingApi { WicApi, GpApi };

	const CEnumTags& GetTags_ImagingApi( void );


	enum ImageFileFormat { BitmapFormat, JpegFormat, TiffFormat, GifFormat, PngFormat, WmpFormat, IconFormat, UnknownImageFormat };

	ImageFileFormat FindImageFileFormat( const TCHAR imageFilePath[] );


	enum GlyphGauge { SmallGlyph, LargeGlyph, _GlyphGaugeCount };

	// implemented by clients that can draw custom images (such as thumbnails) over a transparent image entry in control's image list

	interface ICustomImageDraw
	{
		virtual CSize GetItemImageSize( GlyphGauge glyphGauge = SmallGlyph ) const = 0;
		virtual bool SetItemImageSize( const CSize& imageBoundsSize ) = 0;											// call when UI control drives image bounds size
		virtual bool DrawItemImage( CDC* pDC, const utl::ISubject* pSubject, const CRect& itemImageRect ) = 0;		// e.g. pSubject->GetCode() refers to a fs::CPath

		bool CustomDrawItemImage( const NMCUSTOMDRAW* pDraw, const CRect& itemImageRect )		// for CListCtrl, CTreeCtrl
		{
			// pDraw could point to NMLVCUSTOMDRAW, NMTVCUSTOMDRAW, etc
			ASSERT_PTR( pDraw );
			return DrawItemImage( CDC::FromHandle( pDraw->hdc ), checked_static_cast<const utl::ISubject*>( (const utl::ISubject*)pDraw->lItemlParam ), itemImageRect );
		}
	};
}


enum IconStdSize
{
	DefaultSize,
	SmallIcon,		// 16x16
	MediumIcon,		// 24x24
	LargeIcon,		// 32x32
	HugeIcon_48,	// 48x48
	HugeIcon_96,	// 96x96
	HugeIcon_128,	// 128x128
	HugeIcon_256	// 256x256
};


namespace ui
{
	int GetIconDimension( IconStdSize iconStdSize );
	IconStdSize LookupIconStdSize( int iconDimension, IconStdSize defaultStdSize = DefaultSize );
}


class CIconSize
{
public:
	CIconSize( void ) : m_size( 0, 0 ), m_stdSize( DefaultSize ) {}
	CIconSize( IconStdSize stdSize ) { Reset( stdSize ); }
	CIconSize( const CSize& size ) { Reset( size ); }

	void Reset( IconStdSize stdSize = DefaultSize )
	{
		m_stdSize = stdSize;
		m_size = GetSizeOf( m_stdSize );
	}

	void Reset( const CSize& size )
	{
		m_size = size;
		m_stdSize = FindStdSize( m_size );
	}

	const CSize& GetSize( void ) const { return m_size; }
	IconStdSize GetStdSize( void ) const { return m_stdSize; }

	static CSize GetSizeOf( IconStdSize iconStdSize ) { int iconDimension = ui::GetIconDimension( iconStdSize ); return CSize( iconDimension, iconDimension ); }
	static IconStdSize FindStdSize( const CSize& iconSize, IconStdSize defaultStdSize = DefaultSize ) { return ui::LookupIconStdSize( iconSize.cx, defaultStdSize ); }
private:
	CSize m_size;
	IconStdSize m_stdSize;
public:
	static const CIconSize s_small;
};


struct CIconId
{
	CIconId( UINT id = 0, IconStdSize stdSize = SmallIcon ) : m_id( id ), m_stdSize( stdSize ) {}

	bool IsValid( void ) const { return m_id != 0; }

	CSize GetStdSize( void ) const { return CIconSize::GetSizeOf( m_stdSize ); }
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


namespace gdi
{
	void CreateImageList( CImageList& rOutImageList, const CIconSize& imageSize, int countOrGrowBy, UINT flags = ILC_COLOR32 | ILC_MASK );	// if positive: actual count, if negative: growBy

	inline void CreateEmptyImageList( CImageList& rOutImageList, const CIconSize& imageSize, int growBy = 5, UINT flags = ILC_COLOR32 | ILC_MASK )
	{
		CreateImageList( rOutImageList, imageSize, -growBy, flags );
	}

	inline HICON ExtractIcon( const CImageList& imageList, size_t imagePos ) { return const_cast<CImageList&>( imageList ).ExtractIcon( static_cast<int>( imagePos ) ); }
}


namespace res
{
	HICON LoadIcon( const CIconId& iconId );

	// image-list from bitmap: loads strip from whichever comes first - PNG:32bpp with alpha (if found), or BMP:4/8/24bpp
	int LoadImageListDIB( CImageList& rOutImageList, UINT bitmapId, COLORREF transpColor = color::Auto,
						  int imageCount = -1, bool disabledEffect = false );

	// image-list from an icon-strip of custom size and multiple images
	int LoadImageListIconStrip( CImageList* pOutImageList, CSize* pOutImageSize, UINT iconStripId, UINT ilFlags = ILC_COLOR32 | ILC_MASK );

	// image-list from individual icons
	void LoadImageListIcons( CImageList& rOutImageList, const UINT iconIds[], size_t iconCount, IconStdSize iconStdSize = SmallIcon,
							 UINT ilFlags = ILC_COLOR32 | ILC_MASK );
}


class CIcon;


namespace ui
{
	interface IImageStore
	{
		virtual CIcon* FindIcon( UINT cmdId, IconStdSize iconStdSize = SmallIcon ) const = 0;
		virtual const CIcon* RetrieveIcon( const CIconId& cmdId ) = 0;
		virtual CBitmap* RetrieveBitmap( const CIconId& cmdId, COLORREF transpColor ) = 0;

		typedef std::pair<CBitmap*, CBitmap*> TBitmapPair;		// <bmp_unchecked, bmp_checked>

		virtual TBitmapPair RetrieveMenuBitmaps( const CIconId& cmdId ) = 0;
		virtual TBitmapPair RetrieveMenuBitmaps( const CIconId& cmdId, bool useCheckedBitmaps ) = 0;

		// utils
		const CIcon* RetrieveLargestIcon( UINT cmdId, IconStdSize maxIconStdSize = HugeIcon_48 );
		CBitmap* RetrieveMenuBitmap( const CIconId& cmdId ) { return RetrieveBitmap( cmdId, ::GetSysColor( COLOR_MENU ) ); }
		int BuildImageList( CImageList* pDestImageList, const UINT buttonIds[], size_t buttonCount, const CSize& imageSize );
	};


	ui::IImageStore* GetImageStoresSvc( void );
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
	CSize GetImageIconSize( const CImageList& imageList );

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


namespace gdi
{
	// drawing - transparent bitmaps are drawn transparently (only for SRCCOPY)

	bool DrawBitmap( CDC* pDC, HBITMAP hBitmap, const CPoint& pos, DWORD rop = SRCCOPY );
	bool DrawBitmap( CDC* pDC, HBITMAP hBitmap, const CRect& destRect, DWORD rop = SRCCOPY );		// stretch
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
	BYTE* GetPixelBuffer( void ) const { return static_cast<BYTE*>( dsBm.bmBits ); }

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
	std::auto_ptr<CPalette> m_pPalette;		// self-encapsulated, owned
};


// initialize a BITMAPINFO with proper color table according to bpp

class CBitmapInfoBuffer
{
public:
	CBitmapInfoBuffer( void ) {}

	bool IsValid( void ) const { return !m_buffer.empty(); }
	const BITMAPINFO* GetBitmapInfo( void ) const { ASSERT( IsValid() ); return reinterpret_cast<const BITMAPINFO*>( &m_buffer.front() ); }

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
