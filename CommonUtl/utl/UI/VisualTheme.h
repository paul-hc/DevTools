#ifndef VisualTheme_h
#define VisualTheme_h
#pragma once

#include <uxtheme.h>


class CVisualTheme
{
public:
	CVisualTheme( const wchar_t* pClass, HWND hWnd = nullptr );
	~CVisualTheme();

	static bool IsThemed( void ) { return IsEnabled() && ::IsAppThemed() != FALSE; }
	static bool IsDisabled( void ) { return !IsEnabled(); }

	static bool IsEnabled( void ) { return s_enabled; }
	static void SetEnabled( bool enabled ) { s_enabled = enabled; }

	static bool IsFallbackEnabled( void ) { return s_fallbackEnabled; }
	static void SetFallbackEnabled( bool fallbackEnabled ) { s_fallbackEnabled = fallbackEnabled; }

	bool IsValid( void ) const { return m_hTheme != nullptr; }
	HTHEME GetTheme( void ) const { return m_hTheme; }

	void Close( void );

	static bool SetWindowTheme( HWND hWnd, const wchar_t* pSubAppName, const wchar_t* pSubIdList = nullptr ) { return IsThemed() && HR_OK( ::SetWindowTheme( hWnd, pSubAppName, pSubIdList ) ); }

	bool IsThemePartDefined( int partId, int stateId ) const { return IsValid() && ::IsThemePartDefined( m_hTheme, partId, stateId ) != FALSE; }
	bool IsThemeBackgroundPartiallyTransparent( int partId, int stateId ) const { return IsValid() && ::IsThemeBackgroundPartiallyTransparent( m_hTheme, partId, stateId ) != FALSE; }

	COLORREF GetThemeColor( int partId, int stateId, int propId ) const;
	bool GetThemePartSize( OUT CSize* pPartSize, HDC hdc, int partId, int stateId, THEMESIZE themeSize = TS_DRAW, const RECT* pRect = nullptr ) const;
	bool GetThemeTextExtent( OUT RECT* pExtentRect, HDC hdc, int partId, int stateId, const wchar_t* pText, DWORD textFlags,
							 const RECT* pBoundingRect = nullptr ) const;
	bool GetThemePosition( OUT POINT* pPoint, int partId, int stateId, int propId ) const;
	bool GetThemeRect( OUT RECT* pRect, int partId, int stateId, int propId ) const;
	bool GetThemeBackgroundContentRect( OUT RECT* pContentRect, HDC hdc, int partId, int stateId,  const RECT* pBoundingRect );
	bool GetThemeBackgroundExtent( OUT RECT* pExtentRect, HDC hdc, int partId, int stateId, const RECT* pContentRect );

	bool DrawThemeEdge( HDC hdc, int partId, int stateId, const RECT& rect, UINT edge, UINT flags, RECT* pContentRect = nullptr );
	bool DrawThemeBackground( HDC hdc, int partId, int stateId, const RECT& rect, const RECT* pClipRect = nullptr );
	bool DrawThemeText( HDC hdc, int partId, int stateId, const RECT& rect, const wchar_t* pText, DWORD textFlags );
	bool DrawThemeIcon( HDC hdc, int partId, int stateId, const RECT& rect, const CImageList& imageList, int imagePos );

	static bool DrawThemeParentBackground( HWND hWnd, HDC hdc, const RECT& rect ) { return IsThemed() && HR_OK( ::DrawThemeParentBackground( hWnd, hdc, &rect ) ); }

	// complex drawing
	bool DrawBackground( HWND hWnd, HDC hdc, int partId, int stateId, const RECT& rect, const RECT* pClipRect = nullptr );
	bool DrawEntireBackground( HDC hdc, int partId, int stateId, const RECT& rect, const RECT* pClipRect = nullptr );

	// theme testing
	static bool* GetEnabledPtr( void ) { return &s_enabled; }
	static bool* GetFallbackEnabledPtr( void ) { return &s_fallbackEnabled; }
private:
	static bool IsDisabledState( int partId, int stateId );
private:
	const wchar_t* m_pClass;			// control class
	HTHEME m_hTheme;
private:
	enum { LazyLoad = -1, StaticLib = -2 };

	static bool s_enabled;				// may be disabled to test CVisualThemeFallback
	static bool s_fallbackEnabled;
};


#endif // VisualTheme_h
