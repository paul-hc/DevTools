
#include "stdafx.h"

#ifdef _DEBUG		// no UT code in release builds
#include "ut/ResequenceTests.h"
#include "Resequence.hxx"
#include "StringUtilities.h"

#define new DEBUG_NEW


namespace ut
{
	std::tstring MoveBy( const TCHAR srcText[], const std::vector< int >& selIndexes, seq::Direction direction )
	{
		std::tstring chars = srcText;
		seq::CArraySequence< TCHAR > sequence( &chars[ 0 ], chars.size() );

		seq::MoveBy( sequence, selIndexes, direction );
		return chars;
	}

	std::tstring Resequence( const TCHAR srcText[], const std::vector< int >& selIndexes, seq::MoveTo moveTo )
	{
		std::tstring chars = srcText;
		seq::CArraySequence< TCHAR > sequence( &chars[ 0 ], chars.size() );

		seq::Resequence( sequence, selIndexes, moveTo );
		return chars;
	}

	std::tstring MakeDropSequence( const TCHAR srcText[], int dropIndex, const std::vector< int >& selIndexes )
	{
		std::vector< TCHAR > baselineSeq( srcText, str::end( srcText ) );
		std::vector< TCHAR > newSequence;

		seq::MakeDropSequence( newSequence, baselineSeq, dropIndex, selIndexes );
		return std::tstring( &newSequence.front(), newSequence.size() );
	}
}


CResequenceTests::CResequenceTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CResequenceTests& CResequenceTests::Instance( void )
{
	static CResequenceTests testCase;
	return testCase;
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
	std::vector< int > selIndexes;
	selIndexes.push_back( 2 );		// 'c'
	selIndexes.push_back( 4 );		// 'e'

	ASSERT_EQUAL( _T("AcBeDFG"), ut::MoveBy( _T("ABcDeFG"), selIndexes, seq::Prev ) );
	ASSERT_EQUAL( _T("ABDcFeG"), ut::MoveBy( _T("ABcDeFG"), selIndexes, seq::Next ) );

	ASSERT_EQUAL( _T("AcBeDFG"), ut::Resequence( _T("ABcDeFG"), selIndexes, seq::MovePrev ) );
	ASSERT_EQUAL( _T("ABDcFeG"), ut::Resequence( _T("ABcDeFG"), selIndexes, seq::MoveNext ) );

	ASSERT_EQUAL( _T("cAeBDFG"), ut::Resequence( _T("ABcDeFG"), selIndexes, seq::MoveToStart ) );
	ASSERT_EQUAL( _T("ABDFcGe"), ut::Resequence( _T("ABcDeFG"), selIndexes, seq::MoveToEnd ) );
}

void CResequenceTests::TestDropMove( void )
{
	{
		std::vector< int > selIndexes;
		selIndexes.push_back( 2 );		// 'c'

		ASSERT( seq::ChangesDropSequenceAt( 7, 0, selIndexes ) );
		ASSERT( seq::ChangesDropSequenceAt( 7, 1, selIndexes ) );
		ASSERT( !seq::ChangesDropSequenceAt( 7, 2, selIndexes ) );
		ASSERT( !seq::ChangesDropSequenceAt( 7, 3, selIndexes ) );
		ASSERT( seq::ChangesDropSequenceAt( 7, 4, selIndexes ) );

		ASSERT_EQUAL( _T("cABDeFG"), ut::MakeDropSequence( _T("ABcDeFG"), 0, selIndexes ) );
		ASSERT_EQUAL( _T("AcBDeFG"), ut::MakeDropSequence( _T("ABcDeFG"), 1, selIndexes ) );
		ASSERT_EQUAL( _T("ABcDeFG"), ut::MakeDropSequence( _T("ABcDeFG"), 2, selIndexes ) );
		ASSERT_EQUAL( _T("ABcDeFG"), ut::MakeDropSequence( _T("ABcDeFG"), 3, selIndexes ) );
		ASSERT_EQUAL( _T("ABDceFG"), ut::MakeDropSequence( _T("ABcDeFG"), 4, selIndexes ) );
		ASSERT_EQUAL( _T("ABDecFG"), ut::MakeDropSequence( _T("ABcDeFG"), 5, selIndexes ) );
		ASSERT_EQUAL( _T("ABDeFcG"), ut::MakeDropSequence( _T("ABcDeFG"), 6, selIndexes ) );
		ASSERT_EQUAL( _T("ABDeFGc"), ut::MakeDropSequence( _T("ABcDeFG"), 7, selIndexes ) );
	}

	{
		std::vector< int > selIndexes;
		selIndexes.push_back( 2 );		// 'c'
		selIndexes.push_back( 4 );		// 'e'
		selIndexes.push_back( 5 );		// 'f'

		ASSERT( seq::ChangesDropSequenceAt( 7, 0, selIndexes ) );
		ASSERT( seq::ChangesDropSequenceAt( 7, 2, selIndexes ) );

		ASSERT_EQUAL( _T("cefABDG"), ut::MakeDropSequence( _T("ABcDefG"), 0, selIndexes ) );
		ASSERT_EQUAL( _T("AcefBDG"), ut::MakeDropSequence( _T("ABcDefG"), 1, selIndexes ) );
		ASSERT_EQUAL( _T("ABcefDG"), ut::MakeDropSequence( _T("ABcDefG"), 2, selIndexes ) );
//		ASSERT_EQUAL( _T("ABDcefG"), ut::MakeDropSequence( _T("ABcDefG"), 3, selIndexes ) );
//		ASSERT_EQUAL( _T("ABDGcef"), ut::MakeDropSequence( _T("ABcDefG"), 4, selIndexes ) );
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
