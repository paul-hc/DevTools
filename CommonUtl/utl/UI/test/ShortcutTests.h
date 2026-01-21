#ifndef ShortcutTests_h
#define ShortcutTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "utl/Path.h"
#include "utl/test/UnitTest.h"


class CShortcutTests : public ut::CConsoleTestCase
{
protected:
	CShortcutTests( void );
public:
	static CShortcutTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestValidLinks( void );
	void TestBrokenLinks( void );
	void TestSaveLinkFile( void );
	void TestCreateLinkFile( void );
	void TestFormatParseLink( void );

	void TestLinkProperties( void );
	void TestEnumLinkProperties( void );
public:
	enum TestLink { Dice_png, Dice_png_envVar, Region_CPL, ResourceMonitor_envVar, xDice_png_bad, xDice_png_envVar_bad, _LinkCount };

	static bool MakeTestLinkPath( OUT fs::CPath* pLinkPath, TestLink testLink );
	static CComPtr<IShellLink> LoadShellLink( TestLink testLink );
};


#endif //USE_UT


#endif // ShortcutTests_h
