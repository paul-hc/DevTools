
#include "stdafx.h"
#include "CodeParser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


bool CCodeParser::FindArgumentList( Range<TIterator>* pOutArgList, TIterator itCode, TIterator itLast ) const
{
	ASSERT_PTR( pOutArgList );

	static const std::tstring s_oper = _T("operator");
	std::tstring::const_iterator it = std::search( itCode, itLast, s_oper.begin(), s_oper.end() );

	if ( it != itLast )
		// skip "operator()"
		if ( it = m_lang.FindNextCharThat( it + s_oper.length(), itLast, pred::IsChar( '(' ) ), it != itLast )
			if ( m_lang.SkipPastMatchingBrace( &it, itLast ) )
				if ( FindArgumentList( pOutArgList, it, itLast ) )
					return true;

	if ( it = m_lang.FindNextCharThat( itCode, itLast, pred::IsChar( '(' ) ), it != itLast )
	{
		pOutArgList->SetEmptyRange( it );

		if ( m_lang.SkipPastMatchingBrace( &pOutArgList->m_end, itLast ), it != itLast )
			return true;
	}

	return false;
}
