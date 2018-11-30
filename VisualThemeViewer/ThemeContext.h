#ifndef ThemeContext_h
#define ThemeContext_h
#pragma once

#include "utl/ThemeItem.h"
#include "ThemeStore.h"


struct CThemeContext
{
	CThemeContext( void ) : m_pClass( NULL ), m_pPart( NULL ), m_pState( NULL ) {}

	bool IsValid( void ) const { return m_pClass != NULL && m_pPart != NULL; }

	CThemeItem GetThemeItem( void ) const;

	std::tstring FormatClass( void ) const;
	std::tstring FormatPart( bool asNumber = false ) const;
	std::tstring FormatState( bool asNumber = false ) const;

	std::tstring FormatTheme( void ) const;
	std::tstring FormatThemePartAndState( void ) const;
public:
	CThemeClass* m_pClass;
	CThemePart* m_pPart;
	CThemeState* m_pState;

	static const TCHAR s_paramSep[];
};


#endif // ThemeContext_h
