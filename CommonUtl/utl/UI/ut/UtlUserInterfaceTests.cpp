
#include "stdafx.h"
#include "UtlUserInterfaceTests.h"
#include "ColorTests.h"
#include "SerializationTests.h"
#include "ShellFileSystemTests.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ut
{
	void RegisterUtlUserInterfaceTests( void )
	{
	#ifdef _DEBUG
		// register UTL tests
		CColorTests::Instance();
		CSerializationTests::Instance();
		CShellFileSystemTests::Instance();
	#endif
	}
}
