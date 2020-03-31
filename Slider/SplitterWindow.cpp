
#include "stdafx.h"
#include "SplitterWindow.h"
#include "AlbumThumbListView.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CSplitterWindow::CSplitterWindow( void )
	: CSplitterWnd()
	, m_hasTracked( false )
{
}

CSplitterWindow::~CSplitterWindow()
{
}

void CSplitterWindow::TrackColumnSize( int x, int col )
{
	if ( 0 == col )
	{
		CAlbumThumbListView* pThumbView = (CAlbumThumbListView*)GetPane( 0, 0 );

		ASSERT( pThumbView->IsKindOf( RUNTIME_CLASS( CAlbumThumbListView ) ) );
		x = pThumbView->QuantifyListWidth( x );
		m_hasTracked = true;
	}
	CSplitterWnd::TrackColumnSize( x, col );
}

void CSplitterWindow::RecalcLayout( void )
{
	CSplitterWnd::RecalcLayout();
	if ( m_hasTracked )
	{
		CAlbumThumbListView* pThumbView = (CAlbumThumbListView*)GetPane( 0, 0 );

		m_hasTracked = false;
		if ( !pThumbView->CheckListLayout() )
			pThumbView->Invalidate();
	}
}

void CSplitterWindow::MoveColumnToPos( int x, int col, bool checkLayout /*= false*/ )
{
	TrackColumnSize( x, col );
	if ( checkLayout )
		RecalcLayout();
	else
	{
		m_hasTracked = false;
		CSplitterWnd::RecalcLayout();
	}
}

// Copy & Paste from CSplitterWnd::CreateView
bool CSplitterWindow::DoRecreateWindow( CAlbumThumbListView& rThumbView, const CSize& thumbSize )
{
	// Set the initial size for that pane
	m_pColInfo[ 0 ].nIdealSize = thumbSize.cx;
	m_pRowInfo[ 0 ].nIdealSize = thumbSize.cy;

	CCreateContext context;
	CView* pOldView = (CView*)GetActivePane();

	// If no context specified, generate one from the currently selected client, if possible
	if ( pOldView == NULL )
		pOldView = (CView*)GetPane( 0, 1 );
	if ( pOldView != NULL && pOldView->IsKindOf( RUNTIME_CLASS( CView ) ) )
	{	// Set info about last pane
		ASSERT( context.m_pCurrentFrame == NULL );
		context.m_pLastView = pOldView;
		context.m_pCurrentDoc = pOldView->GetDocument();
		if ( context.m_pCurrentDoc != NULL )
			context.m_pNewDocTemplate = context.m_pCurrentDoc->GetDocTemplate();
	}
	ASSERT_NULL( rThumbView.m_hWnd );					// shouldn't be already created

	DWORD style = AFX_WS_DEFAULT_VIEW & ~WS_BORDER;
	CRect rect( CPoint( 0, 0 ), thumbSize );		// Create with the right size (wrong position)

	if ( !rThumbView.Create( NULL, NULL, style, rect, this, IdFromRowCol( 0, 0 ), &context ) )
	{
		TRACE( _T("Warning: couldn't re-create the thumb view for splitter.\n") );
		return false;		// View will be cleaned up by PostNcDestroy
	}
	//rThumbView.SendMessage( WM_INITIALUPDATE );		// send initial notification message
	return true;
}


// message handlers

BEGIN_MESSAGE_MAP( CSplitterWindow, CSplitterWnd )
	ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()

void CSplitterWindow::OnLButtonDblClk( UINT mkFlags, CPoint point )
{
	CWnd* pThumbView = GetPane( 0, 0 );

	CSplitterWnd::OnLButtonDblClk( mkFlags, point );

	// Toggle the thumb view on/off
	// Call window proc directly for the thumb view
	AfxCallWndProc( pThumbView, pThumbView->m_hWnd, WM_COMMAND, MAKEWPARAM( CK_SHOW_THUMB_VIEW, BN_CLICKED ), NULL );
}
