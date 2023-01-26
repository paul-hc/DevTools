#ifndef ImagePathKey_h
#define ImagePathKey_h
#pragma once

#include "FlexPath.h"
#include "StdHashValue.h"


namespace fs
{
	typedef std::pair<fs::CFlexPath, UINT> TImagePathKey;		// <path, frame pos>
}


template<>
struct std::hash<fs::TImagePathKey>
{
	inline std::size_t operator()( const fs::TImagePathKey& filePath ) const /*noexcept*/
    {
        return utl::GetPairHashValue( filePath );
    }
};


#endif // ImagePathKey_h
