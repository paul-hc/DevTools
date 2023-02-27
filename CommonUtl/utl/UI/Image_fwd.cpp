
#include "stdafx.h"
#include "Image_fwd.h"
#include "ImageStore.h"
#include "DibSection.h"
#include "DibPixels.h"
#include "Pixel.h"
#include "ScopedGdi.h"
#include "ToolImageList.h"
#include "utl/Algorithms.h"
#include "utl/EnumTags.h"
#include "utl/Path.h"
#include "utl/StreamStdTypes.h"
#include <commoncontrols.h>						// IImageList

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


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


namespace gdi
{
	void CreateImageList( CImageList& rOutImageList, const CIconSize& imageSize, int countOrGrowBy, UINT flags /*= ILC_COLOR32 | ILC_MASK*/ )
	{
		int imageCount = 0, growBy = 0;

		if ( countOrGrowBy > 0 )
			imageCount = countOrGrowBy;
		else
			growBy = -countOrGrowBy;

		VERIFY( rOutImageList.Create( imageSize.GetSize().cx, imageSize.GetSize().cy, flags, imageCount, growBy ) );
	}
}


namespace res
{
	HICON LoadIcon( const CIconId& iconId )
	{
		const CSize iconSize = iconId.GetStdSize();
		return (HICON)::LoadImage( CScopedResInst::Get(), MAKEINTRESOURCE( iconId.m_id ), IMAGE_ICON, iconSize.cx, iconSize.cy, LR_DEFAULTCOLOR );
	}

	int LoadImageListDIB( CImageList& rOutImageList, UINT bitmapId, COLORREF transpColor /*= color::Auto*/,
						  int imageCount /*= -1*/, bool disabledEffect /*= false*/ )
	{
		// Use PNG only with 32bpp (alpha channel). PNG 24bpp breaks image list transparency (DIB issues?).
		// Use BMP for 24bpp and lower.

		CDibSection dibSection;
		if ( !dibSection.LoadImage( bitmapId ) )		// PNG or BMP
			return 0;

		if ( color::Auto == transpColor )
			dibSection.SetAutoTranspColor();
		else
			dibSection.SetTranspColor( transpColor );

		if ( disabledEffect )
		{
			CDibPixels pixels( &dibSection );
			pixels.ApplyDisabledGrayOut( ::GetSysColor( COLOR_BTNFACE ), 64 );
		}

		if ( -1 == imageCount )
			imageCount = dibSection.GetSize().cx / dibSection.GetSize().cy;		// assume nearly square images

		CSize buttonSize = dibSection.MakeImageList( rOutImageList, imageCount );
		return imageCount;
	}

	int LoadImageListIconStrip( CImageList* pOutImageList, CSize* pOutImageSize, UINT iconStripId, UINT ilFlags /*= ILC_COLOR32 | ILC_MASK*/ )
	{
		// load a strip from a custom size icon with multiple images; image count is inferred by strip_width/strip_height ratio.
		ASSERT_PTR( pOutImageList );
		ASSERT_PTR( pOutImageSize );

		CIcon stripIcon( (HICON)::LoadImage( CScopedResInst::Get(), MAKEINTRESOURCE( iconStripId ), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_CREATEDIBSECTION ) );
		CSize imageSize( 0, 0 );
		int imageCount = 0;

		if ( stripIcon.IsValid() )
		{
			CIconInfo info( stripIcon.GetHandle() );

			imageSize = stripIcon.GetSize();
			imageCount = imageSize.cx / imageSize.cy;
			imageSize.cx /= imageCount;

			if ( nullptr == pOutImageList->GetSafeHandle() )
				pOutImageList->Create( imageSize.cx, imageSize.cy, ilFlags, imageCount, 0 );		// note: if icon has alpha channel, then no ILC_MASK required (in practice it makes little difference)

			VERIFY( pOutImageList->Add( &info.m_bitmapColor, &info.m_bitmapMask ) != -1 );			// add the strip bitmaps, which will amount to imageCount images
		}

		utl::AssignPtr( pOutImageSize, imageSize );

		return imageCount;
	}

