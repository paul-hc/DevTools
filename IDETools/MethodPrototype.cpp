
#include "pch.h"
#include "MethodPrototype.h"
#include "CppCodeParser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace code
{
	void CMethodPrototype::Reset( const std::tstring& proto )
	{
		m_proto = proto;

		// set all ranges empty at end
		m_templateDecl.setEmpty( m_proto.length() );
		m_argList = m_inlineModifier = m_returnType = m_qualifiedMethod = m_functionName = m_classQualifier = m_postArgListSuffix =
			m_templateDecl;
	}

	/**
		Finds a method declaration/definition components:
		[m_templateDecl][m_inlineModifier] m_returnType m_qualifiedMethod [m_classQualifier] m_argList [m_postArgListSuffix]

		template< typename PathType, typename ObjectType >
		std::pair<ObjectType*, cache::TStatusFlags> CCacheLoader<PathType, ObjectType>::Acquire( const PathType& pathKey ) const throws(std::exception, std::runtime_error)

		Check-out CMethodPrototypeTests for code cases covered.
	*/
	void CMethodPrototype::SplitMethod( const std::tstring& proto )
	{
		Reset( proto );

		CCppCodeParser parser;

		parser.ParseCode( proto );		// split into code slices

		m_templateDecl = parser.LookupSliceRange( CCppCodeParser::TemplateDecl );
		m_inlineModifier = parser.LookupSliceRange( CCppCodeParser::InlineModifier );
		m_returnType = parser.LookupSliceRange( CCppCodeParser::ReturnType );
		m_qualifiedMethod = parser.LookupSliceRange( CCppCodeParser::QualifiedMethod );
		m_functionName = parser.LookupSliceRange( CCppCodeParser::FunctionName );
		m_classQualifier = parser.LookupSliceRange( CCppCodeParser::ClassQualifier );
		m_argList = parser.LookupSliceRange( CCppCodeParser::ArgList );
		m_postArgListSuffix = parser.LookupSliceRange( CCppCodeParser::PostArgListSuffix );
	}

	std::tstring CMethodPrototype::FormatInfo( void ) const
	{
		static const TCHAR s_quote = _T('\'');
		std::tostringstream oss;

		oss
			<< _T("# CMethodPrototype - Prototype components for:") << std::endl
			<< m_proto << std::endl
			<< _T("templateDecl=") << s_quote << str::ExtractString( m_templateDecl, m_proto ) << s_quote << std::endl
			<< _T("inlineModifier=") << s_quote << str::ExtractString( m_inlineModifier, m_proto ) << s_quote << std::endl
			<< _T("returnType=") << s_quote << str::ExtractString( m_returnType, m_proto ) << s_quote << std::endl
			<< _T("functionName=") << s_quote << str::ExtractString( m_functionName, m_proto ) << s_quote << std::endl
			<< _T("qualifiedMethod=") << s_quote << str::ExtractString( m_qualifiedMethod, m_proto ) << s_quote << std::endl
			<< _T("classQualifier=") << s_quote << str::ExtractString( m_classQualifier, m_proto ) << s_quote << std::endl
			<< _T("argList=") << s_quote << str::ExtractString( m_argList, m_proto ) << s_quote << std::endl
			<< _T("postArgListSuffix=") << s_quote << str::ExtractString( m_postArgListSuffix, m_proto ) << s_quote << std::endl;

		return oss.str();
	}

} // namespace code


#include "CodeUtilities.h"
#include "BraceParityStatus.h"


