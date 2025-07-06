
#include "pch.h"
#include "Icon.h"
#include "Imaging.h"
#include "ImageProxy.h"
#include "GroupIconRes.h"
#include "DibSection.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CIcon::CIcon( void )
	: m_hIcon( nullptr )
	, m_hDisabledIcon( nullptr )
	, m_hasAlpha( false )
	, m_pShared( nullptr )
	, m_size( 0, 0 )
	, m_bitsPerPixel( 0 )
	, m_pIconGroup( nullptr )
	, m_groupFramePos( utl::npos )
{
}

CIcon::CIcon( HICON hIcon, const CSize& iconSize /*= CSize( 0, 0 )*/, TBitsPerPixel bitsPerPixel /*= 0*/ )
	: m_hIcon( hIcon )
	, m_hDisabledIcon( nullptr )
	, m_pShared( nullptr )
	, m_hasAlpha( false )
	, m_size( iconSize )
	, m_bitsPerPixel( bitsPerPixel )
	, m_pIconGroup( nullptr )
	, m_groupFramePos( utl::npos )
{
}

CIcon::CIcon( HICON hIcon, const res::CGroupIconEntry& groupIconEntry )
	: m_hIcon( hIcon )
	, m_hDisabledIcon( nullptr )
	, m_pShared( nullptr )
	, m_hasAlpha( ILC_COLOR32 == m_bitsPerPixel )
	, m_size( groupIconEntry.GetSize() )
	, m_bitsPerPixel( groupIconEntry.GetBitsPerPixel() )
	, m_pIconGroup( nullptr )
	, m_groupFramePos( utl::npos )
{
}

CIcon::CIcon( const CIcon& shared )
	: m_hIcon( shared.m_hIcon )
	, m_hDisabledIcon( shared.m_hDisabledIcon )
	, m_pShared( &shared )
	, m_hasAlpha( shared.m_hasAlpha )
	, m_size( shared.m_size )
	, m_bitsPerPixel( shared.m_bitsPerPixel )
	, m_pIconGroup( nullptr )
	, m_groupFramePos( utl::npos )
{
}

CIcon::~CIcon()
{
	Clear();
}

bool CIcon::LazyComputeInfo( void ) const
{
	ASSERT_PTR( m_hIcon );

	if ( m_size.cx != 0 || m_size.cy != 0 )
		return false;		// already evaluated

	CIconInfo iconInfo( m_hIcon );

	if ( !iconInfo.IsValid() )
		 return false;

	m_size = iconInfo.m_size;

	ENSURE( m_size.cx > 0 && m_size.cy > 0 );
	ENSURE( m_bitsPerPixel >= ILC_MASK && m_bitsPerPixel <= ILC_COLOR32 );

	if ( m_hasAlpha != iconInfo.HasAlpha() )	// sometimes it could happen for unusual icon formats, e.g. 256x256 - iconInfo.HasAlpha() is less reliable since the 32bpp icon could still have a mask
		TRACE( _T("CIcon::LazyComputeInfo() - conflicting states: m_hasAlpha=%d vs. iconInfo.HasAlpha()=%d\n"), m_hasAlpha, iconInfo.HasAlpha() );

	return true;			// lazy evaluated
}

CIcon* CIcon::LoadNewIcon( HICON hIcon )
{
	ASSERT_PTR( hIcon );

	CIconInfo iconInfo( hIcon );
	CIcon* pIcon = new CIcon( hIcon, iconInfo.m_size );

	pIcon->SetHasAlpha( iconInfo.HasAlpha() );

	return pIcon;
}

CIcon* CIcon::LoadNewIcon( const CIconId& iconId )
{
	CGroupIconRes groupIcon( iconId.m_id );

	if ( const res::CGroupIconEntry* pGroupIconEntry = groupIcon.FindMatch( iconId.m_stdSize ) )		// highest BPP
		return LoadNewIcon( *pGroupIconEntry );

	if ( HICON hIcon = res::LoadIcon( iconId ) )			// scales to nearest size if the icon doesn't contain the exact size
		return new CIcon( hIcon, iconId.GetSize() );

	return nullptr;
}

