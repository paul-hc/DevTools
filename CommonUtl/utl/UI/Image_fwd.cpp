
#include "pch.h"
#include "Image_fwd.h"
#include "ImageStore.h"
#include "ImageProxy.h"
#include "GroupIconRes.h"
#include "DibSection.h"
#include "DibPixels.h"
#include "Pixel.h"
#include "ScopedGdi.h"
#include "ToolImageList.h"
#include "utl/Algorithms.h"
#include "utl/ContainerOwnership.h"
#include "utl/EnumTags.h"
#include "utl/Path.h"
#include "utl/StdHashValue.h"
#include "utl/StreamStdTypes.h"
#include <afxglobals.h>				// GetGlobalData()
#include <afxdrawmanager.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "Image_fwd.hxx"


const CIconSize CIconSize::s_small( SmallIcon );


namespace ui
{
	const CEnumTags& GetTags_ImageScalingMode( void )
	{
		static const CEnumTags s_tags( _T("Auto Fit (Large Only)|Auto Fit (All)|Fit Width|Fit Height|Actual Size (100%)|Use Zoom Level") );
		return s_tags;
	}

	const CEnumTags& GetTags_ImagingApi( void )
	{
		static const CEnumTags s_tags( _T("Use WIC API|Use GDI+ API (ATL)") );
		return s_tags;
	}

	ImageFileFormat FindImageFileFormat( const TCHAR imageFilePath[] )
	{
		const TCHAR* pExt = path::FindExt( imageFilePath );

		if ( str::Equals<str::IgnoreCase>( _T(".bmp"), pExt ) ||
			 str::Equals<str::IgnoreCase>( _T(".dib"), pExt ) ||
			 str::Equals<str::IgnoreCase>( _T(".rle"), pExt ) )
			return BitmapFormat;
		else if ( str::Equals<str::IgnoreCase>( _T(".jpg"), pExt ) ||
			 str::Equals<str::IgnoreCase>( _T(".jpeg"), pExt ) ||
			 str::Equals<str::IgnoreCase>( _T(".jpe"), pExt ) ||
			 str::Equals<str::IgnoreCase>( _T(".jfif"), pExt ) )
			return JpegFormat;
		else if ( str::Equals<str::IgnoreCase>( _T(".tif"), pExt ) ||
				  str::Equals<str::IgnoreCase>( _T(".tiff"), pExt ) )
			return TiffFormat;
		else if ( str::Equals<str::IgnoreCase>( _T(".gif"), pExt ) )
			return GifFormat;
		else if ( str::Equals<str::IgnoreCase>( _T(".png"), pExt ) )
			return PngFormat;
		else if ( str::Equals<str::IgnoreCase>( _T(".wmp"), pExt ) )
			return WmpFormat;			// Windows Media Photo
		else if ( str::Equals<str::IgnoreCase>( _T(".ico"), pExt ) ||
				  str::Equals<str::IgnoreCase>( _T(".cur"), pExt ) )
			return IconFormat;

		return UnknownImageFormat;
	}


	int GetIconDimension( IconStdSize iconStdSize )
	{
		// note: icon standard size must be invariant to Windows scaling, so we mustn't use GetSystemMetrics(SM_CXSMICON) which varies with scaling
		switch ( iconStdSize )
		{
			default: ASSERT( false );
			case DefaultSize:	return 0;		// load with existing icon size (cound be different than standard sizes)
			case SmallIcon:		return 16;
			case MediumIcon:	return 24;
			case LargeIcon:		return 32;
			case HugeIcon_48:	return 48;
			case HugeIcon_96:	return 96;
			case HugeIcon_128:	return 128;
			case HugeIcon_256:	return 256;
		}
	}

	IconStdSize LookupIconStdSize( int iconDimension, IconStdSize defaultStdSize /*= DefaultSize*/ )
	{
		switch ( iconDimension )
		{
			case 16:	return SmallIcon;
			case 24:	return MediumIcon;
			case 32:	return LargeIcon;
			case 48:	return HugeIcon_48;
			case 96:	return HugeIcon_96;
			case 128:	return HugeIcon_128;
			case 256:	return HugeIcon_256;
		}
		return defaultStdSize;				// if DefaultSize: use custom icon size
	}
}


namespace ui
{
	// CImageListInfo implementation

