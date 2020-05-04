
#include "stdafx.h"

#ifdef _DEBUG		// no UT code in release builds
#include "ut/AlgorithmsTests.h"
#include "ContainerUtilities.h"
#include <deque>

#define new DEBUG_NEW


CAlgorithmsTests::CAlgorithmsTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CAlgorithmsTests& CAlgorithmsTests::Instance( void )
{
	static CAlgorithmsTests s_testCase;
	return s_testCase;
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

void CAlgorithmsTests::Run( void )
{
	__super::Run();

	TestIsOrdered();
}


#endif //_DEBUG
