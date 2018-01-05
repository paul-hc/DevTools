
#include "stdafx.h"
#include "FileObjectCache.h"
#include "FlagTags.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace fs
{
	namespace cache
	{
		const CFlagTags& GetTags_StatusFlags( void )
		{
			static const CFlagTags::FlagDef flagDefs[] =
			{
				{ CacheHit, _T("") },				// silent on cache hits (not the interesting case)
				{ RemoveExpired, _T("(-) remove expired") },
				{ Remove, _T("(-) remove") },
				{ Load, _T("(++) loaded") },
				{ LoadingError, _T("(++) loading error") }
			};
			static const CFlagTags tags( flagDefs, COUNT_OF( flagDefs ) );
			return tags;
		}
	}
}