	TImageListFlags CImageListInfo::GetImageListFlags( TBitsPerPixel bitsPerPixel, bool hasAlpha )
	{
		TImageListFlags ilFlags = 0;

		if ( hasAlpha )
		{
			ASSERT( ILC_COLOR32 == bitsPerPixel );
			ilFlags = ILC_COLOR32;		// alpha-channel image list (no mask)
		}
		else
		{
			switch ( bitsPerPixel )
			{
				default:	ASSERT( false );
				case 1:		ilFlags = ILC_MASK; break;			// monochrome means mask only, no color bitmap
				case 4:		ilFlags = ILC_COLOR4; break;
				case 8:		ilFlags = ILC_COLOR24 /*ILC_COLOR8*/; break;		// issues with image-list transparency not working [PHC 2022-06-18]
				case 16:	ilFlags = ILC_COLOR16; break;
				case 24:	ilFlags = ILC_COLOR24; break;
				case 32:	ilFlags = ILC_COLOR32; break;
			}

			ilFlags |= ILC_MASK;		// masked image list (icon-like)
		}

		return ilFlags;
	}
}


namespace gdi
{
	ui::CImageListInfo CreateImageList( CImageList& rOutImageList, const CIconSize& imageSize, int countOrGrowBy, TImageListFlags ilFlags /*= ILC_COLOR32 | ILC_MASK*/ )
	{
		int imageCount = 0, growBy = 0;

		if ( countOrGrowBy > 0 )
			imageCount = countOrGrowBy;
		else
			growBy = -countOrGrowBy;

		VERIFY( rOutImageList.Create( imageSize.GetSize().cx, imageSize.GetSize().cy, ilFlags, imageCount, growBy ) );
		return ui::CImageListInfo( imageCount, imageSize.GetSize(), ilFlags );
	}
}


namespace res
{
	HICON LoadIcon( const CIconId& iconId, UINT fuLoad /*= LR_DEFAULTCOLOR*/ )
	{
		const CSize iconSize = iconId.GetSize();

		// if the icon doesn't contain the exact size, it loads a scaled icon to the desired size
		return (HICON)::LoadImage( CScopedResInst::Get(), MAKEINTRESOURCE( iconId.m_id ), IMAGE_ICON, iconSize.cx, iconSize.cy, fuLoad );
	}

	ui::CImageListInfo LoadImageListDIB( CImageList& rOutImageList, UINT bitmapId, COLORREF transpColor /*= color::Auto*/,
										 int imageCount /*= -1*/, bool disabledEffect /*= false*/ )
	{
		// Use PNG only with 32bpp (alpha channel). PNG 24bpp breaks image list transparency (DIB issues?).
		// Use BMP for 24bpp and lower.

		CDibSection dibSection;
		ui::CImageListInfo imageListInfo;

		if ( !dibSection.LoadImageResource( bitmapId ) )		// PNG or BMP
			return imageListInfo;

		if ( color::Auto == transpColor )
			dibSection.SetAutoTranspColor();
		else
			dibSection.SetTranspColor( transpColor );

		if ( disabledEffect )
		{
			CDibPixels pixels( &dibSection );
			pixels.ApplyDisableFadeGray( pixel::AlphaFadeMore, false /*, ::GetSysColor( COLOR_BTNFACE )*/ );
		}

		if ( -1 == imageCount )
			imageCount = dibSection.GetSize().cx / dibSection.GetSize().cy;		// assume nearly square images

		imageListInfo = dibSection.MakeImageList( rOutImageList, imageCount );

		return imageListInfo;
	}

	ui::CImageListInfo _LoadImageListIconStrip( CImageList* pOutImageList, CSize* pOutImageSize, UINT iconStripId )
	{
		// load a strip from a custom size icon with multiple images; image count is inferred by strip_width/strip_height ratio.
		ASSERT_PTR( pOutImageList );
		ASSERT_PTR( pOutImageSize );

		CGroupIconRes groupIcon( iconStripId );
		std::pair<TBitsPerPixel, IconStdSize> topIconPair = groupIcon.Front();		// highest color and size

		bool hasAlpha = topIconPair.first == ILC_COLOR32;
		TImageListFlags imageListFlags = ui::CImageListInfo::GetImageListFlags( topIconPair.first, hasAlpha );

		CIcon stripIcon( (HICON)::LoadImage( CScopedResInst::Get(), MAKEINTRESOURCE( iconStripId ), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_CREATEDIBSECTION ) );

		CSize imageSize( 0, 0 );
		int imageCount = 0;

		if ( stripIcon.IsValid() )
		{
			stripIcon.SetHasAlpha( hasAlpha );

			CIconInfo info( stripIcon.GetHandle() );

			imageSize = stripIcon.GetSize();
			imageCount = imageSize.cx / imageSize.cy;
			imageSize.cx /= imageCount;

			if ( nullptr == pOutImageList->GetSafeHandle() )
			{	// note: if icon has alpha channel, then no ILC_MASK required (in practice it makes little difference)
				pOutImageList->Create( imageSize.cx, imageSize.cy, imageListFlags, imageCount, 0 );
			}

			VERIFY( pOutImageList->Add( &info.m_bitmapColor, &info.m_bitmapMask ) != -1 );			// add the strip bitmaps, which will amount to imageCount images
		}

		utl::AssignPtr( pOutImageSize, imageSize );

		return ui::CImageListInfo( imageCount, imageSize, imageListFlags );
	}

