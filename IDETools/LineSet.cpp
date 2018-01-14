
#include "stdafx.h"
#include "LineSet.h"
#include "utl/ComparePredicates.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const TCHAR CLineSet::m_winLineEnd[] = _T("\r\n");
const TCHAR CLineSet::m_unixLineEnd[] = _T("\n");


void CLineSet::ParseText( const std::tstring& text, const TCHAR* pLineEnd /*= m_winLineEnd*/ )
{
	str::Split( m_lines, text.c_str(), pLineEnd );
	m_lastEmptyLine = !m_lines.empty() && m_lines.back().empty();
	if ( m_lastEmptyLine )
		m_lines.pop_back();
}

std::tstring CLineSet::FormatText( const TCHAR* pLineEnd /*= m_winLineEnd*/ ) const
{
	ASSERT_PTR( pLineEnd );

	std::tstring outputText = str::Join( m_lines, pLineEnd );
	if ( m_lastEmptyLine )
		outputText += pLineEnd;
	return outputText;
}

void CLineSet::TrimLines( const TCHAR* pWhiteSpace /*= NULL*/ )
{
	for ( std::vector< std::tstring >::iterator itLine = m_lines.begin(); itLine != m_lines.end(); ++itLine )
		str::Trim( *itLine, pWhiteSpace );
}

void CLineSet::RemoveDuplicateLines( str::CaseType caseType )
{
	std::vector< std::tstring > sourceLines;
	sourceLines.swap( m_lines );
	m_lines.reserve( sourceLines.size() );

	for ( std::vector< std::tstring >::const_iterator itLine = sourceLines.begin(); itLine != sourceLines.end(); ++itLine )
		if ( std::find_if( m_lines.begin(), m_lines.end(), pred::EqualString< std::tstring >( *itLine, caseType ) ) == m_lines.end() &&
			 !( m_lastEmptyLine && itLine->empty() ) )
			m_lines.push_back( *itLine );
}

void CLineSet::Reverse( void )
{
	std::reverse( m_lines.begin(), m_lines.end() );
}

void CLineSet::SortAscending( void )
{
	std::stable_sort( m_lines.begin(), m_lines.end(), pred::LessBy< pred::CompareValue >() );
}

void CLineSet::SortDescending( void )
{
	std::stable_sort( m_lines.rbegin(), m_lines.rend(), pred::LessBy< pred::CompareValue >() );		// sort descending
}
