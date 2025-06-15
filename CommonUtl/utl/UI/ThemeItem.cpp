
#include "pch.h"
#include "ThemeItem.h"
#include "Icon.h"
#include "WndUtils.h"
#include "VisualTheme.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const CThemeItem CThemeItem::s_null;


CThemeItem::CThemeItem( void )
	: m_pThemeClass( nullptr )
	, m_partId( 0 )
{
	std::fill( m_stateId, m_stateId + COUNT_OF( m_stateId ), 0 );
}

CThemeItem::CThemeItem( const wchar_t* pThemeClass, int partId, int stateId /*= 0*/ )
	: m_pThemeClass( pThemeClass )
	, m_partId( partId )
{
	std::fill( m_stateId, m_stateId + COUNT_OF( m_stateId ), stateId );
}

bool CThemeItem::IsThemed( void ) const
{
	return IsValid() && CVisualTheme( m_pThemeClass ).IsValid();
}

CThemeItem& CThemeItem::SetStateId( Status status, int stateId /*= -1*/ )
{
	m_stateId[ status ] = stateId != -1 ? stateId : m_stateId[ Normal ];
	return *this;
}

bool CThemeItem::DrawStatusBackground( Status status, HDC hdc, const RECT& rect, const RECT* pClipRect /*= nullptr*/ ) const
{
	if ( !IsValid() )
		return false;

	CVisualTheme theme( m_pThemeClass );
	return theme.DrawThemeBackground( hdc, m_partId, GetStateId( status ), rect, pClipRect );
}

bool CThemeItem::DrawStatusBackgroundErase( Status status, HWND hWnd, HDC hdc, const RECT& rect, const RECT* pClipRect /*= nullptr*/ ) const
{
	if ( !IsValid() )
		return false;

	CVisualTheme theme( m_pThemeClass );
	return theme.DrawBackground( hWnd, hdc, m_partId, GetStateId( status ), rect, pClipRect );
}

bool CThemeItem::DrawStatusEdge( Status status, HDC hdc, const RECT& rect, UINT edge, UINT flags, RECT* pContentRect /*= nullptr*/ ) const
{
	if ( !IsValid() )
		return false;

	CVisualTheme theme( m_pThemeClass );
	return theme.DrawThemeEdge( hdc, m_partId, GetStateId( status ), rect, edge, flags, pContentRect );
}

bool CThemeItem::DrawStatusText( Status status, HDC hdc, const RECT& rect, const wchar_t* pText, DWORD textFlags ) const
{
	if ( !IsValid() )
		return false;

	CVisualTheme theme( m_pThemeClass );
	if ( theme.IsValid() )
		if ( HasFlag( textFlags, DT_CALCRECT ) )
		{
			CRect extentRect;
			if ( theme.GetThemeTextExtent( extentRect, hdc, m_partId, GetStateId( status ), pText, textFlags, &rect ) )
			{
				const_cast<RECT&>( rect ) = extentRect;
				return true;
			}
		}
		else
			return theme.DrawThemeText( hdc, m_partId, GetStateId( status ), rect, pText, textFlags );

	return false;
}

bool CThemeItem::DrawIcon( HDC hdc, const RECT& rect, const CImageList& imageList, int imagePos ) const
{
	if ( !IsValid() )
		return false;

	CVisualTheme theme( m_pThemeClass );
	return theme.IsValid() && theme.DrawThemeIcon( hdc, m_partId, m_stateId[ Normal ], rect, imageList, imagePos );
}

bool CThemeItem::GetPartSize( CSize* pPartSize, HDC hdc, THEMESIZE themeSize /*= TS_TRUE*/, const RECT* pRect /*= nullptr*/ ) const
{
	if ( !IsValid() )
		return false;

	CVisualTheme theme( m_pThemeClass );
	return theme.IsValid() && theme.GetThemePartSize( pPartSize, hdc, m_partId, m_stateId[ Normal ], themeSize, pRect );
}

bool CThemeItem::MakeBitmap( CBitmap& rBitmap, COLORREF bkColor, const CSize& imageSize, TAlignment alignment /*= NoAlign*/, Status status /*= Normal*/ ) const
{
	if ( rBitmap.GetSafeHandle() != nullptr )
		rBitmap.DeleteObject();

	if ( !IsValid() )
		return false;

	CVisualTheme theme( m_pThemeClass );

	bool success = false;
	CWindowDC screenDC( nullptr );
	CDC memDC;
	if ( memDC.CreateCompatibleDC( &screenDC ) )
		if ( rBitmap.CreateCompatibleBitmap( &screenDC, imageSize.cx, imageSize.cy ) )
		{
			CBitmap* pOldBitmap = memDC.SelectObject( &rBitmap );
			CRect rect( 0, 0, imageSize.cx, imageSize.cy );

			if ( bkColor != CLR_NONE )
			{
				CBrush bkBrush( bkColor );
				memDC.FillRect( &rect, &bkBrush );
			}

			CRect contentRect = rect;

			if ( alignment != NoAlign && theme.IsValid() )
			{	// size to content and apply the alignment
				CSize coreSize;
				if ( theme.GetThemePartSize( &coreSize, memDC, m_partId, GetStateId( status ), TS_TRUE, &rect ) )
				{
					contentRect.SetRect( 0, 0, coreSize.cx, coreSize.cy );
					ui::AlignRect( contentRect, rect, alignment );
				}
			}

			success = theme.DrawThemeBackground( memDC, m_partId, GetStateId( status ), contentRect );
			memDC.SelectObject( pOldBitmap );
		}

	return success && rBitmap.GetSafeHandle() != nullptr;
}

bool CThemeItem::MakeIcon( CIcon& rIcon, const CSize& imageSize, TAlignment alignment /*= NoAlign*/, Status status /*= Normal*/ ) const
{
	enum { TranspBkColor = RGB( 255, 255, 254 ) };	// almost white: so that themes that render with alpha blending don't show weird colours (such as radio button)
	rIcon.Clear();

	CBitmap imageBitmap;
	if ( !MakeBitmap( imageBitmap, TranspBkColor, imageSize, alignment, status ) )
		return false;

	CBitmap maskBitmap;
	if ( gdi::CreateBitmapMask( maskBitmap, imageBitmap, TranspBkColor ) )
		return false;

	rIcon.CreateFromBitmap( imageBitmap, maskBitmap );
	return rIcon.GetHandle() != nullptr;
}
