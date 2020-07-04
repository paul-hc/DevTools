
#include "stdafx.h"

#ifdef _DEBUG		// no UT code in release builds
#include "test/AlgorithmsTests.h"
#include "test/MockObject.h"
#include "ContainerUtilities.h"
#include "StringUtilities.h"
#include "vector_map.h"
#include <deque>

#define new DEBUG_NEW

#include "Resequence.hxx"


namespace pred
{
	struct IsEven
	{
		template< typename NumberT >
		bool operator()( NumberT number ) const
		{
			return 0 == ( number % NumberT( 2 ) );
		}
	};
}


const TCHAR CAlgorithmsTests::s_sep[] = _T(",");

CAlgorithmsTests::CAlgorithmsTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CAlgorithmsTests& CAlgorithmsTests::Instance( void )
{
	static CAlgorithmsTests s_testCase;
	return s_testCase;
}


void CAlgorithmsTests::TestBuffer( void )
{
}

void CAlgorithmsTests::TestLookup( void )
{
}

void CAlgorithmsTests::TestBinaryLookup( void )
{
}

void CAlgorithmsTests::TestIsOrdered( void )
{
	{	// values
		ASSERT( utl::IsOrdered( std::string( "" ) ) );
		ASSERT( utl::IsOrdered( std::string( "0" ) ) );

		// ascending
		ASSERT( utl::IsOrdered( std::string( "01" ) ) );
		ASSERT( utl::IsOrdered( std::string( "012" ) ) );
		ASSERT( utl::IsOrdered( std::string( "3579" ) ) );

		// descending
		ASSERT( utl::IsOrdered( std::string( "10" ) ) );
		ASSERT( utl::IsOrdered( std::string( "210" ) ) );
		ASSERT( utl::IsOrdered( std::string( "9753" ) ) );

		// unordered
		ASSERT( !utl::IsOrdered( std::string( "021" ) ) );
		ASSERT( !utl::IsOrdered( std::string( "1231" ) ) );
		ASSERT( !utl::IsOrdered( std::string( "3215" ) ) );
		ASSERT( !utl::IsOrdered( std::string( "2436" ) ) );
	}

	static fs::CPath f1( _T("f1") ), f2( _T("f2") ), f3( _T("f3") ), f4( _T("f4") ), f5( _T("f5") );

	{	// objects by value
		std::list< fs::CPath > files;
		pred::CompareNaturalPath pathCompare;

		ASSERT( utl::IsOrdered( files, pathCompare ) );

		files.push_back( f1 ); ASSERT( utl::IsOrdered( files, pathCompare ) );		// f1

		// ascending
		files.push_back( f2 ); ASSERT( utl::IsOrdered( files, pathCompare ) );		// f1, f2
		files.push_back( f3 ); ASSERT( utl::IsOrdered( files, pathCompare ) );		// f1, f2, f3
		files.push_back( f5 ); ASSERT( utl::IsOrdered( files, pathCompare ) );		// f1, f2, f3, f5

		// descending
		files.clear();
		files.push_back( f5 ); ASSERT( utl::IsOrdered( files, pathCompare ) );		// f5
		files.push_back( f3 ); ASSERT( utl::IsOrdered( files, pathCompare ) );		// f5, f3
		files.push_back( f2 ); ASSERT( utl::IsOrdered( files, pathCompare ) );		// f5, f3, f2
		files.push_back( f1 ); ASSERT( utl::IsOrdered( files, pathCompare ) );		// f5, f3, f2, f1

		// unordered
		files.clear(); files.push_back( f1 ); files.push_back( f2 ); files.push_back( f1 );
		ASSERT( !utl::IsOrdered( files, pathCompare ) );			// f1, f2, f1

		files.clear(); files.push_back( f1 ); files.push_back( f2 ); files.push_back( f3 ); files.push_back( f1 );
		ASSERT( !utl::IsOrdered( files, pathCompare ) );			// f1, f2, f3, f1

		files.clear(); files.push_back( f3 ); files.push_back( f2 ); files.push_back( f1 ); files.push_back( f5 );
		ASSERT( !utl::IsOrdered( files, pathCompare ) );			// f3, f2, f1, f5

		files.clear(); files.push_back( f3 ); files.push_back( f1 ); files.push_back( f2 ); files.push_back( f5 );
		ASSERT( !utl::IsOrdered( files, pathCompare ) );			// f3, f1, f2, f5
	}

	{	// objects by value
		std::deque< fs::CPath* > files;
		pred::CompareAdapterPtr< pred::CompareNaturalPath, func::PtrToReference > pathPtrCompare;

		ASSERT( utl::IsOrdered( files, pathPtrCompare ) );

		files.push_back( &f1 );
		ASSERT( utl::IsOrdered( files, pathPtrCompare ) );		// f1

		// ascending
		files.push_back( &f2 ); ASSERT( utl::IsOrdered( files, pathPtrCompare ) );		// f1, f2
		files.push_back( &f3 ); ASSERT( utl::IsOrdered( files, pathPtrCompare ) );		// f1, f2, f3
		files.push_back( &f5 ); ASSERT( utl::IsOrdered( files, pathPtrCompare ) );		// f1, f2, f3, f5

		// descending
		files.clear();
		files.push_back( &f5 ); ASSERT( utl::IsOrdered( files, pathPtrCompare ) );		// f5
		files.push_back( &f3 ); ASSERT( utl::IsOrdered( files, pathPtrCompare ) );		// f5, f3
		files.push_back( &f2 ); ASSERT( utl::IsOrdered( files, pathPtrCompare ) );		// f5, f3, f2
		files.push_back( &f1 ); ASSERT( utl::IsOrdered( files, pathPtrCompare ) );		// f5, f3, f2, f1

		// unordered
		files.clear(); files.push_back( &f1 ); files.push_back( &f2 ); files.push_back( &f1 );
		ASSERT( !utl::IsOrdered( files, pathPtrCompare ) );			// f1, f2, f1

		files.clear(); files.push_back( &f1 ); files.push_back( &f2 ); files.push_back( &f3 ); files.push_back( &f1 );
		ASSERT( !utl::IsOrdered( files, pathPtrCompare ) );			// f1, f2, f3, f1

		files.clear(); files.push_back( &f3 ); files.push_back( &f2 ); files.push_back( &f1 ); files.push_back( &f5 );
		ASSERT( !utl::IsOrdered( files, pathPtrCompare ) );			// f3, f2, f1, f5

		files.clear(); files.push_back( &f3 ); files.push_back( &f1 ); files.push_back( &f2 ); files.push_back( &f5 );
		ASSERT( !utl::IsOrdered( files, pathPtrCompare ) );			// f3, f1, f2, f5
	}
}

