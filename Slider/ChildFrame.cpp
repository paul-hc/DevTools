
#include "pch.h"
#include "ChildFrame.h"
#include "Workspace.h"
#include "DocTemplates.h"
#include "MainFrame.h"
#include "IImageView.h"
#include "Application.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNCREATE( CChildFrame, CMDIChildWndEx )

CChildFrame::CChildFrame( void )
	: CMDIChildWndEx()
	, m_pImageView( nullptr )
	, m_pImageAccel( &app::CImageDocTemplate::Instance()->m_accel )
{
}

CChildFrame::~CChildFrame()
{
}

IImageView* CChildFrame::GetImageView( void ) const
{
	return m_pImageView;
}

void CChildFrame::ActivateFrame( int cmdShow ) override
{
	if ( -1 == cmdShow )
	{
		CMainFrame* pMainFrame = app::GetMainFrame();

		if ( HasFlag( CWorkspace::GetFlags(), wf::MdiMaximized ) && 1 == pMainFrame->GetMdiChildCount() )
			cmdShow = SW_SHOWMAXIMIZED;
	}
	__super::ActivateFrame( cmdShow );
}

BOOL CChildFrame::OnCreateClient( CREATESTRUCT* pCS, CCreateContext* pContext ) overrides(CFrameWnd)
{
	pCS;
		//__super::OnCreateClient( pCS, pContext );

	if ( pContext != nullptr && pContext->m_pNewViewClass != nullptr )
		m_pImageView = dynamic_cast<IImageView*>( CreateView( pContext, AFX_IDW_PANE_FIRST ) );

	return m_pImageView != nullptr;
}

BOOL CChildFrame::PreTranslateMessage( MSG* pMsg ) override
{
	return
		__super::PreTranslateMessage( pMsg ) ||
		m_pImageAccel->Translate( pMsg, GetMDIFrame()->m_hWnd );		// image specific
}


// message handlers

BEGIN_MESSAGE_MAP( CChildFrame, CMDIChildWndEx )
	ON_WM_DESTROY()
	ON_WM_WINDOWPOSCHANGING()
	ON_WM_NCLBUTTONDBLCLK()
END_MESSAGE_MAP()

void CChildFrame::OnDestroy( void )
{
	CMainFrame* pMainFrame = app::GetMainFrame();
	if ( 1 == pMainFrame->GetMdiChildCount() )
		SetFlag( CWorkspace::RefData().m_wkspFlags, wf::MdiMaximized, HasFlag( GetStyle(), WS_MAXIMIZE ) );

	__super::OnDestroy();
}

void CChildFrame::OnNcLButtonDblClk( UINT hitTest, CPoint point )
{
	__super::OnNcLButtonDblClk( hitTest, point );

	switch ( hitTest )
	{
		case HTGROWBOX:
		case HTLEFT:
		case HTRIGHT:
		case HTTOP:
		case HTTOPLEFT:
		case HTTOPRIGHT:
		case HTBOTTOM:
		case HTBOTTOMLEFT:
		case HTBOTTOMRIGHT:
			if ( IImageView* pImageView = GetImageView() )
				pImageView->GetScrollView()->SendMessage( WM_COMMAND, ID_RESIZE_VIEW_TO_FIT );
	}
}

void CChildFrame::OnWindowPosChanging( WINDOWPOS* pWndPos )
{
	if ( !CWorkspace::Instance().IsFullScreen() )
		__super::OnWindowPosChanging( pWndPos );
}
