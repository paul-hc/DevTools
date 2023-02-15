
#include "pch.h"
#include "MethodPrototype.h"
#include "CodeParser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace code
{
	void CMethodPrototype::Reset( const std::tstring& proto )
	{
		m_proto = proto;

		// set all ranges empty at end
		m_argList.setEmpty( m_proto.length() );
		m_postArgListSuffix = m_functionName = m_methodQualifiedName = m_typeQualifier = m_returnType = m_inlineModifier = m_templateDecl =
			m_argList;
	}

	/**
		Finds a method declaration/definition components:
		[m_templateDecl][m_inlineModifier] m_returnType m_methodQualifiedName [m_typeQualifier] m_argList [m_postArgListSuffix]

		std::pair<int, int> Func( const CFileItem* pLeft, int depth = 5 ) const
		std::pair<int, int> CPattern::Search( const CFileItem* pLeft, int depth = 5 ) const

		const TCHAR* operator( void ) const
		pred::CompareResult CComparator::operator!=( const CFileItem* pLeft, const CFileItem* pRight ) const

		template< typename PathType, typename ObjectType >
		std::pair<ObjectType*, cache::TStatusFlags> CCacheLoader<PathType, ObjectType>::Acquire( const PathType& pathKey ) const throws(std::exception, std::runtime_error)
	*/
	void CMethodPrototype::SplitMethod( const std::tstring& proto )
	{
		Reset( proto );

		typedef CLanguage<TCHAR> TLanguage;
		const CLanguage<TCHAR>& lang = code::GetCppLanguage<TCHAR>();
		CCodeParser parser( lang );

		{	// FORWARD iteration
			if ( parser.FindArgumentList( &m_argList, proto ) )
			{
				std::tstring::const_iterator itMethod = m_proto.begin(), itEnd = m_proto.end();
				Range<std::tstring::const_iterator> itRange( itMethod + m_argList.m_end );

				itRange.m_end = lang.FindNextSequence( itRange.m_start, itEnd, _T("\r\n") );

				m_postArgListSuffix = pvt::MakeTokenRange( itRange, itMethod );
			}
			else
				TRACE( _T("CMethodPrototype::SplitMethod(): cannot find method prototype in code text\n'%s'\n"), m_proto.c_str() );
		}

		static const std::tstring s_scope = _T("::");
		static const std::tstring s_operator = _T("operator");

		{	// REVERSE iteration
			typedef std::tstring::const_reverse_iterator const_reverse_iterator;

			const_reverse_iterator itEnd = m_proto.rend();
			const_reverse_iterator itArgList = utl::RevIterAtFwdPos( m_proto, m_argList.m_start );

			const_reverse_iterator it = lang.FindNextSequence( itArgList, itEnd, s_operator );

			if ( it != itEnd )
				it += s_operator.length();			// "operator!="
			else
				it = lang.FindNextCharThatNot( itArgList + 1, itEnd, pred::IsIdentifier() );	// "Acquire" or "func"

			const_reverse_iterator itAnchor = it;

			if ( str::EqualsSeq( it, itEnd, s_scope ) )
			{
				m_functionName = pvt::MakeFwdTokenRange( Range<const_reverse_iterator>( it, itArgList ), m_proto );
				it += s_scope.length();
			}

			if ( it != itEnd )
			{
				if ( '>' == *it )
					lang.SkipPastMatchingBrace( &it, itEnd );		// skip template instance "<...>" list

				lang.SkipIdentifier( &it, itEnd );	// "Acquire" or "func"
			}

			m_methodQualifiedName = pvt::MakeFwdTokenRange( Range<const_reverse_iterator>( it, itArgList ), m_proto );
			m_typeQualifier = pvt::MakeFwdTokenRange( Range<const_reverse_iterator>( it, itAnchor ), m_proto );

			itAnchor = ++it;		// rev-start of returnType

			if ( lang.SkipWhitespace( &it, itEnd ) )
				if ( '>' == *it )
					lang.SkipPastMatchingBrace( &it, itEnd );			// skip template instance "<...>" list

			it = lang.FindNextCharThat( it, itEnd, pred::IsSpace() );	// rev-end of returnType
			m_returnType = pvt::MakeFwdTokenRange( Range<const_reverse_iterator>( it, itAnchor ), m_proto );

			//m_returnType
		}
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
			<< _T("methodQualifiedName=") << s_quote << str::ExtractString( m_methodQualifiedName, m_proto ) << s_quote << std::endl
			<< _T("typeQualifier=") << s_quote << str::ExtractString( m_typeQualifier, m_proto ) << s_quote << std::endl
			<< _T("argList=") << s_quote << str::ExtractString( m_argList, m_proto ) << s_quote << std::endl
			<< _T("postArgListSuffix=") << s_quote << str::ExtractString( m_postArgListSuffix, m_proto ) << s_quote << std::endl;

		return oss.str();
	}

	void CMethodPrototype::ShowMessageBox( void ) const
	{
		AfxMessageBox( FormatInfo().c_str() );
	}

} // namespace code