	ui::CImageListInfo LoadImageListIcons( CImageList& rOutImageList, const UINT iconIds[], size_t iconCount, IconStdSize iconStdSize /*= SmallIcon*/,
										   TImageListFlags ilFlags /*= ILC_COLOR32 | ILC_MASK*/ )
	{
		ASSERT_PTR( iconIds );
		const CSize iconSize = CIconSize::GetSizeOf( iconStdSize );

		if ( nullptr == rOutImageList.GetSafeHandle() )
			VERIFY( rOutImageList.Create( iconSize.cx, iconSize.cy, ilFlags, 0, (int)iconCount ) );

		for ( unsigned int i = 0; i != iconCount; ++i )
			if ( iconIds[ i ] != 0 )			// skip separator ids
			{
				CIconId iconId( iconIds[ i ], iconStdSize );

				if ( const CIcon* pIcon = ui::GetImageStoresSvc()->RetrieveIcon( iconId ) )		// try exact icon size
					rOutImageList.Add( pIcon->GetHandle() );
				else if ( HICON hIcon = LoadIcon( iconId ) )									// try to load a scaled icon
				{
					rOutImageList.Add( hIcon );
					::DestroyIcon( hIcon );
				}
				else
					ASSERT( false );		// no image found
			}

		return ui::CImageListInfo( rOutImageList.GetImageCount(), iconSize, ilFlags );
	}

} //namespace res


namespace ui
{
	// CIconKey implementation

	bool CIconEntry::operator==( const CIconEntry& right ) const
	{
		if ( this == &right )
			return true;

		return
			m_bitsPerPixel == right.m_bitsPerPixel &&
			m_dimension == right.m_dimension;
	}
}


namespace ui
{
	// IImageStore implementation

	CIcon* IImageStore::FindIcon( UINT cmdId, IconStdSize iconStdSize /*= SmallIcon*/, TBitsPerPixel bitsPerPixel /*= ILC_COLOR*/ ) const
	{
		REQUIRE( EqMaskedValue( bitsPerPixel, ILC_ALL_COLORS_MASK, bitsPerPixel ) );		// only the valid BPP values are passed?
		CIcon* pFoundIcon = nullptr;

		if ( const CIconGroup* pIconGroup = FindIconGroup( cmdId ) )
			if ( ILC_COLOR == bitsPerPixel )
				pFoundIcon = pIconGroup->FindBestMatchingIcon( iconStdSize );						// best match, i.e. highest color
			else
				pFoundIcon = pIconGroup->FindIcon( ui::CIconEntry( bitsPerPixel, iconStdSize ) );	// exact match

		return pFoundIcon;
	}

	const CIcon* IImageStore::RetrieveLargestIcon( UINT cmdId, IconStdSize maxIconStdSize /*= HugeIcon_48*/ )
	{
		for ( ; maxIconStdSize >= DefaultSize; --(int&)maxIconStdSize )
			if ( const CIcon* pIcon = RetrieveIcon( CIconId( cmdId, maxIconStdSize ) ) )
				return pIcon;

		return nullptr;
	}

	int IImageStore::BuildImageList( CImageList* pDestImageList, const UINT buttonIds[], size_t buttonCount, const CSize& imageSize )
	{
		ASSERT_PTR( pDestImageList->GetSafeHandle() );
		ASSERT( buttonIds != nullptr && buttonCount != 0 );

		IconStdSize iconStdSize = CIconSize::FindStdSize( imageSize );
		int imageCount = 0;

		for ( size_t i = 0; i != buttonCount; ++i )
			if ( buttonIds[ i ] != 0 )			// skip separators
			{
				const CIcon* pIcon = RetrieveIcon( CIconId( buttonIds[ i ], iconStdSize ) );

				if ( nullptr == pIcon )
					pIcon = &CIcon::GetUnknownIcon();

				pDestImageList->Add( pIcon->GetHandle() );
				++imageCount;
			}

		if ( imageCount != pDestImageList->GetImageCount() )
			TRACE( " IImageStore::BuildImageList(): buttonCount=%d  imageListCount=%d\n", buttonCount, pDestImageList->GetImageCount() );

		return imageCount;
	}


