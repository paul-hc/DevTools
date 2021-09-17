
#include "stdafx.h"
#include "CmdLineOptions.h"
#include "utl/ConsoleApplication.h"
#include "utl/FileSystem.h"
#include "utl/RuntimeException.h"
#include "utl/StringUtilities.h"
#include "utl/TimeUtils.h"


CCmdLineOptions::CCmdLineOptions( void )
	: m_pArg( NULL )
	, m_helpMode( false )
{
}

CCmdLineOptions::~CCmdLineOptions()
{
}

bool CCmdLineOptions::ParseValue( std::tstring& rValue, const TCHAR* pArg, const TCHAR* pNameList, CaseCvt caseCvt /*= AsIs*/ )
{
	if ( arg::ParseValuePair( rValue, pArg, pNameList ) )
		switch ( caseCvt )
		{
			case UpperCase: str::ToUpper( rValue ); return true;
			case LowerCase: str::ToLower( rValue ); return true;
		}

	return false;
}

void CCmdLineOptions::ParseCommandLine( int argc, TCHAR* argv[] ) throws_( CRuntimeException )
{
	for ( int i = 1; i != argc; ++i )
	{
		m_pArg = argv[ i ];

		if ( arg::IsSwitch( m_pArg ) )
		{
			const TCHAR* pSwitch = m_pArg + 1;
			std::tstring value;

			if ( arg::EqualsAnyOf( pSwitch, _T("?|H") ) )
			{
				m_helpMode = true;
				return;
			}
			else if ( arg::EqualsAnyOf( pSwitch, _T("UT") ) )
				continue;							// consume known debug args
			else
				ThrowInvalidArgument();
		}
		else
		{
			if ( m_targetRcPath.IsEmpty() )
				m_targetRcPath.Set( m_pArg );
			else if ( !time_utl::IsValid( m_buildTimestamp ) )
				ParseBuildTimestamp( m_pArg );
			else
				throw CRuntimeException( str::Format( _T("Unrecognized argument '%s'"), m_pArg ) );
		}
	}

	PostProcessArguments();
}

void CCmdLineOptions::PostProcessArguments( void ) throws_( CRuntimeException )
{
	if ( m_targetRcPath.IsEmpty() )
		throw CRuntimeException( _T("Missing 'rc_file_path' argument!") );
	else if ( fs::IsReadOnlyFile( m_targetRcPath.GetPtr() ) )
		throw CRuntimeException( _T("Cannot write to read-only file 'rc_file_path'!") );

	if ( !time_utl::IsValid( m_buildTimestamp ) )
		m_buildTimestamp = CTime::GetCurrentTime();		// by default use current date-time for stamping build time
}

void CCmdLineOptions::ParseBuildTimestamp( const std::tstring& value ) throws_( CRuntimeException )
{
	// timestamp argument format: "DD-MM-YYYY H:mm:ss" or "DD/MM/YYYY H:mm:ss"

	CTime buildTimestamp = time_utl::ParseStdTimestamp( value );

	if ( !time_utl::IsValid( buildTimestamp ) )
		throw CRuntimeException( str::Format( _T("Invalid date-time in 'timestamp' argument '%s'"), m_pArg ) );

	m_buildTimestamp = buildTimestamp;
}

void CCmdLineOptions::ThrowInvalidArgument( void ) throws_( CRuntimeException )
{
	throw CRuntimeException( str::Format( _T("invalid argument '%s'"), m_pArg ) );
}
