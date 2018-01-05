
#include "stdafx.h"
#include "EnumTags.h"
#include "StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const TCHAR CEnumTags::m_listSep[] = _T("|");
const TCHAR CEnumTags::m_tagSep[] = _T("|");


CEnumTags::CEnumTags( const std::tstring& uiTags, const TCHAR* pKeyTags /*= NULL*/, int defaultValue /*= -1*/ )
	: m_defaultValue( defaultValue )
{
	Construct( uiTags, pKeyTags );
}

CEnumTags::~CEnumTags()
{
}

void CEnumTags::Construct( const std::tstring& uiTags, const TCHAR* pKeyTags )
{
	str::Split( m_uiTags, uiTags.c_str(), m_listSep );
	if ( pKeyTags != NULL )
		str::Split( m_keyTags, pKeyTags, m_listSep );

	ENSURE( !m_uiTags.empty() );
	ENSURE( m_keyTags.empty() || m_keyTags.size() == m_uiTags.size() );
}

std::tstring CEnumTags::Format( int value, const std::vector< std::tstring >& tags )
{
	ASSERT( value >= 0 && value < (int)tags.size() );
	return tags[ value ];
}

int CEnumTags::Parse( const std::tstring& text, const std::vector< std::tstring >& tags ) const
{
	ASSERT( !tags.empty() );

	for ( unsigned int value = 0; value != tags.size(); ++value )
		if ( pred::Equal == str::CompareNoCase( tags[ value ], text ) )
			return value;

	return m_defaultValue;
}

bool CEnumTags::Contains( const std::vector< std::tstring >& strings, const std::tstring& value )
{
	for ( std::vector< std::tstring >::const_iterator itString = strings.begin();
		  itString != strings.end(); ++itString )
		if ( pred::Equal == str::CompareNoCase( *itString, value ) )
			return true;

	return false;
}