	ui::IImageStore* GetImageStoresSvc( void )
	{
		return CImageStoresSvc::Instance();
	}
}


namespace gdi
{
	bool IsDibSection( HBITMAP hBitmap )
	{
		ASSERT_PTR( hBitmap );
		DIBSECTION dibSection;
		return sizeof( DIBSECTION ) == ::GetObject( hBitmap, sizeof( DIBSECTION ), &dibSection );
	}

	CSize GetBitmapSize( HBITMAP hBitmap )
	{
		ASSERT_PTR( hBitmap );
		BITMAP bmp;

		if ( ::GetObject( hBitmap, sizeof( BITMAP ), &bmp ) != 0 )
			return CSize( bmp.bmWidth, bmp.bmHeight );

		ASSERT( false );								// invalid bitmap
		return CSize( 0, 0 );
	}

	TBitsPerPixel GetBitsPerPixel( HBITMAP hBitmap, bool* pIsDibSection /*= nullptr*/ )
	{
		ASSERT_PTR( hBitmap );
		DIBSECTION dibSection;

		int size = ::GetObject( hBitmap, sizeof( DIBSECTION ), &dibSection );
		ASSERT( size != 0 );									// valid DIB or DDB bitmap

		if ( pIsDibSection != nullptr )
			*pIsDibSection = sizeof( DIBSECTION ) == size;		// bitmap is a DIB section?

		return dibSection.dsBm.bmBitsPixel;
	}

	bool Is32BitBitmap( HBITMAP hBitmap )
	{
		BITMAP bmp;

		if ( sizeof( BITMAP ) == ::GetObject( hBitmap, sizeof( BITMAP ), &bmp ) )			// valid bitmap?
			return 32 == bmp.bmBitsPixel;
		return false;
	}

	bool HasAlphaTransparency( HBITMAP hBitmap )
	{
		bool isDibSection;

		return GetBitsPerPixel( hBitmap, &isDibSection ) >= 32 && isDibSection;				// DIB section with alpha channel?
	}

	bool CreateBitmapMask( CBitmap& rMaskBitmap, HBITMAP hSrcBitmap, COLORREF transpColor )
	{
		rMaskBitmap.DeleteObject();

		CDC memDC, maskDC;
		if ( !memDC.CreateCompatibleDC( nullptr ) || !maskDC.CreateCompatibleDC( nullptr ) )
			return false;

		CSize bitmapSize = gdi::GetBitmapSize( hSrcBitmap );

		rMaskBitmap.CreateBitmap( bitmapSize.cx, bitmapSize.cy, 1, 1, nullptr );		// create monochrome (1 bit) mask bitmap

		CScopedGdiObj scopedBitmap( &memDC, hSrcBitmap );
		CScopedGdi<CBitmap> scopedMaskBitmap( &maskDC, &rMaskBitmap );

		memDC.SetBkColor( transpColor );		// set the background colour of the colour image to the colour you want to be transparent

		// copy the bits from the colour image to the B+W mask... everything with the background colour ends up white while everythig else ends up black
		maskDC.BitBlt( 0, 0, bitmapSize.cx, bitmapSize.cy, &memDC, 0, 0, SRCCOPY );

		// take the new mask and use it to turn the transparent colour in our original colour image to black so the transparency effect will work right
		memDC.BitBlt( 0, 0, bitmapSize.cx, bitmapSize.cy, &maskDC, 0, 0, SRCINVERT );
		return true;
	}


