#ifndef ImagePathKey_h
#define ImagePathKey_h
#pragma once

#include "FlexPath.h"


namespace fs
{
	typedef std::pair<fs::CFlexPath, UINT> TImagePathKey;		// <path, frame pos>
}


#endif // ImagePathKey_h
