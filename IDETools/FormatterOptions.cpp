
#include "stdafx.h"
#include "FormatterOptions.h"
#include "CodeUtilities.h"
#include "ModuleSession.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	TCHAR section_formatting[ 64 ];
	TCHAR section_braces[ 64 ];
	TCHAR section_operators[ 64 ];

	const TCHAR entry_breakSeparators[] = _T("Break separators");
	const TCHAR entry_preserveMultipleWhiteSpace[] = _T("Preserve multiple whitespace");
	const TCHAR entry_deleteTrailingWhiteSpace[] = _T("Delete trailing whitespace");
	const TCHAR entry_linesBetweenFunctionImpls[] = _T("Lines between function impls");
	const TCHAR entry_returnTypeOnSeparateLine[] = _T("Return type on separate line");
	const TCHAR entry_commentOutDefaultParams[] = _T("Comment-out default params");
}


namespace code
{
	CFormatterOptions::CFormatterOptions( void )
		: m_breakSeparators()
		, m_preserveMultipleWhiteSpace( false )
		, m_deleteTrailingWhiteSpace( true )
		, m_linesBetweenFunctionImpls( 1 )
		, m_returnTypeOnSeparateLine( false )
		, m_commentOutDefaultParams( true )
	{
		if ( str::IsEmpty( reg::section_formatting ) )
		{
			str::Copy( reg::section_formatting, std::tstring( reg::section_settings ) + _T("\\Formatting") );
			str::Copy( reg::section_braces, std::tstring( reg::section_formatting ) + _T("\\Braces") );
			str::Copy( reg::section_operators, std::tstring( reg::section_formatting ) + _T("\\Operators") );
		}

		m_breakSeparators.reserve( 5 );
		m_breakSeparators.push_back( _T(",") );
		m_breakSeparators.push_back( _T(";") );
		m_breakSeparators.push_back( _T("&&") );
		m_breakSeparators.push_back( _T("||") );

		// brace rules
		m_braceRules.reserve( 4 );
		m_braceRules.push_back( CBraceRule( _T('('), _T(')'), InsertOneSpace, true ) );
		m_braceRules.push_back( CBraceRule( _T('<'), _T('>'), InsertOneSpace, true ) );
		m_braceRules.push_back( CBraceRule( _T('['), _T(']'), InsertOneSpace ) );
		m_braceRules.push_back( CBraceRule( _T('{'), _T('}'), InsertOneSpace ) );

		// operator rules
		m_operatorRules.reserve( 25 );
		m_operatorRules.push_back( COperatorRule( _T(","), RemoveSpace, InsertOneSpace ) );
		m_operatorRules.push_back( COperatorRule( _T(";"), RemoveSpace, InsertOneSpace ) );
		m_operatorRules.push_back( COperatorRule( _T("."), RemoveSpace, RemoveSpace ) );
		m_operatorRules.push_back( COperatorRule( _T("->"), RemoveSpace, RemoveSpace ) );
		m_operatorRules.push_back( COperatorRule( _T("::"), PreserveSpace, RemoveSpace ) );
		m_operatorRules.push_back( COperatorRule( _T(":"), PreserveSpace, PreserveSpace ) );	// order is important, must come after ::
		m_operatorRules.push_back( COperatorRule( _T("=="), InsertOneSpace, InsertOneSpace ) );
		m_operatorRules.push_back( COperatorRule( _T("!="), InsertOneSpace, InsertOneSpace ) );
		m_operatorRules.push_back( COperatorRule( _T("="), InsertOneSpace, InsertOneSpace ) );		// order is important, must come after ==, !=
		m_operatorRules.push_back( COperatorRule( _T("+="), InsertOneSpace, InsertOneSpace ) );
		m_operatorRules.push_back( COperatorRule( _T("-="), InsertOneSpace, InsertOneSpace ) );
		m_operatorRules.push_back( COperatorRule( _T("&&"), InsertOneSpace, InsertOneSpace ) );
		m_operatorRules.push_back( COperatorRule( _T("||"), InsertOneSpace, InsertOneSpace ) );
		m_operatorRules.push_back( COperatorRule( _T("|"), InsertOneSpace, InsertOneSpace ) );
		m_operatorRules.push_back( COperatorRule( _T("<="), InsertOneSpace, InsertOneSpace ) );
		m_operatorRules.push_back( COperatorRule( _T(">="), InsertOneSpace, InsertOneSpace ) );
		m_operatorRules.push_back( COperatorRule( _T("<<"), InsertOneSpace, InsertOneSpace ) );
		m_operatorRules.push_back( COperatorRule( _T(">>"), InsertOneSpace, InsertOneSpace ) );
		m_operatorRules.push_back( COperatorRule( _T("<"), InsertOneSpace, InsertOneSpace ) );		// order is important, must come after <<
		m_operatorRules.push_back( COperatorRule( _T(">"), InsertOneSpace, InsertOneSpace ) );		// order is important, must come after >
	}

