
#include "pch.h"
#include "StatusProgressService.h"
#include "ControlBar_fwd.h"
#include "WindowTimer.h"
#include <afxstatusbar.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


class CDelayedHideImpl : private ISequenceTimerCallback
{
	CDelayedHideImpl( void ) {}
public:
	static CDelayedHideImpl* Instance( void );

	bool IsStarted( void ) const { return m_pTimerHook.get() != nullptr; }

	void Start( UINT elapseMs );
	void Stop( void ) { OnSequenceStep(); }

	// ISequenceTimerCallback interface
	virtual void OnSequenceStep( void ) implements(ISequenceTimerCallback);
private:
	std::auto_ptr<CTimerSequenceHook> m_pTimerHook;

	enum { CloseEventId = 30 };
};


// CStatusProgressService implementation

CMFCStatusBar* CStatusProgressService::s_pStatusBar = nullptr;
int CStatusProgressService::s_progressPaneIndex = -1;
int CStatusProgressService::s_labelPaneIndex = -1;
int CStatusProgressService::s_progressPaneWidth = 0;

CStatusProgressService::CStatusProgressService( size_t maxPos, size_t pos /*= 0*/ )
	: m_maxPos( 0 )
	, m_autoHide( true )
	, m_autoWrap( false )
	, m_isActive( false )
	, m_hideDelayMs( DefaultHideDelayMs )
{
	REQUIRE( IsInit() );		// CStatusProgressService::InitStatusBarInfo() must be called before using CStatusProgressService class!

	if ( maxPos != 0 )
		StartProgress( maxPos, pos );
}

CStatusProgressService::~CStatusProgressService()
{
	if ( m_autoHide )
		Deactivate();
}

void CStatusProgressService::StartProgress( size_t maxPos, size_t pos /*= 0*/ )
{
	REQUIRE( maxPos != 0 );
	REQUIRE( IsInit() );

	m_maxPos = maxPos;

	if ( Activate() )
		SetPos( pos );
}

bool CStatusProgressService::Activate( void )
{
	if ( IsPaneProgressOn() )
		CDelayedHideImpl::Instance()->Stop();		// cancel pending delayed hide (if any)

	s_pStatusBar->EnablePaneProgressBar( s_progressPaneIndex, static_cast<long>( m_maxPos ) );
	m_isActive = IsPaneProgressOn();

	if ( m_isActive )
		s_pStatusBar->SetPaneWidth( s_progressPaneIndex, s_progressPaneWidth );

	return m_isActive;
}

bool CStatusProgressService::Deactivate( void )
{
	if ( !m_isActive )
		return false;

	m_isActive = false;

	if ( 0 == m_hideDelayMs )
		HideProgressPanes();
	else
	{
		if ( s_labelPaneIndex != -1 )
		{	// de-highlight the label pane
			s_pStatusBar->SetPaneTextColor( s_labelPaneIndex, CLR_NONE );
			s_pStatusBar->SetPaneBackgroundColor( s_labelPaneIndex, CLR_NONE );
		}

		CDelayedHideImpl::Instance()->Start( m_hideDelayMs );
	}

	return true;
}

size_t CStatusProgressService::GetMaxPos( void ) const
{
	REQUIRE( IsInit() );
	REQUIRE( IsActive() );

	ENSURE( m_maxPos == (size_t)mfc::StatusBar_GetPaneInfo( s_pStatusBar, s_progressPaneIndex )->nProgressTotal );
	return m_maxPos;
}

size_t CStatusProgressService::GetPos( void ) const implement
{
	REQUIRE( IsInit() );
	REQUIRE( IsActive() );

	return s_pStatusBar->GetPaneProgress( s_progressPaneIndex );
}

void CStatusProgressService::SetPos( size_t pos ) implement
{
	REQUIRE( IsInit() );
	REQUIRE( IsActive() );

	if ( pos > m_maxPos )
		pos = m_autoWrap ? 0 : m_maxPos;

	s_pStatusBar->SetPaneProgress( s_progressPaneIndex, pos );
}

