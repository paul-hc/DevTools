
#include "stdafx.h"
#include "ProcessCmd.h"
#include "FileSystem_fwd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace utl
{
	intptr_t CProcessCmd::ExecuteProcess( int mode )
	{
		::_flushall();				// flush i/o streams so the console is synched for child process output (if any)

		std::vector< const TCHAR* > argList;
		return ::_tspawnv( mode, m_exePath.c_str(), MakeArgList( argList ) );
	}

	const TCHAR* const* CProcessCmd::MakeArgList( std::vector< const TCHAR* >& rArgList ) const
	{
		rArgList.push_back( m_exePath.c_str() );	// first arg is always the executable itself

		for ( std::vector< std::tstring >::const_iterator itParam = m_params.begin(); itParam != m_params.end(); ++itParam )
			if ( !itParam->empty() )
				rArgList.push_back( itParam->c_str() );

		rArgList.push_back( NULL );					// terminating NULL
		return &rArgList.front();
	}
}
