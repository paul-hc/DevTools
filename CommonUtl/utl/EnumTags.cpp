
#include "stdafx.h"
#include "EnumTags.h"
#include "StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const TCHAR CEnumTags::m_listSep[] = _T("|");
const TCHAR CEnumTags::m_tagSep[] = _T("|");


CEnumTags::CEnumTags( const std::tstring& uiTags, const TCHAR* pKeyTags /*= NULL*/, int defaultValue /*= -1*/, int baseValue /*= 0*/ )
	: m_defaultValue( defaultValue )
	, m_baseValue( baseValue )
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

size_t CEnumTags::TagIndex( int value, const std::vector< std::tstring >& tags ) const
{
	size_t tagIndex = value - m_baseValue;
	ENSURE( tagIndex < tags.size() ); tags;
	return tagIndex;
}

const std::tstring& CEnumTags::_Format( int value, const std::vector< std::tstring >& tags ) const
{
	return tags[ TagIndex( value, tags ) ];
}

bool CEnumTags::_Parse( int& rValue, const std::tstring& text, const std::vector< std::tstring >& tags ) const
{
	ASSERT( !tags.empty() );

	for ( unsigned int value = 0; value != tags.size(); ++value )
		if ( str::EqualString< str::IgnoreCase >( tags[ value ], text ) )
		{
			rValue = value + m_baseValue;
			return true;
		}

	rValue = m_defaultValue;
	return false;
}

bool CEnumTags::Contains( const std::vector< std::tstring >& strings, const std::tstring& value )
{
	for ( std::vector< std::tstring >::const_iterator itString = strings.begin(); itString != strings.end(); ++itString )
		if ( pred::Equal == str::CompareNoCase( *itString, value ) )
			return true;

	return false;
}
