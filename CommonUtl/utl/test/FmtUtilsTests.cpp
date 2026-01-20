
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "test/FmtUtilsTests.h"
#include "TimeUtils.h"
#include "FmtUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ut
{
	static const std::tstring s_filePath = _T("C:\\download\\file.txt");
	// Pick test dates outside summer Daylight Savings - there are incompatibility issues starting with Visual C++ 2022,
	// due to CTime( const FILETIME& fileTime, int nDST = -1 ) constructor implementation!
	//
	static const CTime s_creationTime	= time_utl::ParseTimestamp( _T("29-11-2017 14:10:00") );
	static const CTime s_modifTime		= time_utl::ParseTimestamp( _T("29-11-2017 14:20:00") );
	static const CTime s_accessTime		= time_utl::ParseTimestamp( _T("29-11-2017 14:30:00") );

	static const fs::CFileState& GetStdFileState( void )
	{
		static fs::CFileState s_refState;

		if ( s_refState.m_fullPath.IsEmpty() )
		{
			s_refState.m_fullPath.Set( s_filePath );
			s_refState.m_attributes = CFile::readOnly | CFile::hidden;
			s_refState.m_creationTime = s_creationTime;
			s_refState.m_modifTime = s_modifTime;
			s_refState.m_accessTime = s_accessTime;
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
	ASSERT_EQUAL( _T("C:\\download\\file.txt\t{RH|C=29-11-2017 14:10:00|M=29-11-2017 14:20:00|A=29-11-2017 14:30:00}"), text );
	ASSERT_EQUAL( refState, fmt::ParseClipFileState( newState, text ) );

	// tagged format: A/M/C
	ASSERT_EQUAL( refState, fmt::ParseClipFileState( newState, _T("C:\\download\\file.txt\t{RH|A=29-11-2017 14:30:00|M=29-11-2017 14:20:00|C=29-11-2017 14:10:00}") ) );

	// legacy untagged format
	text = fmt::FormatClipFileState( refState, fmt::FullPath, false );
	ASSERT_EQUAL( _T("C:\\download\\file.txt\t{RH|29-11-2017 14:10:00|29-11-2017 14:20:00|29-11-2017 14:30:00}"), text );			// time-field: C,M,A
	ASSERT_EQUAL( refState, fmt::ParseClipFileState( newState, text ) );

	// tagged format: filename.ext
	static const fs::CPath s_keyPath( _T("C:\\download\\file.txt") );
	text = fmt::FormatClipFileState( refState, fmt::FilenameExt );
	ASSERT_EQUAL( _T("file.txt\t{RH|C=29-11-2017 14:10:00|M=29-11-2017 14:20:00|A=29-11-2017 14:30:00}"), text );
	ASSERT_EQUAL( refState, fmt::ParseClipFileState( newState, text, &s_keyPath ) );		// use key path's directory to qualify "file.txt"
}

void CFmtUtilsTests::TestTouchEntry( void )
{
	static const CTimeSpan _5_mins( 0, 0, 5, 0 );

	fs::CFileState srcState = ut::GetStdFileState(), destState = ut::GetStdFileState();
	ASSERT_EQUAL( _T("C:\\download\\file.txt :: {RH} -> {RH}"), fmt::FormatTouchEntry( srcState, destState ) );
	ASSERT_EQUAL( _T("C:\\download\\file.txt :: {RH|||} -> {RH|||}"), fmt::FormatTouchEntry( srcState, destState, false ) );		// untagged legacy

	destState.m_accessTime += _5_mins;
	ASSERT_EQUAL( _T("C:\\download\\file.txt :: {RH|A=29-11-2017 14:30:00} -> {RH|A=29-11-2017 14:35:00}"), fmt::FormatTouchEntry( srcState, destState ) );
	ASSERT_EQUAL( _T("C:\\download\\file.txt :: {RH|||29-11-2017 14:30:00} -> {RH|||29-11-2017 14:35:00}"), fmt::FormatTouchEntry( srcState, destState, false ) );	// C|M|A

	destState.m_creationTime += _5_mins;
	ASSERT_EQUAL( _T("C:\\download\\file.txt :: {RH|C=29-11-2017 14:10:00|A=29-11-2017 14:30:00} -> {RH|C=29-11-2017 14:15:00|A=29-11-2017 14:35:00}"), fmt::FormatTouchEntry( srcState, destState ) );
	ASSERT_EQUAL( _T("C:\\download\\file.txt :: {RH|29-11-2017 14:10:00||29-11-2017 14:30:00} -> {RH|29-11-2017 14:15:00||29-11-2017 14:35:00}"), fmt::FormatTouchEntry( srcState, destState, false ) );
}

void CFmtUtilsTests::TestFormatKeyShortcut( void )
{
	ASSERT_EQUAL( _T(""), fmt::FormatKeyShortcut( 0, 0 ) );
	ASSERT_EQUAL( _T("Ctrl + X"), fmt::FormatKeyShortcut( 'X', HOTKEYF_CONTROL ) );
	ASSERT_EQUAL( _T("Ctrl + Shift + Alt + M"), fmt::FormatKeyShortcut( 'M', HOTKEYF_CONTROL | HOTKEYF_SHIFT | HOTKEYF_ALT ) );
}


void CFmtUtilsTests::Run( void )
{
	RUN_TEST( TestFileState );
	RUN_TEST( TestTouchEntry );
	RUN_TEST( TestFormatKeyShortcut );
}


#endif //USE_UT
