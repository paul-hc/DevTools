
#include "pch.h"
#include "DibSection.h"
#include "DibPixels.h"
#include "ImagingGdiPlus.h"
#include "ImagingWic.h"
#include "Imaging.h"
#include "Path.h"
#include "WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "Image_fwd.hxx"


CBitmap* CDibSection::s_pNullMask = nullptr;
int CDibSection::s_testFlags = 0;

CDibSection::CDibSection( HBITMAP hDib /*= nullptr*/, bool ownsDib /*= false*/ )
	: CBitmap()
	, m_flags( 0 )
	, m_bitsPerPixel( 0 )
	, m_bitmapSize( 0, 0 )
	, m_transpColor( CLR_NONE )
{
	if ( hDib != nullptr )
	{
		CDibMeta dibMeta( hDib );
		dibMeta.StorePixelFormat();
		AttachDib( dibMeta );
		SetFlag( m_flags, F_NoAutoDelete, ownsDib );
	}
}

CDibSection::CDibSection( const CDibSection* pSrcDib )
	: CBitmap()
	, m_flags( 0 )
	, m_bitsPerPixel( 0 )
	, m_bitmapSize( 0, 0 )
	, m_transpColor( CLR_NONE )
{
	ASSERT_PTR( pSrcDib );
	Copy( pSrcDib );
}

CDibSection::~CDibSection()
{
	if ( HasFlag( m_flags, F_NoAutoDelete ) )
		Detach();
}

bool CDibSection::IsDibSection( void ) const
{
	return IsValid() && gdi::IsDibSection( GetBitmapHandle() );
}

void CDibSection::Clear( void )
{
	m_srcDibMeta = CDibMeta();
	m_flags = 0;
	m_bitsPerPixel = 0;
	m_bitmapSize.cx = m_bitmapSize.cy = 0;

	if ( IsValid() )
		DeleteObject();
}

bool CDibSection::AttachDib( const CDibMeta& dibMeta )
{
	DeleteObject();

	m_srcDibMeta = dibMeta;
	SetFlag( m_flags, F_HasAlpha, dibMeta.HasAlpha() );

	DIBSECTION dibSection;
	if ( dibMeta.m_hDib != nullptr )
		if ( sizeof( DIBSECTION ) == ::GetObject( dibMeta.m_hDib, sizeof( DIBSECTION ), &dibSection ) )
		{
			m_bitsPerPixel = dibSection.dsBm.bmBitsPixel;
			m_bitmapSize.cx = dibSection.dsBm.bmWidth;
			m_bitmapSize.cy = dibSection.dsBm.bmHeight;

			Attach( dibMeta.m_hDib );
			return true;
		}
		else
			ASSERT( false );			// bitmap should be loaded as a DIB

	Clear();
	return false;
}

bool CDibSection::Copy( HBITMAP hSrcBitmap )
{
	ASSERT_PTR( hSrcBitmap );
	CSize bitmapSize = gdi::GetBitmapSize( hSrcBitmap );

	// converts to a bottom-up bitmap even if source is top-down
	CDibMeta dibMeta( (HBITMAP)::CopyImage( hSrcBitmap, IMAGE_BITMAP, bitmapSize.cx, bitmapSize.cy, LR_CREATEDIBSECTION ) );

	dibMeta.StorePixelFormat();
	m_transpColor = CLR_NONE;
	return AttachDib( dibMeta );
}

bool CDibSection::Copy( const CDibSection* pSrcDib )
{
	ASSERT_PTR( pSrcDib );

	CSize bitmapSize = pSrcDib->GetSize();

	// converts to a bottom-up bitmap even if source is top-down
	CDibMeta dibMeta( (HBITMAP)::CopyImage( pSrcDib->GetBitmapHandle(), IMAGE_BITMAP, bitmapSize.cx, bitmapSize.cy, LR_CREATEDIBSECTION ) );

	dibMeta.CopyPixelFormat( pSrcDib->m_srcDibMeta );
	ModifyFlags( m_flags, _CopyMask, pSrcDib->m_flags );
	m_transpColor = pSrcDib->m_transpColor;
	return AttachDib( dibMeta );
}

