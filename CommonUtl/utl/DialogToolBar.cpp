
#include "stdafx.h"
#include "DialogToolBar.h"
#include "Dialog_fwd.h"
#include "Utilities.h"
#include <afxpriv.h>		// for WM_IDLEUPDATECMDUI

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CDialogToolBar::~CDialogToolBar()
{
}

void CDialogToolBar::DDX_Placeholder( CDataExchange* pDX, int placeholderId,
									  int alignToPlaceholder /*= H_AlignLeft | V_AlignBottom*/,
									  UINT toolbarResId /*= 0*/ )
{
	if ( NULL == m_hWnd )
	{
		ASSERT( DialogOutput == pDX->m_bSaveAndValidate );

		CreateToolbar( pDX->m_pDlgWnd, toolbarResId );

		// adjust the size of the toolbar
		CSize idealBarSize;
		GetToolBarCtrl().GetMaxSize( &idealBarSize );

		CWnd* pPlaceholder = ui::AlignToPlaceholder( placeholderId, *this, &idealBarSize, alignToPlaceholder );
		pPlaceholder->DestroyWindow();
		SetDlgCtrlID( placeholderId );
	}
}

void CDialogToolBar::CreateToolbar( CWnd* pParent, const CRect* pAlignScreenRect /*= NULL*/,
									int alignment /*= H_AlignRight | V_AlignCenter*/, UINT toolbarResId /*= 0*/ )
{
	ASSERT_NULL( m_hWnd );

	CreateToolbar( pParent, toolbarResId );

	// adjust the size of the toolbar
	CSize idealBarSize;
	GetToolBarCtrl().GetMaxSize( &idealBarSize );

	CRect barRect( CPoint( 0, 0 ), idealBarSize ), alignRect;

	if ( pAlignScreenRect != NULL )
		alignRect = *pAlignScreenRect;
	else
		pParent->GetWindowRect( &alignRect );

	ui::AlignRect( barRect, alignRect, alignment );
	pParent->ScreenToClient( &alignRect );
	MoveWindow( &barRect );
}

void CDialogToolBar::CreateToolbar( CWnd* pParent, UINT toolbarResId )
{
	ASSERT( NULL == m_hWnd && pParent->GetSafeHwnd() != NULL );

	bool useButtonText = false;
	DWORD tbStyle = TBSTYLE_CUSTOMERASE | TBSTYLE_AUTOSIZE | ( useButtonText ? TBSTYLE_LIST : TBSTYLE_FLAT );	// TBSTYLE_TRANSPARENT has problems with WS_CLIPCHILDREN (smooth groups)
	DWORD style = WS_CHILD | WS_VISIBLE | CBRS_ORIENT_HORZ | CBRS_FLYBY | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS;

	VERIFY( CreateEx( pParent, tbStyle, style ) );
	SetOwner( pParent );

	if ( toolbarResId != 0 )
		VERIFY( LoadToolStrip( toolbarResId ) );
	else if ( m_strip.IsValid() )
	{
		VERIFY( SetButtons( &m_strip.m_buttonIds.front(), (int)m_strip.m_buttonIds.size() ) );
		CSize buttonSize = m_strip.m_imageSize + CSize( BtnEdgeWidth, BtnEdgeHeight );
		SetSizes( buttonSize, m_strip.m_imageSize );			// set new sizes of the buttons

		if ( CImageList* pImageList = m_strip.EnsureImageList() )
			GetToolBarCtrl().SetImageList( pImageList );
	}

	if ( m_disabledStyle != gdi::DisabledStd )
		SetCustomDisabledImageList( m_disabledStyle );
}


// message handlers

BEGIN_MESSAGE_MAP( CDialogToolBar, CToolbarStrip )
	ON_MESSAGE( WM_IDLEUPDATECMDUI, OnIdleUpdateCmdUI )
	ON_NOTIFY_REFLECT_EX( NM_CUSTOMDRAW, OnCustomDrawReflect )
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// OnIdleUpdateCmdUI handles the WM_IDLEUPDATECMDUI message, which is used to update the status of user-interface elements within the MFC framework.
//
// We have to get a little tricky here: CToolBar::OnUpdateCmdUI() expects a CFrameWnd pointer as its first parameter.
// However, it doesn't do anything but pass the parameter on to another function which only requires a CCmdTarget pointer.
// We can get a CWnd pointer to the parent window, which is a CCmdTarget, but may not be a CFrameWnd.
// So, to make CToolBar::OnUpdateCmdUI happy, we will call our CWnd pointer a CFrameWnd pointer temporarily.
//
LRESULT CDialogToolBar::OnIdleUpdateCmdUI( WPARAM wParam, LPARAM lParam )
{
	lParam;
	if ( HasFlag( GetStyle(), WS_VISIBLE ) )
		if ( CFrameWnd* pParent = (CFrameWnd*)GetParent() )
			OnUpdateCmdUI( pParent, (BOOL)wParam );

	return 0L;
}

BOOL CDialogToolBar::OnCustomDrawReflect( NMHDR* pNmHdr, LRESULT* pResult )
{	// required for proper background erase when used in property pages (not using TBSTYLE_TRANSPARENT)
	NMTBCUSTOMDRAW* pCustomDraw = (NMTBCUSTOMDRAW*)pNmHdr;

	if ( CDDS_PREERASE == pCustomDraw->nmcd.dwDrawStage )
		if ( HBRUSH hBkBrush = ui::SendCtlColor( GetParent()->GetSafeHwnd(), pCustomDraw->nmcd.hdc, WM_CTLCOLORDLG ) )
		{
			CBrush dbgBrush;
			if ( false )
			{
				static CBrush dbgDialogBkBrush( color::PastelPink );
				hBkBrush = dbgDialogBkBrush;
			}

			CRect clientRect;
			GetClientRect( &clientRect );
			::FillRect( pCustomDraw->nmcd.hdc, &clientRect, hBkBrush );
			*pResult = CDRF_SKIPDEFAULT;
			return TRUE;		// handled
		}

	*pResult = CDRF_DODEFAULT;
	return FALSE;
}