bool CStatusProgressService::SetLabelText( const std::tstring& text, COLORREF labelTextColor /*= CLR_DEFAULT*/, COLORREF labelBkColor /*= CLR_DEFAULT*/ ) implement
{
	REQUIRE( IsInit() );

	if ( -1 == s_labelPaneIndex )
		return false;

	if ( labelTextColor != CLR_DEFAULT )
		s_pStatusBar->SetPaneTextColor( s_labelPaneIndex, labelTextColor );

	if ( labelBkColor != CLR_DEFAULT )
		s_pStatusBar->SetPaneBackgroundColor( s_labelPaneIndex, labelBkColor );

	if ( !s_pStatusBar->SetPaneText( s_labelPaneIndex, text.c_str() ) )
		return false;

	mfc::StatusBar_ResizePaneToFitText( s_pStatusBar, s_labelPaneIndex );
	return true;
}

void CStatusProgressService::SetDisplayText( bool displayText /*= true*/ )
{
	REQUIRE( IsInit() );

	if ( CMFCStatusBarPaneInfo* pPaneInfo = mfc::StatusBar_GetPaneInfo( s_pStatusBar, s_progressPaneIndex ) )
	{
		pPaneInfo->bProgressText = displayText;
		s_pStatusBar->InvalidatePaneContent( s_progressPaneIndex );
	}
}


void CStatusProgressService::InitStatusBarInfo( CMFCStatusBar* pStatusBar, int progPaneIndex, int labelPaneIndex /*= -1*/ )
{
	s_pStatusBar = pStatusBar;
	s_progressPaneIndex = progPaneIndex;
	s_labelPaneIndex = labelPaneIndex;

	if ( s_pStatusBar != nullptr )
	{
		ENSURE( s_pStatusBar->GetItemID( s_progressPaneIndex ) != 0 );
		ENSURE( -1 == s_labelPaneIndex || s_pStatusBar->GetItemID( s_labelPaneIndex ) != 0 );

		// note: at this early stage, s_pStatusBar->GetPaneWidth( s_progressPaneIndex ) returns 0 since the client rect is empty,
		// so we need store the pPaneInfo->cxText as pane width
		const CMFCStatusBarPaneInfo* pPaneInfo = mfc::StatusBar_GetPaneInfo( s_pStatusBar, s_progressPaneIndex );
		s_progressPaneWidth = pPaneInfo->cxText;

		HideProgressPanesImpl();		// hide panes since progress is off initially
	}
}

bool CStatusProgressService::IsInit( void )
{
	return s_pStatusBar->GetSafeHwnd() != nullptr;
}

bool CStatusProgressService::IsPaneProgressOn( void )
{
	if ( s_pStatusBar->GetSafeHwnd() != nullptr )
		if ( const CMFCStatusBarPaneInfo* pPaneInfo = mfc::StatusBar_GetPaneInfo( s_pStatusBar, s_progressPaneIndex ) )
			return pPaneInfo->nProgressTotal > 0;

	return false;
}

bool CStatusProgressService::HideProgressPanes( void )
{
	if ( nullptr == s_pStatusBar->GetSafeHwnd() || !IsPaneProgressOn() )
		return false;		// destroyed in the meantime or hidden

	HideProgressPanesImpl();
	return true;
}

void CStatusProgressService::HideProgressPanesImpl( void )
{
	ASSERT_PTR( s_pStatusBar->GetSafeHwnd() );

	s_pStatusBar->EnablePaneProgressBar( s_progressPaneIndex, -1 );		// switch it off
	s_pStatusBar->SetPaneWidth( s_progressPaneIndex, 0 );

	if ( s_labelPaneIndex != -1 )
	{
		s_pStatusBar->SetPaneText( s_labelPaneIndex, nullptr );
		s_pStatusBar->SetPaneWidth( s_labelPaneIndex, 0 );
	}
}


// CDelayedHideImpl implementation

CDelayedHideImpl* CDelayedHideImpl::Instance( void )
{
	static CDelayedHideImpl s_singleton;
	return &s_singleton;
}

void CDelayedHideImpl::Start( UINT elapseMs )
{
	if ( UINT_MAX == elapseMs || !CStatusProgressService::IsInit() )
	{
		m_pTimerHook.reset();
		return;		// no delay hiding
	}

	if ( !IsStarted() )
		m_pTimerHook.reset( new CTimerSequenceHook( this ) );

	m_pTimerHook->Start( CStatusProgressService::s_pStatusBar->GetSafeHwnd(), CloseEventId, elapseMs );
}

void CDelayedHideImpl::OnSequenceStep( void )
{
	CStatusProgressService::HideProgressPanes();
	m_pTimerHook.reset();
}
