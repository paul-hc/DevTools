#ifndef Path_hxx
#define Path_hxx
#pragma once

#include "Algorithms.hxx"
#include "Path.h"


namespace path
{
	template< typename ContainerT, typename SrcContainerT >
	size_t JoinUniquePaths( ContainerT& rDestPaths, const SrcContainerT& newPaths )
	{
		size_t oldCount = rDestPaths.size();

		// multiple items optimization: add all and use hash unquifying
		rDestPaths.insert( rDestPaths.end(), newPaths.begin(), newPaths.end() );
		utl::Uniquify( rDestPaths );

		return rDestPaths.size() - oldCount;		// added count
	}
}


#endif // Path_hxx
