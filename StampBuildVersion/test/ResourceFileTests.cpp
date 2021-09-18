
#include "stdafx.h"
#include "ResourceFileTests.h"
#include "Application.h"
#include "CmdLineOptions.h"
#include "ResourceFile.h"
#include "utl/AppTools.h"
#include "utl/FileSystem.h"
#include "utl/ProcessCmd.h"
#include "utl/StringUtilities.h"
#include "utl/TextFileUtils.h"
#include "utl/TimeUtils.h"
#include "utl/TokenIterator.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/TextFileUtils.hxx"


#ifdef _DEBUG		// no UT code in release builds


namespace ut
{
	bool RunApp( const CCmdLineOptions& options )
	{
		try
		{
			app::RunMain( options );
			return true;
		}
		catch ( const std::exception& exc )
		{
			app::ReportException( exc );
			return false;
		}
		catch ( CException* pExc )
		{
			app::ReportException( pExc );
			pExc->Delete();
			return false;
		}
	}

	bool AproxEquals( const CTime& left, const CTime& right )
	{
		ASSERT( time_utl::IsValid( left ) );
		ASSERT( time_utl::IsValid( right ) );
		CTimeSpan delta = right - left;
		return abs( static_cast<long>( delta.GetTimeSpan() ) ) < 2;
	}
}


const TCHAR CResourceFileTests::s_rcFile[] = _T("MyTool_clean.rc");
const std::string CResourceFileTests::s_refTimestamp = "10-11-2022 10:11:22";
const char CResourceFileTests::s_doubleQuote[] = "\"";

CResourceFileTests::CResourceFileTests( void )
	: m_debugChildProcs( false )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CResourceFileTests& CResourceFileTests::Instance( void )
{
	static CResourceFileTests s_testCase;
	return s_testCase;
}

int CResourceFileTests::ExecuteProcess( utl::CProcessCmd& rProcess )
{
	if ( m_debugChildProcs )
		rProcess.AddParam( _T("-debug") );		// to debug command line parsing

	return rProcess.Execute();
}

void CResourceFileTests::TestStampRcFile( void )
{
	ut::CTempFilePool pool( s_rcFile );
	fs::CPath poolDirPath = pool.GetPoolDirPath();

	ASSERT_EQUAL( 1, pool.GetFilePaths().size() );
	ASSERT_EQUAL( s_rcFile, ut::EnumJoinFiles( poolDirPath ) );

	const fs::CPath& targetRcFilePath = pool.GetFilePaths()[ 0 ];

	fs::thr::CopyFile( (ut::GetStdTestFilesDirPath() / s_rcFile).GetPtr(), targetRcFilePath.GetPtr(), false );

	std::string srcText;
	utl::ReadStringFromFile( srcText, targetRcFilePath );

	std::string newText;

	CCmdLineOptions options;

	// equivalent command line: "StampBuildVersion.exe <targetRcFilePath>"
	options.m_targetRcPath = targetRcFilePath;
	ASSERT( ut::RunApp( options ) );
	{
		utl::ReadStringFromFile( newText, targetRcFilePath );
		ASSERT_EQUAL( srcText, newText );			// ensure no change since "BuildTimestamp" is missing
	}

	// equivalent command line: "StampBuildVersion.exe <targetRcFilePath> -a"
	options.m_buildTimestamp = CTime::GetCurrentTime();
	SetFlag( options.m_optionFlags, app::Add_BuildTimestamp );

	ASSERT( ut::RunApp( options ) );
	{
		utl::ReadStringFromFile( newText, targetRcFilePath );
		ASSERT( newText != srcText );

		testRcFile_CurrentTimestamp( newText, options.m_buildTimestamp );
	}

	// equivalent command line: "StampBuildVersion.exe <targetRcFilePath> "10-11-2022 10:11:22" -a"
	ClearFlag( options.m_optionFlags, app::Add_BuildTimestamp );
	options.m_buildTimestamp = time_utl::ParseStdTimestamp( str::FromUtf8( s_refTimestamp.c_str() ) );

	ASSERT( ut::RunApp( options ) );
	{
		utl::ReadStringFromFile( newText, targetRcFilePath );
		ASSERT( newText != srcText );

		testRcFile_RefTimestamp( newText );
	}
}

