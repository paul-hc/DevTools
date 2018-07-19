
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
	// CSectionGuard implementation

	CSectionGuard::CSectionGuard( const std::tstring& sectionName )
	{
		std::tstring sectionMsg = str::Format( _T("* %s.."), sectionName.c_str() );
		TRACE( _T(" %s"), sectionMsg.c_str() );

		if ( CLogger* pLogger = app::GetLoggerPtr() )
			pLogger->Log( sectionMsg.c_str() );
	}

	CSectionGuard::~CSectionGuard()
	{
		std::tstring elapsedMsg = str::Format( _T(". takes %s seconds"), num::FormatNumber( m_timer.ElapsedSeconds() ).c_str() );
		TRACE( _T(" %s\n"), elapsedMsg.c_str() );

		if ( CLogger* pLogger = app::GetLoggerPtr() )
			pLogger->Log( elapsedMsg.c_str() );
	}


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
