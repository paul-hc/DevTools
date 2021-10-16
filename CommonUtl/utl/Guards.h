#ifndef Guards_h
#define Guards_h
#pragma once

#include "Timer.h"


class CLogger;


namespace utl
{
	// always displays elapsed time for a section scope
	//
	class CSectionGuard : private utl::noncopyable
	{
	public:
		CSectionGuard( const std::tstring& sectionName, bool logging = false );
		CSectionGuard( const std::tstring& sectionName, CLogger* pLogger );
		~CSectionGuard();
	private:
		CTimer m_timer;
		CLogger* m_pLogger;
		std::tstring m_sectionName;
	};


	// displays elapsed time for a section scope if timeout is exceeded
	//
	class CSlowSectionGuard : private utl::noncopyable
	{
	public:
		CSlowSectionGuard( const std::tstring& context, double timeoutSecs = 1.0 ) : m_context( context ), m_timeoutSecs( timeoutSecs ) {}
		~CSlowSectionGuard() { Commit(); }

		bool IsTimeout( void ) const { return m_timer.ElapsedSeconds() > m_timeoutSecs; }
		bool Commit( void );
		bool Restart( const std::tstring& context );
	private:
		std::tstring m_context;
		double m_timeoutSecs;
		CTimer m_timer;
	};


	// logs internally various stages and other checkpoints, and times the cummulative execution time in seconds in a given scope
	//
	class CMultiStageTimer : private utl::noncopyable
	{
	public:
		CMultiStageTimer( const TCHAR* pTotalExecTag = s_totalExecTag ) : m_pTotalExecTag( pTotalExecTag ) {}

		void AddStage( const TCHAR tag[] );
		void AddCheckpoint( const TCHAR tag[], double elapsedSecs );

		// output logging
		std::tstring GetOutput( void ) const { return m_os.str(); }

		template< typename OStreamT >
		OStreamT& Report( OStreamT& os )
		{
			if ( !str::IsEmpty( m_pTotalExecTag ) )
				AddCheckpoint( m_pTotalExecTag, m_totalTimer.ElapsedSeconds() );

			return os << m_os.str();
		}
	private:
		CTimer m_totalTimer;				// times overall execution time
		CTimer m_stageTimer;				// times each stage
		const TCHAR* m_pTotalExecTag;
		std::tostringstream m_os;

		static const TCHAR s_totalExecTag[];
	};
}


#include <iosfwd>




#endif // Guards_h
