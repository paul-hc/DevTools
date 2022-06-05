
#include "stdafx.h"
#include "PromptDialog.h"
#include "AppService.h"
#include "resource.h"
#include "wnd/WndUtils.h"
#include "utl/UI/StdColors.h"
#include "utl/UI/Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section[] = _T("PromptDialog");
	static const TCHAR entry_swpFlags[] = _T("SwpFlags");
}


bool CPromptDialog::m_useSetWindowPos = true;
UINT CPromptDialog::m_swpFlags = SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_ASYNCWINDOWPOS | SWP_DRAWFRAME | SWP_FRAMECHANGED | SWP_NOACTIVATE;


CPromptDialog::CPromptDialog( HWND hWndTarget, const std::tstring& changedFields, CWnd* pParent /*= NULL*/ )
	: CLayoutDialog( IDD_APPLY_CHANGES_DIALOG, pParent )
	, m_hWndTarget( hWndTarget )
	, m_changedFields( !changedFields.empty() ? changedFields : _T("<NONE>") )
{
	m_regSection = reg::section;
	ASSERT( ::IsWindow( m_hWndTarget ) );
	m_swpFlags = AfxGetApp()->GetProfileInt( reg::section, reg::entry_swpFlags, m_swpFlags );
}

CPromptDialog::~CPromptDialog()
{
}

bool CPromptDialog::CallSetWindowPos( void )
{
	if ( !m_useSetWindowPos )
		return false;

	ASSERT( ::IsWindow( m_hWndTarget ) );

	CRect rect;
	CSize size;
	bool isChild = HasFlag( ui::GetStyle( m_hWndTarget ), WS_CHILD );
	HWND hWndParent = isChild ? ::GetParent( m_hWndTarget ) : NULL;
	HWND hWndInsertAfter = NULL;

	::GetWindowRect( m_hWndTarget, rect );
	size = rect.Size();
	if ( isChild )
		::ScreenToClient( hWndParent, &rect.TopLeft() );

	bool result = ::SetWindowPos( m_hWndTarget, hWndInsertAfter, rect.left, rect.top, size.cx, size.cy, m_swpFlags ) != FALSE;

	if ( !HasFlag( m_swpFlags, SWP_NOREDRAW ) )
	{
		::InvalidateRect( m_hWndTarget, NULL, TRUE );
		::UpdateWindow( m_hWndTarget );
		if ( hWndParent != NULL )
		{
			::InvalidateRect( hWndParent, NULL, TRUE );
			::UpdateWindow( hWndParent );
		}
	}
	return result;
}

void CPromptDialog::DoDataExchange( CDataExchange* pDX )
{
	ui::DDX_Bool( pDX, IDC_USE_SETWINDOWPOS_CHECK, m_useSetWindowPos );
	ui::DDX_Flag( pDX, IDC_SWP_DRAWFRAME_CHECK, m_swpFlags, SWP_DRAWFRAME );
	ui::DDX_Flag( pDX, IDC_SWP_FRAMECHANGED_CHECK, m_swpFlags, SWP_FRAMECHANGED );
	ui::DDX_Flag( pDX, IDC_SWP_NOREDRAW_CHECK, m_swpFlags, SWP_NOREDRAW );
	ui::DDX_Flag( pDX, IDC_SWP_NOACTIVATE_CHECK, m_swpFlags, SWP_NOACTIVATE );
	ui::DDX_Flag( pDX, IDC_SWP_SHOWWINDOW_CHECK, m_swpFlags, SWP_SHOWWINDOW );
	ui::DDX_Flag( pDX, IDC_SWP_HIDEWINDOW_CHECK, m_swpFlags, SWP_HIDEWINDOW );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		ui::GetDlgItemAs< CStatic >( this, IDC_QUESTION_ICON )->SetIcon( ::LoadIcon( NULL, IDI_QUESTION ) );

		ui::SetDlgItemText( this, IDC_PROMPT_STATIC, wnd::FormatBriefWndInfo( m_hWndTarget ) );
		ui::SetDlgItemText( this, IDC_CHANGED_FIELDS_STATIC, m_changedFields );

		CkUseSetWindowPos();
	}

	CLayoutDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CPromptDialog, CLayoutDialog )
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED( IDC_USE_SETWINDOWPOS_CHECK, CkUseSetWindowPos )
END_MESSAGE_MAP()

void CPromptDialog::OnOK( void )
{
	CLayoutDialog::OnOK();
	AfxGetApp()->WriteProfileInt( reg::section, reg::entry_swpFlags, m_swpFlags );
}

void CPromptDialog::CkUseSetWindowPos( void )
{
	m_useSetWindowPos = IsDlgButtonChecked( IDC_USE_SETWINDOWPOS_CHECK ) != FALSE;

	static const UINT checkIds[] = { IDC_SWP_DRAWFRAME_CHECK, IDC_SWP_FRAMECHANGED_CHECK, IDC_SWP_NOREDRAW_CHECK, IDC_SWP_NOACTIVATE_CHECK, IDC_SWP_SHOWWINDOW_CHECK, IDC_SWP_HIDEWINDOW_CHECK };
	ui::EnableControls( *this, checkIds, COUNT_OF( checkIds ), m_useSetWindowPos );
}

HBRUSH CPromptDialog::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT ctlType )
{
	HBRUSH hBkBrush = CLayoutDialog::OnCtlColor( pDC, pWnd, ctlType );
	if ( ctlType == CTLCOLOR_STATIC && pWnd != NULL )
		switch ( pWnd->GetDlgCtrlID() )
		{
			case IDC_PROMPT_STATIC:
				pDC->SetTextColor( app::HotFieldColor );
				break;
			case IDC_CHANGED_FIELDS_STATIC:
				pDC->SetTextColor( !m_changedFields.empty() ? color::Red : color::Blue );
				break;
		}

	return hBkBrush;
}
