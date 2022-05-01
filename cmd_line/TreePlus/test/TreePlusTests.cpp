
#include "stdafx.h"

#ifdef USE_UT		// no UT code in release builds

#include "TreePlusTests.h"
#include "Application.h"
#include "CmdLineOptions.h"
#include "Table.h"
#include "utl/AppTools.h"
#include "utl/StringUtilities.h"
#include "utl/TextFileIo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/TextFileIo.hxx"


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
L1 DirC/ff3.log\
";

	const std::wstring s_tableText = L"\
Alternative\tCharlotte Gainsbourg [FR]\t1986 Lemon Incest (w. Serge Gainsbourg)\n\
Alternative\tCharlotte Gainsbourg [FR]\t2009 IRM\n\
Alternative\tDavid Byrne\t2004 Grown Backwards\n\
Alternative\tFoo Fighters\t2002 One By One\n\
Alternative\tFoo Fighters\t2006 Skin and Bones\n\
Alternative\tFoo Fighters\t2009 Greatest Hits\n\
Alternative\tNirvana\t1991 Nevermind\n\
Alternative\tNirvana\t1993 In Utero\n\
Romanian\tStefan Hrușcă\tColinde\n\
Romanian\tStefan Hrușcă\tCrăciunul cu Stefan Hrușcă\n\
Soundtrack\tAndrew Lloyd Webber\t1973 Jesus Christ Superstar (2CD)\n\
Soundtrack\tGoran Bregovic\t1988 Time Of The Gypsies (Czas Cyganow)\n\
Soundtrack\tGoran Bregovic\t1993 Arizona Dream\n\
Soundtrack\tGoran Bregovic\t1994 Queen Margot\n\
World\tFranz Zăicescu\tBalcan\n\
World\tFranz Zăicescu\tCântice\n\
";


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
	result = ut::RunTree( options );
	ASSERT_EQUAL( L"L1 DirA", result );

	options.m_dirPath = poolDirPath;
	result = ut::RunTree( options );
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
	result = ut::RunTree( options );
	ASSERT_EQUAL( L"\
_UT|\
├───L1 DirA|\
├───L1 DirB|\
└───L1 DirC"
		, result );

	options.m_maxDepthLevel = 2;
	result = ut::RunTree( options );
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
	result = ut::RunTree( options );
	ASSERT_EQUAL( L"\
L1 DirA|\
    ff_1.log|\
    ff_2.log|\
    ff_3.log"
		, result );

	options.m_dirPath = poolDirPath;
	result = ut::RunTree( options );
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
	result = ut::RunTree( options );
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
	result = ut::RunTree( options );
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
	result = ut::RunTree( options );
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
	result = ut::RunTree( options );
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

