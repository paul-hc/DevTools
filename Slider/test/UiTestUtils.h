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
	public:
		CTestStatusProgress( CWnd* pWnd, double maxSeconds = 10.0 );
	protected:
		virtual LRESULT WindowProc( UINT message, WPARAM wParam, LPARAM lParam ) override;
	private:
		double m_maxSeconds;
		CWindowTimer m_progressTimer;
		CTimer m_elapsedTimer;
		std::auto_ptr<CStatusProgressService> m_pProgressSvc;

		enum { AdvanceTimer = 999 };
	};
}


#endif //USE_UT


#endif // UiTestUtils_h
