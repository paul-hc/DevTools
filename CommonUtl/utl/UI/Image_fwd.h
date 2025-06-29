#ifndef Image_fwd_h
#define Image_fwd_h
#pragma once

#include "ISubject.h"
#include "ScopedGdi.h"
#include "ComparePredicates.h"
#include <commoncontrols.h>			// IImageList, IImageList2


#define RT_PNG	_T("PNG")
#define RT_GIF	_T("GIF")
#define RT_TIFF	_T("TIFF")
#define RT_JPG	_T("JPG")
#define RT_WMP	_T("WMP")


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
		virtual CSize GetItemImageSize( ui::GlyphGauge glyphGauge = ui::SmallGlyph ) const = 0;
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
	HugeIcon_256,	// 256x256

	AnyIconSize		// for filtering
};


namespace ui
{
	int GetIconDimension( IconStdSize iconStdSize );
	inline int GetIconDimension( const CSize& imageSize ) { return std::max( imageSize.cx, imageSize.cy ); }
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

	CSize GetSize( void ) const { return CIconSize::GetSizeOf( m_stdSize ); }
public:
	UINT m_id;
	IconStdSize m_stdSize;
};


// changes the module instance returned by AfxGetResourceHandle(); use default contructor if the resource is a system resource (hInstance is NULL)

class CScopedResInst
{
public:
	CScopedResInst( void ) : m_hOldResInst( AfxGetResourceHandle() ) { AfxSetResourceHandle( (HINSTANCE)WinResource ); }	// resource located in Windows - LoadIcon( IDI_WINLOGO, nullptr )
	CScopedResInst( HINSTANCE hResInst ) : m_hOldResInst( AfxGetResourceHandle() ) { AfxSetResourceHandle( hResInst ); }	// resource located in an existing module
	~CScopedResInst() { AfxSetResourceHandle( m_hOldResInst ); }

	static HINSTANCE Get( void ) { return IsSysResource() ? nullptr : AfxGetResourceHandle(); }

	static HINSTANCE Find( UINT resId, const TCHAR* pResType ) { return Find( MAKEINTRESOURCE( resId ), pResType ); }
	static HINSTANCE Find( const TCHAR* pResName, const TCHAR* pResType )
	{
		pResName, pResType;			// prevent compiler warnings
		return IsSysResource() ? nullptr : AfxFindResourceHandle( pResName, pResType );
	}

	static bool IsSysResource( void ) { return AfxGetResourceHandle() == (HINSTANCE)WinResource; }
private:
	HINSTANCE m_hOldResInst;
	enum { WinResource = -1 };
};


typedef UINT TBitsPerPixel;
typedef UINT TImageListFlags;		// DIB image list color flags

enum
{
	ILC_ONLY_COLORS_MASK = ILC_COLOR4 | ILC_COLOR8 | ILC_COLOR16 | ILC_COLOR24 | ILC_COLOR32,
	ILC_ALL_COLORS_MASK = ILC_MASK | ILC_ONLY_COLORS_MASK
};


namespace ui
{
	struct CImageListInfo
	{
		CImageListInfo( void )
			: m_imageCount( 0 ), m_imageSize( 0, 0 ), m_ilFlags( 0 ) {}

		CImageListInfo( int imageCount, const CSize& imageSize, TImageListFlags ilFlags )
			: m_imageCount( imageCount ), m_imageSize( imageSize ), m_ilFlags( ilFlags ) {}

		bool IsValid( void ) const { return m_ilFlags != 0; }
		bool HasAlpha( void ) const { REQUIRE( IsValid() ); return ILC_COLOR32 == GetBitsPerPixel() && !HasFlag( m_ilFlags, ILC_MASK ); }

		TBitsPerPixel GetBitsPerPixel( void ) const { return GetBitsPerPixel( m_ilFlags ); }
		bool IsMonochrome( void ) const { return 1 == GetBitsPerPixel(); }

