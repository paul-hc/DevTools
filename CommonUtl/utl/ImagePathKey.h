#ifndef ImagePathKey_h
#define ImagePathKey_h
#pragma once

#include "FlexPath.h"


namespace fs
{
	typedef std::pair< fs::CFlexPath, UINT > ImagePathKey;		// <path, frame pos>
}


#endif // ImagePathKey_h