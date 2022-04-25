
#include "stdafx.h"
#include "LineSet.h"
#include "utl/ComparePredicates.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CLineSet implementation

const TCHAR CLineSet::s_winLineEnd[] = _T("\r\n");
const TCHAR CLineSet::s_unixLineEnd[] = _T("\n");


void CLineSet::ParseText( const std::tstring& text, const TCHAR* pLineEnd /*= s_winLineEnd*/ )
{
	str::Split( m_lines, text.c_str(), pLineEnd );
	m_lastEmptyLine = !m_lines.empty() && m_lines.back().empty();

	if ( m_lastEmptyLine )
		m_lines.pop_back();
}

std::tstring CLineSet::FormatText( const TCHAR* pLineEnd /*= s_winLineEnd*/ ) const
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
		if ( std::find_if( m_lines.begin(), m_lines.end(), pred::EqualString<std::tstring>( *itLine, caseType ) ) == m_lines.end() &&
			 !( m_lastEmptyLine && itLine->empty() ) )
			m_lines.push_back( *itLine );
}

void CLineSet::Reverse( void )
{
	std::reverse( m_lines.begin(), m_lines.end() );
}

void CLineSet::SortAscending( void )
{
	std::stable_sort( m_lines.begin(), m_lines.end(), pred::LessValue< pred::CompareValue >() );
}

void CLineSet::SortDescending( void )
{
	std::stable_sort( m_lines.rbegin(), m_lines.rend(), pred::LessValue< pred::CompareValue >() );		// sort descending
}


// CTableSet implementation

const TCHAR CTableSet::s_columnSep[] = _T("\t");

CTableSet::CTableSet( str::CaseType caseType, const TCHAR lineSep[] /*= CLineSet::s_winLineEnd*/, const TCHAR columnSep[] /*= s_columnSep*/ )
	: m_caseType( caseType )
	, m_pLineSep( lineSep )
	, m_pColumnSep( columnSep )
	, m_lastEmptyLine( false )
{
	ASSERT( !str::IsEmpty( m_pLineSep ) && !str::IsEmpty( m_pColumnSep ) );
}

void CTableSet::ParseText( const std::tstring& text )
{
	CLineSet lines;
	lines.ParseText( text, m_pLineSep );
	lines.RemoveDuplicateLines( m_caseType );
	m_lastEmptyLine = lines.HasLastEmptyLine();

	m_rows.resize( lines.m_lines.size() );
	for ( size_t i = 0; i != lines.m_lines.size(); ++i )
		m_rows[ i ].Parse( lines.m_lines[ i ], m_pColumnSep );
}

std::tstring CTableSet::FormatText( void ) const
{
	std::tostringstream os;

	for ( size_t i = 0; i != m_rows.size(); ++i )
	{
		if ( i != 0 )
			os << m_pLineSep;

		m_rows[ i ].StreamOut( os, m_pColumnSep );
	}

	if ( m_lastEmptyLine )
		os << m_pLineSep;

	return os.str();
}

void CTableSet::ResetDuplicateColumns( void )
{
	for ( std::vector< CRow >::reverse_iterator itRow = m_rows.rbegin(), itEnd = m_rows.rend(); itRow != itEnd; )
	{
		std::vector< CRow >::reverse_iterator itNextRow = itRow + 1;		// NextRow means PreviousRow as we reverse iterate

		if ( itNextRow != itEnd )
			itRow->ResetEqualColumns( *itNextRow, m_caseType );

		itRow = itNextRow;
	}
}


// CTableSet::CRow implementation

void CTableSet::CRow::Parse( const std::tstring& rowText, const TCHAR columnSep[] )
{
	str::Split( m_columns, rowText.c_str(), columnSep );
}

std::tostream& CTableSet::CRow::StreamOut( std::tostream& os, const TCHAR columnSep[] ) const
{
	if ( !IsEmpty() )
		for ( size_t columnPos = 0; columnPos != m_columns.size(); ++columnPos )
		{
			if ( columnPos != 0 )
				os << columnSep;

			os << m_columns[ columnPos ];
		}

	return os;
}

bool CTableSet::CRow::EqualsColumns( const CRow& right, size_t columnPos, str::CaseType caseType ) const
{
	if ( HasColumn( columnPos ) != right.HasColumn( columnPos ) )
		return false;

	return HasColumn( columnPos ) && str::EqualString( caseType, GetColumn( columnPos ), right.GetColumn( columnPos ) );
}

void CTableSet::CRow::ResetEqualColumns( const CRow& prevRow, str::CaseType caseType )
{
	for ( size_t columnPos = 0; columnPos != m_columns.size(); ++columnPos )
		if ( EqualsColumns( prevRow, columnPos, caseType ) )
			m_columns[ columnPos ].clear();
}
