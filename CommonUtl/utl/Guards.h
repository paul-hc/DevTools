#ifndef Guards_h
#define Guards_h
#pragma once

#include "utl/Timer.h"


namespace utl
{
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
