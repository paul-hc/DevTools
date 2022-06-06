
#include "stdafx.h"
#include "DialogToolBar.h"
#include "Dialog_fwd.h"
#include "WndUtils.h"
#include <afxpriv.h>		// for WM_IDLEUPDATECMDUI

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CDialogToolBar::~CDialogToolBar()
{
}

void CDialogToolBar::DDX_Placeholder( CDataExchange* pDX, int placeholderId,
									  TAlignment alignToPlaceholder /*= H_AlignLeft | V_AlignBottom*/,
									  UINT toolbarResId /*= 0*/ )
{
	if ( NULL == m_hWnd )
	{
		ASSERT( DialogOutput == pDX->m_bSaveAndValidate );

		CreateReplacePlaceholder( pDX->m_pDlgWnd, placeholderId, alignToPlaceholder, toolbarResId );
	}
}

void CDialogToolBar::DDX_Tandem( CDataExchange* pDX, CWnd* pHostCtrl, int toolbarId, const ui::CTandemLayout& tandemLayout /*= ui::CTandemLayout::s_mateOnRight*/,
								 UINT toolbarResId /*= 0*/ )
{
	if ( NULL == m_hWnd )
	{
		ASSERT( DialogOutput == pDX->m_bSaveAndValidate ); pDX;

		CreateTandem( pHostCtrl, tandemLayout, toolbarResId );

		if ( toolbarId != 0 )
			SetDlgCtrlID( toolbarId );
	}
}

void CDialogToolBar::CreateReplacePlaceholder( CWnd* pParent, int placeholderId, TAlignment alignToPlaceholder /*= H_AlignLeft | V_AlignBottom*/,
											   UINT toolbarResId /*= 0*/ )
{
	ASSERT_NULL( m_hWnd );
	ASSERT_PTR( pParent->GetSafeHwnd() );

	CreateToolbar( pParent, toolbarResId );

	CSize idealBarSize;
	GetToolBarCtrl().GetMaxSize( &idealBarSize );		// adjust the size of the toolbar

	CWnd* pPlaceholder = ui::AlignToPlaceholder( this, placeholderId, &idealBarSize, alignToPlaceholder );

	ui::SetTabOrder( this, pPlaceholder );
	pPlaceholder->DestroyWindow();
	SetDlgCtrlID( placeholderId );
}

void CDialogToolBar::CreateTandem( CWnd* pHostCtrl, const ui::CTandemLayout& tandemLayout /*= ui::CTandemLayout::s_mateOnRight*/, UINT toolbarResId /*= 0*/ )
{
	ASSERT_NULL( m_hWnd );
	ASSERT_PTR( pHostCtrl->GetSafeHwnd() );

	CreateToolbar( pHostCtrl->GetParent(), toolbarResId );

	CSize idealBarSize;
	GetToolBarCtrl().GetMaxSize( &idealBarSize );

	tandemLayout.LayoutTandem( pHostCtrl, this, &idealBarSize );	// also adjust the size of the toolbar
	ui::SetTabOrder( this, pHostCtrl );
}

void CDialogToolBar::CreateToolbar( CWnd* pParent, const CRect* pAlignScreenRect /*= NULL*/,
									TAlignment alignment /*= H_AlignRight | V_AlignCenter*/, UINT toolbarResId /*= 0*/ )
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
	else if ( m_strip.HasButtons() )
		VERIFY( InitToolbarButtons() );
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
	if ( IsWindowVisible() )
		if ( CFrameWnd* pOwner = (CFrameWnd*)GetOwner() )		// important: the Owner may be different than the Parent - via SetOwner()
			OnUpdateCmdUI( pOwner, (BOOL)( wParam != FALSE && !GetEnableUnhandledCmds() ) );

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