		static TImageListFlags GetImageListFlags( TBitsPerPixel bitsPerPixel, bool hasAlpha );		// ILC_COLOR... + ILC_MASK flags
		static TBitsPerPixel GetBitsPerPixel( TImageListFlags ilFlags ) { return ILC_MASK == ilFlags ? 1 : ( ilFlags & ILC_ONLY_COLORS_MASK ); }	// exclude ILC_MASK if not monochrome
	public:
		int m_imageCount;
		CSize m_imageSize;
		TImageListFlags m_ilFlags;
	};
}


namespace gdi
{
	ui::CImageListInfo CreateImageList( CImageList& rOutImageList, const CIconSize& imageSize, int countOrGrowBy, TImageListFlags ilFlags = ILC_COLOR32 | ILC_MASK );	// countOrGrowBy - if positive: actual count, if negative: growBy

	inline ui::CImageListInfo CreateEmptyImageList( CImageList& rOutImageList, const CIconSize& imageSize, int growBy = 5, TImageListFlags ilFlags = ILC_COLOR32 | ILC_MASK )
	{
		return CreateImageList( rOutImageList, imageSize, -growBy, ilFlags );
	}

	inline HICON ExtractIcon( const CImageList& imageList, size_t imagePos ) { return const_cast<CImageList&>( imageList ).ExtractIcon( static_cast<int>( imagePos ) ); }
}


namespace res
{
	HICON LoadIcon( const CIconId& iconId, UINT fuLoad = LR_DEFAULTCOLOR );

	// image-list from bitmap: loads strip from whichever comes first - PNG:32bpp with alpha (if found), or BMP:4/8/24bpp
	ui::CImageListInfo LoadImageListDIB( CImageList& rOutImageList, UINT bitmapId, COLORREF transpColor = color::Auto,
										 int imageCount = -1, bool disabledEffect = false );

	// image-list from an icon-strip of custom size and multiple images.
	//	- just for illustration, so not really used since non-square icons ar non standard!
	ui::CImageListInfo _LoadImageListIconStrip( CImageList* pOutImageList, CSize* pOutImageSize, UINT iconStripId );

	// image-list from individual icons
	ui::CImageListInfo LoadImageListIcons( CImageList& rOutImageList, const UINT iconIds[], size_t iconCount, IconStdSize iconStdSize = SmallIcon,
										   TImageListFlags ilFlags = ILC_COLOR32 | ILC_MASK );
}


namespace ui
{
	struct CIconEntry
	{
		CIconEntry( void ) : m_bitsPerPixel( 0 ), m_dimension( 0 ), m_stdSize( DefaultSize ) {}

		CIconEntry( TBitsPerPixel bitsPerPixel, IconStdSize stdSize )
			: m_bitsPerPixel( bitsPerPixel )
			, m_dimension( ui::GetIconDimension( stdSize ) )
			, m_stdSize( stdSize )
		{
		}

		CIconEntry( TBitsPerPixel bitsPerPixel, int dimension )
			: m_bitsPerPixel( bitsPerPixel )
			, m_dimension( dimension )
			, m_stdSize( ui::LookupIconStdSize( m_dimension ) )
		{
		}

		CIconEntry( TBitsPerPixel bitsPerPixel, const CSize& imageSize )
			: m_bitsPerPixel( bitsPerPixel )
			, m_dimension( ui::GetIconDimension( imageSize ) )
			, m_stdSize( ui::LookupIconStdSize( m_dimension ) )
		{
		}

		bool operator==( const CIconEntry& right ) const;

		CSize GetSize( void ) const { return CSize( m_dimension, m_dimension ); }
	public:
		TBitsPerPixel m_bitsPerPixel;		// ILC_COLOR32/ILC_COLOR24/ILC_COLOR16/ILC_COLOR8/ILC_COLOR4/ILC_MASK
		int m_dimension;					// 16(x16), 24(x24), 32(x32), etc
		IconStdSize m_stdSize;				// SmallIcon/MediumIcon/LargeIcon/...
	};


	struct CIconKey : public CIconEntry
	{
		CIconKey( void ) : CIconEntry(), m_iconResId( 0 ) {}
		CIconKey( UINT iconResId, const CIconEntry& iconEntry ) : CIconEntry( iconEntry ), m_iconResId( iconResId ) {}
	public:
		UINT m_iconResId;
	};
}


class CIcon;
class CIconGroup;


namespace ui
{
	class CToolbarDescr;