	void LoadImageListIcons( CImageList& rOutImageList, const UINT iconIds[], size_t iconCount, IconStdSize iconStdSize /*= SmallIcon*/,
							 UINT ilFlags /*= ILC_COLOR32 | ILC_MASK*/ )
	{
		ASSERT_PTR( iconIds );

		if ( nullptr == rOutImageList.GetSafeHandle() )
		{
			const CSize iconSize = CIconSize::GetSizeOf( iconStdSize );
			VERIFY( rOutImageList.Create( iconSize.cx, iconSize.cy, ilFlags, 0, (int)iconCount ) );
		}

		for ( unsigned int i = 0; i != iconCount; ++i )
			if ( iconIds[ i ] != 0 )			// skip separator ids
			{
				CIconId iconId( iconIds[ i ], iconStdSize );

				if ( const CIcon* pIcon = ui::GetImageStoresSvc()->RetrieveIcon( iconId ) )		// try exact icon size
					rOutImageList.Add( pIcon->GetHandle() );
				else if ( HICON hIcon = LoadIcon( iconId ) )								// try to load a scaled icon
				{
					rOutImageList.Add( hIcon );
					::DestroyIcon( hIcon );
				}
				else
					ASSERT( false );		// no image found
			}
	}

} //namespace res


namespace ui
{
	const CIcon* IImageStore::RetrieveLargestIcon( UINT cmdId, IconStdSize maxIconStdSize /*= HugeIcon_48*/ )
	{
		for ( ; maxIconStdSize >= DefaultSize; --(int&)maxIconStdSize )
			if ( const CIcon* pIcon = RetrieveIcon( CIconId( cmdId, maxIconStdSize ) ) )
				return pIcon;

		return nullptr;
	}

	int IImageStore::BuildImageList( CImageList* pDestImageList, const UINT buttonIds[], size_t buttonCount, const CSize& imageSize )
	{
		ASSERT_PTR( pDestImageList );
		ASSERT( buttonIds != nullptr && buttonCount != 0 );

		if ( nullptr == pDestImageList->GetSafeHandle() )
			gdi::CreateImageList( *pDestImageList, imageSize, CToolImageList::EvalButtonCount( buttonIds, buttonCount ) );

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

	CSize GetImageIconSize( const CImageList& imageList )
	{
		ASSERT_PTR( imageList.GetSafeHandle() );
		int cx, cy;
		VERIFY( ::ImageList_GetIconSize( imageList, &cx, &cy ) );
		return CSize( cx, cy );
	}

	WORD GetBitsPerPixel( HBITMAP hBitmap, bool* pIsDibSection /*= nullptr*/ )
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
		if ( sizeof( BITMAP ) == ::GetObject( hBitmap, sizeof( BITMAP ), &bmp ) )					// valid bitmap?
			return 32 == bmp.bmBitsPixel;
		return false;
	}

	bool HasAlphaTransparency( HBITMAP hBitmap )
	{
		bool isDibSection;
		return GetBitsPerPixel( hBitmap, &isDibSection ) >= 32 && isDibSection;				// DIB section with alpha channel?
	}

	bool HasAlphaTransparency( const CImageList& imageList, int imagePos /*= 0*/ )
	{
		ASSERT_PTR( imageList.GetSafeHandle() );
		CComPtr<IImageList> ipImageList;
		if ( HR_OK( HIMAGELIST_QueryInterface( imageList.GetSafeHandle(), IID_PPV_ARGS( &ipImageList ) ) ) )
		{
			DWORD dwFlags;
			if ( HR_OK( ipImageList->GetItemFlags( imagePos, &dwFlags ) ) )
				return HasFlag( dwFlags, ILIF_ALPHA );
		}
		return false;
	}

	bool HasMask( const CImageList& imageList, int imagePos /*= 0*/ )
	{
		IMAGEINFO info;
		if ( imageList.GetImageInfo( imagePos, &info ) )
			return info.hbmMask != nullptr;
		return false;
	}