CIcon* CIcon::LoadNewIcon( const res::CGroupIconEntry& groupIconEntry )
{	// load from resources the exact icon frame
	bool hasAlpha = ILC_COLOR32 == groupIconEntry.GetBitsPerPixel();

	if ( HICON hIcon = CGroupIconRes::LoadIconEntry( groupIconEntry ) )
		return SetHasAlpha( new CIcon( hIcon, groupIconEntry.GetSize(), groupIconEntry.GetBitsPerPixel() ), hasAlpha );

	return nullptr;
}

CIcon* CIcon::LoadNewExactIcon( const CIconId& iconId )
{	// loads only if the icon contains the exact size
	TBitsPerPixel bitsPerPixel;

	if ( CGroupIconRes( iconId.m_id ).ContainsSize( iconId.m_stdSize, &bitsPerPixel ) )
		if ( HICON hIcon = res::LoadIcon( iconId ) )
		{
			return SetHasAlpha( new CIcon( hIcon, iconId.GetSize(), bitsPerPixel ), 32 == bitsPerPixel );
		}

	return nullptr;
}

HICON CIcon::Detach( void )
{
	HICON hIcon = m_hIcon;

	if ( !IsShared() && m_hDisabledIcon != nullptr )
		::DestroyIcon( m_hDisabledIcon );

	m_hIcon = nullptr;
	m_hDisabledIcon = nullptr;
	m_pShared = nullptr;
	m_hasAlpha = false;
	m_size.cx = m_size.cy = 0;
	m_bitsPerPixel = 0;

	return hIcon;
}

void CIcon::Clear( void )
{
	bool shared = IsShared();

	if ( HICON hIcon = Detach() )
		if ( !shared )
			::DestroyIcon( hIcon );
}

CIcon& CIcon::SetHandle( HICON hIcon, bool hasAlpha /*= true*/ )
{
	Clear();

	m_hIcon = hIcon;
	SetHasAlpha( hasAlpha );
	return *this;
}

void CIcon::SetHasAlpha( bool hasAlpha )
{
	m_hasAlpha = hasAlpha;

	if ( m_hasAlpha )
		m_bitsPerPixel = ILC_COLOR32;
}

CIcon* CIcon::SetHasAlpha( CIcon* pIcon, bool hasAlpha )
{
	if ( pIcon != nullptr )					// safe if NULL pIcon
		pIcon->SetHasAlpha( hasAlpha );

	return pIcon;
}

void CIcon::SetShared( const CIcon& shared )
{
	Clear();
	m_hIcon = shared.m_hIcon;
	m_hasAlpha = shared.m_hasAlpha;
}

const CSize& CIcon::GetSize( void ) const
{	// lazy size computation
	ASSERT_PTR( m_hIcon );
	if ( IsShared() )
		return m_pShared->GetSize();

	LazyComputeInfo();
	return m_size;
}

HICON CIcon::GetDisabledIcon( void ) const
{
	if ( IsValid() && nullptr == m_hDisabledIcon )
	{
		CIconProxy iconProxy( this );
		CDibSection disabledBitmap;

		if ( disabledBitmap.CreateDisabledEffectDIB32( &iconProxy, m_bitsPerPixel, gdi::Dis_FadeGray ) )
		{
			CIconInfo iconInfo( m_hIcon );

			m_hDisabledIcon = gdi::CreateIcon( disabledBitmap, iconInfo.m_bitmapMask );
		}
	}

	return m_hDisabledIcon;
}


void CIcon::Draw( HDC hDC, const CPoint& pos, bool enabled /*= true*/ ) const
{
	DrawStretch( hDC, CRect( pos, GetSize() ), enabled );
}

void CIcon::DrawDisabled( HDC hDC, const CPoint& pos ) const
{
	DrawStretch( hDC, CRect( pos, GetSize() ), false );
}

