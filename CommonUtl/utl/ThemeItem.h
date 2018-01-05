#ifndef ThemeItem_h
#define ThemeItem_h
#pragma once


class CIcon;


struct CThemeItem
{
	CThemeItem( void );
	CThemeItem( const wchar_t* pThemeClass, int partId, int stateId );

	bool IsValid( void ) const { return m_pThemeClass != NULL; }
	bool IsThemed( void ) const;

	bool DrawBackground( HDC hdc, const RECT& rect, const RECT* pClipRect = NULL ) const;
	bool DrawBackgroundErase( HWND hWnd, HDC hdc, const RECT& rect, const RECT* pClipRect = NULL ) const;		// also erase for transparent stateId
	bool DrawEdge( HDC hdc, const RECT& rect, UINT edge, UINT flags, RECT* pContentRect = NULL ) const;
	bool DrawText( HDC hdc, const RECT& rect, const wchar_t* pText, DWORD textFlags ) const;
	bool DrawIcon( HDC hdc, const RECT& rect, const CImageList& imageList, int imagePos ) const;

	bool GetPartSize( CSize* pPartSize, HDC hdc, THEMESIZE themeSize = TS_TRUE, const RECT* pRect = NULL ) const;

	// status dependent drawing
	enum Status { Normal, Hot, Pressed, Disabled, StatusCount };

	int GetStateId( Status status = Normal ) const { return m_stateId[ status ]; }
	CThemeItem& SetStateId( Status status, int stateId = -1 );

	bool DrawStatusBackground( Status status, HDC hdc, const RECT& rect, const RECT* pClipRect = NULL ) const;
	bool DrawStatusBackgroundErase( Status status, HWND hWnd, HDC hdc, const RECT& rect, const RECT* pClipRect = NULL ) const;
	bool DrawStatusEdge( Status status, HDC hdc, const RECT& rect, UINT edge, UINT flags, RECT* pContentRect = NULL ) const;
	bool DrawStatusText( Status status, HDC hdc, const RECT& rect, const wchar_t* pText, DWORD textFlags ) const;

	// conversion to image
	bool MakeBitmap( CBitmap& rBitmap, COLORREF bkColor, const CSize& imageSize, int alignment = 0 /*NoAlign*/, Status status = Normal ) const;
	bool MakeIcon( CIcon& rIcon, const CSize& imageSize, int alignment = 0 /*NoAlign*/, Status status = Normal ) const;
public:
	const wchar_t* m_pThemeClass;
	int m_partId;
	int m_stateId[ StatusCount ];

	static const CThemeItem m_null;
};


// inline code

inline bool CThemeItem::DrawBackground( HDC hdc, const RECT& rect, const RECT* pClipRect /*= NULL*/ ) const
{
	return DrawStatusBackground( Normal, hdc, rect, pClipRect );
}

inline bool CThemeItem::DrawBackgroundErase( HWND hWnd, HDC hdc, const RECT& rect, const RECT* pClipRect /*= NULL*/ ) const
{
	return DrawStatusBackgroundErase( Normal, hWnd, hdc, rect, pClipRect );
}

inline bool CThemeItem::DrawEdge( HDC hdc, const RECT& rect, UINT edge, UINT flags, RECT* pContentRect /*= NULL*/ ) const
{
	return DrawStatusEdge( Normal, hdc, rect, edge, flags, pContentRect );
}

inline bool CThemeItem::DrawText( HDC hdc, const RECT& rect, const wchar_t* pText, DWORD textFlags ) const
{
	return DrawStatusText( Normal, hdc, rect, pText, textFlags );
}


#endif // ThemeItem_h
