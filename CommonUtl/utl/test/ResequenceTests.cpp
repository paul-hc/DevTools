
#include "stdafx.h"

#ifdef _DEBUG		// no UT code in release builds
#include "test/ResequenceTests.h"
#include "StringUtilities.h"

#define new DEBUG_NEW

#include "Resequence.hxx"


namespace ut
{
	std::string MoveBy( const char srcText[], const std::vector< int >& selIndexes, seq::Direction direction )
	{
		std::string chars = srcText;
		seq::CSequenceAdapter< char > sequence( &chars[ 0 ], chars.size() );

		seq::MoveBy( sequence, selIndexes, direction );
		return chars;
	}

	std::string Resequence( const char srcText[], const std::vector< int >& selIndexes, seq::MoveTo moveTo )
	{
		std::string chars = srcText;
		seq::CSequenceAdapter< char > sequence( &chars[ 0 ], chars.size() );

		seq::Resequence( sequence, selIndexes, moveTo );
		return chars;
	}

	std::string MakeDropSequence( const char srcText[], int dropIndex, const std::vector< int >& selIndexes )
	{
		std::vector< char > baselineSeq( srcText, str::end( srcText ) );
		std::vector< char > newSequence;

		seq::MakeDropSequence( newSequence, baselineSeq, dropIndex, selIndexes );
		return std::string( &newSequence.front(), newSequence.size() );
	}
}


CResequenceTests::CResequenceTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CResequenceTests& CResequenceTests::Instance( void )
{
	static CResequenceTests s_testCase;
	return s_testCase;
}

void CResequenceTests::TestCanMove( void )
{
	using namespace seq;

	ASSERT( !CanMoveIndex( 1, 0, MovePrev ) );
	ASSERT( !CanMoveIndex( 1, 0, MoveNext ) );
	ASSERT( !CanMoveIndex( 1, 0, MoveToStart ) );
	ASSERT( !CanMoveIndex( 1, 0, MoveToEnd ) );

	ASSERT( !CanMoveIndex( 2, 0, MovePrev ) );
	ASSERT( CanMoveIndex( 2, 0, MoveNext ) );
	ASSERT( !CanMoveIndex( 2, 0, MoveToStart ) );
	ASSERT( CanMoveIndex( 2, 0, MoveToEnd ) );
	ASSERT( CanMoveIndex( 2, 1, MovePrev ) );
	ASSERT( CanMoveIndex( 2, 1, MoveToStart ) );
}

void CResequenceTests::TestResequence( void )
{
	std::vector< int > sel_CE;
	sel_CE.push_back( 2 );		// 'C'
	sel_CE.push_back( 4 );		// 'E'

	ASSERT_EQUAL( "aCbEdfg", ut::MoveBy( "abCdEfg", sel_CE, seq::Prev ) );
	ASSERT_EQUAL( "abdCfEg", ut::MoveBy( "abCdEfg", sel_CE, seq::Next ) );

	ASSERT_EQUAL( "aCbEdfg", ut::Resequence( "abCdEfg", sel_CE, seq::MovePrev ) );
	ASSERT_EQUAL( "abdCfEg", ut::Resequence( "abCdEfg", sel_CE, seq::MoveNext ) );

	ASSERT_EQUAL( "CaEbdfg", ut::Resequence( "abCdEfg", sel_CE, seq::MoveToStart ) );
	ASSERT_EQUAL( "abdfCgE", ut::Resequence( "abCdEfg", sel_CE, seq::MoveToEnd ) );
}

void CResequenceTests::TestDropMove( void )
{
	{	// single selection
		std::vector< int > sel_C;
		sel_C.push_back( 2 );		// 'C'

		ASSERT( seq::ChangesDropSequenceAt( 7, 0, sel_C ) );
		ASSERT( seq::ChangesDropSequenceAt( 7, 1, sel_C ) );
		ASSERT( !seq::ChangesDropSequenceAt( 7, 2, sel_C ) );		// drop on itself (NIL for single selection)
		ASSERT( !seq::ChangesDropSequenceAt( 7, 3, sel_C ) );		// drop on next (still NIL for single selection)
		ASSERT( seq::ChangesDropSequenceAt( 7, 4, sel_C ) );

		ASSERT_EQUAL( "Cabdefg", ut::MakeDropSequence( "abCdefg", 0, sel_C ) );
		ASSERT_EQUAL( "aCbdefg", ut::MakeDropSequence( "abCdefg", 1, sel_C ) );
		ASSERT_EQUAL( "abCdefg", ut::MakeDropSequence( "abCdefg", 2, sel_C ) );		// drop on itself (NIL)
		ASSERT_EQUAL( "abCdefg", ut::MakeDropSequence( "abCdefg", 3, sel_C ) );		// drop on next (still NIL)

		ASSERT_EQUAL( "abdCefg", ut::MakeDropSequence( "abCdefg", 4, sel_C ) );
		ASSERT_EQUAL( "abdeCfg", ut::MakeDropSequence( "abCdefg", 5, sel_C ) );
		ASSERT_EQUAL( "abdefCg", ut::MakeDropSequence( "abCdefg", 6, sel_C ) );
		ASSERT_EQUAL( "abdefgC", ut::MakeDropSequence( "abCdefg", 7, sel_C ) );		// past end
	}

	{	// multiple selection
		std::vector< int > sel_CEF;
		sel_CEF.push_back( 2 );		// 'C'
		sel_CEF.push_back( 4 );		// 'E'
		sel_CEF.push_back( 5 );		// 'F'

		ASSERT( seq::ChangesDropSequenceAt( 7, 0, sel_CEF ) );
		ASSERT( seq::ChangesDropSequenceAt( 7, 1, sel_CEF ) );
		ASSERT( seq::ChangesDropSequenceAt( 7, 2, sel_CEF ) );
		ASSERT( seq::ChangesDropSequenceAt( 7, 3, sel_CEF ) );

		ASSERT_EQUAL( "CEFabdg", ut::MakeDropSequence( "abCdEFg", 0, sel_CEF ) );
		ASSERT_EQUAL( "aCEFbdg", ut::MakeDropSequence( "abCdEFg", 1, sel_CEF ) );
		ASSERT_EQUAL( "abCEFdg", ut::MakeDropSequence( "abCdEFg", 2, sel_CEF ) );

		ASSERT_EQUAL( "abCEFdg", ut::MakeDropSequence( "abCdEFg", 3, sel_CEF ) );		// same effect as drop at 2
		ASSERT_EQUAL( "abdCEFg", ut::MakeDropSequence( "abCdEFg", 4, sel_CEF ) );
		ASSERT_EQUAL( "abdCEFg", ut::MakeDropSequence( "abCdEFg", 5, sel_CEF ) );		// same effect as drop at 4
		ASSERT_EQUAL( "abdCEFg", ut::MakeDropSequence( "abCdEFg", 6, sel_CEF ) );		// same effect as drop at 4, 5
		ASSERT_EQUAL( "abdgCEF", ut::MakeDropSequence( "abCdEFg", 7, sel_CEF ) );		// drop past end
	}
}

void CResequenceTests::Run( void )
{
	__super::Run();

	TestCanMove();
	TestResequence();
	TestDropMove();
}


#endif //_DEBUG