	interface IImageStore
	{
		virtual CIconGroup* FindIconGroup( UINT cmdId ) const = 0;
		virtual const CIcon* RetrieveIcon( const CIconId& cmdId ) = 0;
		virtual CBitmap* RetrieveBitmap( const CIconId& cmdId, COLORREF transpColor ) = 0;

		typedef std::pair<CBitmap*, CBitmap*> TBitmapPair;		// <bmp_unchecked, bmp_checked>

		virtual TBitmapPair RetrieveMenuBitmaps( const CIconId& cmdId ) = 0;
		virtual TBitmapPair RetrieveMenuBitmaps( const CIconId& cmdId, bool useCheckedBitmaps ) = 0;

		virtual void QueryToolbarDescriptors( std::vector<ui::CToolbarDescr*>& rToolbarDescrs ) const = 0;
		virtual void QueryToolbarsWithButton( std::vector<ui::CToolbarDescr*>& rToolbarDescrs, UINT cmdId ) const = 0;
		virtual void QueryIconGroups( std::vector<CIconGroup*>& rIconGroups ) const = 0;
		virtual void QueryIconKeys( std::vector<ui::CIconKey>& rIconKeys, IconStdSize iconStdSize = AnyIconSize ) const = 0;

		// utils
		CIcon* FindIcon( UINT cmdId, IconStdSize iconStdSize = SmallIcon, TBitsPerPixel bitsPerPixel = ILC_COLOR ) const;		// ILC_COLOR means best match, i.e. highest color
		const CIcon* RetrieveLargestIcon( UINT cmdId, IconStdSize maxIconStdSize = HugeIcon_48 );
		CBitmap* RetrieveMenuBitmap( const CIconId& cmdId ) { return RetrieveBitmap( cmdId, ::GetSysColor( COLOR_MENU ) ); }
		int BuildImageList( CImageList* pDestImageList, const UINT buttonIds[], size_t buttonCount, const CSize& imageSize );
	};


	ui::IImageStore* GetImageStoresSvc( void );
}


/// DIB & DDB bitmap info

namespace ui { interface IImageProxy; }


namespace gdi
{
	// Bitmap orientation: known at creation time in BITMAPINFO, must be passed from creation (can't be retrofitted).
	//	if BITMAPINFO::biHeight < 0 is a top-down DIB and its origin is the top-left corner
	//	if positive is bottom-up (default) and its origin is the bottom-left corner
	//
	enum Orientation { BottomUp, TopDown };


	bool IsDibSection( HBITMAP hBitmap );

	CSize GetBitmapSize( HBITMAP hBitmap );

	TBitsPerPixel GetBitsPerPixel( HBITMAP hBitmap, bool* pIsDibSection = nullptr );
	bool Is32BitBitmap( HBITMAP hBitmap );					// DIB/DDB

	// stride: number of bytes per scan line (aka pitch)
	inline UINT GetDibStride( UINT width, UINT bitsPerPixel ) { return ( ( ( width * bitsPerPixel ) + 31 ) / 32 ) * 4; }

	// total size of the RGB DIB
	inline UINT GetDibBufferSize( UINT height, UINT stride ) { return height * stride; }

	bool HasAlphaTransparency( HBITMAP hBitmap );			// DIB section with alpha channel?

	bool CreateBitmapMask( CBitmap& rMaskBitmap, HBITMAP hSrcBitmap, COLORREF transpColor );

	/// disabled gray look for icons, bitmaps, imagelists (good-looking, faded) - caller owns the gray bitmap:
	HBITMAP CreateFadedGrayDIBitmap( const ui::IImageProxy* pImageProxy, TBitsPerPixel srcBPP, COLORREF transpColor = CLR_NONE );
	HBITMAP CreateFadedGrayDIBitmap( HBITMAP hBitmapSrc, TBitsPerPixel srcBPP = 0, COLORREF transpColor = CLR_NONE );


	/// image list properties:

