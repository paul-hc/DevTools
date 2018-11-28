
#include "stdafx.h"
#include "ThemeContext.h"
#include "utl/StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const TCHAR CThemeContext::s_paramSep[] = _T(", ");

CThemeItem CThemeContext::GetThemeItem( void ) const
{
	if ( !IsValid() )
		return CThemeItem::m_null;

	return CThemeItem( m_pClass->m_className.c_str(), m_pPart->m_partId, m_pState != NULL ? m_pState->m_stateId : 0 );
}

std::tstring CThemeContext::FormatClass( void ) const
{
	if ( m_pClass != NULL )
		return str::Format( _T("L\"%s\""), m_pClass->m_className.c_str() );

	return std::tstring();
}

std::tstring CThemeContext::FormatPart( bool asNumber /*= false*/ ) const
{
	if ( m_pPart != NULL )
		if ( asNumber )
			return str::Format( _T("%d"), m_pPart->m_partId );
		else
			return m_pPart->m_partName;

	return std::tstring();
}

std::tstring CThemeContext::FormatState( bool asNumber /*= false*/ ) const
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

std::tstring CThemeContext::FormatTheme( void ) const
{
	// L"EDIT", EP_EDITTEXT, ETS_NORMAL
	std::tstring text = FormatClass();
	stream::Tag( text, FormatPart(), s_paramSep );
	stream::Tag( text, FormatState(), s_paramSep );
	return text;
}

std::tstring CThemeContext::FormatThemePartAndState( void ) const
{
	// EP_EDITTEXT, ETS_NORMAL
	std::tstring text = FormatPart();
	stream::Tag( text, FormatState(), s_paramSep );
	return text;
}