	bool CreateBitmapMask( CBitmap& rMaskBitmap, HBITMAP hSrcBitmap, COLORREF transpColor )
	{
		rMaskBitmap.DeleteObject();

		CDC memDC, maskDC;
		if ( !memDC.CreateCompatibleDC( nullptr ) || !maskDC.CreateCompatibleDC( nullptr ) )
			return false;

		CSize bitmapSize = GetBitmapSize( hSrcBitmap );

		rMaskBitmap.CreateBitmap( bitmapSize.cx, bitmapSize.cy, 1, 1, nullptr );		// create monochrome (1 bit) mask bitmap

		CScopedGdiObj scopedBitmap( &memDC, hSrcBitmap );
		CScopedGdi< CBitmap > scopedMaskBitmap( &maskDC, &rMaskBitmap );

		memDC.SetBkColor( transpColor );		// set the background colour of the colour image to the colour you want to be transparent

		// copy the bits from the colour image to the B+W mask... everything with the background colour ends up white while everythig else ends up black
		maskDC.BitBlt( 0, 0, bitmapSize.cx, bitmapSize.cy, &memDC, 0, 0, SRCCOPY );

		// take the new mask and use it to turn the transparent colour in our original colour image to black so the transparency effect will work right
		memDC.BitBlt( 0, 0, bitmapSize.cx, bitmapSize.cy, &maskDC, 0, 0, SRCINVERT );
		return true;
	}


	HICON CreateIcon( HBITMAP hImageBitmap, HBITMAP hMaskBitmap )
	{
		// imageBitmap and maskBitmap should have the same size, maskBitmap should be monochrome
		ICONINFO iconInfo;
		iconInfo.fIcon = TRUE;
		iconInfo.hbmMask = hMaskBitmap;
		iconInfo.hbmColor = hImageBitmap;

		return CreateIconIndirect( &iconInfo );
	}

	HICON CreateIcon( HBITMAP hImageBitmap, COLORREF transpColor )
	{
		CBitmap maskBitmap;
		if ( !CreateBitmapMask( maskBitmap, hImageBitmap, transpColor ) )
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
		return DrawBitmap( pDC, hBitmap, destRect, rop );
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

			succeeded = detail::Blit( pDC, destRect, &memDC, srcRect, rop );
			pDC->SetStretchBltMode( oldStretchMode );
		}
		return false;
	}
}


// CDibMeta implementation

