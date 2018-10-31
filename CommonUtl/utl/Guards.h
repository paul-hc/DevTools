#ifndef Guards_h
#define Guards_h
#pragma once

#include "utl/Timer.h"


class CLogger;


namespace utl
{
	// always displays elapsed time for a section scope
	//
	class CSectionGuard : public utl::noncopyable
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
	class CSlowSectionGuard : public utl::noncopyable
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
}


#endif // Guards_h
