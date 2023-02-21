
#include "pch.h"
#include "IterationSlices.h"
#include "CodeUtilities.h"
#include "utl/CodeParsing.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace code
{
	CIterationSlices::CIterationSlices( void )
	{
		Reset( std::tstring() );
	}

	void CIterationSlices::Reset( const std::tstring& codeText )
	{
		m_codeText = codeText;
		m_pCodeText = m_codeText.c_str();

		// set all ranges empty at end
		m_leadingWhiteSpace.SetEmptyRange( static_cast<int>( m_codeText.length() ) );
		m_containerType = m_containerName = m_valueType =
			m_leadingWhiteSpace;

		m_isConst = m_isMfcList = false;
		m_pObjSelOp = _T(".");
		m_libraryType = STL;
	}

	void CIterationSlices::ParseStatement( const std::tstring& codeText ) throws_( mfc::CRuntimeException )
	{
		Reset( codeText );

		int pos = 0;

		str::skipWhiteSpace( pos, m_pCodeText );
		m_leadingWhiteSpace.m_start = 0;
		m_leadingWhiteSpace.m_end = pos;
		m_isConst = str::skipToken( pos, m_pCodeText, _T("const") );

		m_containerType.m_start = m_containerType.m_end = pos;

		while ( m_pCodeText[ pos ] != _T('\0') )
			if ( '<' == m_pCodeText[ pos ] )
			{
				int bracketEndPos = code::FindPosMatchingBracket( pos, codeText );

				if ( -1 == bracketEndPos )
					throw new mfc::CRuntimeException( str::Format( _T("Syntax error: cannot find ending template brace for statement '%s'"), m_pCodeText ) );

				m_valueType.SetRange( ++pos, bracketEndPos );
				m_valueType.Trim( codeText );

				m_containerType.m_end = pos = bracketEndPos + 1;
				break;
			}
			else if ( str::isCharOneOf( m_pCodeText[ pos ], _T("*& \t\r\n") ) )
			{
				m_containerType.m_end = pos;
				break;
			}
			else
				++pos;

		if ( m_containerType.IsEmpty() )
			throw new mfc::CRuntimeException( str::Format( _T("Syntax error: cannot find container type in statement '%s'"), m_pCodeText ) );

		str::skipWhiteSpace( pos, m_pCodeText );

		if ( m_pCodeText[ pos ] == _T('*') )
			m_pObjSelOp = _T("->");	// use pointer selector

		str::skipCharSet( pos, m_pCodeText, _T("*&") );
		str::skipWhiteSpace( pos, m_pCodeText );
		m_isConst |= str::skipToken( pos, m_pCodeText, _T("const") );

		m_containerName.m_start = m_containerName.m_end = pos;
		str::skipNotCharSet( m_containerName.m_end, m_pCodeText, _T(",; \t\r\n") );

		if ( m_containerName.IsEmpty() )
			throw new mfc::CRuntimeException( str::Format( _T("Syntax error: cannot find container variable in statement '%s'"), m_pCodeText ) );

		ExtractIteratorName();

		CString containerType = m_containerType.getString( m_pCodeText );

		m_isMfcList = containerType.Find( _T("List") ) != -1;

		if ( containerType.Find( _T("Array") ) != -1 ||
			 containerType.Find( _T("List") ) != -1 ||
			 containerType.Find( _T("Map") ) != -1 )
			m_libraryType = MFC;
		else
			m_libraryType = STL;

		Trace();
	}

	/**
		sample container statement: "rObject.GetParent()->GetTags<TCHAR>()"
	*/
	void CIterationSlices::ExtractIteratorName( void )
	{
		m_iteratorName.clear();

		std::tstring containerName = m_containerName.MakeToken( m_codeText );
		std::tstring::const_reverse_iterator it = containerName.rbegin(), itEnd = containerName.rend();
		const CLanguage<TCHAR>& lang = GetCppLang<TCHAR>();

		if ( it != itEnd && code::IsBracket( *it ) )
			lang.SkipPastMatchingBracket( &it, itEnd );		// skip "()"

		if ( it != itEnd && code::IsBracket( *it ) )		// "<TCHAR>"
			lang.SkipPastMatchingBracket( &it, itEnd );

		if ( it != itEnd )
			lang.SkipWhitespace( &it, itEnd );

		Range<std::tstring::const_reverse_iterator> itRange( it );
		std::tstring core;

		if ( pred::IsIdentifier()( *itRange.m_start ) )
			if ( lang.SkipIdentifier( &itRange.m_end, itEnd ) )
				core = str::ExtractString( itRange );		// "GetTags"

		if ( core.length() > 1 )
		{
			TokenRange coreRange( 0, core.length() );
			const TCHAR* pCore = core.c_str();

			if ( 'p' == pCore[ coreRange.m_start ] || 'r' == pCore[ coreRange.m_start ] )
				if ( pred::IsUpper()( pCore[ coreRange.m_start + 1 ] ) )
					++coreRange.m_start;

			str::skipToken( coreRange.m_start, pCore, _T("Ref"), str::IgnoreCase );
			str::skipToken( coreRange.m_start, pCore, _T("Get"), str::IgnoreCase );

			if ( pCore[ coreRange.m_end - 1 ] == _T('s') )
				--coreRange.m_end;

			if ( coreRange.IsNonEmpty() )
			{
				m_iteratorName = coreRange.GetToken( pCore );
				m_iteratorName[0] = str::CharTraits::ToUpper( m_iteratorName[0] );		// capitalize first letter
			}
		}
	}

	void CIterationSlices::Trace( void )
	{
	#ifdef _DEBUG
		std::tstring message = str::Format(
			_T("Container components for:\n'%s'\n")
			_T("m_containerType='%s'\n")
			_T("m_containerName='%s'\n")
			_T("m_valueType='%s'\n")
			_T("m_iteratorName='%s'"),
			m_pCodeText,
			(LPCTSTR)m_containerType.getString( m_pCodeText ),
			(LPCTSTR)m_valueType.getString( m_pCodeText ),
			(LPCTSTR)m_containerName.getString( m_pCodeText ),
			m_iteratorName.c_str() );

		TRACE( _T("# CIterationSlices::Trace(): %s\n"), message.c_str() );
	#endif
	}

} //namespace code
