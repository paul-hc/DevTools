
#include "pch.h"
#include "StatusBarProgressService.h"
#include "ControlBar_fwd.h"
#include <afxstatusbar.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CStatusBarProgressService implementation

CMFCStatusBar* CStatusBarProgressService::s_pStatusBar = nullptr;
int CStatusBarProgressService::s_progressPaneIndex = -1;
int CStatusBarProgressService::s_labelPaneIndex = -1;
int CStatusBarProgressService::s_progressPaneWidth = 0;

CStatusBarProgressService::CStatusBarProgressService( size_t maxPos, size_t pos /*= 0*/ )
	: m_maxPos( maxPos )
	, m_autoHide( true )
	, m_autoWrap( false )
{
	REQUIRE( IsInit() );		// CStatusBarProgressService::InitStatusBarInfo() must be called before using CStatusBarProgressService class!

	if ( Open() )
		SetPos( pos );
}

CStatusBarProgressService::~CStatusBarProgressService()
{
	if ( m_autoHide )
		Close();
}

void CStatusBarProgressService::InitStatusBarInfo( CMFCStatusBar* pStatusBar, int progPaneIndex, int labelPaneIndex /*= -1*/ )
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

		HideProgressPanes();		// hide panes since progress is off initially
	}
}

bool CStatusBarProgressService::Open( void )
{
	s_pStatusBar->EnablePaneProgressBar( s_progressPaneIndex, static_cast<long>( m_maxPos ) );
	m_isOpen = IsPaneProgressOn() && GetPos() <= m_maxPos;

	if ( m_isOpen )
		s_pStatusBar->SetPaneWidth( s_progressPaneIndex, s_progressPaneWidth );

	return m_isOpen;
}

bool CStatusBarProgressService::Close( void )
{
	if ( !m_isOpen )
		return false;

	HideProgressPanes();
	return true;
}

size_t CStatusBarProgressService::GetPos( void ) const implement
{
	REQUIRE( IsInit() );
	REQUIRE( IsOpen() );

	return s_pStatusBar->GetPaneProgress( s_progressPaneIndex );
}

void CStatusBarProgressService::SetPos( size_t pos ) implement
{
	REQUIRE( IsInit() );
	REQUIRE( IsOpen() );

	if ( pos > m_maxPos )
		pos = m_autoWrap ? 0 : m_maxPos;

	s_pStatusBar->SetPaneProgress( s_progressPaneIndex, pos );
}

bool CStatusBarProgressService::SetLabelText( const std::tstring& text, COLORREF labelTextColor /*= CLR_DEFAULT*/ ) implement
{
	REQUIRE( IsInit() );

	if ( -1 == s_labelPaneIndex )
		return false;

	if ( labelTextColor != CLR_DEFAULT )
		s_pStatusBar->SetPaneTextColor( s_labelPaneIndex, labelTextColor );

	if ( !s_pStatusBar->SetPaneText( s_labelPaneIndex, text.c_str() ) )
		return false;

	mfc::StatusBar_ResizePaneToFitText( s_pStatusBar, s_labelPaneIndex );
	return true;
}

void CStatusBarProgressService::SetDisplayText( bool displayText /*= true*/ )
{
	REQUIRE( IsInit() );

	if ( CMFCStatusBarPaneInfo* pPaneInfo = mfc::StatusBar_GetPaneInfo( s_pStatusBar, s_progressPaneIndex ) )
	{
		pPaneInfo->bProgressText = displayText;
		s_pStatusBar->InvalidatePaneContent( s_progressPaneIndex );
	}
}

bool CStatusBarProgressService::IsInit( void )
{
	return s_pStatusBar->GetSafeHwnd() != nullptr;
}

bool CStatusBarProgressService::IsPaneProgressOn( void )
{
	if ( s_pStatusBar != nullptr )
		if ( const CMFCStatusBarPaneInfo* pPaneInfo = mfc::StatusBar_GetPaneInfo( s_pStatusBar, s_progressPaneIndex ) )
			return pPaneInfo->nProgressTotal > 0;

	return false;
}

void CStatusBarProgressService::HideProgressPanes( void )
{
	s_pStatusBar->EnablePaneProgressBar( s_progressPaneIndex, -1 );		// switch it off
	s_pStatusBar->SetPaneWidth( s_progressPaneIndex, 0 );

	if ( s_labelPaneIndex != -1 )
	{
		s_pStatusBar->SetPaneText( s_labelPaneIndex, nullptr );
		s_pStatusBar->SetPaneWidth( s_labelPaneIndex, 0 );
	}
}
