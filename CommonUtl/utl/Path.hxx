#ifndef Path_hxx
#define Path_hxx
#pragma once

#include <unordered_set>


namespace path
{
	template< typename ContainerT >
	inline size_t UniquifyPaths( ContainerT& rPaths )
	{
		std::unordered_set< typename ContainerT::value_type > uniquePathIndex;
		return UniquifyPaths( rPaths, uniquePathIndex );
	}
}


#endif // Path_hxx