// much better results than CopyPixels: color mapping, top-downess independence, etc
//
bool CDibSection::Convert( const CDibSection& srcDib, UINT destBitsPerPixel )
{
	ASSERT( srcDib.IsValid() );

	if ( srcDib.m_bitsPerPixel == destBitsPerPixel )
		if ( !HasFlag( s_testFlags, ForceCvtEqualBpp | ForceCvtCopyPixels ) )
			return Copy( &srcDib );						// straight CopyImage (no format conversion)

	CSize bitmapSize = srcDib.GetSize();
	CBitmapInfoBuffer bitmapInfo;
	bitmapInfo.CreateDibInfo( bitmapSize.cx, bitmapSize.cy, destBitsPerPixel, &srcDib );

	CDibPixels destPixels;
	if ( !CreateDIBSection( destPixels, *bitmapInfo.GetBitmapInfo() ) )
		return false;

	// retain original source information
	m_srcDibMeta.CopyPixelFormat( srcDib.m_srcDibMeta );
	SetMaskedValue( m_flags, F_IsPng | F_AutoTranspColor, srcDib.m_flags );
	m_transpColor = CLR_NONE;

	if ( !HasFlag( s_testFlags, ForceCvtCopyPixels ) )
	{
		CScopedBitmapMemDC scopedDestBitmap( this );
		CDC* pDestMemDC = GetBitmapMemDC();
		CRect rect( 0, 0, bitmapSize.cx, bitmapSize.cy );

		if ( srcDib.HasTransparency() )
		{
			COLORREF fillColor = GetAutoFillColor();
			ui::FillRect( *pDestMemDC, rect, fillColor );					// fill background
			if ( !HasAlpha() )
				m_transpColor = fillColor;									// store the mapped transparent color

			if ( Error == srcDib.DrawTransparent( pDestMemDC, rect ) )		// alpha-blend/draw transparently
				return false;

			GdiFlush();

			if ( HasAlpha() )
				destPixels.ForEach( func::ReplaceColor( fillColor, color::Black, 0 ) );		// fillColor -> transparent (0 alpha, i.e. opacity)
		}
		else
		{
			srcDib.Blit( pDestMemDC, rect );								// blit
			GdiFlush();

			if ( HasAlpha() )
				destPixels.SetOpaque();										// BitBlt takes care of BGR, but leaves alpha channel dangling
		}
	}
	else
	{
		CScopedBitmapMemDC scopedDestBitmap( this, nullptr, m_bitsPerPixel < 24 );		// we need a mem DC only for 1/4/8/16 bpp

		CDibPixels srcPixels( &srcDib );
		destPixels.CopyPixels( srcPixels );
	}

	return true;
}

COLORREF CDibSection::GetAutoFillColor( COLORREF fillColor /*= GetSysColor( COLOR_BTNFACE )*/ ) const
{
	if ( m_bitsPerPixel <= 16 )			// indexed or 16 bit: uses color mapping
	{
		CScopedBitmapMemDC scopedBitmap( this );
		fillColor = ui::GetMappedColor( fillColor, GetBitmapMemDC() );		// find the nearest color in color table
	}
	return fillColor;
}

// mostly experimental; certain combinations of src/dest bpp do not work, there is no color mapping, etc
//
bool CDibSection::CopyPixels( const CDibSection& srcDib, bool keepOrientation /*= false*/ )
{
	ASSERT( srcDib.IsValid() );

	CSize bitmapSize = srcDib.GetSize();
	// DIBs are usually bottom-up;
	// it's not necessary to preserve source orientation since pixel addressing takes care of translating it correctly
	if ( keepOrientation && gdi::TopDown == srcDib.GetSrcMeta().m_orientation )
		bitmapSize.cy = -bitmapSize.cy;				// negative height to indicate top-down bitmap

	CBitmapInfoBuffer bitmapInfo;
	bitmapInfo.CreateDibInfo( bitmapSize.cx, bitmapSize.cy, srcDib.m_bitsPerPixel, &srcDib );

	CDibPixels pixels;
	if ( !CreateDIBSection( pixels, *bitmapInfo.GetBitmapInfo() ) )
		return false;

	CScopedBitmapMemDC scopedDestBitmap( this, nullptr, m_bitsPerPixel < 24 );		// we need a mem DC only for 1/4/8/16 bpp

	CDibPixels srcPixels( &srcDib );
	pixels.CopyPixels( srcPixels );
	return true;
}

void CDibSection::SetTranspColor( COLORREF transpColor )
{
	if ( 32 == m_bitsPerPixel )
		return;

	m_transpColor = transpColor;
	ClearFlag( m_flags, F_AutoTranspColor );
}

