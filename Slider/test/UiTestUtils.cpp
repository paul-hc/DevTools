
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "UiTestUtils.h"
#include "utl/UI/StatusProgressService.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ut
{
	// CTestStatusProgress implementation

	CTestStatusProgress* CTestStatusProgress::s_pRunning = nullptr;

	CTestStatusProgress::CTestStatusProgress( CWnd* pWnd, double maxSeconds )
		: CWindowHook( true )
		, m_maxSeconds( maxSeconds )
		, m_progressTimer( pWnd, AdvanceTimer, 50 )
		, m_pProgressSvc( new CStatusProgressService( 50 ) )
	{
		m_pProgressSvc->SetAutoWrap();
		m_pProgressSvc->SetDisplayText();
		m_pProgressSvc->SetLabelText( _T("Testing progress service:"), color::Red /*, color::LightYellow*/ );

		HookWindow( pWnd->GetSafeHwnd() );
		m_progressTimer.Start();

		ASSERT_NULL( s_pRunning );
		s_pRunning = this;
	}

	CTestStatusProgress::~CTestStatusProgress()
	{
		if ( IsHooked() )		// restartable timer still on?
			Kill();

		ASSERT( s_pRunning == this );
		s_pRunning = nullptr;
	}

	CTestStatusProgress* CTestStatusProgress::Start( CWnd* pWnd, double maxSeconds )
	{
		if ( s_pRunning != nullptr )
			s_pRunning->Kill();

		return new CTestStatusProgress( pWnd, maxSeconds );
	}

	void CTestStatusProgress::Kill( void )
	{
		m_progressTimer.Stop();
		UnhookWindow();
	}

	LRESULT CTestStatusProgress::WindowProc( UINT message, WPARAM wParam, LPARAM lParam ) override
	{
		if ( WM_TIMER == message )
			if ( m_progressTimer.IsHit( wParam ) )
			{
				m_pProgressSvc->Advance();

				if ( m_elapsedTimer.ElapsedSeconds() > m_maxSeconds )
					Kill();

				return 0L;
			}

		return CWindowHook::WindowProc( message, wParam, lParam );
	}
}


#endif //USE_UT
