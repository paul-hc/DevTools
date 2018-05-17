#ifndef UndoChangeLogTests_h
#define UndoChangeLogTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "utl/ut/UnitTest.h"


class CUndoChangeLogTests : public ut::CConsoleTestCase
{
	CUndoChangeLogTests( void );
public:
	static CUndoChangeLogTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestLoadLog( void );
	void TestSaveLog( void );
};


#endif //_DEBUG


#endif // UndoChangeLogTests_h