	HBITMAP CreateFadedGrayDIBitmap( const ui::IImageProxy* pImageProxy, TBitsPerPixel srcBPP, COLORREF transpColor /*= CLR_NONE*/ )
	{	// creates a bitmap with disabled gray look - inspired by CMFCToolBarImages::GrayImages()
		ASSERT( pImageProxy != nullptr && !pImageProxy->IsEmpty() );

		if ( GetGlobalData()->m_nBitsPerPixel <= 8 )
			return nullptr;

		const CSize& srcImageSize = pImageProxy->GetSize();
		CDC destMemDC;

		destMemDC.CreateCompatibleDC( nullptr );		// make DEST memory DC compatible with the Screen DC, to select a new hGrayDib

		BITMAPINFO grayBmpInfo;

		// fill in the BITMAPINFOHEADER:
		grayBmpInfo.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
		grayBmpInfo.bmiHeader.biWidth = srcImageSize.cx;
		grayBmpInfo.bmiHeader.biHeight = srcImageSize.cy;
		grayBmpInfo.bmiHeader.biPlanes = 1;
		grayBmpInfo.bmiHeader.biBitCount = 32;
		grayBmpInfo.bmiHeader.biCompression = BI_RGB;
		grayBmpInfo.bmiHeader.biSizeImage = srcImageSize.cx * srcImageSize.cy;
		grayBmpInfo.bmiHeader.biXPelsPerMeter = 0;
		grayBmpInfo.bmiHeader.biYPelsPerMeter = 0;
		grayBmpInfo.bmiHeader.biClrUsed = 0;
		grayBmpInfo.bmiHeader.biClrImportant = 0;

		COLORREF* pBits = nullptr;
		HBITMAP hGrayDib = ::CreateDIBSection( destMemDC.m_hDC, &grayBmpInfo, DIB_RGB_COLORS, (void**)&pBits, nullptr, 0 );
		CBitmap grayDibBitmap;

		if ( hGrayDib != nullptr )
			grayDibBitmap.Attach( hGrayDib );			// for temporary ownership for convenient error handling
		else
			return nullptr;

		HBITMAP hOldDestBitmap = (HBITMAP)destMemDC.SelectObject( hGrayDib );

		if ( nullptr == hOldDestBitmap )
			return nullptr;

		if ( srcBPP <= 8 && pImageProxy->HasTransparency() )
		{
			enum { TransparentBlack = RGB( 1, 1, 1 ) };

			ASSERT( CLR_NONE == transpColor );
			transpColor = TransparentBlack;

			// fill background with a non color::Black color to protect background from fading, which is undesirable - we just want to fade the foreground colors
			pImageProxy->FillBackground( &destMemDC, CPoint( 0, 0 ), transpColor );
		}

		// draw the original image:
		pImageProxy->Draw( &destMemDC, CPoint( 0, 0 ), transpColor );

		// direct BGRA (RGBQUAD) pixel modification:
		DIBSECTION ds;

		if ( 0 == ::GetObject( hGrayDib, sizeof( DIBSECTION ), &ds ) || ds.dsBm.bmBitsPixel != 32 || nullptr == ds.dsBm.bmBits )
		{
			ASSERT( FALSE );
			return nullptr;
		}

		// apply disabled effects to each pixel - fade and convert to gray-scale:
		size_t rgbQuadCount = ds.dsBm.bmWidth * ds.dsBm.bmHeight;
		func::DisableFadeGray disableFunc( pixel::AlphaFadeMore, false, transpColor );		// IMP: skip pre-multiply alpha so that it doesn't blend to gray - we just want to fade
		typedef CPixelBGRA* TPixelIterator;

		for ( TPixelIterator pPixel = (CPixelBGRA*)ds.dsBm.bmBits, pPixelEnd = pPixel + rgbQuadCount; pPixel != pPixelEnd; ++pPixel )
		{
			if ( srcBPP <= 8 )
				if ( transpColor != CLR_NONE && pPixel->GetColor() != transpColor )
					pPixel->m_alpha = 128;		// allow color fading: use fake alpha transparency for transparent (non-background) pixels

			disableFunc( *pPixel );
		}

		destMemDC.SelectObject( hOldDestBitmap );

		grayDibBitmap.Detach();			// success, release temporary ownership
		return hGrayDib;				// caller owns the gray bitmap
	}

	HBITMAP CreateFadedGrayDIBitmap( HBITMAP hBitmapSrc, TBitsPerPixel srcBPP /*= 0*/, COLORREF transpColor /*= CLR_NONE*/ )
	{
		CBitmapProxy bitmapProxy( hBitmapSrc );

		if ( bitmapProxy.IsEmpty() )
			return nullptr;

		if ( 0 == srcBPP )
			srcBPP = gdi::GetBitsPerPixel( hBitmapSrc );

		return CreateFadedGrayDIBitmap( &bitmapProxy, srcBPP, transpColor );
	}


	/// image list properties:

	CComPtr<IImageList> QueryImageListItf( HIMAGELIST hImageList )
	{
		ASSERT_PTR( hImageList );
		CComPtr<IImageList> pImageList;

		HR_OK( ::HIMAGELIST_QueryInterface( hImageList, IID_PPV_ARGS( &pImageList ) ) );
		return pImageList;
	}