COLORREF CDibSection::FindAutoTranspColor( void ) const
{
	if ( HasAlpha() )
		return CLR_NONE;

	CScopedBitmapMemDC scopedBitmap( this );
	CDC* pMemDC = GetBitmapMemDC();

	const COLORREF cornerColors[] =
	{
		pMemDC->GetPixel( 0, 0 ),
		pMemDC->GetPixel( 0, m_bitmapSize.cy - 1 ),
		pMemDC->GetPixel( m_bitmapSize.cx - 1, 0 ),
		pMemDC->GetPixel( m_bitmapSize.cx - 1, m_bitmapSize.cy - 1 )
	};
	const COLORREF* itColorEnd = END_OF( cornerColors );

	ptrdiff_t counts[] =
	{
		std::count( cornerColors, itColorEnd, cornerColors[ 0 ] ),
		std::count( cornerColors, itColorEnd, cornerColors[ 1 ] ),
		std::count( cornerColors, itColorEnd, cornerColors[ 2 ] ),
		std::count( cornerColors, itColorEnd, cornerColors[ 3 ] )
	};

	size_t maxOccurPos = std::distance( counts, std::max_element( counts, END_OF( counts ) ) );
	if ( 1 == counts[ maxOccurPos ] )
		maxOccurPos = 0;					// if ambiguous, pick the color at origin
	COLORREF autoTranspColor = cornerColors[ maxOccurPos ];			// transparent color is likely the one with most occurences in corners
	TRACE( _T(" - CDibSection::FindAutoTranspColor(): 0x%06X\n"), autoTranspColor );
	return autoTranspColor;
}

bool CDibSection::SetAutoTranspColor( void )
{
	m_transpColor = FindAutoTranspColor();
	SetFlag( m_flags, F_AutoTranspColor, m_transpColor != CLR_NONE );
	return m_transpColor != CLR_NONE;
}

bool CDibSection::LoadFromFile( const TCHAR* pFilePath, ui::ImagingApi api /*= ui::WicApi*/, UINT framePos /*= 0*/ )
{
	if ( !AttachDib( ui::LoadImageFromFile( pFilePath, api, framePos ) ) )
		return false;

	SetFlag( m_flags, F_IsPng, path::MatchExt( pFilePath, _T(".png") ) );
	return true;
}

bool CDibSection::LoadPngResource( UINT pngId, ui::ImagingApi api /*= ui::WicApi*/ )
{
	if ( !AttachDib( ui::LoadPngResource( pngId, api ) ) )
		return false;

	SetFlag( m_flags, F_IsPng );
	return true;
}

bool CDibSection::LoadBitmapResource( UINT bitmapId )
{
	if ( !AttachDib( gdi::LoadBitmapAsDib( MAKEINTRESOURCE( bitmapId ) ) ) )
		return false;

	ClearFlag( m_flags, F_IsPng );
	return true;
}

bool CDibSection::LoadImageResource( UINT imageId, ui::ImagingApi api /*= ui::WicApi*/ )
{
	if ( LoadPngResource( imageId, api ) )		// first try to load PNG image from resources
		return true;

	return LoadBitmapResource( imageId );
}

ui::CImageListInfo CDibSection::MakeImageList( CImageList& rDestImageList, int imageCount, bool preserveThis /*= false*/ )
{
	REQUIRE( IsValid() );
	REQUIRE( 0 == ( m_bitmapSize.cx % imageCount ) );	// imageCount lines-up with bitmap width consistently?

	// imply image size from bitmap width and image count
	ui::CImageListInfo imageListInfo( imageCount, m_bitmapSize, GetImageListFlags() );

	imageListInfo.m_imageSize.cx /= imageCount;
	ENSURE( m_bitmapSize.cx == imageListInfo.m_imageSize.cx * imageCount );		// whole division (no remainder)?

	CreateEmptyImageList( rDestImageList, imageListInfo.m_imageSize, imageCount );

	if ( HasAlpha() )
		rDestImageList.Add( this, s_pNullMask );		// use alpha channel (no ILC_MASK required)
	else
	{
		if ( HasTranspColor() )
		{
			if ( preserveThis )
			{
				// NOTE: this DIB will get altered, i.e. transparent background gets converted to black (due to image list internals).
				//	We work on a copy so that we preserve this.
				CDibSection dupDibBitmap( this );
				rDestImageList.Add( &dupDibBitmap, m_transpColor );
			}
			else
				rDestImageList.Add( this, m_transpColor );		// we don't care if this temporary DIB bitmap gets modified
		}
		else if ( imageListInfo.IsMonochrome() )
			rDestImageList.Add( this, this );		// format compatibility: must be saved in MS Paint as monochrome bitmap!
		else
			rDestImageList.Add( this, s_pNullMask );
	}

	return imageListInfo;
}

TImageListFlags CDibSection::CreateEmptyImageList( CImageList& rDestImageList, const CSize& imageSize, int imageCount ) const
{
	REQUIRE( IsValid() );

	TImageListFlags ilFlags = GetImageListFlags();

	rDestImageList.Create( imageSize.cx, imageSize.cy, ilFlags, imageCount, 0 );
	return ilFlags;
}

