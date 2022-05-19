
#include "stdafx.h"
#include "Icon.h"
#include "Imaging.h"
#include "GroupIcon.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CIcon::CIcon( void )
	: m_hIcon( NULL )
	, m_hasAlpha( false )
	, m_pShared( NULL )
	, m_size( 0, 0 )
{
}

CIcon::CIcon( HICON hIcon, const CSize& iconSize /*= CSize( 0, 0 )*/ )
	: m_hIcon( hIcon )
	, m_hasAlpha( false )
	, m_pShared( NULL )
	, m_size( iconSize )
{
}

CIcon::CIcon( const CIcon& shared )
	: m_hIcon( shared.m_hIcon )
	, m_hasAlpha( shared.m_hasAlpha )
	, m_pShared( &shared )
	, m_size( shared.m_size )
{
}

CIcon::~CIcon()
{
	Release();
}

// scales to nearest size if the icon doesn't contain the exact size

CIcon* CIcon::NewIcon( HICON hIcon )
{
	ASSERT_PTR( hIcon );

	CIconInfo iconInfo( hIcon );
	CIcon* pIcon = new CIcon( hIcon, iconInfo.m_size );
	pIcon->SetHasAlpha( iconInfo.HasAlpha() );

	return pIcon;
}

CIcon* CIcon::NewIcon( const CIconId& iconId )
{
	bool hasAlpha = CGroupIcon( iconId.m_id ).Contains( ILC_COLOR32, iconId.m_stdSize );

	if ( HICON hIcon = res::LoadIcon( iconId ) )
		return SetHasAlpha( new CIcon( hIcon, iconId.GetStdSize() ), hasAlpha );
	return NULL;
}

// loads only if the icon contains the exact size

CIcon* CIcon::NewExactIcon( const CIconId& iconId )
{
	TBitsPerPixel bitsPerPixel;
	if ( CGroupIcon( iconId.m_id ).ContainsSize( iconId.m_stdSize, &bitsPerPixel ) )
		if ( HICON hIcon = res::LoadIcon( iconId ) )
			return SetHasAlpha( new CIcon( hIcon, iconId.GetStdSize() ), 32 == bitsPerPixel );

	return NULL;
}

HICON CIcon::Detach( void )
{
	HICON hIcon = m_hIcon;

	m_hIcon = NULL;
	m_hasAlpha = false;
	m_pShared = NULL;
	m_size = CSize( 0, 0 );
	return hIcon;
}

void CIcon::Release( void )
{
	bool shared = IsShared();

	if ( HICON hIcon = Detach() )
		if ( !shared )
			::DestroyIcon( hIcon );
}

CIcon& CIcon::SetHandle( HICON hIcon, bool hasAlpha /*= true*/ )
{
	Release();
	m_hIcon = hIcon;
	SetHasAlpha( hasAlpha );
	return *this;
}

CIcon* CIcon::SetHasAlpha( CIcon* pIcon, bool hasAlpha )
{
	if ( pIcon != NULL )					// safe if NULL pIcon
		pIcon->SetHasAlpha( hasAlpha );

	return pIcon;
}

void CIcon::SetShared( const CIcon& shared )
{
	Release();
	m_hIcon = shared.m_hIcon;
	m_hasAlpha = shared.m_hasAlpha;
}

const CSize& CIcon::GetSize( void ) const
{	// lazy size computation
	ASSERT_PTR( m_hIcon );
	if ( IsShared() )
		return m_pShared->GetSize();

	if ( 0 == m_size.cx || 0 == m_size.cy )
		m_size = ComputeSize( m_hIcon );

	ASSERT( m_size.cx > 0 && m_size.cy > 0 );
	return m_size;
}

CSize CIcon::ComputeSize( HICON hIcon )
{
	ASSERT_PTR( hIcon );
	CIconInfo info( hIcon );
	return info.m_size;
}

void CIcon::Draw( HDC hDC, const CPoint& pos, bool enabled /*= true*/ ) const
{
	ASSERT_PTR( m_hIcon );
	ASSERT_PTR( hDC );

	if ( enabled )
		::DrawIconEx( hDC, pos.x, pos.y, m_hIcon, GetSize().cx, GetSize().cy, 0, NULL, DI_NORMAL | DI_COMPAT );
	else
		DrawDisabled( hDC, pos );
}

