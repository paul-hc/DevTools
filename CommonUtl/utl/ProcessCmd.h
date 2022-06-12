#ifndef ProcessCmd_h
#define ProcessCmd_h
#pragma once

#include <process.h>
#include "Path.h"


namespace utl
{
	class CProcessCmd
	{
	public:
		CProcessCmd( const TCHAR* pExePath ) : m_exePath( pExePath ) { ASSERT( !m_exePath.IsEmpty() ); }

		template< typename ValueT >
		void AddParam( const ValueT& value ) { m_params.push_back( arg::AutoEnquote( value ) ); }

		const fs::CPath& GetExePath( void ) const { return m_exePath; }
		std::tstring MakeCommandLine( void ) const;		// excluding the executable path argv[0]

		int Execute( void ) { return static_cast<int>( ExecuteProcess( _P_WAIT ) ); }			// waits for completion
		HANDLE Spawn( int mode = _P_NOWAIT ) { return reinterpret_cast<HANDLE>( ExecuteProcess( mode ) ); }
	private:
		intptr_t ExecuteProcess( int mode );
		const TCHAR* const* MakeArgList( std::vector< const TCHAR* >& rArgList ) const;
	private:
		fs::CPath m_exePath;
		std::vector< std::tstring > m_params;
	};
}


#endif // ProcessCmd_h
