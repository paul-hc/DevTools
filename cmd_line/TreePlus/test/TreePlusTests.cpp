
#include "stdafx.h"

#ifdef USE_UT		// no UT code in release builds

#include "TreePlusTests.h"
#include "Application.h"
#include "CmdLineOptions.h"
#include "utl/StringUtilities.h"
#include "utl/TextFileIo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ut
{
	static const wchar_t s_flatPaths[] = L"\
FileA Arşiţă şi pârjol.log|\
FileB Σέρτζιο Ματαρέλα.log|\
FileC Которому Кажется.log|\
L1 DirA/ff_1.log|\
L1 DirA/ff_2.log|\
L1 DirA/ff_3.log|\
L1 DirB/ff_1.log|\
L1 DirB/ff_2.log|\
L1 DirB/ff_3.log|\
L1 DirB/L2 DirA/fff_1.log|\
L1 DirB/L2 DirA/fff_2.log|\
L1 DirB/L2 DirA/fff_3.log|\
L1 DirB/L2 DirA/L3 DirA/ffff_1.log|\
L1 DirB/L2 DirA/L3 DirA/ffff_2.log|\
L1 DirB/L2 DirC/fff_1.log|\
L1 DirB/L2 DirC/fff_2.log|\
L1 DirB/L2 DirC/fff_3.log|\
L1 DirC/ff1.log|\
L1 DirC/ff2.log|\
L1 DirC/ff3.log";


	void ParseResultStream( std::vector< std::wstring >& rLines, std::wistream& is )
	{
		rLines.clear();

		for ( std::wstring line; std::getline( is, line ); )
			rLines.push_back( line );

		if ( !rLines.empty() )
		{
			// strip to root directory name to facilitate testing (strip the ...\UT\ prefix)
			rLines.front() = fs::CPath( rLines.front() ).GetFilename();

			// cut the last empty line
			if ( rLines.back().empty() )
				rLines.pop_back();
		}
	}

	std::wstring RunTree( const CCmdLineOptions& options )
	{
		std::wstringstream stream;
		app::RunMain( stream, options );

		std::vector< std::wstring > lines;
		ParseResultStream( lines, stream );
		return str::Join( lines, L"|" );
	}
}


CTreePlusTests::CTreePlusTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CTreePlusTests& CTreePlusTests::Instance( void )
{
	static CTreePlusTests s_testCase;
	return s_testCase;
}

void CTreePlusTests::TestOnlyDirectories( void )
{
	ut::CTempFilePool pool( ut::s_flatPaths );
	const fs::TDirPath& poolDirPath = pool.GetPoolDirPath();

	std::wstring result;
	CCmdLineOptions options;

	options.m_dirPath = poolDirPath / _T("L1 DirA");
	result = ut::RunTree(  options );
	ASSERT_EQUAL( L"L1 DirA", result );

	options.m_dirPath = poolDirPath;
	result = ut::RunTree(  options );
	ASSERT_EQUAL( L"\
_UT|\
├───L1 DirA|\
├───L1 DirB|\
│   ├───L2 DirA|\
│   │   └───L3 DirA|\
│   └───L2 DirC|\
└───L1 DirC"
		, result );

	options.m_maxDepthLevel = 1;
	result = ut::RunTree(  options );
	ASSERT_EQUAL( L"\
_UT|\
├───L1 DirA|\
├───L1 DirB|\
└───L1 DirC"
		, result );

	options.m_maxDepthLevel = 2;
	result = ut::RunTree(  options );
	ASSERT_EQUAL( L"\
_UT|\
├───L1 DirA|\
├───L1 DirB|\
│   ├───L2 DirA|\
│   └───L2 DirC|\
└───L1 DirC"
		, result );
}

