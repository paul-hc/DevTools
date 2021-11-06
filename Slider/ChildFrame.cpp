
#include "stdafx.h"
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


IMPLEMENT_DYNCREATE( CChildFrame, CMDIChildWnd )

CChildFrame::CChildFrame( void )
	: CMDIChildWnd()
	, m_pImageView( NULL )
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

void CChildFrame::ActivateFrame( int cmdShow )
{
	if ( -1 == cmdShow )
	{
		CMainFrame* pMainFrame = app::GetMainFrame();

		if ( HasFlag( CWorkspace::GetFlags(), wf::MdiMaximized ) && 1 == pMainFrame->GetMdiChildCount() )
			cmdShow = SW_SHOWMAXIMIZED;
	}
	CMDIChildWnd::ActivateFrame( cmdShow );
}

BOOL CChildFrame::PreCreateWindow( CREATESTRUCT& rCS )
{
	return CMDIChildWnd::PreCreateWindow( rCS );
}

BOOL CChildFrame::OnCreateClient( CREATESTRUCT* pCS, CCreateContext* pContext )
{
	pCS;

	if ( pContext != NULL && pContext->m_pNewViewClass != NULL )
		m_pImageView = dynamic_cast<IImageView*>( CreateView( pContext, AFX_IDW_PANE_FIRST ) );
	return m_pImageView != NULL;
}

BOOL CChildFrame::PreTranslateMessage( MSG* pMsg )
{
	return
		CMDIChildWnd::PreTranslateMessage( pMsg ) ||
		m_pImageAccel->Translate( pMsg, GetMDIFrame()->m_hWnd );		// image specific
}


// message handlers

BEGIN_MESSAGE_MAP( CChildFrame, CMDIChildWnd )
	ON_WM_DESTROY()
	ON_WM_WINDOWPOSCHANGING()
	ON_WM_NCLBUTTONDBLCLK()
END_MESSAGE_MAP()

void CChildFrame::OnDestroy( void )
{
	CMainFrame* pMainFrame = app::GetMainFrame();
	if ( 1 == pMainFrame->GetMdiChildCount() )
		SetFlag( CWorkspace::RefData().m_wkspFlags, wf::MdiMaximized, HasFlag( GetStyle(), WS_MAXIMIZE ) );

	CMDIChildWnd::OnDestroy();
}

void CChildFrame::OnNcLButtonDblClk( UINT hitTest, CPoint point )
{
	CMDIChildWnd::OnNcLButtonDblClk( hitTest, point );

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
		CMDIChildWnd::OnWindowPosChanging( pWndPos );
}
