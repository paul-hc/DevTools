
#include "stdafx.h"
#include "CommandModelSerializerTests.h"
#include "CommandModelSerializer.h"
#include "FileCommands.h"
#include "utl/ContainerUtilities.h"
#include "utl/FileSystem.h"
#include "utl/FmtUtils.h"
#include "utl/TimeUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#ifdef _DEBUG		// no UT code in release builds


namespace ut
{
	static const std::string s_inputLog =
		"  <RENAME> \t \n"
		" C:\\my\\download\\scan\\doctor.pdf  -> \tDr. Metz.pdf  \t\n"
		"C:\\my\\download\\scan\\doctor2.pdf -> Dr. Metz2.pdf\n"
		"<END OF BATCH>\n"
		"\n"
		"<RENAME>\n"
		"C:\\my\\download\\scan\\doctor3.pdf -> Dr. Metz3.pdf\n"
		"<END OF BATCH>\n"
		"\n"
		"<TOUCH 17-10-2005 8:00:00>\n"
		"C:\\my\\download\\file.txt :: {RHSA|17-07-1992 9:21:17||} -> {RA||17-07-1992 9:30:00|}\n"
		"C:\\my\\download\\info.txt :: {A|31-03-2004 7:30:00||} -> {HS|||1-04-2004 9:30:53}\n"
		"<END OF BATCH>\n"
		"\n"
		"<RENAME 03-10-2008 13:00:00>\n"
		"C:\\my\\download\\kierkegaard_works.PDF -> Kierkegaard Works.pdf\n"
		"C:\\my\\download\\water-tax.Doc -> Water Tax.Doc\n"
		"<END OF BATCH>\n"
		" <RENAME 8-06-2010 14:30:00>\t \t\n"
		"C:\\my\\download\\Apt Barbu.PDF -> Apt Barbu - confirmation.pdf\n"
		"<END OF BATCH>\n"
		"\n"
		"<RENAME 16-05-2012 19:21:02>\n"
		"C:\\my\\download\\BA Itinerary.pdf -> BA My Itinerary.pdf\n"
		"<END OF BATCH>\n"
		"\n"
		"<TOUCH 01-07-2018 8:00:00>\n"
		"C:\\my\\download\\exams.png :: {0x20|17-07-1992 9:21:17||} -> {0x21||17-07-1992 9:30:00|}\n"
		"<END OF BATCH>\n"
		"\n"
		"[REDO SECTION]\n"
		"<RENAME 17-07-2018 17:00:00>\n"
		"  C:\\my\\download\\How to Talk.pdf ->  How to Talk to Anyone.pdf.pdf \n"
		"<END OF BATCH>\n"
		;

	static const std::string s_outputLog =
		"[UNDO SECTION]\n"
		"<RENAME>\n"
		"C:\\my\\download\\scan\\doctor.pdf -> Dr. Metz.pdf\n"
		"C:\\my\\download\\scan\\doctor2.pdf -> Dr. Metz2.pdf\n"
		"<END OF BATCH>\n"
		"\n"
		"<RENAME>\n"
		"C:\\my\\download\\scan\\doctor3.pdf -> Dr. Metz3.pdf\n"
		"<END OF BATCH>\n"
		"\n"
		"<TOUCH 17-10-2005 08:00:00>\n"
		"C:\\my\\download\\file.txt :: {RHSA|17-07-1992 09:21:17||} -> {RA||17-07-1992 09:30:00|}\n"
		"C:\\my\\download\\info.txt :: {A|31-03-2004 07:30:00||} -> {HS|||01-04-2004 09:30:53}\n"
		"<END OF BATCH>\n"
		"\n"
		"<RENAME 03-10-2008 13:00:00>\n"
		"C:\\my\\download\\kierkegaard_works.PDF -> Kierkegaard Works.pdf\n"
		"C:\\my\\download\\water-tax.Doc -> Water Tax.Doc\n"
		"<END OF BATCH>\n"
		"\n"
		"<RENAME 08-06-2010 14:30:00>\n"
		"C:\\my\\download\\Apt Barbu.PDF -> Apt Barbu - confirmation.pdf\n"
		"<END OF BATCH>\n"
		"\n"
		"<RENAME 16-05-2012 19:21:02>\n"
		"C:\\my\\download\\BA Itinerary.pdf -> BA My Itinerary.pdf\n"
		"<END OF BATCH>\n"
		"\n"
		"<TOUCH 01-07-2018 08:00:00>\n"
		"C:\\my\\download\\exams.png :: {A|17-07-1992 09:21:17||} -> {RA||17-07-1992 09:30:00|}\n"
		"<END OF BATCH>\n"
		"\n"
		"[REDO SECTION]\n"
		"\n"
		"<RENAME 17-07-2018 17:17:17>\n"
		"C:\\my\\download\\How to Talk.pdf -> How to Talk to Anyone.pdf.pdf\n"
		"<END OF BATCH>\n"
		;
}


