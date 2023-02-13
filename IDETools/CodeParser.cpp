
#include "pch.h"
#include "CodeParser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


bool CCodeParser::FindArgumentList( TokenRange* pOutArgList, const std::tstring& proto, size_t offset /*= 0*/ ) const
{
	ASSERT_PTR( pOutArgList );

	std::tstring::const_iterator itProto = proto.begin() + offset, itEnd = proto.end();

	static const std::tstring s_oper = _T("operator");
	std::tstring::const_iterator it = std::search( itProto, itEnd, s_oper.begin(), s_oper.end() );

	if ( it != itEnd )
		// skip "operator()"
		if ( it = m_lang.FindNextCharThat( it + s_oper.length(), itEnd, pred::IsChar( '(' ) ), it != itEnd )
			if ( m_lang.SkipPastMatchingBrace( &it, itEnd ) )
				if ( FindArgumentList( pOutArgList, proto, std::distance( itProto, it ) ) )
					return true;

	if ( it = m_lang.FindNextCharThat( itProto, itEnd, pred::IsChar( '(' ) ), it != itEnd )
	{
		pOutArgList->SetEmptyRange( pvt::Distance( proto.begin(), it ) );

		if ( m_lang.SkipPastMatchingBrace( &it, itEnd ), it != itEnd )
		{
			pOutArgList->m_end = pvt::Distance( proto.begin(), it );
			return true;
		}
	}

	return false;
}
