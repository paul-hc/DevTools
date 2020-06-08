#ifndef ImageTests_h
#define ImageTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "utl/test/UnitTest.h"


class CImageTests : public ut::CGraphicTestCase
{
	CImageTests( void );
public:
	static CImageTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestGroupIcon( void );
	void TestIcon( void );
	void TestImageList( void );
};


#endif //_DEBUG


#endif // ImageTests_h
