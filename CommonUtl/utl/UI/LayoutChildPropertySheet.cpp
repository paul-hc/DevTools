
#include "stdafx.h"
#include "LayoutChildPropertySheet.h"
#include "LayoutEngine.h"
#include "AccelTable.h"
#include "WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


static ACCEL s_sheetKeys[] =
{
	{ FVIRTKEY | FCONTROL, VK_NEXT, ID_NEXT_PANE },		// Ctrl + Page-Down
	{ FVIRTKEY | FCONTROL, VK_PRIOR, ID_PREV_PANE }		// Ctrl + Page-Up
};


CLayoutChildPropertySheet::CLayoutChildPropertySheet( UINT selPageIndex /*= 0*/ )
	: CLayoutBasePropertySheet( _T("<detail sheet>"), NULL, selPageIndex )
	, m_autoDeletePages( true )
	, m_tabMargins( 0, 0, 2, 2 )
	, m_accel( s_sheetKeys, COUNT_OF( s_sheetKeys ) )
{
}

CLayoutChildPropertySheet::~CLayoutChildPropertySheet()
{
	if ( m_autoDeletePages )
		DeleteAllPages();
}

void CLayoutChildPropertySheet::DDX_DetailSheet( CDataExchange* pDX, UINT frameStaticId, bool singleLineTab /*= false*/ )
{
	// frameStaticId: static control sibling after which the child sheet is positioned
	if ( NULL == m_hWnd )
	{
		CreateChildSheet( pDX->m_pDlgWnd );
		if ( singleLineTab )
			GetTabControl()->ModifyStyle( TCS_MULTILINE, 0 );
		ui::AlignToPlaceholder( this, frameStaticId )->DestroyWindow();
		SetDlgCtrlID( frameStaticId );
		SetSheetModified( false );			// initially not modified
	}
}

void CLayoutChildPropertySheet::CreateChildSheet( CWnd* pParent )
{
	ASSERT_NULL( m_hWnd );

	// WS_EX_CONTROLPARENT is important for the child sheet: it prevents infinite loop on WM_ACTIVATE
	VERIFY( Create( pParent, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_GROUP | WS_CLIPCHILDREN, WS_EX_CONTROLPARENT ) );
}

CButton* CLayoutChildPropertySheet::GetSheetButton( UINT buttonId ) const
{
	if ( CButton* pSheetButton = (CButton*)GetParent()->GetDlgItem( buttonId ) )		// usually parent has the button
		return pSheetButton;

	return CLayoutBasePropertySheet::GetSheetButton( buttonId );
}

CRect CLayoutChildPropertySheet::GetTabControlRect( void )
{
	CRect tabRect;
	GetClientRect( &tabRect );

	tabRect.left += m_tabMargins.left;
	tabRect.top += m_tabMargins.top;
	tabRect.right -= m_tabMargins.right;
	tabRect.bottom -= m_tabMargins.bottom;
	return tabRect;
}

void CLayoutChildPropertySheet::LayoutSheet( void )
{
	if ( CTabCtrl* pTabCtrl = GetTabControl() )
	{
		CRect tabRect = GetTabControlRect();

		pTabCtrl->MoveWindow( &tabRect );

		CRect pageRect = tabRect;
		pTabCtrl->AdjustRect( FALSE, &pageRect );
		--pageRect.left;
		++pageRect.top;

		LayoutPages( pageRect );
	}
}


// message handlers

BEGIN_MESSAGE_MAP( CLayoutChildPropertySheet, CLayoutBasePropertySheet )
	ON_MESSAGE( PSM_CHANGED, OnPageChanged )
	ON_COMMAND_RANGE( ID_NEXT_PANE, ID_PREV_PANE, OnNavigatePage )
END_MESSAGE_MAP()

BOOL CLayoutChildPropertySheet::PreTranslateMessage( MSG* pMsg )
{
	if ( m_accel.Translate( pMsg, m_hWnd ) )
		return TRUE;

	if ( CAccelTable::IsKeyMessage( pMsg ) )
		if ( GetParent()->PreTranslateMessage( pMsg ) )
			return TRUE;				// accelerator handled by parent dialog

	return CLayoutBasePropertySheet::PreTranslateMessage( pMsg );
}

LRESULT CLayoutChildPropertySheet::OnPageChanged( WPARAM wParam, LPARAM lParam )
{
	HWND hPage = (HWND)wParam;
	hPage, lParam;

	if ( m_manageOkButtonState )
		if ( CWnd* pOkButton = GetSheetButton( IDOK ) )
			ui::EnableWindow( *pOkButton );

	if ( CWnd* pApplyNowButton = GetSheetButton( ID_APPLY_NOW ) )
		ui::EnableWindow( *pApplyNowButton );

	return Default();
}

void CLayoutChildPropertySheet::OnNavigatePage( UINT cmdId )
{
	int pageCount = GetPageCount();
	if ( 0 == pageCount )
		return;

	int activePageIndex = GetActiveIndex();

	switch ( cmdId )
	{
		case ID_NEXT_PANE: ++activePageIndex; break;
		case ID_PREV_PANE: --activePageIndex; break;
	}

	if ( activePageIndex >= 0 && activePageIndex < pageCount )
		SetActivePage( activePageIndex );
}
