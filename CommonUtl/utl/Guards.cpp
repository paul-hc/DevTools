
#include "stdafx.h"
#include "Guards.h"
#include "AppTools.h"
#include "Logger.h"
#include "StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace utl
{
	// CSectionGuard implementation

	CSectionGuard::CSectionGuard( const std::tstring& sectionName, bool logging /*= false*/ )
		: m_timer()
		, m_pLogger( logging ? app::GetLogger() : NULL )
		, m_sectionName( sectionName )
	{
	}

	CSectionGuard::CSectionGuard( const std::tstring& sectionName, CLogger* pLogger )
		: m_timer()
		, m_pLogger( pLogger )
		, m_sectionName( sectionName )
	{
	}

	CSectionGuard::~CSectionGuard()
	{
		// report elapsed time at the end of scope so that it doesn't interfere with output tracing in the meantime
		std::tstring text = str::Format( _T("* %s... takes %s"), m_sectionName.c_str(), m_timer.FormatElapsedDuration( 3 ).c_str() );
		TRACE( _T(" %s\n"), text.c_str() );

		if ( m_pLogger != NULL )
			m_pLogger->Log( text.c_str() );
	}


	// CSlowSectionGuard implementation

	bool CSlowSectionGuard::Commit( void )
	{
		if ( !IsTimeout() || m_context.empty() )
			return false;

		// log the message that it takes longer than the threshold
		std::tstring message = str::Format( _T("(*) Slow section for '%s': it takes %s"), m_context.c_str(), m_timer.FormatElapsedDuration( 3 ).c_str() );
		TRACE( _T(" %s\n"), message.c_str() );

		if ( CLogger* pLogger = app::GetLogger() )
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


	// CMultiStageTimer implementation

	const TCHAR CMultiStageTimer::s_totalExecTag[] = _T("Total execution time");

	void CMultiStageTimer::AddStage( const TCHAR tag[] )
	{
		AddCheckpoint( tag, m_stageTimer.ElapsedSeconds() );
		m_stageTimer.Restart();
	}

	void CMultiStageTimer::AddCheckpoint( const TCHAR tag[], double externalElapsedSecs )
	{
		m_os << tag << _T(": ") << CTimer::FormatSeconds( externalElapsedSecs ) << std::endl;
	}
}
