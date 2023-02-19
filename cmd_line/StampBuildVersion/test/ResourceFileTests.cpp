
#include "stdafx.h"

#ifdef USE_UT		// no UT code in release builds
#include "ResourceFileTests.h"
#include "Application.h"
#include "CmdLineOptions.h"
#include "ResourceFile.h"
#include "utl/AppTools.h"
#include "utl/FileSystem.h"
#include "utl/ProcessCmd.h"
#include "utl/StringUtilities.h"
#include "utl/TextFileIo.h"
#include "utl/TimeUtils.h"
#include "utl/TokenIterator.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/TextFileIo.hxx"


namespace ut
{
	const TCHAR* s_rcFiles[] =
	{
		_T("MyTool_ANSI.rc"),
		_T("MyTool_UTF8_bom.rc"),
		_T("MyTool_UTF16_LE_bom.rc"),
		_T("MyTool_UTF16_be_bom.rc")
	};
	const std::string s_refTimestamp = "10-11-2022 10:11:22";
	const char s_doubleQuote[] = "\"";

	enum FileEncoding { RC_ANSI, RC_UTF8_bom, RC_UTF16_LE_bom, RC_UTF16_be_bom };

	bool AproxEquals( const CTime& left, const CTime& right )
	{
		ASSERT( time_utl::IsValid( left ) );
		ASSERT( time_utl::IsValid( right ) );
		CTimeSpan delta = right - left;
		return abs( static_cast<long>( delta.GetTimeSpan() ) ) < 2;
	}


	// impl
	void testEach_StampRcFile( const TCHAR* pRcFilePath );
	void testRcFile_CurrentTimestamp( const std::string& newText, const CTime& baselineTimestamp );
	void testRcFile_RefTimestamp( const std::string& newText );
}


CResourceFileTests::CResourceFileTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CResourceFileTests& CResourceFileTests::Instance( void )
{
	static CResourceFileTests s_testCase;
	return s_testCase;
}

void CResourceFileTests::TestStampRcFile( void )
{
	ut::testEach_StampRcFile( ut::s_rcFiles[ ut::RC_ANSI ] );
	ut::testEach_StampRcFile( ut::s_rcFiles[ ut::RC_UTF8_bom ] );
	ut::testEach_StampRcFile( ut::s_rcFiles[ ut::RC_UTF16_LE_bom ] );
	ut::testEach_StampRcFile( ut::s_rcFiles[ ut::RC_UTF16_be_bom ] );
}

void CResourceFileTests::FuncTest_StampRcFile( void )
{
	ut::CTempFilePool pool( ut::s_rcFiles[ ut::RC_ANSI ] );
	fs::TDirPath poolDirPath = pool.GetPoolDirPath();

	ASSERT_EQUAL( 1, pool.GetFilePaths().size() );
	ASSERT_EQUAL( ut::s_rcFiles[ ut::RC_ANSI ], ut::EnumJoinFiles( poolDirPath ) );

	const fs::CPath& targetRcFilePath = pool.GetFilePaths()[ 0 ];

	fs::thr::CopyFile( (ut::GetStdTestFilesDirPath() / ut::s_rcFiles[ ut::RC_ANSI ]).GetPtr(), targetRcFilePath.GetPtr(), false );

	std::string srcText;
	io::ReadStringFromFile( srcText, targetRcFilePath );

	std::string newText;

	// command line as: "StampBuildVersion.exe <targetRcFilePath>"
	{
		utl::CProcessCmd cmd( __targv[ 0 ] );
		cmd.AddParam( targetRcFilePath );

		ASSERT_EQUAL( 0, cmd.Execute() );		// no execution errors?
		{
			io::ReadStringFromFile( newText, targetRcFilePath );
			ASSERT_EQUAL( srcText, newText );			// ensure no change since "BuildTimestamp" is missing
		}
	}

	// command line as: "StampBuildVersion.exe <targetRcFilePath> -a"
	{
		const CTime baselineTimestamp = CTime::GetCurrentTime();
		utl::CProcessCmd cmd( __targv[ 0 ] );
		cmd.AddParam( targetRcFilePath );
		cmd.AddParam( _T("-a") );						// force add the "BuildTimestamp" entry

		ASSERT_EQUAL( 0, cmd.Execute() );		// no execution errors?
		{
			io::ReadStringFromFile( newText, targetRcFilePath );
			ASSERT( newText != srcText );

			ut::testRcFile_CurrentTimestamp( newText, baselineTimestamp );
		}
	}

	// command line as: "StampBuildVersion.exe <targetRcFilePath> "10-11-2022 10:11:22" -a"
	{
		utl::CProcessCmd cmd( __targv[ 0 ] );
		cmd.AddParam( targetRcFilePath );
		cmd.AddParam( ut::s_refTimestamp );
		cmd.AddParam( _T("-a") );						// force add the "BuildTimestamp" entry

		ASSERT_EQUAL( 0, cmd.Execute() );		// no execution errors?
		{
			io::ReadStringFromFile( newText, targetRcFilePath );
			ASSERT( newText != srcText );

			ut::testRcFile_RefTimestamp( newText );
		}
	}
}


