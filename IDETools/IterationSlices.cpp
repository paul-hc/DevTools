
#include "pch.h"
#include "IterationSlices.h"
#include "CppParser.h"
#include "utl/Language.h"
#include <set>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/Language.hxx"


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

	void CIterationSlices::ParseCode( const std::tstring& codeText ) throws_( CRuntimeException )
	{
		Reset( codeText );

		static const std::tstring s_const = _T("const");
		CCppCodeParser codeParser( &m_codeText );

		int pos = 0;

		codeParser.SkipWhitespace( &pos );
		m_leadingWhiteSpace.m_start = 0;
		m_leadingWhiteSpace.m_end = pos;
		m_isConst = codeParser.SkipMatchingToken( &pos, s_const );

		m_containerType.m_start = m_containerType.m_end = pos;

		while ( m_pCodeText[ pos ] != '\0' )
			if ( '<' == m_pCodeText[ pos ] )
			{
				int bracketEndPos = codeParser.FindPosMatchingBracket( pos );

				if ( -1 == bracketEndPos )
					throw CRuntimeException( str::Format( _T("Syntax error: cannot find ending template brace for statement '%s'"), m_pCodeText ), UTL_FILE_LINE );

				m_valueType.SetRange( ++pos, bracketEndPos );
				m_valueType.Trim( codeText );

				m_containerType.m_end = pos = bracketEndPos + 1;
				break;
			}
			else if ( str::IsAnyOf( m_pCodeText[ pos ], _T("*& \t\r\n") ) )
			{
				m_containerType.m_end = pos;
				break;
			}
			else
				++pos;

		if ( m_containerType.IsEmpty() )
			throw CRuntimeException( str::Format( _T("Syntax error: cannot find container type in statement '%s'"), m_pCodeText ), UTL_FILE_LINE );

		codeParser.SkipWhitespace( &pos );

		if ( '*' == m_pCodeText[ pos ] )
			m_pObjSelOp = _T("->");				// use pointer selector

		codeParser.SkipAnyOf( &pos, _T("*&") );
		codeParser.SkipWhitespace( &pos );
		m_isConst |= codeParser.SkipMatchingToken( &pos, s_const );

		m_containerName.m_start = m_containerName.m_end = pos;
		codeParser.SkipAnyNotOf( &m_containerName.m_end, _T(",; \t\r\n") );

		if ( m_containerName.IsEmpty() )
			throw CRuntimeException( str::Format( _T("Syntax error: cannot find container variable in statement '%s'"), m_pCodeText ), UTL_FILE_LINE );

		ExtractIteratorName();

		std::tstring containerType = code::ExtractIdentifier( m_containerType.GetToken( m_pCodeText ), 0 );

		m_isMfcList = IsMfcList( containerType );
		m_libraryType = IsMfcContainer( containerType ) ? MFC : STL;

		//Trace();
	}

	bool CIterationSlices::IsMfcList( const std::tstring& containerType )
	{
		static std::set<std::tstring> s_mfcListTypes;
		if ( s_mfcListTypes.empty() )
			str::SplitSet( s_mfcListTypes, _T("CList|CStringList|CObList|CTypedPtrList|CPtrList"), _T("|") );

		return s_mfcListTypes.find( containerType ) != s_mfcListTypes.end();
	}

	bool CIterationSlices::IsMfcContainer( const std::tstring& containerType )
	{
		static std::set<std::tstring> s_mfcCollTypes;
		if ( s_mfcCollTypes.empty() )
		{
			static const TCHAR s_types[] =
				_T("CArray|CTypedPtrArray|CTypedPtrMap|CObArray|CByteArray|CDWordArray|CPtrArray|CStringArray|CWordArray|CUIntArray|")
				_T("CMap|CMapPtrToWord|CMapPtrToPtr|CMapStringToOb|CMapStringToPtr|CMapStringToString|CMapWordToOb|CMapWordToPtr");

			str::SplitSet( s_mfcCollTypes, s_types, _T("|") );
		}

		return IsMfcList( containerType ) || s_mfcCollTypes.find( containerType ) != s_mfcCollTypes.end();
	}

	/**
		sample container statement: "rObject.GetParent()->GetTags<TCHAR>()"
	*/
	void CIterationSlices::ExtractIteratorName( void )
	{
		m_iteratorName.clear();

		std::tstring containerName = m_containerName.MakeToken( m_codeText );
		std::tstring::const_reverse_iterator it = containerName.rbegin(), itEnd = containerName.rend();
		const CLanguage<TCHAR>& lang = GetLangCpp<TCHAR>();

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
			CCppCodeParser parser( &core );
			TokenRange coreRange( 0, static_cast<int>( core.length() ) );
			const TCHAR* pCore = core.c_str();

			if ( 'p' == core[ coreRange.m_start ] || 'r' == core[ coreRange.m_start ] )
				if ( core.length() > 1 && pred::IsUpper()( core[ coreRange.m_start + 1 ] ) )
					++coreRange.m_start;

			{
				using namespace str::ignore_case;

				parser.SkipMatchingToken( &coreRange.m_start, _T("Get") );
				parser.SkipMatchingToken( &coreRange.m_start, _T("Ref") );
			}

			if ( 's' == core[ coreRange.m_end - 1 ] )
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
