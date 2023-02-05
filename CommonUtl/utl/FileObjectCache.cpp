
#include "pch.h"
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
			static const CFlagTags::FlagDef s_flagDefs[] =
			{
				{ CacheHit, _T("") },				// silent on cache hits (not the interesting case)
				{ RemoveExpired, _T("(-) remove expired") },
				{ Remove, _T("(-) remove") },
				{ Load, _T("(++) loaded") },
				{ LoadingError, _T("(++) loading error") }
			};
			static const CFlagTags s_tags( s_flagDefs, COUNT_OF( s_flagDefs ) );
			return s_tags;
		}
	}
}
