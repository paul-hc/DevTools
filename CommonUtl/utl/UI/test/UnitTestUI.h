#ifndef UnitTestUI_h
#define UnitTestUI_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "utl/test/UnitTest.h"
#include "utl/Path_fwd.h"
#include <shobjidl_core.h>		// for PCIDLIST_ABSOLUTE, IContextMenu


class CValueTags;


namespace ut
{
	const CValueTags& GetTags_SIGDN( void );

	void TracePidlNames( PCIDLIST_ABSOLUTE pidl );
	int TrackContextMenu( IContextMenu* pCtxMenu );

	// convert SIGDN_DESKTOPABSOLUTEPARSING to SIGDN_DESKTOPABSOLUTEEDITING (slow)
	std::tstring ParsingToEditingName( const shell::TPath& shellPath );
	void QueryEditingNames( OUT std::vector<std::tstring>& rEditingNames, const std::vector<shell::TPath>& shellPaths, const std::tstring& stripCommonName = str::GetEmpty() );

	std::tstring JoinEditingNames( const std::vector<shell::TPath>& shellPaths, const std::tstring& stripCommonName = str::GetEmpty(), const TCHAR* pSep = _T("\n") );

	template< typename StrVectorT >		// TCharPtr/std::tstring/fs::CPath
	void TraceItems( const StrVectorT& items )
	{
		TRACE( "\nTracing %d items:\n", items.size() );
		for ( size_t i = 0; i != items.size(); ++i )
		{
			TRACE( _T( "\t%s\n" ), str::traits::GetCharPtr( items[i] ) );
		}
	}

	template< typename StrVectorT >		// TCharPtr/std::tstring/fs::CPath
	void TraceItemsPos( const StrVectorT& items )
	{
		TRACE( "\nTracing %d items:\n", items.size() );
		for ( size_t i = 0; i != items.size(); ++i )
		{
			TRACE( _T( "\t[%d]\t%s\n" ), i, str::traits::GetCharPtr( items[i] ) );
		}
	}
}


#endif //USE_UT


#endif // UnitTestUI_h
