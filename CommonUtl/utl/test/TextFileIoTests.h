#ifndef TextFileIoTests_h
#define TextFileIoTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "UnitTest.h"


namespace ut { class CTempFilePool; }


class CTextFileIoTests : public ut::CConsoleTestCase
{
	CTextFileIoTests( void );
public:
	static CTextFileIoTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestByteOrderMark( void );
	void TestTextFileWithEncoding( void );
	void TestFileBom_NarrowIo( void );
	void TestFileBom_WideIo( void );
	void TestFileIo( void );
};


#endif //_DEBUG


#endif // TextFileIoTests_h
