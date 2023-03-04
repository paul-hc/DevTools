
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "test/EnvironmentTests.h"
#include "StringParsing.h"
#include "StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace func
{
	struct KeyToValue
	{
		std::tstring operator()( const std::tstring& key ) const
		{
			if ( key == _T("NAME") )
				return _T("UTL");
			else if ( key == _T("MAJOR") )
				return _T("3");
			else if ( key == _T("MINOR") )
				return _T("12");
			return str::Format( _T("?%s?"), key.c_str() );
		}
	};
}


CEnvironmentTests::CEnvironmentTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CEnvironmentTests& CEnvironmentTests::Instance( void )
{
	static CEnvironmentTests s_testCase;
	return s_testCase;
}

void CEnvironmentTests::TestQueryEnclosedIdentifiers( void )
{
	// namespace str => any enclosed item, regardless of syntax
	{
		std::vector<std::string> items;
		str::QueryEnclosedItems( items, std::string("lead_%MY_STUFF%_mid_%MY_TOOLS%_trail"), "%", "%", false );
		ASSERT_EQUAL( "MY_STUFF,MY_TOOLS", str::Join( items, "," ) );
	}
	{
		std::vector<std::string> items;
		str::QueryEnclosedItems( items, std::string("lead_%MY_STUFF%_mid_%MY_T.OOLS%_trail"), "%", "%", true, false );
		ASSERT_EQUAL( "%MY_STUFF%,%MY_T.OOLS%", str::Join( items, "," ) );
	}

	// namespace code => only enclosed identifiers (A..Z|a..z|0..9|_)
	{
		std::vector<std::string> identifiers;
		code::QueryEnclosedIdentifiers( identifiers, std::string("lead_%MY_STUFF%_mid_%MY_TOOLS%_trail"), "%", "%", false );
		ASSERT_EQUAL( "MY_STUFF,MY_TOOLS", str::Join( identifiers, "," ) );
	}
	{
		std::vector<std::string> identifiers;
		code::QueryEnclosedIdentifiers( identifiers, std::string("lead_%MY_STUFF%_mid_%MY_T.OOLS%_trail"), "%", "%", true );
		ASSERT_EQUAL( "%MY_STUFF%", str::Join( identifiers, "," ) );		// exclude "MY_T.OOLS" - not a identifier
	}
	{
		std::vector<std::string> identifiers;
		code::QueryEnclosedIdentifiers( identifiers, std::string("lead_%MY_STUFF%_mid_%MY_TOOLS%_trail"), "%", "%", true );
		ASSERT_EQUAL( "%MY_STUFF%,%MY_TOOLS%", str::Join( identifiers, "," ) );
	}
}

void CEnvironmentTests::TestEnvironVariables( void )
{
	ASSERT( !env::HasAnyVariable( _T("") ) );
	ASSERT( !env::HasAnyVariable( _T("%%") ) );			// empty identifier
	ASSERT( !env::HasAnyVariable( _T("$()") ) );		// empty identifier

	ASSERT( env::HasAnyVariable( _T("%MY_TOOLS%") ) );
	ASSERT( env::HasAnyVariable( _T("lead_%MY_TOOLS%_trail") ) );
	ASSERT( env::HasAnyVariable( _T("lead_$(MY_TOOLS)_trail") ) );

	// not enclosed:
	ASSERT( !env::HasAnyVariable( _T("%MY_TOOLS") ) );
	ASSERT( !env::HasAnyVariable( _T("MY_TOOLS%") ) );
	ASSERT( !env::HasAnyVariable( _T("&(MY_TOOLS") ) );
	ASSERT( !env::HasAnyVariable( _T("MY_TOOLS)") ) );

	ASSERT( !env::HasAnyVariable( _T("%MY_TOO.LS%") ) );	// invalid identifier

	{
		// set 2 environment variables in this process
		env::SetVariableValue( _T("_VAR1_"), _T("X:\\Dir_A") );
		env::SetVariableValue( _T("_VAR2_"), _T("Y:\\Dir_B") );

		const TCHAR* pSource = _T("|%_VAR1_%|$(_VAR2_)|");

		std::tstring expanded = env::ExpandPaths( pSource );
		ASSERT_EQUAL( _T("|X:\\Dir_A|Y:\\Dir_B|"), expanded );

		std::tstring encoded = env::UnExpandPaths( expanded, pSource );
		ASSERT_EQUAL( _T("|%_VAR1_%|$(_VAR2_)|"), encoded );
	}
}