CCommandModelSerializerTests::CCommandModelSerializerTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CCommandModelSerializerTests& CCommandModelSerializerTests::Instance( void )
{
	static CCommandModelSerializerTests testCase;
	return testCase;
}

void CCommandModelSerializerTests::TestLoadLog( void )
{
	CCommandModelSerializer serializer;
	CCommandModel model;

	std::istringstream iss( ut::s_inputLog );
	serializer.Load( iss, &model );

	// UNDO section:
	ASSERT_EQUAL( 7, model.GetUndoStack().size() );

	std::deque< utl::ICommand* >::const_iterator itStackCmd = model.GetUndoStack().begin();
	std::vector< utl::ICommand* >::const_iterator itSubCmd;

	const cmd::CFileMacroCmd* pMacro;
	const CRenameFileCmd* pRenameCmd;
	const CTouchFileCmd* pTouchCmd;

	{
		pMacro = checked_static_cast< const cmd::CFileMacroCmd* >( *itStackCmd++ );
		itSubCmd = pMacro->GetSubCommands().begin();
		ASSERT_EQUAL( cmd::RenameFile, pMacro->GetTypeID() );
		ASSERT_EQUAL( CTime(), pMacro->GetTimestamp() );
		ASSERT_EQUAL( 2, pMacro->GetSubCommands().size() );

		pRenameCmd = checked_static_cast< const CRenameFileCmd* >( *itSubCmd++ );
		ASSERT_EQUAL( cmd::RenameFile, pRenameCmd->GetTypeID() );
		ASSERT_EQUAL( _T("C:\\my\\download\\scan\\doctor.pdf"), pRenameCmd->m_srcPath );
		ASSERT_EQUAL( _T("C:\\my\\download\\scan\\Dr. Metz.pdf"), pRenameCmd->m_destPath );

		pRenameCmd = checked_static_cast< const CRenameFileCmd* >( *itSubCmd++ );
		ASSERT_EQUAL( _T("C:\\my\\download\\scan\\doctor2.pdf"), pRenameCmd->m_srcPath );
		ASSERT_EQUAL( _T("C:\\my\\download\\scan\\Dr. Metz2.pdf"), pRenameCmd->m_destPath );
	}
	{
		pMacro = checked_static_cast< const cmd::CFileMacroCmd* >( *itStackCmd++ );
		itSubCmd = pMacro->GetSubCommands().begin();
		ASSERT_EQUAL( cmd::RenameFile, pMacro->GetTypeID() );
		ASSERT_EQUAL( CTime(), pMacro->GetTimestamp() );
		ASSERT_EQUAL( 1, pMacro->GetSubCommands().size() );

		pRenameCmd = checked_static_cast< const CRenameFileCmd* >( *itSubCmd++ );
		ASSERT_EQUAL( _T("C:\\my\\download\\scan\\doctor3.pdf"), pRenameCmd->m_srcPath );
		ASSERT_EQUAL( _T("C:\\my\\download\\scan\\Dr. Metz3.pdf"), pRenameCmd->m_destPath );
	}
	{
		pMacro = checked_static_cast< const cmd::CFileMacroCmd* >( *itStackCmd++ );
		itSubCmd = pMacro->GetSubCommands().begin();
		ASSERT_EQUAL( cmd::TouchFile, pMacro->GetTypeID() );
		ASSERT_EQUAL( CTime( 2005, 10, 17, 8, 0, 0 ), pMacro->GetTimestamp() );
		ASSERT_EQUAL( 2, pMacro->GetSubCommands().size() );

		{
			pTouchCmd = checked_static_cast< const CTouchFileCmd* >( *itSubCmd++ );

			ASSERT_EQUAL( _T("C:\\my\\download\\file.txt"), pTouchCmd->m_srcPath );
			ASSERT_EQUAL( pTouchCmd->m_srcPath, pTouchCmd->m_srcState.m_fullPath );

			ASSERT_EQUAL( _T("RHSA"), fmt::FormatFileAttributes( pTouchCmd->m_srcState.m_attributes ) );
			ASSERT_EQUAL( CTime( 1992, 7, 17, 9, 21, 17 ), pTouchCmd->m_srcState.m_creationTime );
			ASSERT_EQUAL( CTime(), pTouchCmd->m_srcState.m_modifTime );
			ASSERT_EQUAL( CTime(), pTouchCmd->m_srcState.m_accessTime );

			ASSERT_EQUAL( pTouchCmd->m_srcPath, pTouchCmd->m_destState.m_fullPath );
			ASSERT_EQUAL( _T("RA"), fmt::FormatFileAttributes( pTouchCmd->m_destState.m_attributes ) );
			ASSERT_EQUAL( CTime(), pTouchCmd->m_destState.m_creationTime );
			ASSERT_EQUAL( CTime( 1992, 7, 17, 9, 30, 0 ), pTouchCmd->m_destState.m_modifTime );
			ASSERT_EQUAL( CTime(), pTouchCmd->m_destState.m_accessTime );
		}

		{
			pTouchCmd = checked_static_cast< const CTouchFileCmd* >( *itSubCmd++ );

			ASSERT_EQUAL( _T("C:\\my\\download\\info.txt"), pTouchCmd->m_srcPath );
			ASSERT_EQUAL( pTouchCmd->m_srcPath, pTouchCmd->m_srcState.m_fullPath );

			ASSERT_EQUAL( _T("A"), fmt::FormatFileAttributes( pTouchCmd->m_srcState.m_attributes ) );
			ASSERT_EQUAL( CTime( 2004, 3, 31, 7, 30, 0 ), pTouchCmd->m_srcState.m_creationTime );
			ASSERT_EQUAL( CTime(), pTouchCmd->m_srcState.m_modifTime );
			ASSERT_EQUAL( CTime(), pTouchCmd->m_srcState.m_accessTime );

			ASSERT_EQUAL( pTouchCmd->m_srcPath, pTouchCmd->m_destState.m_fullPath );
			ASSERT_EQUAL( _T("HS"), fmt::FormatFileAttributes( pTouchCmd->m_destState.m_attributes ) );
			ASSERT_EQUAL( CTime(), pTouchCmd->m_destState.m_creationTime );
			ASSERT_EQUAL( CTime(), pTouchCmd->m_destState.m_modifTime );
			ASSERT_EQUAL( CTime( 2004, 4, 1, 9, 30, 53 ), pTouchCmd->m_destState.m_accessTime );
		}
	}
	{
		pMacro = checked_static_cast< const cmd::CFileMacroCmd* >( *itStackCmd++ );
		itSubCmd = pMacro->GetSubCommands().begin();
		ASSERT_EQUAL( cmd::RenameFile, pMacro->GetTypeID() );
		ASSERT_EQUAL( CTime( 2008, 10, 3, 13, 0, 0 ), pMacro->GetTimestamp() );
		ASSERT_EQUAL( 2, pMacro->GetSubCommands().size() );

		pRenameCmd = checked_static_cast< const CRenameFileCmd* >( *itSubCmd++ );
		ASSERT_EQUAL( _T("C:\\my\\download\\kierkegaard_works.PDF"), pRenameCmd->m_srcPath );
		ASSERT_EQUAL( _T("C:\\my\\download\\Kierkegaard Works.pdf"), pRenameCmd->m_destPath );

		pRenameCmd = checked_static_cast< const CRenameFileCmd* >( *itSubCmd++ );
		ASSERT_EQUAL( _T("C:\\my\\download\\water-tax.Doc"), pRenameCmd->m_srcPath );
		ASSERT_EQUAL( _T("C:\\my\\download\\Water Tax.Doc"), pRenameCmd->m_destPath );
	}
	{
		pMacro = checked_static_cast< const cmd::CFileMacroCmd* >( *itStackCmd++ );
		itSubCmd = pMacro->GetSubCommands().begin();
		ASSERT_EQUAL( cmd::RenameFile, pMacro->GetTypeID() );
		ASSERT_EQUAL( CTime( 2010, 6, 8, 14, 30, 0 ), pMacro->GetTimestamp() );
		ASSERT_EQUAL( 1, pMacro->GetSubCommands().size() );

		pRenameCmd = checked_static_cast< const CRenameFileCmd* >( *itSubCmd++ );
		ASSERT_EQUAL( _T("C:\\my\\download\\Apt Barbu.PDF"), pRenameCmd->m_srcPath );
		ASSERT_EQUAL( _T("C:\\my\\download\\Apt Barbu - confirmation.pdf"), pRenameCmd->m_destPath );
	}
	{
		pMacro = checked_static_cast< const cmd::CFileMacroCmd* >( *itStackCmd++ );
		itSubCmd = pMacro->GetSubCommands().begin();
		ASSERT_EQUAL( cmd::RenameFile, pMacro->GetTypeID() );
		ASSERT_EQUAL( CTime( 2012, 5, 16, 19, 21, 2 ), pMacro->GetTimestamp() );
		ASSERT_EQUAL( 1, pMacro->GetSubCommands().size() );

		pRenameCmd = checked_static_cast< const CRenameFileCmd* >( *itSubCmd++ );
		ASSERT_EQUAL( _T("C:\\my\\download\\BA Itinerary.pdf"), pRenameCmd->m_srcPath );
		ASSERT_EQUAL( _T("C:\\my\\download\\BA My Itinerary.pdf"), pRenameCmd->m_destPath );
	}
	{
		pMacro = checked_static_cast< const cmd::CFileMacroCmd* >( *itStackCmd++ );
		itSubCmd = pMacro->GetSubCommands().begin();
		ASSERT_EQUAL( cmd::TouchFile, pMacro->GetTypeID() );
		ASSERT_EQUAL( CTime( 2018, 7, 1, 8, 0, 0 ), pMacro->GetTimestamp() );
		ASSERT_EQUAL( 1, pMacro->GetSubCommands().size() );

		{
			pTouchCmd = checked_static_cast< const CTouchFileCmd* >( *itSubCmd++ );

			ASSERT_EQUAL( _T("C:\\my\\download\\exams.png"), pTouchCmd->m_srcPath );
			ASSERT_EQUAL( pTouchCmd->m_srcPath, pTouchCmd->m_srcState.m_fullPath );

			ASSERT_EQUAL( 0x20, pTouchCmd->m_srcState.m_attributes );
			ASSERT_EQUAL( CTime( 1992, 7, 17, 9, 21, 17 ), pTouchCmd->m_srcState.m_creationTime );
			ASSERT_EQUAL( CTime(), pTouchCmd->m_srcState.m_modifTime );
			ASSERT_EQUAL( CTime(), pTouchCmd->m_srcState.m_accessTime );

			ASSERT_EQUAL( pTouchCmd->m_srcPath, pTouchCmd->m_destState.m_fullPath );
			ASSERT_EQUAL( 0x21, pTouchCmd->m_destState.m_attributes );
			ASSERT_EQUAL( CTime(), pTouchCmd->m_destState.m_creationTime );
			ASSERT_EQUAL( CTime( 1992, 7, 17, 9, 30, 0 ), pTouchCmd->m_destState.m_modifTime );
			ASSERT_EQUAL( CTime(), pTouchCmd->m_destState.m_accessTime );
		}
	}


	// REDO section:
	ASSERT_EQUAL( 1, model.GetRedoStack().size() );
	itStackCmd = model.GetRedoStack().begin();

	{
		pMacro = checked_static_cast< const cmd::CFileMacroCmd* >( *itStackCmd++ );
		itSubCmd = pMacro->GetSubCommands().begin();
		ASSERT_EQUAL( cmd::RenameFile, pMacro->GetTypeID() );
		ASSERT_EQUAL( CTime( 2018, 7, 17, 17, 17, 17 ), pMacro->GetTimestamp() );
		ASSERT_EQUAL( 1, pMacro->GetSubCommands().size() );

		pRenameCmd = checked_static_cast< const CRenameFileCmd* >( *itSubCmd++ );
		ASSERT_EQUAL( cmd::RenameFile, pRenameCmd->GetTypeID() );
		ASSERT_EQUAL( _T("C:\\my\\download\\How to Talk.pdf"), pRenameCmd->m_srcPath );
		ASSERT_EQUAL( _T("How to Talk to Anyone.pdf.pdf"), pRenameCmd->m_destPath );
	}
}

void CCommandModelSerializerTests::TestSaveLog( void )
{
	CCommandModelSerializer serializer;
	CCommandModel model;

	{
		std::istringstream iss( ut::s_inputLog );
		serializer.Load( iss, &model );

		ASSERT_EQUAL( 7, model.GetUndoStack().size() );
	}

	{
		std::ostringstream oss;
		serializer.Save( oss, model );

		ASSERT( oss.str() == ut::s_outputLog );
	}

	// roundtrip test
	{
		CCommandModelSerializer newSerializer;
		CCommandModel newModel;

		std::istringstream iss( ut::s_outputLog );
		newSerializer.Load( iss, &newModel );

		ASSERT( model.GetUndoStack().size() == newModel.GetUndoStack().size() );
	}
}

void CCommandModelSerializerTests::Run( void )
{
	__super::Run();

	TestLoadLog();
	TestSaveLog();
}


#endif //_DEBUG
