
#include "stdafx.h"

#ifdef _DEBUG		// no UT code in release builds
#include "test/MockObject.h"
#include "ContainerUtilities.h"
#include "StringUtilities.h"

#define new DEBUG_NEW


namespace ut
{
	int CMockObject::s_instanceCount = 0;
}


#endif //_DEBUG
