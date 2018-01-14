
#include "stdafx.h"
#include "Guards.h"
#include "BaseApp.h"
#include "Logger.h"
#include "StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace utl
{
	// CSlowSectionGuard implementation

	bool CSlowSectionGuard::Commit( void )
	{
		if ( !IsTimeout() || m_context.empty() )
			return false;

		// log the message that it takes longer than the threshold
		std::tstring message = str::Format( _T("(*) Slow section for '%s': it takes %s seconds"), m_context.c_str(), num::FormatNumber( m_timer.ElapsedSeconds() ).c_str() );
		TRACE( _T(" %s\n"), message.c_str() );

		if ( CLogger* pLogger = app::GetLoggerPtr() )
			pLogger->Log( message.c_str() );

		m_context.clear();		// mark it as done
		return true;
	}

	bool CSlowSectionGuard::Restart( const std::tstring& context )
	{
		bool slow = Commit();
		m_context = context;
		m_timer.Restart();
		return slow;
	}
}
