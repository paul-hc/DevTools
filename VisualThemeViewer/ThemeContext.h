#ifndef ThemeContext_h
#define ThemeContext_h
#pragma once

#include "utl/StringUtilities.h"
#include "utl/ThemeItem.h"
#include "ThemeStore.h"


struct CThemeContext
{
	CThemeContext( void ) : m_pClass( NULL ), m_pPart( NULL ), m_pState( NULL ) {}

	bool IsValid( void ) const { return m_pClass != NULL && m_pPart != NULL; }

	CThemeItem GetThemeItem( void ) const
	{
		if ( !IsValid() )
			return CThemeItem::m_null;
		return CThemeItem( m_pClass->m_className.c_str(), m_pPart->m_partId, m_pState != NULL ? m_pState->m_stateId : 0 );
	}

	std::tstring FormatPart( bool asNumber = false ) const
	{
		if ( m_pPart != NULL )
			if ( asNumber )
				return str::Format( _T("%d"), m_pPart->m_partId );
			else
				return m_pPart->m_partName;

		return std::tstring();
	}

	std::tstring FormatState( bool asNumber = false ) const
	{
		if ( m_pState != NULL )
			if ( asNumber )
				return str::Format( _T("%d"), m_pState->m_stateId );
			else
				return m_pState->m_stateName;
		else if ( asNumber && m_pPart != NULL )
			return _T("0");
		return std::tstring();
	}
public:
	CThemeClass* m_pClass;
	CThemePart* m_pPart;
	CThemeState* m_pState;
};


#endif // ThemeContext_h
