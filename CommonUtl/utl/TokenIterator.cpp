
#include "stdafx.h"
#include "TokenIterator.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace str
{
	void f( const std::tstring& text )
	{
		for ( str::CTokenIterator<> it( text ); !it.AtEnd(); )
			if ( it.Matches( _T('x') ) )
			{
			}
			else if ( it.Matches( _T("tag") ) )
			{
			}
			else
				++it;
	}

	void g( const std::string& text )
	{
		for ( str::CTokenIterator< pred::TCompareNoCase, char > it( text ); !it.AtEnd(); )
			if ( it.Matches( 'x' ) )
			{
			}
			else if ( it.Matches( "tag" ) )
			{
			}
			else
				++it;
	}
}