	CComPtr<IImageList> QueryImageListItf( HIMAGELIST hImageList );
	CComPtr<IImageList2> QueryImageList2Itf( HIMAGELIST hImageList );
	DWORD GetImageIconFlags( HIMAGELIST hImageList, int imagePos = 0 );
	bool HasAlphaTransparency( HIMAGELIST hImageList, int imagePos = 0 );
	bool HasMask( HIMAGELIST hImageList, int imagePos = 0 );
	CSize GetImageIconSize( HIMAGELIST hImageList );

	/// icon from bitmap(s):

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
	CDibMeta( HBITMAP hDib = nullptr )
		: m_hDib( hDib ), m_orientation( gdi::BottomUp ), m_bitsPerPixel( 0 ), m_channelCount( 0 ) {}

	bool Reset( HBITMAP hDib = nullptr );				// deletes (replaces) existing DIB
	bool AssignIfValid( const CDibMeta& srcDib );		// deletes (replaces) existing DIB

	bool IsValid( void ) const { return m_hDib != nullptr; }
	bool HasAlpha( void ) const { return 32 == m_bitsPerPixel && 4 == m_channelCount; }

	TImageListFlags GetImageListFlags( void ) const { REQUIRE( IsValid() ); return ui::CImageListInfo::GetImageListFlags( m_bitsPerPixel, HasAlpha() ); }

	bool StorePixelFormat( void );
	void StorePixelFormat( const BITMAPINFO& dibInfo );
	void CopyPixelFormat( const CDibMeta& right );
private:
	void StoreChannelCount( bool isDibSection );		// based on m_bitsPerPixel
public:
	HBITMAP m_hDib;
	gdi::Orientation m_orientation;		// known at creation time in BITMAPINFO, must be passed from creation (can't be retrofitted)
	TBitsPerPixel m_bitsPerPixel;
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


class CDibSectionTraits : public CBitmapInfo		// DIB only
{
public:
	CDibSectionTraits( HBITMAP hDib );
	~CDibSectionTraits();

	void Build( HBITMAP hDib );

	bool IsValid( void ) const { return IsDibSection(); }

	UINT GetStride( void ) const { return gdi::GetDibStride( GetWidth(), GetBitsPerPixel() ); }
	UINT GetBufferSize( void ) const { return gdi::GetDibBufferSize( GetHeight(), GetStride() ); }

	UINT GetColorTableMaxSize( void ) const { return IsIndexed() ? ( 1 << GetBitsPerPixel() ) : 0; }
	UINT GetUsedColorTableSize( void ) const { return dsBmih.biClrUsed != 0 ? dsBmih.biClrUsed : GetColorTableMaxSize(); }

	// the DIB must be selected into pDC
	COLORREF GetColorAt( const CDC* pDC, int index ) const;
	const std::vector<RGBQUAD>& GetColorTable( const CDC* pDC );

	CPalette* MakeColorPalette( const CDC* pDC );			// palette is owned - created only for BPP <= 256
	CPalette* MakeColorPaletteFallback( const CDC* pDC );	// palette is owned - creates a fallback halftone palette for hi-color DIBs

	template< typename PixelFunc >
	bool ForEachInColorTable( const bmp::CSharedAccess& dib, PixelFunc func );
private:
	// hide DDB base methods
	using CBitmapInfo::IsDibSection;
	using CBitmapInfo::IsDDB;
private:
	HBITMAP m_hDib;							// no ownership
	std::vector<RGBQUAD> m_colorTable;		// self-encapsulated
	std::auto_ptr<CPalette> m_pPalette;		// self-encapsulated, owned
};


// initialize a BITMAPINFO with proper color table according to bpp

class CBitmapInfoBuffer
{
public:
	CBitmapInfoBuffer( void ) {}

	bool IsValid( void ) const { return !m_buffer.empty(); }
	const BITMAPINFO* GetBitmapInfo( void ) const { ASSERT( IsValid() ); return reinterpret_cast<const BITMAPINFO*>( &m_buffer.front() ); }

	BITMAPINFO* CreateDibInfo( int width, int height, UINT bitsPerPixel, const bmp::CSharedAccess* pSrcDib = nullptr );
	BITMAPINFO* CreateDibInfo( UINT bitsPerPixel, const bmp::CSharedAccess& sourceDib );
private:
	std::vector<BYTE> m_buffer;
};


#endif // Image_fwd_h