void ut::testEach_StampRcFile( const TCHAR* pRcFilePath )
{
	ut::CTempFilePool pool( pRcFilePath );
	fs::TDirPath poolDirPath = pool.GetPoolDirPath();

	ASSERT_EQUAL( 1, pool.GetFilePaths().size() );
	ASSERT_EQUAL( pRcFilePath, ut::EnumJoinFiles( poolDirPath ) );

	const fs::CPath& targetRcFilePath = pool.GetFilePaths()[ 0 ];

	fs::thr::CopyFile( (ut::GetStdTestFilesDirPath() / pRcFilePath).GetPtr(), targetRcFilePath.GetPtr(), false );

	std::string srcText;
	io::ReadStringFromFile( srcText, targetRcFilePath );

	std::string newText;

	CCmdLineOptions options;

	// equivalent command line: "StampBuildVersion.exe <targetRcFilePath>"
	options.m_targetRcPath = targetRcFilePath;
	app::RunMain( options );
	{
		io::ReadStringFromFile( newText, targetRcFilePath );
		ASSERT_EQUAL( srcText, newText );			// ensure no change since "BuildTimestamp" is missing
	}

	// equivalent command line: "StampBuildVersion.exe <targetRcFilePath> -a"
	options.m_buildTimestamp = CTime::GetCurrentTime();
	SetFlag( options.m_optionFlags, app::Add_BuildTimestamp );

	app::RunMain( options );
	{
		io::ReadStringFromFile( newText, targetRcFilePath );
		ASSERT( newText != srcText );

		testRcFile_CurrentTimestamp( newText, options.m_buildTimestamp );
	}

	// equivalent command line: "StampBuildVersion.exe <targetRcFilePath> "10-11-2022 10:11:22" -a"
	ClearFlag( options.m_optionFlags, app::Add_BuildTimestamp );
	options.m_buildTimestamp = time_utl::ParseStdTimestamp( str::FromUtf8( ut::s_refTimestamp.c_str() ) );

	app::RunMain( options );
	{
		io::ReadStringFromFile( newText, targetRcFilePath );
		ASSERT( newText != srcText );

		testRcFile_RefTimestamp( newText );
	}
}

void ut::testRcFile_CurrentTimestamp( const std::string& newText, const CTime& baselineTimestamp )
{
	rc::TTokenIterator it( newText );
	ASSERT( it.FindToken( "VALUE \"BuildTimestamp\"" ) );
	ASSERT( it.FindToken( s_doubleQuote, false ) );

	std::string tsValue;
	ASSERT( it.ExtractEnclosedText( tsValue, s_doubleQuote ) );

	CTime parsedTimestamp = time_utl::ParseStdTimestamp( str::FromAnsi( tsValue.c_str() ) );
	ASSERT( ut::AproxEquals( baselineTimestamp, parsedTimestamp ) );
}

void ut::testRcFile_RefTimestamp( const std::string& newText )
{
	rc::TTokenIterator it( newText );
	ASSERT( it.FindToken( "VALUE \"BuildTimestamp\"" ) );
	ASSERT( it.FindToken( s_doubleQuote, false ) );

	std::string tsValue;
	ASSERT( it.ExtractEnclosedText( tsValue, s_doubleQuote ) );

	ASSERT_EQUAL( ut::s_refTimestamp, tsValue );
}


void CResourceFileTests::Run( void )
{
	RUN_TEST( TestStampRcFile );
	RUN_TEST( FuncTest_StampRcFile );
}


#endif //USE_UT