void CEnvironmentTests::TestReplaceEnvVar_VcMacroToWindows( void )
{
	ASSERT_EQUAL( "", env::GetReplaceEnvVar_VcMacroToWindows( "" ) );
	ASSERT_EQUAL( "mid", env::GetReplaceEnvVar_VcMacroToWindows( "mid" ) );
	ASSERT_EQUAL( "lead_%MY_STUFF%_mid_%MY_TOOLS%_trail", env::GetReplaceEnvVar_VcMacroToWindows( "lead_%MY_STUFF%_mid_%MY_TOOLS%_trail" ) );
	ASSERT_EQUAL( "lead_%MY_STUFF%_mid_%MY_TOOLS%_trail", env::GetReplaceEnvVar_VcMacroToWindows( "lead_%MY_STUFF%_mid_$(MY_TOOLS)_trail" ) );
	ASSERT_EQUAL( "lead_%MY_STUFF%_mid_%MY_TOOLS%_trail", env::GetReplaceEnvVar_VcMacroToWindows( "lead_$(MY_STUFF)_mid_$(MY_TOOLS)_trail" ) );
	ASSERT_EQUAL( "lead_%MY_STUFF%_mid_$(MY_TOOLS)_trail", env::GetReplaceEnvVar_VcMacroToWindows( "lead_$(MY_STUFF)_mid_$(MY_TOOLS)_trail", 1 ) );
}

void CEnvironmentTests::TestExpandEnvironment( void )
{
	const std::tstring v_MY_TOOLS = env::GetVariableValue( _T("MY_TOOLS") );
	const std::tstring v_UTL_ADDITIONAL_INCLUDE = env::GetVariableValue( _T("UTL_ADDITIONAL_INCLUDE") );

	if ( v_MY_TOOLS.empty() || v_UTL_ADDITIONAL_INCLUDE.empty() )
		return;				// not defined on this device

	// expand STRINGS:
	ASSERT_EQUAL( v_MY_TOOLS, env::ExpandStrings( _T("%MY_TOOLS%") ) );
	ASSERT_EQUAL( v_UTL_ADDITIONAL_INCLUDE, env::ExpandStrings( _T("%UTL_ADDITIONAL_INCLUDE%") ) );
	ASSERT_EQUAL( v_MY_TOOLS + _T("_mid_") + v_UTL_ADDITIONAL_INCLUDE, env::ExpandStrings( _T("%MY_TOOLS%_mid_%UTL_ADDITIONAL_INCLUDE%") ) );

	ASSERT_EQUAL( _T("$(UTL_ADDITIONAL_INCLUDE)"), env::ExpandStrings( _T("$(UTL_ADDITIONAL_INCLUDE)") ) );								// no VC Macro expansion
	ASSERT_EQUAL( v_MY_TOOLS + _T(";$(UTL_ADDITIONAL_INCLUDE)"), env::ExpandStrings( _T("%MY_TOOLS%;$(UTL_ADDITIONAL_INCLUDE)") ) );	// just MY_TOOLS expansion (no VC Macro expansion)

	// expand PATHS:
	ASSERT_EQUAL( v_MY_TOOLS, env::ExpandPaths( _T("$(MY_TOOLS)") ) );
	ASSERT_EQUAL( v_MY_TOOLS + _T(";") + v_UTL_ADDITIONAL_INCLUDE, env::ExpandPaths( _T("%MY_TOOLS%;$(UTL_ADDITIONAL_INCLUDE)") ) );		// both Windows and VC Macro expansion

/*
	MY_TOOLS=C:\my\Tools
	UTL_ADDITIONAL_INCLUDE=C:\dev\DevTools\CommonUtl;C:\dev\DevTools\CommonUtl\utl
*/
}

void CEnvironmentTests::TestExpandKeysToValues( void )
{
	ASSERT_EQUAL( _T(""),
		str::ExpandKeysToValues( _T(""), _T("["), _T("]"), func::KeyToValue() ) );
	ASSERT_EQUAL( _T("About"),
		str::ExpandKeysToValues( _T("About"), _T("["), _T("]"), func::KeyToValue() ) );
	ASSERT_EQUAL( _T("??"),
		str::ExpandKeysToValues( _T("[]"), _T("["), _T("]"), func::KeyToValue() ) );
	ASSERT_EQUAL( _T("?[X]?"),
		str::ExpandKeysToValues( _T("[X]"), _T("["), _T("]"), func::KeyToValue(), true ) );

	ASSERT_EQUAL( _T("About UTL vesion 3.12 release ?FOO?."),
		str::ExpandKeysToValues( _T("About [NAME] vesion [MAJOR].[MINOR] release [FOO]."), _T("["), _T("]"), func::KeyToValue() ) );
}


void CEnvironmentTests::Run( void )
{
	RUN_TEST( TestQueryEnclosedIdentifiers );
	RUN_TEST( TestEnvironVariables );
	RUN_TEST( TestReplaceEnvVar_VcMacroToWindows );
	RUN_TEST( TestExpandEnvironment );
	RUN_TEST( TestExpandKeysToValues );
}


#endif //USE_UT