void CIcon::DrawStretch( HDC hDC, const CRect& destRect, bool enabled /*= true*/ ) const
{
	ASSERT_PTR( m_hIcon );
	ASSERT_PTR( hDC );

	if ( enabled )
		::DrawIconEx( hDC, destRect.left, destRect.top, m_hIcon, destRect.Width(), destRect.Height(), 0, nullptr, DI_NORMAL | DI_COMPAT );
	else
	{
		if ( GetDisabledIcon() != nullptr )
			::DrawIconEx( hDC, destRect.left, destRect.top, m_hDisabledIcon, destRect.Width(), destRect.Height(), 0, nullptr, DI_NORMAL | DI_COMPAT);
		else
			::DrawState( hDC, ::GetSysColorBrush( COLOR_3DSHADOW ), nullptr, (LPARAM)m_hIcon, 0,
						 destRect.left, destRect.top, destRect.Width(), destRect.Height(), DST_ICON | DSS_DISABLED /*DSS_UNION*/ );
	}
}

const CIcon& CIcon::GetUnknownIcon( void )
{	// missing icon placeholder
	static const CIcon s_unknownIcon( res::LoadIcon( IDI_UNKNOWN ) );

	return s_unknownIcon;
}

ui::CIconEntry CIcon::GetGroupIconEntry( void ) const
{
	if ( HasIconGroup() )
		return m_pIconGroup->GetAt( m_groupFramePos ).first;

	return ui::CIconEntry( GetBitsPerPixel(), GetSize() );
}

void CIcon::ResetIconGroup( CIconGroup* pIconGroup /*= nullptr*/, size_t groupFramePos /*= utl::npos*/ )
{
	m_pIconGroup = pIconGroup;
	m_groupFramePos = groupFramePos;

	ENSURE( ( m_pIconGroup != nullptr ) == ( m_groupFramePos != utl::npos ) );		// consistent?
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
			CWindowDC screenDC( nullptr );
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

	return rBitmap.GetSafeHandle() != nullptr;
}

void CIcon::CreateFromBitmap( HBITMAP hImageBitmap, HBITMAP hMaskBitmap )
{
	Clear();

	m_hIcon = gdi::CreateIcon( hImageBitmap, hMaskBitmap );
	m_size = gdi::GetBitmapSize( hImageBitmap );
}

void CIcon::CreateFromBitmap( HBITMAP hImageBitmap, COLORREF transpColor )
{
	Clear();

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

	if ( hIcon != nullptr )
	{
		VERIFY( ::GetIconInfo( hIcon, &m_info ) );

		BITMAP bmp;		// Note: m_info.hbmColor and m_info.hbmMask are DDBs.
		VERIFY( ::GetObject( m_info.hbmColor != nullptr ? m_info.hbmColor : m_info.hbmMask, sizeof( bmp ), &bmp ) != 0 );

		m_size.cx = bmp.bmWidth;
		m_size.cy = bmp.bmHeight;

		// pass ownership to bitmap data members
		m_bitmapColor.Attach( m_info.hbmColor );
		m_bitmapMask.Attach( m_info.hbmMask );
	}
	else
	{
		m_size.cx = m_size.cy = 0;
	}
}

bool CIconInfo::MakeDibSection( CBitmap& rDibSection ) const
{
	rDibSection.DeleteObject();

	// CAREFUL:
	//	If it does not have an alpha channel (32 bpp) it dulls the colours and has slight pixel errors.
	//	Use this if m_hasAlpha, otherwise better use MakeBitmap with transparent colour.

	if ( m_info.hbmColor != nullptr )
		rDibSection.Attach( (HBITMAP)::CopyImage( m_info.hbmColor, IMAGE_BITMAP, m_size.cx, m_size.cy, LR_CREATEDIBSECTION ) );		// copy colour bitmap as DIB
	else if ( m_info.hbmMask != nullptr )				// monochrome icons have only the mask bitmap
		rDibSection.Attach( (HBITMAP)::CopyImage( m_info.hbmMask, IMAGE_BITMAP, m_size.cx, m_size.cy, LR_MONOCHROME ) );			// copy monochrome bitmap

	return rDibSection.GetSafeHandle() != nullptr;
}