void CTreePlusTests::TestTableInput( void )
{
	ut::CTempFilePool pool( _T("TableInput.txt|TabbedOutput.txt") );
	const fs::TDirPath& inTextPath = pool.GetFilePaths()[ 0 ];
	const fs::TDirPath& outTextPath = pool.GetFilePaths()[ 1 ];

	io::WriteStringToFile( inTextPath, ut::s_tableText, fs::ANSI_UTF8 );

	std::tstring arg0 = app::GetModulePath().Get(), arg1 = str::Format( _T("in=%s"), inTextPath.GetPtr() ), arg2 = str::Format( _T("out=%s"), outTextPath.GetPtr() );
	const TCHAR* cmdLine[] = { arg0.c_str(), arg1.c_str(), arg2.c_str() };
	CCmdLineOptions options;

	options.ParseCommandLine( COUNT_OF( cmdLine ), cmdLine );
	ASSERT_PTR( options.GetTable() );
	testTable( options.GetTable()->GetRoot() );

	// TabGuides tabbed output
	{
		ut::RunTree( options );

		std::wstring result;
		ASSERT_EQUAL( fs::ANSI_UTF8, io::ReadStringFromFile( result, outTextPath ) );

		ASSERT_EQUAL( L"\
Alternative\n\
\tCharlotte Gainsbourg [FR]\n\
\t\t1986 Lemon Incest (w. Serge Gainsbourg)\n\
\t\t2009 IRM\n\
\tDavid Byrne\n\
\t\t2004 Grown Backwards\n\
\tFoo Fighters\n\
\t\t2002 One By One\n\
\t\t2006 Skin and Bones\n\
\t\t2009 Greatest Hits\n\
\tNirvana\n\
\t\t1991 Nevermind\n\
\t\t1993 In Utero\n\
Romanian\n\
\tStefan Hrușcă\n\
\t\tColinde\n\
\t\tCrăciunul cu Stefan Hrușcă\n\
Soundtrack\n\
\tAndrew Lloyd Webber\n\
\t\t1973 Jesus Christ Superstar (2CD)\n\
\tGoran Bregovic\n\
\t\t1988 Time Of The Gypsies (Czas Cyganow)\n\
\t\t1993 Arizona Dream\n\
\t\t1994 Queen Margot\n\
World\n\
\tFranz Zăicescu\n\
\t\tBalcan\n\
\t\tCântice\n\
"
		, result );
	}

	// GraphGuides compact output
	options.m_guidesProfileType = GraphGuides;				// use '-gs=G' argument
	options.m_optionFlags.Set( app::SkipFileGroupLine );	// keep compact lines
	{
		ut::RunTree( options );

		std::wstring result;
		ASSERT_EQUAL( fs::ANSI_UTF8, io::ReadStringFromFile( result, outTextPath ) );

		ASSERT_EQUAL( L"\
Alternative\n\
├───Charlotte Gainsbourg [FR]\n\
│       1986 Lemon Incest (w. Serge Gainsbourg)\n\
│       2009 IRM\n\
├───David Byrne\n\
│       2004 Grown Backwards\n\
├───Foo Fighters\n\
│       2002 One By One\n\
│       2006 Skin and Bones\n\
│       2009 Greatest Hits\n\
└───Nirvana\n\
        1991 Nevermind\n\
        1993 In Utero\n\
Romanian\n\
└───Stefan Hrușcă\n\
        Colinde\n\
        Crăciunul cu Stefan Hrușcă\n\
Soundtrack\n\
├───Andrew Lloyd Webber\n\
│       1973 Jesus Christ Superstar (2CD)\n\
└───Goran Bregovic\n\
        1988 Time Of The Gypsies (Czas Cyganow)\n\
        1993 Arizona Dream\n\
        1994 Queen Margot\n\
World\n\
└───Franz Zăicescu\n\
        Balcan\n\
        Cântice\n\
"
		, result );
	}
}

void CTreePlusTests::testTable( const CTextCell* pTableRoot )
{
	ASSERT_PTR( pTableRoot );
	ASSERT_EQUAL( 4, pTableRoot->GetChildren().size() );
	ASSERT_EQUAL( _T("Alternative"), pTableRoot->GetChildren()[ 0 ]->GetName() );
	ASSERT_EQUAL( _T("Romanian"), pTableRoot->GetChildren()[ 1 ]->GetName() );
	ASSERT_EQUAL( _T("Soundtrack"), pTableRoot->GetChildren()[ 2 ]->GetName() );
	ASSERT_EQUAL( _T("World"), pTableRoot->GetChildren()[ 3 ]->GetName() );

	std::vector< CTextCell* > children;

	const CTextCell* pCell = pTableRoot->DeepFindCell( _T("Soundtrack\tGoran Bregovic") );
	ASSERT_PTR( pCell );
	ASSERT_EQUAL( _T("Goran Bregovic"), pCell->GetName() );
	ASSERT_EQUAL( 3, pCell->GetChildren().size() );
	ASSERT_EQUAL( _T("1988 Time Of The Gypsies (Czas Cyganow)"), pCell->GetChildren()[ 0 ]->GetName() );
	ASSERT_EQUAL( _T("1993 Arizona Dream"), pCell->GetChildren()[ 1 ]->GetName() );
	ASSERT_EQUAL( _T("1994 Queen Margot"), pCell->GetChildren()[ 2 ]->GetName() );

	pCell->QuerySubFolders( children );
	ASSERT_EQUAL( 0, children.size() );
	pCell->QueryLeafs( children );
	ASSERT_EQUAL( 3, children.size() );

	pCell = pTableRoot->DeepFindCell( _T("Soundtrack\tGoran Bregovic\t1993 Arizona Dream") );
	ASSERT_PTR( pCell );
	ASSERT( pCell->GetChildren().empty() );
	ASSERT_EQUAL( _T("1993 Arizona Dream"), pCell->GetName() );
	ASSERT_EQUAL( _T("Soundtrack\tGoran Bregovic\t1993 Arizona Dream"), pCell->MakePath( pTableRoot ) );
}


void CTreePlusTests::Run( void )
{
	__super::Run();

	TestOnlyDirectories();
	TestFilesAndDirectories();
	TestTableInput();
}


#endif //USE_UT