	CFormatterOptions::~CFormatterOptions()
	{
	}

	static const TCHAR slashSep[] = _T("/");

	void CFormatterOptions::LoadFromRegistry( void )
	{
		m_preserveMultipleWhiteSpace = AfxGetApp()->GetProfileInt( reg::section_formatting, reg::entry_preserveMultipleWhiteSpace, m_preserveMultipleWhiteSpace ) != FALSE;
		m_deleteTrailingWhiteSpace = AfxGetApp()->GetProfileInt( reg::section_formatting, reg::entry_deleteTrailingWhiteSpace, m_deleteTrailingWhiteSpace ) != FALSE;
		m_linesBetweenFunctionImpls = AfxGetApp()->GetProfileInt( reg::section_formatting, reg::entry_linesBetweenFunctionImpls, m_linesBetweenFunctionImpls );
		m_returnTypeOnSeparateLine = AfxGetApp()->GetProfileInt( reg::section_formatting, reg::entry_returnTypeOnSeparateLine, m_returnTypeOnSeparateLine ) != FALSE;
		m_commentOutDefaultParams = AfxGetApp()->GetProfileInt( reg::section_formatting, reg::entry_commentOutDefaultParams, m_commentOutDefaultParams ) != FALSE;

		str::Split( m_breakSeparators, (LPCTSTR)AfxGetApp()->GetProfileString( reg::section_formatting, reg::entry_breakSeparators, str::Join( m_breakSeparators, slashSep ).c_str() ), slashSep );

		for ( std::vector< CBraceRule >::iterator itBrace = m_braceRules.begin(); itBrace != m_braceRules.end(); ++itBrace )
			( *itBrace ).LoadFromRegistry();

		for ( std::vector< COperatorRule >::iterator itOpRule = m_operatorRules.begin(); itOpRule != m_operatorRules.end(); ++itOpRule )
			( *itOpRule ).LoadFromRegistry();
	}

	void CFormatterOptions::SaveToRegistry( void ) const
	{
		AfxGetApp()->WriteProfileString( reg::section_formatting, reg::entry_breakSeparators, str::Join( m_breakSeparators, slashSep ).c_str() );
		AfxGetApp()->WriteProfileInt( reg::section_formatting, reg::entry_preserveMultipleWhiteSpace, m_preserveMultipleWhiteSpace );
		AfxGetApp()->WriteProfileInt( reg::section_formatting, reg::entry_deleteTrailingWhiteSpace, m_deleteTrailingWhiteSpace );
		AfxGetApp()->WriteProfileInt( reg::section_formatting, reg::entry_linesBetweenFunctionImpls, m_linesBetweenFunctionImpls );
		AfxGetApp()->WriteProfileInt( reg::section_formatting, reg::entry_returnTypeOnSeparateLine, m_returnTypeOnSeparateLine );
		AfxGetApp()->WriteProfileInt( reg::section_formatting, reg::entry_commentOutDefaultParams, m_commentOutDefaultParams );

		for ( std::vector< CBraceRule >::const_iterator itBrace = m_braceRules.begin(); itBrace != m_braceRules.end(); ++itBrace )
			( *itBrace ).SaveToRegistry();

		for ( std::vector< COperatorRule >::const_iterator itOpRule = m_operatorRules.begin(); itOpRule != m_operatorRules.end(); ++itOpRule )
			( *itOpRule ).SaveToRegistry();
	}

	CFormatterOptions::CBraceRule* CFormatterOptions::FindBraceRule( TCHAR chr ) const
	{
		for ( std::vector< CBraceRule >::const_iterator itBrace = m_braceRules.begin(); itBrace != m_braceRules.end(); ++itBrace )
			if ( chr == itBrace->m_braceOpen || chr == itBrace->m_braceClose )
				return const_cast<CBraceRule*>( &*itBrace );

		return NULL;
	}