bool CDibMeta::StorePixelFormat( void )
{
	DIBSECTION dibSection;
	int size = ::GetObject( m_hDib, sizeof( DIBSECTION ), &dibSection );
	if ( 0 == size )			// not a valid bitmap
	{
		m_bitsPerPixel = 0;
		m_channelCount = 0;
		return false;
	}

	m_bitsPerPixel = dibSection.dsBmih.biBitCount;
	StoreChannelCount( size == sizeof( DIBSECTION ) );
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


// CDibSectionInfo implementation

CDibSectionInfo::CDibSectionInfo( HBITMAP hDib )
	: CBitmapInfo( hDib )
	, m_hDib( hDib )
{
	Build( hDib );
}

CDibSectionInfo::~CDibSectionInfo()
{
}

void CDibSectionInfo::Build( HBITMAP hDib )
{
	CBitmapInfo::Build( hDib );
	ASSERT( !CBitmapInfo::IsValid() || IsDibSection() );		// must be used only for DIB sections
}

COLORREF CDibSectionInfo::GetColorAt( const CDC* pDC, int index ) const
{
	ASSERT( HasColorTable() );
	ASSERT_PTR( pDC->GetSafeHdc() );
	ASSERT( m_hDib == pDC->GetCurrentBitmap()->GetSafeHandle() );				// the DIB must be selected into pDC

	RGBQUAD rgb;
	::GetDIBColorTable( pDC->m_hDC, index, 1, &rgb );
	return RGB( rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue );
}

const std::vector< RGBQUAD >& CDibSectionInfo::GetColorTable( const CDC* pDC )
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

CPalette* CDibSectionInfo::MakeColorPalette( const CDC* pDC )
{
	ASSERT( !GetColorTable( pDC ).empty() );

	if ( nullptr == m_pPalette.get() )
	{
		m_pPalette.reset( new CPalette() );
		if ( m_colorTable.size() > 256 )
			m_pPalette->CreateHalftonePalette( const_cast<CDC*>( pDC ) );
		else
		{
			std::vector< BYTE > buffer( sizeof( LOGPALETTE ) + ( sizeof( PALETTEENTRY ) * m_colorTable.size() ) );
			LOGPALETTE* pLogPalette = reinterpret_cast<LOGPALETTE*>(  &buffer.front() );

			pLogPalette->palVersion = 0x300;
			pLogPalette->palNumEntries = static_cast<WORD>( m_colorTable.size() );

			for( size_t i = 0; i != m_colorTable.size(); ++i )
				pLogPalette->palPalEntry[ i ] = gdi::ToPaletteEntry( m_colorTable[ i ] );

			if ( !m_pPalette->CreatePalette( pLogPalette ) )
				m_pPalette.reset();
		}
	}

	return m_pPalette.get();
}


// CBitmapInfoBuffer implementation

BITMAPINFO* CBitmapInfoBuffer::CreateDibInfo( int width, int height, UINT bitsPerPixel, const bmp::CSharedAccess* pSrcDib /*= nullptr*/ )
{
	std::vector< RGBQUAD > rgbTable;
	if ( bitsPerPixel <= 8 )			// got to build a color table
	{
		if ( pSrcDib != nullptr && pSrcDib->GetHandle() != nullptr )
		{
			CDibSectionInfo srcInfo( pSrcDib->GetHandle() );
			if ( srcInfo.GetBitsPerPixel() == bitsPerPixel )							// same color table size
			{
				CScopedBitmapMemDC scopedSrcBitmap( pSrcDib );
				rgbTable = srcInfo.GetColorTable( pSrcDib->GetBitmapMemDC() );		// use source color table
			}
		}

		if ( rgbTable.empty() )
			CSysColorTable::MakeRgbTable( rgbTable, static_cast<size_t>( 1 ) << bitsPerPixel );
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
	CSize srcBitmapSize = gdi::GetBitmapSize( sourceDib.GetHandle() );
	return CreateDibInfo( srcBitmapSize.cx, srcBitmapSize.cy, bitsPerPixel, &sourceDib );
}


// CSysColorTable implementation

const std::vector< RGBQUAD >& CSysColorTable::GetSysRgbTable( void )
{
	static std::vector< RGBQUAD > rgbTable;
	if ( rgbTable.empty() )
	{
		std::auto_ptr<CPalette> pPalette( new CPalette() );
		{
			CWindowDC screenDC( nullptr );
			pPalette->CreateHalftonePalette( &screenDC );
		}

		std::vector< PALETTEENTRY > entries;
		entries.resize( pPalette->GetEntryCount() );
		pPalette->GetPaletteEntries( 0, (UINT)entries.size(), &entries.front() );

		rgbTable.reserve( entries.size() );
		for ( std::vector< PALETTEENTRY >::const_iterator itEntry = entries.begin(); itEntry != entries.end(); ++itEntry )
			rgbTable.push_back( gdi::ToRgbQuad( *itEntry ) );
	}

	return rgbTable;
}

void CSysColorTable::MakeRgbTable( RGBQUAD* pRgbTable, size_t size )
{
	ASSERT( size != 0 && 0 == ( size % 2 ) );		// size must be multiple of 2
	ASSERT( size <= 256 );

	const std::vector< RGBQUAD >& sysTable = GetSysRgbTable();
	const size_t halfSize = size >> 1;

	// copy system colors in 2 chunks: first half, last half
	utl::Copy( sysTable.begin(), sysTable.begin() + halfSize, pRgbTable );				// first half from the beginning
	utl::Copy( sysTable.end() - halfSize, sysTable.end(), pRgbTable + halfSize );		// second half from the end
}

void CSysColorTable::MakeColorTable( std::vector< COLORREF >& rColorTable, size_t size )
{
	MakeRgbTable( (std::vector< RGBQUAD >&)rColorTable, size );							// same storeage

	for ( std::vector< COLORREF >::iterator itColor = rColorTable.begin(); itColor != rColorTable.end(); ++itColor )
		*itColor = CPixelBGR( (const RGBQUAD&)*itColor ).GetColor();
}
