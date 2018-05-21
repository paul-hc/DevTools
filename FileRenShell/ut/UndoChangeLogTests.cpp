
#include "stdafx.h"
#include "UndoChangeLogTests.h"
#include "UndoChangeLog.h"
#include "utl/FileSystem.h"
#include "utl/TimeUtl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#ifdef _DEBUG		// no UT code in release builds


namespace ut
{
	static const std::string s_inputLog =
		" C:\\my\\download\\scan\\doctor.pdf  -> \tDr. Metz.pdf  \t\n"
		"C:\\my\\download\\scan\\doctor2.pdf -> Dr. Metz2.pdf\n"
		"<END OF BATCH>\n"
		"\n"
		"C:\\my\\download\\scan\\doctor3.pdf -> Dr. Metz3.pdf\n"
		"<END OF BATCH>\n"
		"\n"
		"<RENAME>\n"
		"C:\\my\\download\\kierkegaard_works.PDF -> Kierkegaard Works.pdf\n"
		"C:\\my\\download\\water tax.Doc -> Water Tax.Doc\n"
		"<END OF BATCH>\n"
		" <RENAME 8-06-1996 14:30:00>\t \t\n"
		"C:\\my\\download\\Apt Barbu.PDF -> Apt Barbu - confirmation.pdf\n"
		"<END OF BATCH>\n"
		"\n"
		"<RENAME 16-05-2018 19:21:02>\n"
		"C:\\my\\download\\BA Itinerary.pdf -> BA My Itinerary.pdf\n"
		"<END OF BATCH>\n"
		"\n"
		"<TOUCH 01-07-2017 8:00:00>\n"
		"C:\\my\\download\\exams.png :: {0x20|17-07-1992 9:21:17||} -> {0x21||17-07-1992 9:30:00|}\n"
		"<END OF BATCH>\n"
		;

	static const std::string s_outputLog =
		"<RENAME>\n"
		"C:\\my\\download\\scan\\doctor.pdf -> Dr. Metz.pdf\n"
		"C:\\my\\download\\scan\\doctor2.pdf -> Dr. Metz2.pdf\n"
		"<END OF BATCH>\n"
		"\n"
		"<RENAME>\n"
		"C:\\my\\download\\scan\\doctor3.pdf -> Dr. Metz3.pdf\n"
		"<END OF BATCH>\n"
		"\n"
		"<RENAME>\n"
		"C:\\my\\download\\kierkegaard_works.PDF -> Kierkegaard Works.pdf\n"
		"C:\\my\\download\\water tax.Doc -> Water Tax.Doc\n"
		"<END OF BATCH>\n"
		"\n"
		"<RENAME 08-06-1996 14:30:00>\n"
		"C:\\my\\download\\Apt Barbu.PDF -> Apt Barbu - confirmation.pdf\n"
		"<END OF BATCH>\n"
		"\n"
		"<RENAME 16-05-2018 19:21:02>\n"
		"C:\\my\\download\\BA Itinerary.pdf -> BA My Itinerary.pdf\n"
		"<END OF BATCH>\n"
		"\n"
		"<TOUCH 01-07-2017 08:00:00>\n"
		"C:\\my\\download\\exams.png :: {A|17-07-1992 09:21:17||} -> {RA||17-07-1992 09:30:00|}\n"
		"<END OF BATCH>\n"
		;
}


CUndoChangeLogTests::CUndoChangeLogTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CUndoChangeLogTests& CUndoChangeLogTests::Instance( void )
{
	static CUndoChangeLogTests testCase;
	return testCase;
}

