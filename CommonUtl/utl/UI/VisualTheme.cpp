
#include "stdafx.h"
#include "VisualTheme.h"
#include "VisualThemeFallback.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma comment( lib, "uxtheme" )		// link statically with uxtheme.lib
#if ( WINVER < 0x0501 )
	#pragma message("Add uxtheme.dll to the list of delay loaded DLLs in the project settings.")
#endif


bool CVisualTheme::s_enabled = true;
bool CVisualTheme::s_fallbackEnabled = true;

CVisualTheme::CVisualTheme( const wchar_t* pClass, HWND hWnd /*= nullptr*/ )
	: m_pClass( pClass )
	, m_hTheme( nullptr )
{
	if ( IsThemed() )
		m_hTheme = ::OpenThemeData( hWnd, m_pClass );
}

CVisualTheme::~CVisualTheme()
{
	Close();
}

void CVisualTheme::Close( void )
{
	if ( m_hTheme != nullptr )
	{
		::CloseThemeData( m_hTheme );
		m_hTheme = nullptr;
	}
}

COLORREF CVisualTheme::GetThemeColor( int partId, int stateId, int propId ) const
{
	COLORREF color;
	if ( IsValid() && HR_OK( ::GetThemeColor( m_hTheme, partId, stateId, propId, &color ) ) )
		return color;
	return CLR_NONE;
}

bool CVisualTheme::GetThemePartSize( OUT CSize* pPartSize, HDC hdc, int partId, int stateId, THEMESIZE themeSize /*= TS_DRAW*/, const RECT* pRect /*= nullptr*/ ) const
{
	ASSERT_PTR( pPartSize );
	pPartSize->cx = pPartSize->cy = 0;
	return IsValid() && HR_OK( ::GetThemePartSize( m_hTheme, hdc, partId, stateId, pRect, themeSize, pPartSize ) );
}

bool CVisualTheme::GetThemeTextExtent( OUT RECT* pExtentRect, HDC hdc, int partId, int stateId, const wchar_t* pText, DWORD textFlags, const RECT* pBoundingRect /*= nullptr*/ ) const
{
	ASSERT_PTR( pExtentRect );
	return IsValid() && HR_OK( ::GetThemeTextExtent( m_hTheme, hdc, partId, stateId, pText, -1, textFlags, pBoundingRect, pExtentRect ) );
}

bool CVisualTheme::GetThemePosition( OUT POINT* pPoint, int partId, int stateId, int propId ) const
{
	ASSERT_PTR( pPoint );
	return IsValid() && HR_OK( ::GetThemePosition( m_hTheme, partId, stateId, propId, pPoint ) );
}

bool CVisualTheme::GetThemeRect( OUT RECT* pRect, int partId, int stateId, int propId ) const
{
	ASSERT_PTR( pRect );
	return IsValid() && HR_OK( ::GetThemeRect( m_hTheme, partId, stateId, propId, pRect ) );
}

bool CVisualTheme::GetThemeBackgroundContentRect( OUT RECT* pContentRect, HDC hdc, int partId, int stateId, const RECT* pBoundingRect )
{
	ASSERT_PTR( pContentRect );
	return IsValid() && HR_OK( ::GetThemeBackgroundContentRect( m_hTheme, hdc, partId, stateId, pBoundingRect, pContentRect ) );
}

bool CVisualTheme::GetThemeBackgroundExtent( OUT RECT* pExtentRect, HDC hdc, int partId, int stateId, const RECT* pContentRect )
{
	ASSERT_PTR( pExtentRect );
	return IsValid() && HR_OK( ::GetThemeBackgroundExtent( m_hTheme, hdc, partId, stateId, pContentRect, pExtentRect ) );
}


bool CVisualTheme::DrawThemeEdge( HDC hdc, int partId, int stateId, const RECT& rect, UINT edge, UINT flags, RECT* pContentRect /*= nullptr*/ )
{
	if ( !IsValid() && s_fallbackEnabled )
		return CVisualThemeFallback::Instance().DrawEdge( hdc, rect, edge, flags );

	return HR_OK( ::DrawThemeEdge( m_hTheme, hdc, partId, stateId, &rect, edge, flags, pContentRect ) );
}

bool CVisualTheme::DrawThemeBackground( HDC hdc, int partId, int stateId, const RECT& rect, const RECT* pClipRect /*= nullptr*/ )
{
	if ( !IsValid() && s_fallbackEnabled )
		return CVisualThemeFallback::Instance().DrawBackground( m_pClass, partId, stateId, hdc, rect );

	return HR_OK( ::DrawThemeBackground( m_hTheme, hdc, partId, stateId, &rect, pClipRect ) );
}

bool CVisualTheme::DrawThemeText( HDC hdc, int partId, int stateId, const RECT& rect, const wchar_t* pText, DWORD textFlags )
{
	return IsValid() && HR_OK( ::DrawThemeText( m_hTheme, hdc, partId, stateId, pText, -1, textFlags, 0, &rect ) );
}

bool CVisualTheme::DrawThemeIcon( HDC hdc, int partId, int stateId, const RECT& rect, const CImageList& imageList, int imagePos )
{
	return IsValid() && HR_OK( ::DrawThemeIcon( m_hTheme, hdc, partId, stateId, &rect, imageList.GetSafeHandle(), imagePos ) );
}

bool CVisualTheme::DrawBackground( HWND hWnd, HDC hdc, int partId, int stateId, const RECT& rect, const RECT* pClipRect /*= nullptr*/ )
{
	if ( IsThemeBackgroundPartiallyTransparent( partId, stateId ) )
		DrawThemeParentBackground( hWnd, hdc, rect );

	return DrawThemeBackground( hdc, partId, stateId, rect, pClipRect );
}


bool CVisualTheme::DrawEntireBackground( HDC hdc, int partId, int stateId, const RECT& rect, const RECT* pClipRect /*= nullptr*/ )
{
	CRect boundsRect = rect;

	// some classes draw inside the bounds, so require some stretching to cover entirely
	if ( m_pClass != nullptr )
		if ( 0 == _wcsicmp( L"BUTTON", m_pClass ) )
			boundsRect.InflateRect( 1, 1 );		// stretch both
		else if ( 0 == _wcsicmp( L"TOOLBAR", m_pClass ) )
			boundsRect.InflateRect( 0, 1 );		// stretch vertically

	return DrawThemeBackground( hdc, partId, stateId, boundsRect, pClipRect );
}

bool CVisualTheme::IsDisabledState( int partId, int stateId )
{
	switch ( partId )
	{
		case vt::MENU_POPUPITEM:
			switch ( stateId )
			{
				case vt::MPI_DISABLED:
				case vt::MPI_DISABLEDHOT:
					return true;
			}
			break;
		case vt::MENU_BARITEM:
			switch ( stateId )
			{
				case vt::MBI_DISABLED:
				case vt::MBI_DISABLEDHOT:
				case vt::MBI_DISABLEDPUSHED:
					return true;
			}
			break;
		case vt::BP_PUSHBUTTON:
			switch ( stateId )
			{
				case vt::PBS_DISABLED:
					return true;
			}
			break;
	}
	return false;
}
