#ifndef CatalogStorageTests_h
#define CatalogStorageTests_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "utl/test/UnitTest.h"


class CFileAttr;
interface ICatalogStorage;


class CCatalogStorageTests : public ut::CConsoleTestCase
{
	CCatalogStorageTests( void );
public:
	static CCatalogStorageTests& Instance( void );

	// ut::ITestCase interface
	virtual void Run( void );
private:
	void TestBuildImageArchive( void );
	void TestAlbumSaveAs( void );

	void _TestLoadImageArchive( const fs::TStgDocPath& docStgPath, size_t srcImageCount ) throws_( CException* );
	void _TestAlbumFileAttr( ICatalogStorage* pCatalogStorage, const CFileAttr* pFileAttr ) throws_( CException* );
};


#endif //USE_UT


#endif // CatalogStorageTests_h