namespace code
{
	/** OLD CODE:
		Finds a method declaration/definition components:
		[m_templateDecl][m_inlineModifier] m_returnType m_qualifiedMethod [m_classQualifier] m_argList [m_postArgListSuffix]

		std::pair<int, int> Func( const CFileItem* pLeft, int depth = 5 ) const
		std::pair<int, int> CPattern::Search( const CFileItem* pLeft, int depth = 5 ) const

		const TCHAR* operator( void ) const
		pred::CompareResult CComparator::operator!=( const CFileItem* pLeft, const CFileItem* pRight ) const

		template< typename PathType, typename ObjectType >
		std::pair<ObjectType*, cache::TStatusFlags> CCacheLoader<PathType, ObjectType>::Acquire( const PathType& pathKey ) const throws(std::exception, std::runtime_error)
	*/
	void CMethodPrototypeOld::SplitMethod( const std::tstring& proto )
	{
		Reset( proto );

		const TCHAR* pPrototype = m_proto.c_str();
		BraceParityStatus braceStatus;
		m_argList = braceStatus.findArgList( pPrototype, 0, _T("("), DocLang_Cpp, true );

		//* m_argList *//
		if ( 2 == m_argList.getLength() )
		{	// handle the case: "bool operator()( const string& str )"
			TokenRange nextArgList = braceStatus.findArgList( pPrototype, m_argList.m_end, _T("("), DocLang_Cpp );

			if ( !nextArgList.IsEmpty() )
				m_argList = nextArgList;
		}

		//* m_postArgListSuffix *//
		m_postArgListSuffix.m_start = m_argList.m_end;
		m_postArgListSuffix.m_end = m_languageEngine.findString( pPrototype, code::lineEnd, m_postArgListSuffix.m_start ).m_start;

		//* m_qualifiedMethod *//
		m_qualifiedMethod.m_end = m_argList.m_start;
		while ( m_qualifiedMethod.m_end > 0 && _istspace( pPrototype[ m_qualifiedMethod.m_end - 1 ] ) )
			--m_qualifiedMethod.m_end;

		m_qualifiedMethod.m_start = m_qualifiedMethod.m_end;

		TokenRange operatorRange = m_languageEngine.findString( pPrototype, _T("operator") );

		if ( !operatorRange.IsEmpty() &&
			 operatorRange.m_end < m_argList.m_start &&
			 !_istalnum( pPrototype[ operatorRange.m_end ] ) )
		{
			m_qualifiedMethod.m_start = operatorRange.m_start;
		}

		for ( ;; )
		{
			while ( m_qualifiedMethod.m_start > 0 && !_istspace( pPrototype[ m_qualifiedMethod.m_start - 1 ] ) )
				if ( '>' == pPrototype[ m_qualifiedMethod.m_start ] )		// closing class template instance brace?
				{	// bug fix: capture entire template instance => skip backwards to the matching '<' opening brace
					if ( !code::ide_tools::SkipBraceBackwards( &m_qualifiedMethod.m_start, pPrototype, m_qualifiedMethod.m_start ) )
						TRACE( _T(" ? CppMethodComponents::splitMethod() - Bad syntax in source code: no reverse matching brace at pos=%d in:\n  code='%s'\n"),
							   m_qualifiedMethod.m_start, pPrototype );

					--m_qualifiedMethod.m_start;		// fishy syntax in pPrototype, just ignore
				}
				else
					--m_qualifiedMethod.m_start;

			if ( isValidBraceChar( pPrototype[ m_qualifiedMethod.m_start ], _T("(") ) )
			{
				int openBracePos = BraceParityStatus().reverseFindMatchingBracePos( pPrototype, m_qualifiedMethod.m_start, DocLang_Cpp );

				if ( openBracePos != -1 )
					m_qualifiedMethod.m_start = openBracePos;
			}
			else
				break;
		}

		//* m_returnType *//
		m_returnType.assign( 0, m_qualifiedMethod.m_start );
		// skip trailing whitespaces
		while ( m_returnType.m_end > 0 && _istspace( pPrototype[ m_returnType.m_end - 1 ] ) )
			--m_returnType.m_end;
		// skip leading whitespaces
		while ( _istspace( pPrototype[ m_returnType.m_start ] ) )
			++m_returnType.m_start;

		//* m_classQualifier *//
		static const TCHAR* scopeAccessOperator = _T("::");
		static int scopeAccessOpLength = str::Length( scopeAccessOperator );

		m_classQualifier.setEmpty( m_qualifiedMethod.m_start );

		const TCHAR* lastScopeAccess = std::find_end( pPrototype + m_qualifiedMethod.m_start,
													  pPrototype + m_qualifiedMethod.m_end,
													  scopeAccessOperator,
													  scopeAccessOperator + scopeAccessOpLength );

		if ( lastScopeAccess != pPrototype + m_qualifiedMethod.m_end )
			// Found the last "::" -> extract the type qualifier
			m_classQualifier.m_end = int( lastScopeAccess - pPrototype ) + scopeAccessOpLength;

		//* m_functionName *
		if ( !m_qualifiedMethod.IsEmpty() )
			m_functionName.assign( m_classQualifier.IsEmpty() ? m_qualifiedMethod.m_start : m_classQualifier.m_end, m_qualifiedMethod.m_end );

		//* m_templateDecl *//
		m_templateDecl.setEmpty( m_returnType.m_start );
		if ( str::isTokenMatch( pPrototype, _T("template"), m_templateDecl.m_start ) )
		{
			TokenRange templTypeList = BraceParityStatus().findArgList( pPrototype, m_templateDecl.m_start, _T("<"),
																		DocLang_Cpp );

			if ( !templTypeList.IsEmpty() && templTypeList.m_end <= m_returnType.m_end )
			{
				m_templateDecl.m_end = templTypeList.m_end;
				m_returnType.m_start = m_templateDecl.m_end;
				while ( _istspace( pPrototype[ m_returnType.m_start ] ) )
					++m_returnType.m_start;
			}
		}

		//* m_inlineModifier *//
		m_inlineModifier.setEmpty( m_returnType.m_start );
		if ( str::isTokenMatch( pPrototype, _T("inline"), m_inlineModifier.m_start ) )
		{
			m_inlineModifier.setLength( str::Length( _T("inline") ) );
			m_returnType.m_start = m_inlineModifier.m_end;
			while ( _istspace( pPrototype[ m_returnType.m_start ] ) )
				++m_returnType.m_start;
		}

		TRACE( _T("%s\n"), FormatInfo().c_str() );
	}
}
