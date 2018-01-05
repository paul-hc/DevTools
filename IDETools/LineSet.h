#ifndef LineSet_h
#define LineSet_h
#pragma once


// preserves last empty line, if present

struct CLineSet
{
	CLineSet( void ) : m_lastEmptyLine( false ) {}
	CLineSet( const std::tstring& text, const TCHAR* pLineEnd = m_winLineEnd ) { ParseText( text, pLineEnd ); }

	void ParseText( const std::tstring& text, const TCHAR* pLineEnd = m_winLineEnd );
	std::tstring FormatText( const TCHAR* pLineEnd = m_winLineEnd ) const;

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

	static const TCHAR m_winLineEnd[];
	static const TCHAR m_unixLineEnd[];
};


#endif // LineSet_h
