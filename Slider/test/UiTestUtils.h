#ifndef UiTestUtils_h
#define UiTestUtils_h
#pragma once


#ifdef USE_UT		// no UT code in release builds

#include "utl/Timer.h"
#include "utl/UI/WindowHook.h"
#include "utl/UI/WindowTimer.h"


class CStatusProgressService;


namespace ut
{
	class CTestStatusProgress : public CWindowHook
	{
		CTestStatusProgress( CWnd* pWnd, double maxSeconds );		// auto-delete
	public:
		virtual ~CTestStatusProgress();

		static CTestStatusProgress* Start( CWnd* pWnd, double maxSeconds = 10.0 );
	protected:
		void Kill( void );

		virtual LRESULT WindowProc( UINT message, WPARAM wParam, LPARAM lParam ) override;
	private:
		double m_maxSeconds;
		CWindowTimer m_progressTimer;
		CTimer m_elapsedTimer;
		std::auto_ptr<CStatusProgressService> m_pProgressSvc;

		static CTestStatusProgress* s_pRunning;

		enum { AdvanceTimer = 999 };
	};
}


#endif //USE_UT


#endif // UiTestUtils_h