void CAlgorithmsTests::TestQuery( void )
{
	std::vector< fs::CPath > paths;
	str::Split( paths, _T("f1,f2,f3"), s_sep );

	{
		ASSERT_EQUAL( _T("f1"), utl::Front( paths ) );
		ASSERT_EQUAL( _T("f3"), utl::Back( paths ) );

		std::vector< fs::CPath > newPaths = paths;
		utl::Front( newPaths ) = fs::CPath( _T("s1") );
		utl::Back( newPaths ) = fs::CPath( _T("e3") );
		ASSERT_EQUAL( _T("s1,f2,e3"), str::Join( newPaths, s_sep ) );
	}
}

void CAlgorithmsTests::TestAssignment( void )
{
	std::vector< fs::CPath > paths;
	str::Split( paths, _T("f1,f2,f3"), s_sep );

	std::vector< fs::CPath > otherPaths;
	str::Split( otherPaths, _T("o1,o2"), s_sep );

	{
		std::set< std::tstring > files;

		utl::InsertFrom( std::inserter( files, files.end() ), paths, func::tor::StringOf() );
		ASSERT_EQUAL( _T("f1,f2,f3"), str::Join( files, s_sep ) );
	}

	{
		std::vector< std::tstring > files;

		utl::Assign( files, paths, func::tor::StringOf() );
		ASSERT_EQUAL( _T("f1,f2,f3"), str::Join( files, s_sep ) );

		utl::Append( files, otherPaths, func::tor::StringOf() );
		ASSERT_EQUAL( _T("f1,f2,f3,o1,o2"), str::Join( files, s_sep ) );

		utl::Prepend( files, otherPaths, func::tor::StringOf() );
		ASSERT_EQUAL( _T("o1,o2,f1,f2,f3,o1,o2"), str::Join( files, s_sep ) );
	}
}

