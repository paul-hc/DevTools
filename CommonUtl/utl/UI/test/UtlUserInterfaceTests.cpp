
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "UtlUserInterfaceTests.h"
#include "ColorTests.h"
#include "ResourceTests.h"
#include "SerializationTests.h"
#include "ShellPidlTests.h"
#include "ShortcutTests.h"
#include "ShellFileSystemTests.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ut
{
	void RegisterUtlUserInterfaceTests( void )
	{
		// register UTL UI console tests
		CShortcutTests::Instance();
		CColorTests::Instance();
		CResourceTests::Instance();
		CSerializationTests::Instance();
		CShellPidlTests::Instance();
		CShellFileSystemTests::Instance();
	}
}


#endif //USE_UT
