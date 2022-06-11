
#include "stdafx.h"

#ifdef USE_UT		// no UT code in release builds
#include "test/MockObject.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ut
{
	int CMockObject::s_instanceCount = 0;
}


#endif //USE_UT
