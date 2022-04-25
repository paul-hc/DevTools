#ifndef LineSet_h
#define LineSet_h
#pragma once


// preserves last empty line, if present

struct CLineSet
{
	CLineSet( void ) : m_lastEmptyLine( false ) {}
	CLineSet( const std::tstring& text, const TCHAR* pLineEnd = s_winLineEnd ) { ParseText( text, pLineEnd ); }

	void ParseText( const std::tstring& text, const TCHAR* pLineEnd = s_winLineEnd );
	std::tstring FormatText( const TCHAR* pLineEnd = s_winLineEnd ) const;

	bool HasLastEmptyLine( void ) const { return m_lastEmptyLine; }
	void RemoveLastEmptyLine( void ) { m_lastEmptyLine = false; }

	void TrimLines( const TCHAR* pWhiteSpace = NULL );
	void RemoveEmptyLines( void ) { str::RemoveEmptyItems( m_lines ); }
	void RemoveDuplicateLines( str::CaseType caseType );

	void Reverse( void );
	void SortAscending( void );
	void SortDescending( void );
private:
	bool m_lastEmptyLine;
public:
	std::vector< std::tstring > m_lines;

	static const TCHAR s_winLineEnd[];
	static const TCHAR s_unixLineEnd[];
};


class CTableSet
{
public:
	CTableSet( str::CaseType caseType, const TCHAR lineSep[] = CLineSet::s_winLineEnd, const TCHAR columnSep[] = s_columnSep );

	void ParseText( const std::tstring& text );		// + remove duplicate rows
	std::tstring FormatText( void ) const;

	void ResetDuplicateColumns( void );		// in reverse order, so that only one unique value is present
private:
	class CRow
	{
	public:
		CRow( void ) {}

		void Parse( const std::tstring& rowText, const TCHAR columnSep[] );
		std::tostream& StreamOut( std::tostream& os, const TCHAR columnSep[] ) const;

		bool IsEmpty( void ) const { return m_columns.empty(); }
		bool HasColumn( size_t columnPos ) const { return columnPos < m_columns.size(); }
		const std::tstring& GetColumn( size_t columnPos ) const { return HasColumn( columnPos ) ? m_columns[ columnPos ] : str::GetEmpty(); }

		bool EqualsColumns( const CRow& right, size_t columnPos, str::CaseType caseType ) const;
		void ResetEqualColumns( const CRow& prevRow, str::CaseType caseType );
	private:
		std::vector< std::tstring > m_columns;
	};
private:
	str::CaseType m_caseType;
	const TCHAR* m_pLineSep;
	const TCHAR* m_pColumnSep;

	std::vector< CRow > m_rows;
	bool m_lastEmptyLine;

	static const TCHAR s_columnSep[];
};


#endif // LineSet_h
