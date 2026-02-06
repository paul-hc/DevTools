
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
				FLAG_TAG_KEY( CacheHit, "" ),				// silent on cache hits (not the interesting case)
				FLAG_TAG_KEY( RemoveExpired, "(-) remove expired" ),
				FLAG_TAG_KEY( Remove, "(-) remove" ),
				FLAG_TAG_KEY( Load, "(++) loaded" ),
				FLAG_TAG_KEY( LoadingError, "(++) loading error" )
			};
			static const CFlagTags s_tags( s_flagDefs, COUNT_OF( s_flagDefs ) );
			return s_tags;
		}
	}
}
