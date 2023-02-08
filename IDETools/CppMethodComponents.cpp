
#include "stdafx.h"
#include "CppMethodComponents.h"
#include "CodeUtilities.h"
#include "BraceParityStatus.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace code
{
	CppMethodComponents::CppMethodComponents( const TCHAR* pMethodPrototype )
		: m_languageEngine( DocLang_Cpp )
		, m_pMethodPrototype( pMethodPrototype )
		, m_methodLength( str::Length( m_pMethodPrototype ) )
		, m_templateDecl( m_methodLength )
		, m_inlineModifier( m_methodLength )
		, m_returnType( m_methodLength )
		, m_methodName( m_methodLength )
		, m_typeQualifier( m_methodLength )
		, m_argList( m_methodLength )
		, m_postArgListSuffix( m_methodLength )
	{
		ASSERT_PTR( m_pMethodPrototype );
	}

	/**
		Finds a method declaration/definition components:
		[m_templateDecl][m_inlineModifier] m_returnType m_methodName [m_typeQualifier] m_argList [m_postArgListSuffix]

	*/
	void CppMethodComponents::splitMethod( const TCHAR validArgListOpenBraces[] /*= _T("(")*/ )
	{
		BraceParityStatus braceStatus;
		m_argList = braceStatus.findArgList( m_pMethodPrototype, 0, _T("("), DocLang_Cpp, true );

		//* m_argList *//
		if ( 2 == m_argList.getLength() )
		{	// handle the case: "bool operator()( const string& str )"
			TokenRange nextArgList = braceStatus.findArgList( m_pMethodPrototype, m_argList.m_end, _T("("), DocLang_Cpp );

			if ( !nextArgList.IsEmpty() )
				m_argList = nextArgList;
		}

		//* m_postArgListSuffix *//
		m_postArgListSuffix.m_start = m_argList.m_end;
		m_postArgListSuffix.m_end = m_languageEngine.findString( m_pMethodPrototype, code::lineEnd, m_postArgListSuffix.m_start ).m_start;

		//* m_methodName *//
		m_methodName.m_end = m_argList.m_start;
		while ( m_methodName.m_end > 0 && _istspace( m_pMethodPrototype[ m_methodName.m_end - 1 ] ) )
			--m_methodName.m_end;

		m_methodName.m_start = m_methodName.m_end;

		TokenRange operatorRange = m_languageEngine.findString( m_pMethodPrototype, _T("operator") );

		if ( !operatorRange.IsEmpty() &&
			 operatorRange.m_end < m_argList.m_start &&
			 !_istalnum( m_pMethodPrototype[ operatorRange.m_end ] ) )
		{
			m_methodName.m_start = operatorRange.m_start;
		}

		for ( ;; )
		{
			while ( m_methodName.m_start > 0 && !_istspace( m_pMethodPrototype[ m_methodName.m_start - 1 ] ) )
				if ( '>' == m_pMethodPrototype[ m_methodName.m_start ] )		// closing class template instance brace?
				{	// bug fix: capture entire template instance => skip backwards to the matching '<' opening brace
					if ( !code::ide_tools::SkipBraceBackwards( &m_methodName.m_start, m_pMethodPrototype, m_methodName.m_start ) )
						TRACE( _T(" ? CppMethodComponents::splitMethod() - Bad syntax in source code: no reverse matching brace at pos=%d in:\n  code='%s'\n"),
							   m_methodName.m_start, m_pMethodPrototype );

					--m_methodName.m_start;		// fishy syntax in m_pMethodPrototype, just ignore
				}
				else
					--m_methodName.m_start;

			if ( isValidBraceChar( m_pMethodPrototype[ m_methodName.m_start ], validArgListOpenBraces ) )
			{
				int openBracePos = BraceParityStatus().reverseFindMatchingBracePos( m_pMethodPrototype, m_methodName.m_start, DocLang_Cpp );

				if ( openBracePos != -1 )
					m_methodName.m_start = openBracePos;
			}
			else
				break;
		}

		//* m_returnType *//
		m_returnType.assign( 0, m_methodName.m_start );
		// skip trailing whitespaces
		while ( m_returnType.m_end > 0 && _istspace( m_pMethodPrototype[ m_returnType.m_end - 1 ] ) )
			--m_returnType.m_end;
		// skip leading whitespaces
		while ( _istspace( m_pMethodPrototype[ m_returnType.m_start ] ) )
			++m_returnType.m_start;

		//* m_typeQualifier *//
		static const TCHAR* scopeAccessOperator = _T("::");
		static int scopeAccessOpLength = str::Length( scopeAccessOperator );

		m_typeQualifier.setEmpty( m_methodName.m_start );

		const TCHAR* lastScopeAccess = std::find_end( m_pMethodPrototype + m_methodName.m_start,
													  m_pMethodPrototype + m_methodName.m_end,
													  scopeAccessOperator,
													  scopeAccessOperator + scopeAccessOpLength );

		if ( lastScopeAccess != m_pMethodPrototype + m_methodName.m_end )
			// Found the last "::" -> extract the type qualifier
			m_typeQualifier.m_end = int( lastScopeAccess - m_pMethodPrototype ) + scopeAccessOpLength;

		//* m_templateDecl *//
		m_templateDecl.setEmpty( m_returnType.m_start );
		if ( str::isTokenMatch( m_pMethodPrototype, _T("template"), m_templateDecl.m_start ) )
		{
			TokenRange templTypeList = BraceParityStatus().findArgList( m_pMethodPrototype, m_templateDecl.m_start, _T("<"),
																		DocLang_Cpp );

			if ( !templTypeList.IsEmpty() && templTypeList.m_end <= m_returnType.m_end )
			{
				m_templateDecl.m_end = templTypeList.m_end;
				m_returnType.m_start = m_templateDecl.m_end;
				while ( _istspace( m_pMethodPrototype[ m_returnType.m_start ] ) )
					++m_returnType.m_start;
			}
		}

		//* m_inlineModifier *//
		m_inlineModifier.setEmpty( m_returnType.m_start );
		if ( str::isTokenMatch( m_pMethodPrototype, _T("inline"), m_inlineModifier.m_start ) )
		{
			m_inlineModifier.setLength( str::Length( _T("inline") ) );
			m_returnType.m_start = m_inlineModifier.m_end;
			while ( _istspace( m_pMethodPrototype[ m_returnType.m_start ] ) )
				++m_returnType.m_start;
		}

		TRACE( _T("%s"), FormatInfo().c_str() );
	}

	std::tstring CppMethodComponents::FormatInfo( void ) const
	{
		return str::Format(
			_T("#CppMethodComponents - Prototype components for:\n%s\ntemplateDecl='%s'\ninlineModifier='%s'\nreturnType='%s'\n")
			_T("m_methodName='%s'\ntypeQualifier='%s'\nargList='%s'\npostArgListSuffix='%s'\n"),
			m_pMethodPrototype,
			(LPCTSTR)m_templateDecl.getString( m_pMethodPrototype ),
			(LPCTSTR)m_inlineModifier.getString( m_pMethodPrototype ),
			(LPCTSTR)m_returnType.getString( m_pMethodPrototype ),
			(LPCTSTR)m_methodName.getString( m_pMethodPrototype ),
			(LPCTSTR)m_typeQualifier.getString( m_pMethodPrototype ),
			(LPCTSTR)m_argList.getString( m_pMethodPrototype ),
			(LPCTSTR)m_postArgListSuffix.getString( m_pMethodPrototype )
		);
	}

	void CppMethodComponents::showMessageBox( void ) const
	{
		AfxMessageBox( FormatInfo().c_str() );
	}

} // namespace code
