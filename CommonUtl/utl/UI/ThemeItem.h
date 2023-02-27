#ifndef ThemeItem_h
#define ThemeItem_h
#pragma once

#include "ui_fwd.h"


class CIcon;


struct CThemeItem
{
	CThemeItem( void );
	CThemeItem( const wchar_t* pThemeClass, int partId, int stateId = 0 );

	bool IsValid( void ) const { return m_pThemeClass != nullptr; }
	bool IsThemed( void ) const;

	bool DrawBackground( HDC hdc, const RECT& rect, const RECT* pClipRect = nullptr ) const;
	bool DrawBackgroundErase( HWND hWnd, HDC hdc, const RECT& rect, const RECT* pClipRect = nullptr ) const;		// also erase for transparent stateId
	bool DrawEdge( HDC hdc, const RECT& rect, UINT edge, UINT flags, RECT* pContentRect = nullptr ) const;
	bool DrawText( HDC hdc, const RECT& rect, const wchar_t* pText, DWORD textFlags ) const;
	bool DrawIcon( HDC hdc, const RECT& rect, const CImageList& imageList, int imagePos ) const;

	bool GetPartSize( CSize* pPartSize, HDC hdc, THEMESIZE themeSize = TS_TRUE, const RECT* pRect = nullptr ) const;

	// status dependent drawing
	enum Status { Normal, Hot, Pressed, Disabled, _StatusCount };

	int GetStateId( Status status = Normal ) const { return m_stateId[ status ]; }
	CThemeItem& SetStateId( Status status, int stateId = -1 );

	bool DrawStatusBackground( Status status, HDC hdc, const RECT& rect, const RECT* pClipRect = nullptr ) const;
	bool DrawStatusBackgroundErase( Status status, HWND hWnd, HDC hdc, const RECT& rect, const RECT* pClipRect = nullptr ) const;
	bool DrawStatusEdge( Status status, HDC hdc, const RECT& rect, UINT edge, UINT flags, RECT* pContentRect = nullptr ) const;
	bool DrawStatusText( Status status, HDC hdc, const RECT& rect, const wchar_t* pText, DWORD textFlags ) const;

	// conversion to image
	bool MakeBitmap( CBitmap& rBitmap, COLORREF bkColor, const CSize& imageSize, TAlignment alignment = NoAlign, Status status = Normal ) const;
	bool MakeIcon( CIcon& rIcon, const CSize& imageSize, TAlignment alignment = NoAlign, Status status = Normal ) const;
public:
	const wchar_t* m_pThemeClass;
	int m_partId;
	int m_stateId[ _StatusCount ];

	static const CThemeItem m_null;
};


// inline code

inline bool CThemeItem::DrawBackground( HDC hdc, const RECT& rect, const RECT* pClipRect /*= nullptr*/ ) const
{
	return DrawStatusBackground( Normal, hdc, rect, pClipRect );
}

inline bool CThemeItem::DrawBackgroundErase( HWND hWnd, HDC hdc, const RECT& rect, const RECT* pClipRect /*= nullptr*/ ) const
{
	return DrawStatusBackgroundErase( Normal, hWnd, hdc, rect, pClipRect );
}

inline bool CThemeItem::DrawEdge( HDC hdc, const RECT& rect, UINT edge, UINT flags, RECT* pContentRect /*= nullptr*/ ) const
{
	return DrawStatusEdge( Normal, hdc, rect, edge, flags, pContentRect );
}

inline bool CThemeItem::DrawText( HDC hdc, const RECT& rect, const wchar_t* pText, DWORD textFlags ) const
{
	return DrawStatusText( Normal, hdc, rect, pText, textFlags );
}


#endif // ThemeItem_h