void CTreePlusTests::TestFilesAndDirectories( void )
{
	ut::CTempFilePool pool( ut::s_flatPaths );
	const fs::TDirPath& poolDirPath = pool.GetPoolDirPath();

	std::wstring result;

	CCmdLineOptions options;
	options.m_optionFlags |= app::DisplayFiles;

	options.m_dirPath = poolDirPath / _T("L1 DirA");
	result = ut::RunTree(  options );
	ASSERT_EQUAL( L"\
L1 DirA|\
    ff_1.log|\
    ff_2.log|\
    ff_3.log"
		, result );

	options.m_dirPath = poolDirPath;
	result = ut::RunTree(  options );
	ASSERT_EQUAL( L"\
_UT|\
│   FileA Arşiţă şi pârjol.log|\
│   FileB Σέρτζιο Ματαρέλα.log|\
│   FileC Которому Кажется.log|\
│|\
├───L1 DirA|\
│       ff_1.log|\
│       ff_2.log|\
│       ff_3.log|\
│|\
├───L1 DirB|\
│   │   ff_1.log|\
│   │   ff_2.log|\
│   │   ff_3.log|\
│   │|\
│   ├───L2 DirA|\
│   │   │   fff_1.log|\
│   │   │   fff_2.log|\
│   │   │   fff_3.log|\
│   │   │|\
│   │   └───L3 DirA|\
│   │           ffff_1.log|\
│   │           ffff_2.log|\
│   │|\
│   └───L2 DirC|\
│           fff_1.log|\
│           fff_2.log|\
│           fff_3.log|\
│|\
└───L1 DirC|\
        ff1.log|\
        ff2.log|\
        ff3.log"
		, result );

	options.m_maxDepthLevel = 1;
	result = ut::RunTree(  options );
	ASSERT_EQUAL( L"\
_UT|\
│   FileA Arşiţă şi pârjol.log|\
│   FileB Σέρτζιο Ματαρέλα.log|\
│   FileC Которому Кажется.log|\
│|\
├───L1 DirA|\
├───L1 DirB|\
└───L1 DirC"
		, result );

	options.m_maxDepthLevel = 2;
	result = ut::RunTree(  options );
	ASSERT_EQUAL( L"\
_UT|\
│   FileA Arşiţă şi pârjol.log|\
│   FileB Σέρτζιο Ματαρέλα.log|\
│   FileC Которому Кажется.log|\
│|\
├───L1 DirA|\
│       ff_1.log|\
│       ff_2.log|\
│       ff_3.log|\
│|\
├───L1 DirB|\
│   │   ff_1.log|\
│   │   ff_2.log|\
│   │   ff_3.log|\
│   │|\
│   ├───L2 DirA|\
│   └───L2 DirC|\
└───L1 DirC|\
        ff1.log|\
        ff2.log|\
        ff3.log"
		, result );

	options.m_maxDepthLevel = utl::npos;

	options.m_maxDirFiles = 1;
	result = ut::RunTree(  options );
	ASSERT_EQUAL( L"\
_UT|\
│   FileA Arşiţă şi pârjol.log|\
│   (+ 2 more files)|\
│|\
├───L1 DirA|\
│       ff_1.log|\
│       (+ 2 more files)|\
│|\
├───L1 DirB|\
│   │   ff_1.log|\
│   │   (+ 2 more files)|\
│   │|\
│   ├───L2 DirA|\
│   │   │   fff_1.log|\
│   │   │   (+ 2 more files)|\
│   │   │|\
│   │   └───L3 DirA|\
│   │           ffff_1.log|\
│   │           (+ 1 more files)|\
│   │|\
│   └───L2 DirC|\
│           fff_1.log|\
│           (+ 2 more files)|\
│|\
└───L1 DirC|\
        ff1.log|\
        (+ 2 more files)"
		, result );

	options.m_maxDirFiles = 0;
	result = ut::RunTree(  options );
	ASSERT_EQUAL( L"\
_UT|\
│   (3 files)|\
│|\
├───L1 DirA|\
│       (3 files)|\
│|\
├───L1 DirB|\
│   │   (3 files)|\
│   │|\
│   ├───L2 DirA|\
│   │   │   (3 files)|\
│   │   │|\
│   │   └───L3 DirA|\
│   │           (2 files)|\
│   │|\
│   └───L2 DirC|\
│           (3 files)|\
│|\
└───L1 DirC|\
        (3 files)"
		, result );
}


void CTreePlusTests::Run( void )
{
	__super::Run();

	TestOnlyDirectories();
	TestFilesAndDirectories();
}


#endif //USE_UT
