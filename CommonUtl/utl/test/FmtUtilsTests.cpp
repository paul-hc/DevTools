
#include "stdafx.h"

#ifdef USE_UT		// no UT code in release builds
#include "test/FmtUtilsTests.h"
#include "FmtUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ut
{
	static const std::tstring s_filePath = _T("C:\\download\\file.txt");
	static const CTime s_ct( 2017, 7, 1, 14, 10, 0 );
	static const CTime s_mt( 2017, 7, 1, 14, 20, 0 );
	static const CTime s_at( 2017, 7, 1, 14, 30, 0 );

	static const fs::CFileState& GetStdFileState( void )
	{
		static fs::CFileState s_refState;

		if ( s_refState.m_fullPath.IsEmpty() )
		{
			s_refState.m_fullPath.Set( s_filePath );
			s_refState.m_attributes = CFile::readOnly | CFile::hidden;
			s_refState.m_creationTime = s_ct;
			s_refState.m_modifTime = s_mt;
			s_refState.m_accessTime = s_at;
		}
		return s_refState;
	}
}


CFmtUtilsTests::CFmtUtilsTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CFmtUtilsTests& CFmtUtilsTests::Instance( void )
{
	static CFmtUtilsTests s_testCase;
	return s_testCase;
}

void CFmtUtilsTests::TestFileState( void )
{
	const fs::CFileState& refState = ut::GetStdFileState();

	std::tstring text;
	fs::CFileState newState;

	// tagged format
	text = fmt::FormatClipFileState( refState );
	ASSERT_EQUAL( _T("C:\\download\\file.txt\t{RH|C=01-07-2017 14:10:00|M=01-07-2017 14:20:00|A=01-07-2017 14:30:00}"), text );
	ASSERT_EQUAL( refState, fmt::ParseClipFileState( newState, text ) );

	// tagged format: A/M/C
	ASSERT_EQUAL( refState, fmt::ParseClipFileState( newState, _T("C:\\download\\file.txt\t{RH|A=01-07-2017 14:30:00|M=01-07-2017 14:20:00|C=01-07-2017 14:10:00}") ) );

	// legacy untagged format
	text = fmt::FormatClipFileState( refState, fmt::FullPath, false );
	ASSERT_EQUAL( _T("C:\\download\\file.txt\t{RH|01-07-2017 14:10:00|01-07-2017 14:20:00|01-07-2017 14:30:00}"), text );			// time-field: C,M,A
	ASSERT_EQUAL( refState, fmt::ParseClipFileState( newState, text ) );

	// tagged format: filename.ext
	static const fs::CPath s_keyPath( _T("C:\\download\\file.txt") );
	text = fmt::FormatClipFileState( refState, fmt::FilenameExt );
	ASSERT_EQUAL( _T("file.txt\t{RH|C=01-07-2017 14:10:00|M=01-07-2017 14:20:00|A=01-07-2017 14:30:00}"), text );
	ASSERT_EQUAL( refState, fmt::ParseClipFileState( newState, text, &s_keyPath ) );		// use key path's directory to qualify "file.txt"
}

void CFmtUtilsTests::TestTouchEntry( void )
{
	static const CTimeSpan _5_mins( 0, 0, 5, 0 );

	fs::CFileState srcState = ut::GetStdFileState(), destState = ut::GetStdFileState();
	ASSERT_EQUAL( _T("C:\\download\\file.txt :: {RH} -> {RH}"), fmt::FormatTouchEntry( srcState, destState ) );
	ASSERT_EQUAL( _T("C:\\download\\file.txt :: {RH|||} -> {RH|||}"), fmt::FormatTouchEntry( srcState, destState, false ) );		// untagged legacy

	destState.m_accessTime += _5_mins;
	ASSERT_EQUAL( _T("C:\\download\\file.txt :: {RH|A=01-07-2017 14:30:00} -> {RH|A=01-07-2017 14:35:00}"), fmt::FormatTouchEntry( srcState, destState ) );
	ASSERT_EQUAL( _T("C:\\download\\file.txt :: {RH|||01-07-2017 14:30:00} -> {RH|||01-07-2017 14:35:00}"), fmt::FormatTouchEntry( srcState, destState, false ) );	// C|M|A

	destState.m_creationTime += _5_mins;
	ASSERT_EQUAL( _T("C:\\download\\file.txt :: {RH|C=01-07-2017 14:10:00|A=01-07-2017 14:30:00} -> {RH|C=01-07-2017 14:15:00|A=01-07-2017 14:35:00}"), fmt::FormatTouchEntry( srcState, destState ) );
	ASSERT_EQUAL( _T("C:\\download\\file.txt :: {RH|01-07-2017 14:10:00||01-07-2017 14:30:00} -> {RH|01-07-2017 14:15:00||01-07-2017 14:35:00}"), fmt::FormatTouchEntry( srcState, destState, false ) );
}


void CFmtUtilsTests::Run( void )
{
	__super::Run();

	TestFileState();
	TestTouchEntry();
}


#endif //USE_UT
