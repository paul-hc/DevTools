
#include "stdafx.h"
#include "MainFrm.h"
#include "utl/MenuUtilities.h"
#include "utl/Utilities.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNAMIC( CMainFrame, CFrameWnd )

CMainFrame::CMainFrame( void )
{
}

CMainFrame::~CMainFrame()
{
}

BOOL CMainFrame::PreCreateWindow( CREATESTRUCT& cs )
{
	if( !CFrameWnd::PreCreateWindow( cs ) )
		return FALSE;
	// ZU ERLEDIGEN: Ändern Sie hier die Fensterklasse oder das Erscheinungsbild, indem Sie
	//  CREATESTRUCT cs modifizieren.

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.lpszClass = AfxRegisterWndClass( 0 );
	return TRUE;
}

BOOL CMainFrame::OnCmdMsg( UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	// Ansichtfenster erhält ersten Eindruck vom Befehl
	if ( m_wndView.OnCmdMsg( nID, nCode, pExtra, pHandlerInfo ) )
		return TRUE;

	// andernfalls die Standardbehandlung durchführen
	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}


// message handlers

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_COMMAND_RANGE( IDM_VIEW_LARGEICONS, IDM_VIEW_REPORT, OnViewMode )
	ON_UPDATE_COMMAND_UI_RANGE( IDM_VIEW_LARGEICONS, IDM_VIEW_REPORT, OnUpdateViewMode )
	ON_COMMAND( IDM_VIEW_CUSTOMMENU, OnUseCustomMenu )
	ON_UPDATE_COMMAND_UI( IDM_VIEW_CUSTOMMENU, OnUpdateUseCustomMenu )
END_MESSAGE_MAP()

int CMainFrame::OnCreate( LPCREATESTRUCT lpCreateStruct )
{
	if ( CFrameWnd::OnCreate( lpCreateStruct ) == -1 )
		return -1;

	// create a view to occupy the client area of the frame
	if ( !m_wndView.Create( NULL, NULL, AFX_WS_DEFAULT_VIEW, CRect( 0, 0, 0, 0 ), this, AFX_IDW_PANE_FIRST, NULL ) )
	{
		TRACE( "Failed to create view window\n" );
		return -1;
	}

	return 0;
}

void CMainFrame::OnSetFocus( CWnd* pOldWnd )
{
	m_wndView.SetFocus();
}

void CMainFrame::OnUpdateViewMode( CCmdUI* pCmdUI )
{
	pCmdUI->Enable();

	UINT selCmdId = IDM_VIEW_LARGEICONS + m_wndView.GetViewMode();
	ui::SetRadio( pCmdUI, selCmdId == pCmdUI->m_nID );
}

void CMainFrame::OnViewMode( UINT cmdId )
{
	m_wndView.SetListViewMode( static_cast< ListViewMode >( cmdId - IDM_VIEW_LARGEICONS ) );
}

void CMainFrame::OnUseCustomMenu( void )
{
	m_wndView.SetUseCustomMenu( !m_wndView.UseCustomMenu() );		// toggle
}

void CMainFrame::OnUpdateUseCustomMenu( CCmdUI* pCmdUI )
{
	// TODO: Code für die Befehlsbehandlungsroutine zum Aktualisieren der Benutzeroberfläche hier einfügen
	pCmdUI->Enable();
	pCmdUI->SetCheck( m_wndView.UseCustomMenu() ? MF_CHECKED : MF_UNCHECKED );
}