	TokenSpacing CFormatterOptions::MustSpaceBrace( TCHAR chr ) const
	{
		CBraceRule* brace = FindBraceRule( chr );

		return brace != NULL ? brace->m_spacing : PreserveSpace;
	}

	bool CFormatterOptions::IsArgListBrace( TCHAR chr ) const
	{
		const CBraceRule* pBrace = FindBraceRule( chr );
		return pBrace != NULL && pBrace->m_isArgList;
	}

	CFormatterOptions::COperatorRule* CFormatterOptions::FindOperatorRule( const TCHAR* pOpStart ) const
	{
		for ( std::vector< COperatorRule >::const_iterator itOpRule = m_operatorRules.begin(); itOpRule != m_operatorRules.end(); ++itOpRule )
			if ( 0 == _tcsnicmp( itOpRule->m_pOperator, pOpStart, _tcslen( itOpRule->m_pOperator ) ) )
				return const_cast<COperatorRule*>( &*itOpRule );

		return NULL;
	}

	const std::tstring* CFormatterOptions::FindBreakSeparator( const TCHAR* pBreakSepStart ) const
	{
		for ( std::vector< std::tstring >::const_iterator itBreakSep = m_breakSeparators.begin(); itBreakSep != m_breakSeparators.end(); ++itBreakSep )
			if ( 0 == _tcsnicmp( itBreakSep->c_str(), pBreakSepStart, itBreakSep->length() ) )
				return &*itBreakSep;

		return NULL;
	}

	std::tstring CFormatterOptions::GetArgListOpenBraces( void ) const
	{
		std::tstring argListBraces;
		argListBraces.reserve( m_braceRules.size() );
		for ( std::vector< CBraceRule >::const_iterator itBrace = m_braceRules.begin(); itBrace != m_braceRules.end(); ++itBrace )
			if ( itBrace->m_isArgList )
				argListBraces += itBrace->m_braceOpen;

		return argListBraces;
	}

	TokenSpacing CFormatterOptions::SpacingFromChar( TCHAR chr )
	{
		switch ( chr )
		{
			case _T('0'):
				return RemoveSpace;
			case _T('1'):
				return InsertOneSpace;
			default:
				ASSERT( false );
			case _T('-'):
				return PreserveSpace;
		}
	}

	TCHAR CFormatterOptions::SpacingToChar( TokenSpacing spacing )
	{
		switch ( spacing )
		{
			case RemoveSpace:
				return _T('0');
			case InsertOneSpace:
				return _T('1');
			default:
				ASSERT( false );
			case PreserveSpace:
				return _T('-');
		}
	}


	// CFormatterOptions::CBraceRule implementation

	void CFormatterOptions::CBraceRule::LoadFromRegistry( void )
	{
		std::tstring value = (LPCTSTR)AfxGetApp()->GetProfileString( reg::section_braces, m_regEntry.c_str() );
		if ( 2 == value.length() )
		{
			m_spacing = CFormatterOptions::SpacingFromChar( value[ 0 ] );
			m_isArgList = ( value[ 1 ] != _T('0') );
		}
	}

	void CFormatterOptions::CBraceRule::SaveToRegistry( void ) const
	{
		TCHAR value[ 3 ] = { CFormatterOptions::SpacingToChar( m_spacing ), m_isArgList ? _T('1') : _T('0'), _T('\0') };
		AfxGetApp()->WriteProfileString( reg::section_braces, m_regEntry.c_str(), value );
	}


	// CFormatterOptions::COperatorRule implementation

	void CFormatterOptions::COperatorRule::LoadFromRegistry( void )
	{
		std::tstring value = (LPCTSTR)AfxGetApp()->GetProfileString( reg::section_operators, m_regEntry.c_str() );
		if ( 2 == value.length() )
		{
			m_spaceBefore = CFormatterOptions::SpacingFromChar( value[ 0 ] );
			m_spaceAfter = CFormatterOptions::SpacingFromChar( value[ 1 ] );
		}
	}

	void CFormatterOptions::COperatorRule::SaveToRegistry( void ) const
	{
		TCHAR value[ 3 ] = { CFormatterOptions::SpacingToChar( m_spaceBefore ), CFormatterOptions::SpacingToChar( m_spaceAfter ), _T('\0') };
		AfxGetApp()->WriteProfileString( reg::section_operators, m_regEntry.c_str(), value );
	}

} //namespace code
