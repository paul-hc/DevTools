#ifndef UndoLogSerializerTests_h
#define UndoLogSerializerTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "utl/ut/UnitTest.h"


class CUndoLogSerializerTests : public ut::CConsoleTestCase
{
	CUndoLogSerializerTests( void );
public:
	static CUndoLogSerializerTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestLoadLog( void );
	void TestSaveLog( void );
};


#endif //_DEBUG


#endif // UndoLogSerializerTests_h