void CIcon::DrawDisabled( HDC hDC, const CPoint& pos ) const
{
	ASSERT_PTR( m_hIcon );
	ASSERT_PTR( hDC );

	::DrawState( hDC, GetSysColorBrush( COLOR_3DSHADOW ), NULL, (LPARAM)m_hIcon, 0,
		pos.x, pos.y, GetSize().cx, GetSize().cy, DST_ICON | DSS_UNION );				// DSS_DISABLED looks uglier
}

const CIcon& CIcon::GetUnknownIcon( void )
{	// missing icon placeholder
	static const CIcon s_unknownIcon( res::LoadIcon( IDI_UNKNOWN ) );

	return s_unknownIcon;
}
bool CIcon::MakeBitmap( CBitmap& rBitmap, COLORREF transpColor ) const
{
	rBitmap.DeleteObject();

	if ( IsValid() )
		if ( m_hasAlpha )
		{
			CIconInfo info( m_hIcon );
			info.MakeDibSection( rBitmap );		// LR_DEFAULTCOLOR leaves a black background
				// ...ignore transpColor
		}
		else
		{
			const CSize& iconSize = GetSize();
			CWindowDC screenDC( NULL );
			CDC memDC;
			if ( memDC.CreateCompatibleDC( &screenDC ) )
				if ( rBitmap.CreateCompatibleBitmap( &screenDC, iconSize.cx, iconSize.cy ) )
				{
					CBitmap* pOldBitmap = memDC.SelectObject( &rBitmap );
					CRect rect( CPoint( 0, 0 ), iconSize );

					if ( transpColor != CLR_NONE )
					{
						CBrush bkBrush( transpColor );
						memDC.FillRect( &rect, &bkBrush );
					}
					Draw( memDC, rect.TopLeft() );

					memDC.SelectObject( pOldBitmap );
				}
		}

	return rBitmap.GetSafeHandle() != NULL;
}

void CIcon::CreateFromBitmap( HBITMAP hImageBitmap, HBITMAP hMaskBitmap )
{
	Release();

	m_hIcon = gdi::CreateIcon( hImageBitmap, hMaskBitmap );
	m_size = gdi::GetBitmapSize( hImageBitmap );
}

void CIcon::CreateFromBitmap( HBITMAP hImageBitmap, COLORREF transpColor )
{
	Release();

	m_hIcon = gdi::CreateIcon( hImageBitmap, transpColor );
	m_size = gdi::GetBitmapSize( hImageBitmap );
}


// CIconInfo implementation

CIconInfo::CIconInfo( HICON hIcon, bool isCursor /*= false*/ )
{
	Init( hIcon, isCursor );
}

CIconInfo::~CIconInfo()
{
}

void CIconInfo::Init( HICON hIcon, bool isCursor )
{
	ZeroMemory( &m_info, sizeof( m_info ) );
	m_info.fIcon = !isCursor;

	if ( hIcon != NULL )
	{
		VERIFY( ::GetIconInfo( hIcon, &m_info ) );

		BITMAP bmp;
		VERIFY( ::GetObject( m_info.hbmColor != NULL ? m_info.hbmColor : m_info.hbmMask, sizeof( bmp ), &bmp ) != 0 );
		m_size.cx = bmp.bmWidth;
		m_size.cy = bmp.bmHeight;

		// pass ownership to bitmap data members
		m_bitmapColor.Attach( m_info.hbmColor );
		m_bitmapMask.Attach( m_info.hbmMask );
	}
	else
		m_size.cx = m_size.cy = 0;
}

bool CIconInfo::MakeDibSection( CBitmap& rDibSection ) const
{
	rDibSection.DeleteObject();

	// CAREFUL:
	//	If it does not have an alpha channel (32 bpp) it dulls the colours and has slight pixel errors.
	//	Use this if m_hasAlpha, otherwise better use MakeBitmap with transparent colour.

	if ( m_info.hbmColor != NULL )
		rDibSection.Attach( (HBITMAP)::CopyImage( m_info.hbmColor, IMAGE_BITMAP, m_size.cx, m_size.cy, LR_CREATEDIBSECTION ) );		// copy colour bitmap as DIB
	else if ( m_info.hbmMask != NULL )				// monochrome icons have only the mask bitmap
		rDibSection.Attach( (HBITMAP)::CopyImage( m_info.hbmMask, IMAGE_BITMAP, m_size.cx, m_size.cy, LR_MONOCHROME ) );			// copy monochrome bitmap

	return rDibSection.GetSafeHandle() != NULL;
}
