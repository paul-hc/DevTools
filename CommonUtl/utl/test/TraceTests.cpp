
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "test/TraceTests.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif



CTraceTests::CTraceTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CTraceTests& CTraceTests::Instance( void )
{
	static CTraceTests s_testCase;
	return s_testCase;
}

void CTraceTests::TestTracing( void )
{
	// check Debug Output window for tracing results:
	TRACE_( "\n" );

	{
		ATLTRACE( "'ATLTRACE-a'\n" );
		ATLTRACE( L"'ATLTRACE-w'\n" );

		TRACE( "'TRACE-a'\n" );
		TRACE( L"'TRACE-w'\n" );
		TRACE_( "'TRACE_-a'\n" );
		TRACE_( L"'TRACE_-w'\n" );
		TRACE_FL( "'TRACE_FL-a'\n" );
		TRACE_FL( L"'TRACE_FL-w'\n" );
		TRACE_FL2( "'TRACE_FL2-a'\n" );
		TRACE_FL2( L"'TRACE_FL2-w'\n" );
		TRACE_FL3( "'TRACE_FL3-a'\n" );
		TRACE_FL3( L"'TRACE_FL3-w'\n" );
	}

	{
		std::vector<std::string> aItems;
		std::vector<std::wstring> wItems;
		std::vector<fs::CPath> pathItems;

		str::Split( aItems, "apple|grape|plum|orange|mellon", "|" );
		str::Split( wItems, L"APPLE|GRAPE|PLUM|ORANGE|MELLON", L"|" );
		str::Split( pathItems, L"C:\\|C:\\my\\file-1.h|C:/dev/code/DevTools/CommonUtl/utl/Algorithms.h", L"|" );

		TRACE_ITEMS( aItems );
		TRACE_ITEMS( wItems, "Wide Items" );
		TRACE_ITEMS( pathItems, "Path Items" );

		TRACE_ITEMS( wItems, "Wide Items", 3 );
		TRACE_ITEMS( wItems, "Wide Items", 2 );
		TRACE_ITEMS( wItems, "Wide Items", wItems.size() );
	}

	// TRACE with preprocessor operators: stringify and token-pasting:
	{
		TRACE_( "\nSTRINGIFY(Message): " );
		TRACE_( STRINGIFY( Message ) );

		TRACE_( "\nT_STRINGIFY(Message): " );
		TRACE_( T_STRINGIFY( Message ) );

		TRACE_( "\nT_CAT2(Left, Right): " );
		TRACE_( T_CAT2( STRINGIFY( Left ), STRINGIFY( Right ) ) );

		TRACE_( "\nT_CAT3(Left, Mid, Right): " );
		TRACE_( T_CAT3( STRINGIFY( Left ), STRINGIFY( Mid ), STRINGIFY( Right ) ) );

		TRACE_( "\nT_CAT2(\"Left\", \"Right\"): " );
		TRACE_( T_CAT2( "Left", "Right" ) );

		TRACE_( "\nT_CAT3(\"Left\", \"Mid\", \"Right\"): " );
		TRACE_( T_CAT3( "Left", "Mid", "Right" ) );

		TRACE_( "\n" );
	}
}


void CTraceTests::Run( void )
{
	RUN_TEST( TestTracing );
}


#endif //USE_UT