void CAlgorithmsTests::TestInsert( void )
{
	{
		std::vector< int > numbers;
		utl::AddSorted( numbers, 5 ); ASSERT_EQUAL( _T("5"), str::Join( numbers, s_sep ) );
		utl::AddSorted( numbers, 9 ); ASSERT_EQUAL( _T("5,9"), str::Join( numbers, s_sep ) );
		utl::AddSorted( numbers, 1 ); ASSERT_EQUAL( _T("1,5,9"), str::Join( numbers, s_sep ) );

		ASSERT( !utl::AddUnique( numbers, 5 ) ); ASSERT_EQUAL( _T("1,5,9"), str::Join( numbers, s_sep ) );
		ASSERT( utl::AddUnique( numbers, 12 ) ); ASSERT_EQUAL( _T("1,5,9,12"), str::Join( numbers, s_sep ) );

		std::vector< int > moreNumbers;
		moreNumbers.push_back( 2 );
		moreNumbers.push_back( 5 );
		moreNumbers.push_back( 4 );

		utl::JoinUnique( numbers, moreNumbers.begin(), moreNumbers.end() );
		ASSERT_EQUAL( _T("1,5,9,12,2,4"), str::Join( numbers, s_sep ) );

		utl::PushUnique( numbers, 6, 2 );
		ASSERT_EQUAL( _T("1,5,6,9,12,2,4"), str::Join( numbers, s_sep ) );

		utl::PushUnique( numbers, 20 );
		ASSERT_EQUAL( _T("1,5,6,9,12,2,4,20"), str::Join( numbers, s_sep ) );
	}

	{
		std::vector< ut::TMockInt* > numbers;
		pred::LessPtr< pred::CompareMockItemPtr > lessPtr;

		utl::AddSorted( numbers, new ut::TMockInt( 5 ), lessPtr ); ASSERT_EQUAL( "5", ut::JoinPtrs( numbers, s_sep ) );
		utl::AddSorted( numbers, new ut::TMockInt( 9 ), lessPtr ); ASSERT_EQUAL( "5,9", ut::JoinPtrs( numbers, s_sep ) );
		utl::AddSorted( numbers, new ut::TMockInt( 1 ), lessPtr ); ASSERT_EQUAL( "1,5,9", ut::JoinPtrs( numbers, s_sep ) );

		std::vector< ut::TMockInt* > evenNumbers;
		evenNumbers.push_back( new ut::TMockInt( 6 ) );
		evenNumbers.push_back( new ut::TMockInt( 4 ) );
		evenNumbers.push_back( new ut::TMockInt( 8 ) );

		utl::AddSorted( numbers, evenNumbers.begin(), evenNumbers.end(), lessPtr ); ASSERT_EQUAL( "1,4,5,6,8,9", ut::JoinPtrs( numbers, s_sep ) );

		utl::ClearOwningContainer( numbers );
		ASSERT( !ut::CMockObject::HasInstances() );
	}

	{
		std::vector< fs::CPath > paths, dups;
		str::Split( paths, _T("1,2,3,2,1"), s_sep );

		utl::Uniquify< pred::IsEquivalentPath >( paths, &dups );
		ASSERT_EQUAL( "1,2,3", str::Join( paths, "," ) );
		ASSERT_EQUAL( "1,2", str::Join( dups, "," ) );
	}
}