bool CDibSection::CreateDIBSection( CDibPixels& rPixels, const BITMAPINFO& dibInfo )
{
	DeleteObject();

	// if pSrcDC not null and has the source bitmap selected, it will copy the source colour table
	void* pPixels;
	if ( HBITMAP hDib = ::CreateDIBSection( nullptr, &dibInfo, DIB_RGB_COLORS, &pPixels, nullptr, 0 ) )
	{
		CDibMeta dibMeta( hDib );
		dibMeta.StorePixelFormat( dibInfo );
		AttachDib( dibMeta );

		rPixels.Init( *this );
		ENSURE( m_srcDibMeta.m_orientation == rPixels.GetOrientation() );
		ENSURE( pPixels == rPixels.Begin<BYTE>() );
		return true;
	}

	return GetSafeHandle() != nullptr;
}

bool CDibSection::CreateDIBSection( CDibPixels& rPixels, int width, int height, UINT bitsPerPixel )
{
	ASSERT( bitsPerPixel > 8 );			// this shan't be used for DIBs with color table, which require full BITMAPINFO with colour table

	BITMAPINFO dibInfo; ZeroMemory( &dibInfo, sizeof( dibInfo ) );

	dibInfo.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
	dibInfo.bmiHeader.biWidth = width;
	dibInfo.bmiHeader.biHeight = height;			// negative height will create a top-down DIB
	dibInfo.bmiHeader.biPlanes = 1;
	dibInfo.bmiHeader.biBitCount = static_cast<WORD>( bitsPerPixel );
	dibInfo.bmiHeader.biCompression = BI_RGB;

	return CreateDIBSection( rPixels, dibInfo );
}

bool CDibSection::Blit( CDC* pDC, const CRect& rect, DWORD rop /*= SRCCOPY*/ ) const
{
	ASSERT( CanDraw() );
	CScopedBitmapMemDC scopedBitmap( this, pDC );	// creates a compatible memory DC
	return DoBlit( pDC, rect, rop );
}

CDibSection::DrawResult CDibSection::DrawTransparent( CDC* pDC, const CRect& rect, BYTE srcAlpha /*= 255*/, BYTE blendOp /*= AC_SRC_OVER*/ ) const
{
	ASSERT( CanDraw() );

	CScopedBitmapMemDC scopedBitmap( this, pDC );

	if ( !HasAlpha() && HasTranspColor() )
		if ( pDC->TransparentBlt( rect.left, rect.top, rect.Width(), rect.Height(), GetBitmapMemDC(), 0, 0, m_bitmapSize.cx, m_bitmapSize.cy, m_transpColor ) )
			return TranspDrawn;

	if ( DoAlphaBlend( pDC, rect, srcAlpha, blendOp ) )
		return AlphaBlended;

	return Error;
}

CDibSection::DrawResult CDibSection::Draw( CDC* pDC, const CRect& rect ) const
{
	ASSERT( CanDraw() );

	DrawResult result = DrawTransparent( pDC, rect, 255, AC_SRC_OVER );
	if ( Error == result )
		if ( Blit( pDC, rect ) )
			result = Blitted;

	return result;
}

bool CDibSection::DoAlphaBlend( CDC* pDC, const CRect& rect, BYTE srcAlpha /*= 255*/, BYTE blendOp /*= AC_SRC_OVER*/ ) const
{
	ASSERT( CanDraw() );
	ASSERT( GetBitmapMemDC() );

	BLENDFUNCTION blendFunc;
	blendFunc.BlendOp = blendOp;
	blendFunc.BlendFlags = 0;
	blendFunc.SourceConstantAlpha = srcAlpha;
	blendFunc.AlphaFormat = HasAlpha() ? AC_SRC_ALPHA : 0;

	if ( !pDC->AlphaBlend( rect.left, rect.top, rect.Width(), rect.Height(), GetBitmapMemDC(), 0, 0, m_bitmapSize.cx, m_bitmapSize.cy, blendFunc ) )
	{
		TRACE( _T(" * CDibSection::DoAlphaBlend() failed!\n") );
		return false;
	}
	return true;
}

bool CDibSection::DoBlit( CDC* pDC, const CRect& rect, DWORD rop /*= SRCCOPY*/ ) const
{
	ASSERT( CanDraw() );
	ASSERT( GetBitmapMemDC() );

	if ( m_bitmapSize == rect.Size() )
		return pDC->BitBlt( rect.left, rect.top, m_bitmapSize.cx, m_bitmapSize.cy, GetBitmapMemDC(), 0, 0, rop ) != FALSE;

	int oldStretchBltMode = pDC->SetStretchBltMode( COLORONCOLOR );			// prevent rough shrinking edges
	bool result = pDC->StretchBlt( rect.left, rect.top, rect.Width(), rect.Height(), GetBitmapMemDC(), 0, 0, m_bitmapSize.cx, m_bitmapSize.cy, rop ) != FALSE;
	pDC->SetStretchBltMode( oldStretchBltMode );
	return result;
}
