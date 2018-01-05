#ifndef WndSearchPattern_h
#define WndSearchPattern_h
#pragma once

#include "utl/StringUtilities.h"


struct CWndSearchPattern
{
	CWndSearchPattern( void );

	void Save( void ) const;
	void Load( void );

	bool Matches( HWND hWnd ) const;
	bool IsMatch( const std::tstring& text, const std::tstring& part ) const { return str::Matches( text.c_str(), part.c_str(), m_matchCase, m_matchWhole ); }

	std::tstring FormatNotFound( void ) const;
public:
	bool m_fromBeginning;
	bool m_forward;
	bool m_matchCase;
	bool m_matchWhole;
	bool m_refreshNow;

	int m_id;
	std::tstring m_wndClass;
	std::tstring m_caption;
	HWND m_handle;					// when searching for an actual handle (not persistent)
};


#endif // WndSearchPattern_h