	CComPtr<IImageList2> QueryImageList2Itf( HIMAGELIST hImageList )
	{
		ASSERT_PTR( hImageList );
		CComPtr<IImageList2> pImageList2;

		HR_OK( ::HIMAGELIST_QueryInterface( hImageList, IID_PPV_ARGS( &pImageList2 ) ) );
		return pImageList2;
	}

	DWORD GetImageIconFlags( HIMAGELIST hImageList, int imagePos /*= 0*/ )
	{
		if ( CComPtr<IImageList> pImageList = gdi::QueryImageListItf( hImageList ) )
		{
			DWORD dwFlags;
			if ( HR_OK( pImageList->GetItemFlags( imagePos, &dwFlags ) ) )
				return dwFlags;
		}
		return 0;
	}

	bool HasAlphaTransparency( HIMAGELIST hImageList, int imagePos /*= 0*/ )
	{
		DWORD dwFlags = gdi::GetImageIconFlags( hImageList, imagePos );

		return HasFlag( dwFlags, ILIF_ALPHA );
	}

	bool HasMask( HIMAGELIST hImageList, int imagePos /*= 0*/ )
	{
		ASSERT_PTR( hImageList );
		IMAGEINFO info;

		if ( ImageList_GetImageInfo( hImageList, imagePos, &info ) )
			return info.hbmMask != nullptr;

		return false;
	}

	CSize GetImageIconSize( HIMAGELIST hImageList )
	{
		ASSERT_PTR( hImageList );
		int cx, cy;

		VERIFY( ::ImageList_GetIconSize( hImageList, &cx, &cy ) );
		return CSize( cx, cy );
	}


	/// icon from bitmap(s):

	HICON CreateIcon( HBITMAP hImageBitmap, HBITMAP hMaskBitmap )
	{
		// imageBitmap and maskBitmap should have the same size, maskBitmap should be monochrome
		ICONINFO iconInfo;

		iconInfo.fIcon = TRUE;
		iconInfo.hbmMask = hMaskBitmap;
		iconInfo.hbmColor = hImageBitmap;

		return ::CreateIconIndirect( &iconInfo );
	}

	HICON CreateIcon( HBITMAP hImageBitmap, COLORREF transpColor )
	{
		CBitmap maskBitmap;

		if ( !gdi::CreateBitmapMask( maskBitmap, hImageBitmap, transpColor ) )
			return nullptr;

		return gdi::CreateIcon( hImageBitmap, maskBitmap );
	}

} //namespace gdi


namespace gdi
{
	namespace detail
	{
		bool Blit( CDC* pDC, const CRect& destRect, CDC* pSrcDC, const CRect& srcRect, DWORD rop )
		{
			return pDC->StretchBlt( destRect.left, destRect.top, destRect.Width(), destRect.Height(), pSrcDC, srcRect.left, srcRect.top, srcRect.Width(), srcRect.Height(), rop ) != FALSE;
		}

		bool AlphaBlend( CDC* pDC, const CRect& destRect, CDC* pSrcDC, const CRect& srcRect, BYTE srcAlpha = 255, BYTE blendOp = AC_SRC_OVER )
		{
			BLENDFUNCTION blendFunc;
			blendFunc.BlendOp = blendOp;
			blendFunc.BlendFlags = 0;
			blendFunc.SourceConstantAlpha = srcAlpha;
			blendFunc.AlphaFormat = AC_SRC_ALPHA;

			return pDC->AlphaBlend( destRect.left, destRect.top, destRect.Width(), destRect.Height(), pSrcDC, srcRect.left, srcRect.top, srcRect.Width(), srcRect.Height(), blendFunc ) != FALSE;
		}
	}


	bool DrawBitmap( CDC* pDC, HBITMAP hBitmap, const CPoint& pos, DWORD rop /*= SRCCOPY*/ )
	{
		CRect destRect( pos, gdi::GetBitmapSize( hBitmap ) );
		return gdi::DrawBitmap( pDC, hBitmap, destRect, rop );
	}

	bool DrawBitmap( CDC* pDC, HBITMAP hBitmap, const CRect& destRect, DWORD rop /*= SRCCOPY*/ )
	{
		ASSERT_PTR( hBitmap );

		CDC memDC;
		bool succeeded = false;

		if ( memDC.CreateCompatibleDC( pDC ) )
		{
			CScopedGdiObj scopedBitmap( &memDC, hBitmap );
			CRect srcRect( CPoint( 0, 0 ), gdi::GetBitmapSize( hBitmap ) );
			int oldStretchMode = pDC->SetStretchBltMode( COLORONCOLOR );

			succeeded = gdi::detail::Blit( pDC, destRect, &memDC, srcRect, rop );
			pDC->SetStretchBltMode( oldStretchMode );
		}
		return false;
	}
}


