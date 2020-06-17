#ifndef ImageArchiveStgTests_h
#define ImageArchiveStgTests_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds

#include "utl/test/UnitTest.h"


class CFileAttr;
interface IImageArchiveStg;


class CImageArchiveStgTests : public ut::CConsoleTestCase
{
	CImageArchiveStgTests( void );
public:
	static CImageArchiveStgTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestBuildImageArchive( void );
	void _TestLoadImageArchive( const fs::CPath& docStgPath, size_t srcImageCount ) throws_( CException* );
	void _TestAlbumFileAttr( IImageArchiveStg* pImageStorage, const CFileAttr* pFileAttr ) throws_( CException* );
};


#endif //_DEBUG


#endif // ImageArchiveStgTests_h
