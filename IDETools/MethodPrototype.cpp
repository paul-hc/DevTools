
#include "stdafx.h"
#include "MethodPrototype.h"
//#include "CodeParser.h"
#include "CodeUtilities.h"
#include "BraceParityStatus.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace code
{
	void CMethodPrototype::Reset( const std::tstring& methodPrototype )
	{
		m_methodPrototype = methodPrototype;

		// set all ranges empty at end
		m_argList.setEmpty( m_methodPrototype.length() );
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
	void CMethodPrototype::SplitMethod( const std::tstring& methodPrototype )
	{
		m_methodPrototype = methodPrototype;

		const TCHAR* pMethodPrototype = m_methodPrototype.c_str();
		BraceParityStatus braceStatus;
		m_argList = braceStatus.findArgList( pMethodPrototype, 0, _T("("), DocLang_Cpp, true );

		//* m_argList *//
		if ( 2 == m_argList.getLength() )
		{	// handle the case: "bool operator()( const string& str )"
			TokenRange nextArgList = braceStatus.findArgList( pMethodPrototype, m_argList.m_end, _T("("), DocLang_Cpp );

			if ( !nextArgList.IsEmpty() )
				m_argList = nextArgList;
		}

		//* m_postArgListSuffix *//
		m_postArgListSuffix.m_start = m_argList.m_end;
		m_postArgListSuffix.m_end = m_languageEngine.findString( pMethodPrototype, code::lineEnd, m_postArgListSuffix.m_start ).m_start;

		//* m_methodQualifiedName *//
		m_methodQualifiedName.m_end = m_argList.m_start;
		while ( m_methodQualifiedName.m_end > 0 && _istspace( pMethodPrototype[ m_methodQualifiedName.m_end - 1 ] ) )
			--m_methodQualifiedName.m_end;

		m_methodQualifiedName.m_start = m_methodQualifiedName.m_end;

		TokenRange operatorRange = m_languageEngine.findString( pMethodPrototype, _T("operator") );

		if ( !operatorRange.IsEmpty() &&
			 operatorRange.m_end < m_argList.m_start &&
			 !_istalnum( pMethodPrototype[ operatorRange.m_end ] ) )
		{
			m_methodQualifiedName.m_start = operatorRange.m_start;
		}

		for ( ;; )
		{
			while ( m_methodQualifiedName.m_start > 0 && !_istspace( pMethodPrototype[ m_methodQualifiedName.m_start - 1 ] ) )
				if ( '>' == pMethodPrototype[ m_methodQualifiedName.m_start ] )		// closing class template instance brace?
				{	// bug fix: capture entire template instance => skip backwards to the matching '<' opening brace
					if ( !code::ide_tools::SkipBraceBackwards( &m_methodQualifiedName.m_start, pMethodPrototype, m_methodQualifiedName.m_start ) )
						TRACE( _T(" ? CppMethodComponents::splitMethod() - Bad syntax in source code: no reverse matching brace at pos=%d in:\n  code='%s'\n"),
							   m_methodQualifiedName.m_start, pMethodPrototype );

					--m_methodQualifiedName.m_start;		// fishy syntax in pMethodPrototype, just ignore
				}
				else
					--m_methodQualifiedName.m_start;

			if ( isValidBraceChar( pMethodPrototype[ m_methodQualifiedName.m_start ], _T("(") ) )
			{
				int openBracePos = BraceParityStatus().reverseFindMatchingBracePos( pMethodPrototype, m_methodQualifiedName.m_start, DocLang_Cpp );

				if ( openBracePos != -1 )
					m_methodQualifiedName.m_start = openBracePos;
			}
			else
				break;
		}

		//* m_returnType *//
		m_returnType.assign( 0, m_methodQualifiedName.m_start );
		// skip trailing whitespaces
		while ( m_returnType.m_end > 0 && _istspace( pMethodPrototype[ m_returnType.m_end - 1 ] ) )
			--m_returnType.m_end;
		// skip leading whitespaces
		while ( _istspace( pMethodPrototype[ m_returnType.m_start ] ) )
			++m_returnType.m_start;

		//* m_typeQualifier *//
		static const TCHAR* scopeAccessOperator = _T("::");
		static int scopeAccessOpLength = str::Length( scopeAccessOperator );

		m_typeQualifier.setEmpty( m_methodQualifiedName.m_start );

		const TCHAR* lastScopeAccess = std::find_end( pMethodPrototype + m_methodQualifiedName.m_start,
													  pMethodPrototype + m_methodQualifiedName.m_end,
													  scopeAccessOperator,
													  scopeAccessOperator + scopeAccessOpLength );

		if ( lastScopeAccess != pMethodPrototype + m_methodQualifiedName.m_end )
			// Found the last "::" -> extract the type qualifier
			m_typeQualifier.m_end = int( lastScopeAccess - pMethodPrototype ) + scopeAccessOpLength;

		//* m_functionName *
		if ( !m_methodQualifiedName.IsEmpty() )
			m_functionName.assign( m_typeQualifier.IsEmpty() ? m_methodQualifiedName.m_start : m_typeQualifier.m_end, m_methodQualifiedName.m_end );

		//* m_templateDecl *//
		m_templateDecl.setEmpty( m_returnType.m_start );
		if ( str::isTokenMatch( pMethodPrototype, _T("template"), m_templateDecl.m_start ) )
		{
			TokenRange templTypeList = BraceParityStatus().findArgList( pMethodPrototype, m_templateDecl.m_start, _T("<"),
																		DocLang_Cpp );

			if ( !templTypeList.IsEmpty() && templTypeList.m_end <= m_returnType.m_end )
			{
				m_templateDecl.m_end = templTypeList.m_end;
				m_returnType.m_start = m_templateDecl.m_end;
				while ( _istspace( pMethodPrototype[ m_returnType.m_start ] ) )
					++m_returnType.m_start;
			}
		}

		//* m_inlineModifier *//
		m_inlineModifier.setEmpty( m_returnType.m_start );
		if ( str::isTokenMatch( pMethodPrototype, _T("inline"), m_inlineModifier.m_start ) )
		{
			m_inlineModifier.setLength( str::Length( _T("inline") ) );
			m_returnType.m_start = m_inlineModifier.m_end;
			while ( _istspace( pMethodPrototype[ m_returnType.m_start ] ) )
				++m_returnType.m_start;
		}

		TRACE( _T("%s"), FormatInfo().c_str() );
	}

	std::tstring CMethodPrototype::FormatInfo( void ) const
	{
		const TCHAR* pMethodPrototype = m_methodPrototype.c_str();

		return str::Format(
			_T("#CMethodPrototype - Prototype components for:\n%s\ntemplateDecl='%s'\ninlineModifier='%s'\nreturnType='%s'\n")
			_T("m_methodQualifiedName='%s'\ntypeQualifier='%s'\nargList='%s'\npostArgListSuffix='%s'\n"),
			pMethodPrototype,
			(LPCTSTR)m_templateDecl.getString( pMethodPrototype ),
			(LPCTSTR)m_inlineModifier.getString( pMethodPrototype ),
			(LPCTSTR)m_returnType.getString( pMethodPrototype ),
			(LPCTSTR)m_methodQualifiedName.getString( pMethodPrototype ),
			(LPCTSTR)m_typeQualifier.getString( pMethodPrototype ),
			(LPCTSTR)m_argList.getString( pMethodPrototype ),
			(LPCTSTR)m_postArgListSuffix.getString( pMethodPrototype )
		);
	}

	void CMethodPrototype::ShowMessageBox( void ) const
	{
		AfxMessageBox( FormatInfo().c_str() );
	}

} // namespace code
