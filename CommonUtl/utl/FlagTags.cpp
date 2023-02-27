
#include "pch.h"
#include "FlagTags.h"
#include "StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const TCHAR CFlagTags::m_listSep[] = _T("|");
const TCHAR CFlagTags::m_tagSep[] = _T("|");


CFlagTags::CFlagTags( const std::tstring& uiTags, const TCHAR* pKeyTags /*= nullptr*/ )
{
	str::Split( m_uiTags, uiTags.c_str(), m_listSep );
	if ( pKeyTags != nullptr )
		str::Split( m_keyTags, pKeyTags, m_listSep );

	ENSURE( !m_uiTags.empty() );
	ENSURE( m_keyTags.empty() || m_keyTags.size() == m_uiTags.size() );
}

CFlagTags::CFlagTags( const FlagDef flagDefs[], unsigned int count, const std::tstring& uiTags /*= std::tstring()*/ )
{
	ASSERT( count < MaxBits );

	std::vector<std::tstring> srcUiTags;
	str::Split( srcUiTags, uiTags.c_str(), m_listSep );
	ASSERT( srcUiTags.empty() || srcUiTags.size() == count );

	m_keyTags.resize( MaxBits );
	m_uiTags.resize( MaxBits );

	int maxPos = 0;
	for ( unsigned int i = 0; i != count; ++i )
	{
		int pos = FindBitPos( flagDefs[ i ].m_flag );
		m_keyTags[ pos ] = flagDefs[ i ].m_pKeyTag;
		m_uiTags[ pos ] = !srcUiTags.empty() ? srcUiTags[ i ] : m_keyTags[ pos ];
		maxPos = std::max<int>( pos, maxPos );
	}

	// remove undefined bit flags
	m_keyTags.erase( m_keyTags.begin() + maxPos + 1, m_keyTags.end() );
	m_uiTags.erase( m_uiTags.begin() + maxPos + 1, m_uiTags.end() );

	ENSURE( !m_uiTags.empty() );
	ENSURE( m_keyTags.empty() || m_keyTags.size() == m_uiTags.size() );
}

CFlagTags::~CFlagTags()
{
}

int CFlagTags::GetFlagsMask( void ) const
{
	REQUIRE( m_uiTags.size() < MaxBits );		// avoid overflow

	unsigned int mask = 0;
	for ( size_t pos = 0; pos != m_uiTags.size(); ++pos )
		if ( !m_uiTags[ pos ].empty() )			// flag is defined if tag is not empty
			mask |= ( 1 << pos );

	return static_cast<int>( mask );
}

std::tstring CFlagTags::Format( int flags, const std::vector<std::tstring>& tags, const TCHAR* pSep )
{
	ASSERT( !tags.empty() );

	std::vector<std::tstring> flagsOn; flagsOn.reserve( tags.size() );

	for ( size_t pos = 0; pos != tags.size(); ++pos )
		if ( !tags[ pos ].empty() )					// flag is defined
			if ( HasBitFlag( flags, static_cast<int>( pos ) ) )
				flagsOn.push_back( tags[ pos ] );

	return str::Join( flagsOn, pSep );
}

void CFlagTags::Parse( int* pFlags, const std::tstring& text, const std::vector<std::tstring>& tags, const TCHAR* pSep, str::CaseType caseType )
{
	ASSERT( !tags.empty() );
	ASSERT_PTR( pFlags );

	std::vector<std::tstring> flagsOn;
	if ( !str::IsEmpty( pSep ) )
		str::Split( flagsOn, text.c_str(), pSep );
	else
	{	// assume tag is single character
		ASSERT( text.length() <= tags.size() );
		flagsOn.reserve( text.length() );

		for ( std::tstring::const_iterator itCh = text.begin(); itCh != text.end(); ++itCh )
			flagsOn.push_back( std::tstring( 1, *itCh ) );
	}

	// preserve unknown bits: set each known flag individually
	for ( size_t pos = 0; pos != tags.size(); ++pos )
		if ( !tags[ pos ].empty() )					// flag is defined
			SetBitFlag( *pFlags, static_cast<int>( pos ), Contains( flagsOn, tags[ pos ], caseType ) );
}

bool CFlagTags::Contains( const std::vector<std::tstring>& strings, const std::tstring& value, str::CaseType caseType )
{
	for ( std::vector<std::tstring>::const_iterator itString = strings.begin(); itString != strings.end(); ++itString )
		if ( str::EqualsN_ByCase( caseType, itString->c_str(), value.c_str(), utl::npos ) )
			return true;

	return false;
}

int CFlagTags::FindBitPos( unsigned int flag )
{	// assuming flag it's a single bit
	ASSERT( flag != 0 );
	int pos = 0;				// aka power of 2 exponent
	for ( ; !( flag & 1 ); flag >>= 1 )
		++pos;
	return pos;
}

const std::tstring& CFlagTags::LookupTag( TagType tag, int flag ) const
{
	int pos = LookupBitPos( flag );
	return KeyTag == tag ? m_keyTags[ pos ] : m_uiTags[ pos ];
}

int CFlagTags::FindFlag( TagType tag, const std::tstring& flagOn ) const
{
	const std::vector<std::tstring>& tags = KeyTag == tag ? m_keyTags : m_uiTags;

	for ( size_t i = 0; i != tags.size(); ++i )
		if ( str::EqualString<str::IgnoreCase>( tags[ i ], flagOn ) )
			return ToBitFlag( static_cast<int>( i ) );

	return -1;
}