void CAlgorithmsTests::TestRemove( void )
{
	static const char sep[] = ",";
	{
		std::vector< int > numbers;
		ut::SplitValues( numbers, "0,0,0,1,2,3,4,0,0,3,4,5,6,0", "," );
		ASSERT_EQUAL( "0,0,0,1,2,3,4,0,0,3,4,5,6,0", str::Join( numbers, sep ) );

		ASSERT_EQUAL( 6, utl::Remove( numbers, 0 ) );
		ASSERT_EQUAL( "1,2,3,4,3,4,5,6", str::Join( numbers, sep ) );
	}
	{
		std::vector< int > numbers;
		ASSERT_EQUAL( 0, utl::Uniquify( numbers ) );

		numbers.push_back( 1 );
		ASSERT_EQUAL( 0, utl::Uniquify( numbers ) );

		ut::SplitValues( numbers, "0,0,0,1,2,3,4,0,0,3,4,5,6,0", "," );
		ASSERT_EQUAL( "0,0,0,1,2,3,4,0,0,3,4,5,6,0", str::Join( numbers, sep ) );

		ASSERT_EQUAL( 7, utl::Uniquify( numbers ) );
		ASSERT_EQUAL( "0,1,2,3,4,5,6", str::Join( numbers, sep ) );
	}

	{
		std::vector< short > numbers( 10 );
		std::generate( numbers.begin(), numbers.end(), func::GenNumSeq< short >( 1 ) );
		ASSERT_EQUAL( "1,2,3,4,5,6,7,8,9,10", str::Join( numbers, sep ) );

		utl::RemoveIf( numbers, pred::IsEven() );

		utl::RemoveExisting( numbers, 5 );
		ASSERT_EQUAL( "1,3,7,9", str::Join( numbers, sep ) );

		ASSERT( !utl::RemoveValue( numbers, 10 ) );
		ASSERT_EQUAL( "1,3,7,9", str::Join( numbers, sep ) );

		ASSERT( utl::RemoveValue( numbers, 9 ) );
		ASSERT_EQUAL( "1,3,7", str::Join( numbers, sep ) );
	}

	{
		std::vector< int > left, right;
		ut::SplitValues( left, "1,2,3,4", sep );
		ut::SplitValues( right, "0,2,3,7,9", sep );
		ASSERT( !utl::EmptyIntersection( left, right ) );

		utl::RemoveIntersection( left, right );

		ASSERT( utl::EmptyIntersection( left, right ) );
		ASSERT_EQUAL( "1,4", str::Join( left, sep ) );
		ASSERT_EQUAL( "0,7,9", str::Join( right, sep ) );
	}

	{
		std::vector< int > left, right;
		ut::SplitValues( left, "0,2,3,7,9", sep );
		ut::SplitValues( right, "1,2,3,4", sep );

		ASSERT_EQUAL( 2, utl::RemoveLeftDuplicates( left, right ) );
		ASSERT_EQUAL( "0,7,9", str::Join( left, sep ) );
	}
}

void CAlgorithmsTests::TestMixedTypes( void )
{
	std::vector< ut::CMockObject* > mixedNumbers;

	utl::GenerateN( mixedNumbers, 3, func::GenNewMockSeq< int >( 1, 2 ) );				// 3 ut::CMockValue<int>, starting at 1 step 2
	ASSERT_EQUAL( "1,3,5", ut::JoinPtrs( mixedNumbers, s_sep ) );

	utl::GenerateN( mixedNumbers, 2, func::GenNewMockSeq< double >( 2.5, 2 ) );			// 2 ut::CMockValue<double>, starting at 2.5 step 2
	ASSERT_EQUAL( "1,3,5,2.5,4.5", ut::JoinPtrs( mixedNumbers, s_sep ) );

	utl::GenerateN( mixedNumbers, 2, func::GenNewMockSeq< double >( 0.5, 0.2 ), 0 );	// prepend 2 ut::CMockValue<double>, starting at 0.5 step 0.2
	ASSERT_EQUAL( "0.5,0.7,1,3,5,2.5,4.5", ut::JoinPtrs( mixedNumbers, s_sep ) );

	std::vector< ut::CMockValue< int >* > integers;
	utl::QueryWithType< ut::CMockValue< int > >( integers, mixedNumbers );
	ASSERT_EQUAL( "1,3,5", ut::JoinPtrs( integers, s_sep ) );

	std::vector< ut::CMockValue< double >* > doubles;
	utl::QueryWithType< ut::CMockValue< double > >( doubles, mixedNumbers );
	ASSERT_EQUAL( "0.5,0.7,2.5,4.5", ut::JoinPtrs( doubles, s_sep ) );

	{
		std::vector< ut::CMockValue< int >* > otherIntegers;
		utl::AddWithType< ut::CMockValue< int > >( otherIntegers, mixedNumbers );
		ASSERT_EQUAL( "1,3,5", ut::JoinPtrs( otherIntegers, s_sep ) );
	}

	{
		std::vector< ut::CMockValue< double >* > otherDoubles;
		utl::AddWithoutType< ut::CMockValue< int > >( otherDoubles, mixedNumbers );
		ASSERT_EQUAL( "0.5,0.7,2.5,4.5", ut::JoinPtrs( otherDoubles, s_sep ) );
	}

	{
		std::vector< ut::CMockObject* > otherMix = mixedNumbers;
		ASSERT_EQUAL( 0, utl::RemoveWithType< ut::CMockValue< short > >( otherMix ) );
		ASSERT_EQUAL( "0.5,0.7,1,3,5,2.5,4.5", ut::JoinPtrs( otherMix, s_sep ) );

		ASSERT_EQUAL( 4, utl::RemoveWithType< ut::CMockValue< double > >( otherMix ) );
		ASSERT_EQUAL( "1,3,5", ut::JoinPtrs( otherMix, s_sep ) );

		ASSERT_EQUAL( 3, utl::RemoveWithoutType< ut::CMockValue< double > >( otherMix ) );
		ASSERT_EQUAL( "", ut::JoinPtrs( otherMix, s_sep ) );
	}

	utl::ClearOwningContainer( mixedNumbers );
	ASSERT( !ut::CMockObject::HasInstances() );
}