void CUndoChangeLogTests::TestLoadLog( void )
{
	CUndoChangeLog log;
	std::istringstream iss( ut::s_inputLog );
	log.Load( iss );

	ASSERT_EQUAL( 5, log.GetRenameUndoStack().Get().size() );
	ASSERT_EQUAL( 1, log.GetTouchUndoStack().Get().size() );

	{
		std::list< CUndoChangeLog::CBatch< fs::TPathPairMap > >::const_iterator itStack;
		fs::TPathPairMap::const_iterator itPair;

		{
			itStack = log.GetRenameUndoStack().Get().begin();
			ASSERT_EQUAL( CTime(), itStack->m_timestamp );
			ASSERT_EQUAL( 2, itStack->m_batch.size() );

			itPair = itStack->m_batch.begin();
			ASSERT_EQUAL( _T("C:\\my\\download\\scan\\doctor.pdf"), itPair->first.Get() );
			ASSERT_EQUAL( _T("C:\\my\\download\\scan\\Dr. Metz.pdf"), itPair->second.Get() );
			++itPair;
			ASSERT_EQUAL( _T("C:\\my\\download\\scan\\doctor2.pdf"), itPair->first.Get() );
			ASSERT_EQUAL( _T("C:\\my\\download\\scan\\Dr. Metz2.pdf"), itPair->second.Get() );
		}
		{
			++itStack;
			ASSERT_EQUAL( CTime(), itStack->m_timestamp );
			ASSERT_EQUAL( 1, itStack->m_batch.size() );

			itPair = itStack->m_batch.begin();
			ASSERT_EQUAL( _T("C:\\my\\download\\scan\\doctor3.pdf"), itPair->first.Get() );
			ASSERT_EQUAL( _T("C:\\my\\download\\scan\\Dr. Metz3.pdf"), itPair->second.Get() );
		}
		{
			++itStack;
			ASSERT_EQUAL( CTime(), itStack->m_timestamp );
			ASSERT_EQUAL( 2, itStack->m_batch.size() );

			itPair = itStack->m_batch.begin();
			ASSERT_EQUAL( _T("C:\\my\\download\\kierkegaard_works.PDF"), itPair->first.Get() );
			ASSERT_EQUAL( _T("C:\\my\\download\\Kierkegaard Works.pdf"), itPair->second.Get() );
			++itPair;
			ASSERT_EQUAL( _T("C:\\my\\download\\water tax.Doc"), itPair->first.Get() );
			ASSERT_EQUAL( _T("C:\\my\\download\\Water Tax.Doc"), itPair->second.Get() );
		}
		{
			++itStack;
			ASSERT_EQUAL( CTime( 1996, 6, 8, 14, 30, 0 ), itStack->m_timestamp );
			ASSERT_EQUAL( 1, itStack->m_batch.size() );

			itPair = itStack->m_batch.begin();
			ASSERT_EQUAL( _T("C:\\my\\download\\Apt Barbu.PDF"), itPair->first.Get() );
			ASSERT_EQUAL( _T("C:\\my\\download\\Apt Barbu - confirmation.pdf"), itPair->second.Get() );
		}
		{
			++itStack;
			ASSERT_EQUAL( CTime( 2018, 5, 16, 19, 21, 02 ), itStack->m_timestamp );
			ASSERT_EQUAL( 1, itStack->m_batch.size() );

			itPair = itStack->m_batch.begin();
			ASSERT_EQUAL( _T("C:\\my\\download\\BA Itinerary.pdf"), itPair->first.Get() );
			ASSERT_EQUAL( _T("C:\\my\\download\\BA My Itinerary.pdf"), itPair->second.Get() );
		}
	}

	{
		std::list< CUndoChangeLog::CBatch< fs::TFileStatePairMap > >::const_iterator itStack;
		fs::TFileStatePairMap::const_iterator itPair;

		{
			itStack = log.GetTouchUndoStack().Get().begin();
			ASSERT_EQUAL( CTime( 2017, 7, 1, 8, 0, 0 ), itStack->m_timestamp );
			ASSERT_EQUAL( 1, itStack->m_batch.size() );

			itPair = itStack->m_batch.begin();
			ASSERT_EQUAL( _T("C:\\my\\download\\exams.png"), itPair->first.m_fullPath.Get() );
			ASSERT_EQUAL( 0x20, itPair->first.m_attributes );
			ASSERT_EQUAL( CTime( 1992, 7, 17, 9, 21, 17 ), itPair->first.m_creationTime );
			ASSERT_EQUAL( CTime(), itPair->first.m_modifTime );
			ASSERT_EQUAL( CTime(), itPair->first.m_accessTime );

			ASSERT_EQUAL( _T("C:\\my\\download\\exams.png"), itPair->second.m_fullPath.Get() );
			ASSERT_EQUAL( 0x21, itPair->second.m_attributes );
			ASSERT_EQUAL( CTime(), itPair->second.m_creationTime );
			ASSERT_EQUAL( CTime( 1992, 7, 17, 9, 30, 0 ), itPair->second.m_modifTime );
			ASSERT_EQUAL( CTime(), itPair->second.m_accessTime );
		}
	}
}

#include <fstream>

void CUndoChangeLogTests::TestSaveLog( void )
{
	CUndoChangeLog log;

	{
		std::istringstream iss( ut::s_inputLog );
		log.Load( iss );

		ASSERT_EQUAL( 5, log.GetRenameUndoStack().Get().size() );
		ASSERT_EQUAL( 1, log.GetTouchUndoStack().Get().size() );
	}

	{
		std::ostringstream oss;
		log.Save( oss );

		ASSERT( oss.str() == ut::s_outputLog );
	}

	// the ultimate roundtrip test
	{
		std::istringstream iss( ut::s_outputLog );
		CUndoChangeLog newLog;
		newLog.Load( iss );

		ASSERT( newLog.GetRenameUndoStack().Get() == log.GetRenameUndoStack().Get() );
		ASSERT( newLog.GetTouchUndoStack().Get() == log.GetTouchUndoStack().Get() );
	}
}

void CUndoChangeLogTests::Run( void )
{
	__super::Run();

	TestLoadLog();
	TestSaveLog();
}


#endif //_DEBUG
