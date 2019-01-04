#ifndef UtlUserInterfaceTests_h
#define UtlUserInterfaceTests_h
#pragma once

#include "ColorTests.h"
#include "SerializationTests.h"
#include "ShellFileSystemTests.h"


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


#endif // UtlUserInterfaceTests_h