void CAlgorithmsTests::TestCompareContents( void )
{
	static const char sep[] = ",";
	{
		std::vector< std::string > items1, items2, strayItems;
		str::Split( items1, "t1,t2,t3", sep );
		str::Split( items2, "t2,t3,t1", sep );
		str::Split( strayItems, "t,x,y,z", sep );

		ASSERT( items1 != items2 );
		ASSERT( utl::SameContents( items1, items2 ) );
		ASSERT( utl::SameContents( items2, items1 ) );
		ASSERT( !utl::SameContents( items1, strayItems ) );

		ASSERT( !utl::EmptyIntersection( items1, items2 ) );
		ASSERT( utl::EmptyIntersection( items2, strayItems ) );

		{
			std::vector< std::string > sequence;
			str::Split( sequence, "z,x", sep );

			std::vector< short > seqIndexes;
			utl::QuerySubSequenceIndexes( seqIndexes, strayItems, sequence );
			ASSERT_EQUAL( "3,1", str::Join( seqIndexes, "," ) );
		}
	}

	{
		std::vector< std::string > source;
		str::Split( source, "a,b,c,d", sep );

		std::vector< UINT > selIndexes;
		ut::SplitValues( selIndexes, "3,0,1", sep );

		std::vector< std::string > sequence;
		utl::QuerySubSequenceFromIndexes( sequence, source, selIndexes );
		ASSERT_EQUAL( "d,a,b", str::Join( sequence, sep ) );
	}

	{
		std::vector< fs::CPath > path1, path2, strayPaths;
		str::Split( path1, _T("p1,p2,p3"), s_sep );
		str::Split( path2, _T("p2,p3,p1"), s_sep );
		str::Split( strayPaths, _T("x,y,z"), s_sep );

		ASSERT( path1 != path2 );
		ASSERT( utl::SameContents( path1, path2, pred::IsPathEquivalent() ) );
		ASSERT( !utl::SameContents( path1, strayPaths, pred::IsPathEquivalent() ) );
	}
}

void CAlgorithmsTests::TestAdvancePos( void )
{
}

void CAlgorithmsTests::TestOwningContainer( void )
{
}

void CAlgorithmsTests::Test_vector_map( void )
{
	utl::vector_map< int, std::tstring > items;
	items[ 7 ] = _T("i7");
	items[ 9 ] = _T("i9");
	items[ 3 ] = _T("i3");
	items[ 1 ] = _T("i1");
	ASSERT_EQUAL( "1 3 7 9", ut::JoinKeys( items, _T(" ") ) );
	ASSERT_EQUAL( "i1,i3,i7,i9", ut::JoinValues( items, _T(",") ) );

	ASSERT( items.find( 3 ) != items.end() );
	ASSERT_EQUAL( _T("i3"), items.find( 3 )->second );

	ASSERT( items.find( 4 ) == items.end() );

	items.EraseKey( 3 );
	ASSERT_EQUAL( "1 7 9", ut::JoinKeys( items, _T(" ") ) );

	items[ 7 ] = _T("i7 B");
	ASSERT_EQUAL( _T("i7 B"), items.find( 7 )->second );

	items[ 5 ] = _T("i5");
	ASSERT_EQUAL( _T("i5"), items.find( 5 )->second );
}


void CAlgorithmsTests::Run( void )
{
	__super::Run();

	TestBuffer();
	TestLookup();
	TestBinaryLookup();
	TestIsOrdered();
	TestQuery();
	TestAssignment();
	TestInsert();
	TestRemove();
	TestMixedTypes();
	TestCompareContents();
	TestAdvancePos();
	TestOwningContainer();
	Test_vector_map();
}


#endif //_DEBUG
