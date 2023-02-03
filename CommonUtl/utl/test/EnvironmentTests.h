#ifndef EnvironmentTests_h
#define EnvironmentTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "UnitTest.h"


class CEnvironmentTests : public ut::CConsoleTestCase
{
	CEnvironmentTests( void );
public:
	static CEnvironmentTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestFindEnclosedIdentifier( void );
	void TestQueryEnclosedItems( void );
	void TestReplaceEnvVar_VcMacroToWindows( void );
	void TestEnvironVariables( void );
	void TestExpandEnvironment( void );
	void TestExpandKeysToValues( void );
};


#endif //USE_UT


#endif // EnvironmentTests_h
