
#include "pch.h"
#include "Options.h"
#include "utl/RuntimeException.h"
#include "utl/StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


COptions::COptions( void )
	: m_pArg( nullptr )
	, m_helpMode( false )
{
}

void COptions::ParseCommandLine( int argc, TCHAR* argv[] ) throws_( CRuntimeException )
{
	for ( int i = 1; i != argc; ++i )
	{
		m_pArg = argv[ i ];

		if ( arg::IsSwitch( m_pArg ) )
		{
			const TCHAR* pSwitch = m_pArg + 1;

			if ( arg::EqualsAnyOf( pSwitch, _T("?|H") ) )
			{
				m_helpMode = true;
				return;
			}
			else if ( arg::EqualsAnyOf( pSwitch, _T("UT|DEBUG|NDEBUG|NODEBUG") ) )
				continue;							// consume known debug args
			else
				throw CRuntimeException( str::Format( _T("invalid argument '%s'"), m_pArg ) );
		}
		else
			throw CRuntimeException( str::Format( _T("Unrecognized argument '%s'"), m_pArg ) );
	}
}