// CDibMeta implementation

bool CDibMeta::Reset( HBITMAP hDib /*= nullptr*/ )
{
	if ( m_hDib != nullptr && hDib != m_hDib )
		::DeleteObject( m_hDib );

	m_hDib = hDib;

	return StorePixelFormat();		// works with NULL m_hDib
}

bool CDibMeta::AssignIfValid( const CDibMeta& srcDib )
{
	if ( !srcDib.IsValid() )
		return false;

	Reset( srcDib.m_hDib );
	return true;
}

bool CDibMeta::StorePixelFormat( void )
{
	DIBSECTION dibSection;
	utl::ZeroStruct( &dibSection );

	int size = ::GetObject( m_hDib, sizeof( DIBSECTION ), &dibSection );
	bool isDibSection = size == sizeof( DIBSECTION );

	if ( 0 == size )			// not a valid bitmap
	{
		m_bitsPerPixel = 0;
		m_channelCount = 0;
		return false;
	}

	if ( isDibSection )
		m_bitsPerPixel = dibSection.dsBmih.biBitCount;
	else
		m_bitsPerPixel = dibSection.dsBm.bmBitsPixel;

	StoreChannelCount( isDibSection );
	return true;
}

void CDibMeta::StorePixelFormat( const BITMAPINFO& dibInfo )
{
	m_orientation = dibInfo.bmiHeader.biHeight < 0 ? gdi::TopDown : gdi::BottomUp;		// a top-down DIB if height is negative
	m_bitsPerPixel = dibInfo.bmiHeader.biBitCount;
	StoreChannelCount( true );
}

void CDibMeta::CopyPixelFormat( const CDibMeta& right )
{
	m_bitsPerPixel = right.m_bitsPerPixel;
	m_channelCount = right.m_channelCount;
}

void CDibMeta::StoreChannelCount( bool isDibSection )
{
	switch ( m_bitsPerPixel )
	{
		case ILC_COLOR32:
			m_channelCount = isDibSection ? 4 : 3;		// DIB32 sections have alpha channel
			break;
		case ILC_COLOR24:
		case ILC_COLOR16:
			m_channelCount = 3;
			break;
		case ILC_COLOR8:
		case ILC_COLOR4:
		case ILC_MASK:
			m_channelCount = 1;
			break;
		default:
			m_channelCount = 0;
			break;
	}
}


// CBitmapInfo implementation

#ifdef _DEBUG


std::tstring CBitmapInfo::FormatDbg( void ) const
{
	std::tostringstream oss;

	if ( !IsValid() )
		oss << _T("NULL bitmap");
	else
	{
		if ( IsDibSection() )
			oss << _T("DIB Section") << std::endl;
		else
			oss << _T("DDB") << std::endl;

		using namespace stream;

		oss
			<< _T("size: ") << GetBitmapSize() << std::endl
			<< _T("Bits-per-pixel: ") << GetBitsPerPixel() << std::endl;

		if ( HasAlphaChannel() )
			oss << _T("Alpha Channel") << std::endl;
	}
	return oss.str();
}


#endif // _DEBUG


// CDibSectionTraits implementation

CDibSectionTraits::CDibSectionTraits( HBITMAP hDib )
	: CBitmapInfo( hDib )
	, m_hDib( hDib )
{
	Build( hDib );
}

CDibSectionTraits::~CDibSectionTraits()
{
}

void CDibSectionTraits::Build( HBITMAP hDib )
{
	CBitmapInfo::Build( hDib );
	ASSERT( !CBitmapInfo::IsValid() || IsDibSection() );		// must be used only for DIB sections
}

COLORREF CDibSectionTraits::GetColorAt( const CDC* pDC, int index ) const
{
	ASSERT( HasColorTable() );
	ASSERT_PTR( pDC->GetSafeHdc() );
	ASSERT( m_hDib == pDC->GetCurrentBitmap()->GetSafeHandle() );				// the DIB must be selected into pDC

	RGBQUAD rgb;
	::GetDIBColorTable( pDC->m_hDC, index, 1, &rgb );
	return RGB( rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue );
}