#include "CodeUtilities.h"
#include "BraceParityStatus.h"


namespace code
{
	/** OLD CODE:
		Finds a method declaration/definition components:
		[m_templateDecl][m_inlineModifier] m_returnType m_methodQualifiedName [m_typeQualifier] m_argList [m_postArgListSuffix]

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

		//* m_methodQualifiedName *//
		m_methodQualifiedName.m_end = m_argList.m_start;
		while ( m_methodQualifiedName.m_end > 0 && _istspace( pPrototype[ m_methodQualifiedName.m_end - 1 ] ) )
			--m_methodQualifiedName.m_end;

		m_methodQualifiedName.m_start = m_methodQualifiedName.m_end;

		TokenRange operatorRange = m_languageEngine.findString( pPrototype, _T("operator") );

		if ( !operatorRange.IsEmpty() &&
			 operatorRange.m_end < m_argList.m_start &&
			 !_istalnum( pPrototype[ operatorRange.m_end ] ) )
		{
			m_methodQualifiedName.m_start = operatorRange.m_start;
		}

		for ( ;; )
		{
			while ( m_methodQualifiedName.m_start > 0 && !_istspace( pPrototype[ m_methodQualifiedName.m_start - 1 ] ) )
				if ( '>' == pPrototype[ m_methodQualifiedName.m_start ] )		// closing class template instance brace?
				{	// bug fix: capture entire template instance => skip backwards to the matching '<' opening brace
					if ( !code::ide_tools::SkipBraceBackwards( &m_methodQualifiedName.m_start, pPrototype, m_methodQualifiedName.m_start ) )
						TRACE( _T(" ? CppMethodComponents::splitMethod() - Bad syntax in source code: no reverse matching brace at pos=%d in:\n  code='%s'\n"),
							   m_methodQualifiedName.m_start, pPrototype );

					--m_methodQualifiedName.m_start;		// fishy syntax in pPrototype, just ignore
				}
				else
					--m_methodQualifiedName.m_start;

			if ( isValidBraceChar( pPrototype[ m_methodQualifiedName.m_start ], _T("(") ) )
			{
				int openBracePos = BraceParityStatus().reverseFindMatchingBracePos( pPrototype, m_methodQualifiedName.m_start, DocLang_Cpp );

				if ( openBracePos != -1 )
					m_methodQualifiedName.m_start = openBracePos;
			}
			else
				break;
		}

		//* m_returnType *//
		m_returnType.assign( 0, m_methodQualifiedName.m_start );
		// skip trailing whitespaces
		while ( m_returnType.m_end > 0 && _istspace( pPrototype[ m_returnType.m_end - 1 ] ) )
			--m_returnType.m_end;
		// skip leading whitespaces
		while ( _istspace( pPrototype[ m_returnType.m_start ] ) )
			++m_returnType.m_start;

		//* m_typeQualifier *//
		static const TCHAR* scopeAccessOperator = _T("::");
		static int scopeAccessOpLength = str::Length( scopeAccessOperator );

		m_typeQualifier.setEmpty( m_methodQualifiedName.m_start );

		const TCHAR* lastScopeAccess = std::find_end( pPrototype + m_methodQualifiedName.m_start,
													  pPrototype + m_methodQualifiedName.m_end,
													  scopeAccessOperator,
													  scopeAccessOperator + scopeAccessOpLength );

		if ( lastScopeAccess != pPrototype + m_methodQualifiedName.m_end )
			// Found the last "::" -> extract the type qualifier
			m_typeQualifier.m_end = int( lastScopeAccess - pPrototype ) + scopeAccessOpLength;

		//* m_functionName *
		if ( !m_methodQualifiedName.IsEmpty() )
			m_functionName.assign( m_typeQualifier.IsEmpty() ? m_methodQualifiedName.m_start : m_typeQualifier.m_end, m_methodQualifiedName.m_end );

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
