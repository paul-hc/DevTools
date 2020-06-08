
#include "stdafx.h"

#ifdef _DEBUG		// no UT code in release builds
#include "UtlUserInterfaceTests.h"
#include "ColorTests.h"
#include "SerializationTests.h"
#include "ShellFileSystemTests.h"

#define new DEBUG_NEW


namespace ut
{
	void RegisterUtlUserInterfaceTests( void )
	{
		// register UTL tests
		CColorTests::Instance();
		CSerializationTests::Instance();
		CShellFileSystemTests::Instance();
	}
}


#endif //_DEBUG