const std::vector<RGBQUAD>& CDibSectionTraits::GetColorTable( const CDC* pDC )
{
	ASSERT_PTR( pDC->GetSafeHdc() );
	ASSERT( m_hDib == pDC->GetCurrentBitmap()->GetSafeHandle() );				// the DIB must be selected into pDC

	if ( HasColorTable() && m_colorTable.empty() )
	{
		m_colorTable.resize( GetUsedColorTableSize() );															// max count
		UINT actualColors = ::GetDIBColorTable( *pDC, 0, (UINT)m_colorTable.size(), &m_colorTable.front() );
		m_colorTable.resize( actualColors );
	}

	return m_colorTable;
}

CPalette* CDibSectionTraits::MakeColorPalette( const CDC* pDC )
{
	if ( nullptr == m_pPalette.get() && IsIndexed() )
	{
		GetColorTable( pDC );		// ensure the color table is initialized
		REQUIRE( m_colorTable.size() <= 256 );

		m_pPalette.reset( new CPalette() );

		std::vector<BYTE> buffer( sizeof( LOGPALETTE ) + ( sizeof( PALETTEENTRY ) * m_colorTable.size() ) );
		LOGPALETTE* pLogPalette = reinterpret_cast<LOGPALETTE*>(  &buffer.front() );

		pLogPalette->palVersion = 0x300;
		pLogPalette->palNumEntries = static_cast<WORD>( m_colorTable.size() );

		for( size_t i = 0; i != m_colorTable.size(); ++i )
			pLogPalette->palPalEntry[ i ] = gdi::ToPaletteEntry( m_colorTable[ i ] );

		if ( !m_pPalette->CreatePalette( pLogPalette ) )
			m_pPalette.reset();
	}

	return m_pPalette.get();
}

CPalette* CDibSectionTraits::MakeColorPaletteFallback( const CDC* pDC )
{
	if ( IsIndexed() )
		return MakeColorPalette( pDC );

	m_pPalette.reset( new CPalette() );
	m_pPalette->CreateHalftonePalette( const_cast<CDC*>( pDC ) );

	return m_pPalette.get();
}


// CBitmapInfoBuffer implementation

BITMAPINFO* CBitmapInfoBuffer::CreateDibInfo( int width, int height, UINT bitsPerPixel, const bmp::CSharedAccess* pSrcDib /*= nullptr*/ )
{
	std::vector<RGBQUAD> rgbTable;

	if ( bitsPerPixel <= 8 )			// got to build a color table
	{
		if ( pSrcDib != nullptr && pSrcDib->GetBitmapHandle() != nullptr )
		{
			CDibSectionTraits srcTraits( pSrcDib->GetBitmapHandle() );

			if ( srcTraits.GetBitsPerPixel() == bitsPerPixel )							// same color table size
			{
				CScopedBitmapMemDC scopedSrcBitmap( pSrcDib );
				rgbTable = srcTraits.GetColorTable( pSrcDib->GetBitmapMemDC() );		// use source color table
			}
		}

		if ( rgbTable.empty() )
			ui::halftone::MakeRgbTable( rgbTable, static_cast<size_t>( 1 ) << bitsPerPixel );
	}

	m_buffer.resize( sizeof( BITMAPINFOHEADER ) + rgbTable.size() * sizeof( RGBQUAD ) );

	BITMAPINFO* pBitmapInfo = reinterpret_cast<BITMAPINFO*>( &m_buffer.front() );
	ZeroMemory( &pBitmapInfo->bmiHeader, sizeof( BITMAPINFOHEADER ) );

	pBitmapInfo->bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
	pBitmapInfo->bmiHeader.biWidth = width;
	pBitmapInfo->bmiHeader.biHeight = height;			// negative height will create a top-down DIB
	pBitmapInfo->bmiHeader.biPlanes = 1;
	pBitmapInfo->bmiHeader.biBitCount = static_cast<WORD>( bitsPerPixel );
	pBitmapInfo->bmiHeader.biCompression = BI_RGB;
	pBitmapInfo->bmiHeader.biClrUsed = pBitmapInfo->bmiHeader.biClrImportant = (DWORD)rgbTable.size();

	// copy the color table
	utl::Copy( rgbTable.begin(), rgbTable.end(), (RGBQUAD*)pBitmapInfo->bmiColors );
	return pBitmapInfo;
}

BITMAPINFO* CBitmapInfoBuffer::CreateDibInfo( UINT bitsPerPixel, const bmp::CSharedAccess& sourceDib )
{
	CSize srcBitmapSize = gdi::GetBitmapSize( sourceDib.GetBitmapHandle() );

	return CreateDibInfo( srcBitmapSize.cx, srcBitmapSize.cy, bitsPerPixel, &sourceDib );
}