void CResourceFileTests::FuncTest_StampRcFile( void )
{
	ut::CTempFilePool pool( s_rcFile );
	fs::CPath poolDirPath = pool.GetPoolDirPath();

	ASSERT_EQUAL( 1, pool.GetFilePaths().size() );
	ASSERT_EQUAL( s_rcFile, ut::EnumJoinFiles( poolDirPath ) );

	const fs::CPath& targetRcFilePath = pool.GetFilePaths()[ 0 ];

	fs::thr::CopyFile( (ut::GetStdTestFilesDirPath() / s_rcFile).GetPtr(), targetRcFilePath.GetPtr(), false );

	std::string srcText;
	utl::ReadStringFromFile( srcText, targetRcFilePath );

	std::string newText;

	// command line as: "StampBuildVersion.exe <targetRcFilePath>"
	{
		utl::CProcessCmd cmd( __targv[ 0 ] );
		cmd.AddParam( targetRcFilePath );

		ASSERT_EQUAL( 0, ExecuteProcess( cmd ) );		// no execution errors?
		{
			utl::ReadStringFromFile( newText, targetRcFilePath );
			ASSERT_EQUAL( srcText, newText );			// ensure no change since "BuildTimestamp" is missing
		}
	}

	// command line as: "StampBuildVersion.exe <targetRcFilePath> -a"
	{
		const CTime baselineTimestamp = CTime::GetCurrentTime();
		utl::CProcessCmd cmd( __targv[ 0 ] );
		cmd.AddParam( targetRcFilePath );
		cmd.AddParam( _T("-a") );						// force add the "BuildTimestamp" entry

		ASSERT_EQUAL( 0, ExecuteProcess( cmd ) );		// no execution errors?
		{
			utl::ReadStringFromFile( newText, targetRcFilePath );
			ASSERT( newText != srcText );

			testRcFile_CurrentTimestamp( newText, baselineTimestamp );
		}
	}

	// command line as: "StampBuildVersion.exe <targetRcFilePath> "10-11-2022 10:11:22" -a"
	{
		utl::CProcessCmd cmd( __targv[ 0 ] );
		cmd.AddParam( targetRcFilePath );
		cmd.AddParam( s_refTimestamp );
		cmd.AddParam( _T("-a") );						// force add the "BuildTimestamp" entry

		ASSERT_EQUAL( 0, ExecuteProcess( cmd ) );		// no execution errors?
		{
			utl::ReadStringFromFile( newText, targetRcFilePath );
			ASSERT( newText != srcText );

			testRcFile_RefTimestamp( newText );
		}
	}
}


void CResourceFileTests::testRcFile_CurrentTimestamp( const std::string& newText, const CTime& baselineTimestamp )
{
	rc::TTokenIterator it( newText );
	ASSERT( it.FindToken( "VALUE \"BuildTimestamp\"" ) );
	ASSERT( it.FindToken( s_doubleQuote, false ) );

	std::string tsValue;
	ASSERT( it.ExtractEnclosedText( tsValue, s_doubleQuote ) );

	CTime parsedTimestamp = time_utl::ParseStdTimestamp( str::FromAnsi( tsValue.c_str() ) );
	ASSERT( ut::AproxEquals( baselineTimestamp, parsedTimestamp ) );
}

void CResourceFileTests::testRcFile_RefTimestamp( const std::string& newText )
{
	rc::TTokenIterator it( newText );
	ASSERT( it.FindToken( "VALUE \"BuildTimestamp\"" ) );
	ASSERT( it.FindToken( s_doubleQuote, false ) );

	std::string tsValue;
	ASSERT( it.ExtractEnclosedText( tsValue, s_doubleQuote ) );

	ASSERT_EQUAL( s_refTimestamp, tsValue );
}


void CResourceFileTests::Run( void )
{
	__super::Run();

	TestStampRcFile();
	FuncTest_StampRcFile();
}


#endif //_DEBUG
