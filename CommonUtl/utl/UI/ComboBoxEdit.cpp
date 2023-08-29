
#include "pch.h"
#include "ComboBoxEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CComboBoxEdit implementation

CComboBoxEdit::CComboBoxEdit( void )
	: CComboBox()
	, m_useExactMatch( true )
{
}

CComboBoxEdit::~CComboBoxEdit()
{
}

void CComboBoxEdit::SubclassDetails( const COMBOBOXINFO& cbInfo )
{
	if ( cbInfo.hwndList != nullptr && nullptr == m_pDropList.get() )
	{
		m_pDropList.reset( new CComboDropList( this, m_useExactMatch ) );
		m_pDropList->SubclassWindow( cbInfo.hwndList );
	}
}

void CComboBoxEdit::PreSubclassWindow( void )
{
	__super::PreSubclassWindow();

	COMBOBOXINFO cbInfo = { sizeof( COMBOBOXINFO ) };
	if ( GetComboBoxInfo( &cbInfo ) )
		SubclassDetails( cbInfo );
}

BOOL CComboBoxEdit::PreTranslateMessage( MSG* pMsg )
{
	return __super::PreTranslateMessage( pMsg );
}


// CComboDropList implementation

CComboDropList::CComboDropList( CComboBox* pParentCombo, bool useExactMatch )
	: m_pParentCombo( pParentCombo )
	, m_autoDelete( useExactMatch )
	, m_useExactMatch( useExactMatch )
	, m_trackingMenu( false )
{
	ASSERT_PTR( m_pParentCombo->GetSafeHwnd() );
}

CComboDropList* CComboDropList::MakeSubclass( CComboBox* pParentCombo, bool useExactMatch, bool autoDelete /*= true*/ )
{
	ASSERT_PTR( pParentCombo );

	CComboDropList* pDropList = nullptr;
	COMBOBOXINFO cbInfo = { sizeof( COMBOBOXINFO ) };

	if ( pParentCombo->GetComboBoxInfo( &cbInfo ) )
	{
		pDropList = new CComboDropList( pParentCombo, useExactMatch );
		pDropList->m_autoDelete = autoDelete;
		pDropList->SubclassWindow( cbInfo.hwndList );
	}

	return pDropList;
}

void CComboDropList::PostNcDestroy( void ) overrides(CWnd)
{
	if ( m_autoDelete )
		delete this;
}

BEGIN_MESSAGE_MAP( CComboDropList, CWnd )
	ON_WM_CONTEXTMENU()
	ON_WM_CAPTURECHANGED()
	ON_MESSAGE( LB_FINDSTRING, OnLBFindString )
END_MESSAGE_MAP()

void CComboDropList::OnContextMenu( CWnd* pWnd, CPoint point )
{
	__super::OnContextMenu( pWnd, point );

	m_trackingMenu = true;
	m_pParentCombo->SendMessage( WM_CONTEXTMENU, (WPARAM)pWnd->GetSafeHwnd(), MAKELPARAM( point.x, point.y ) );
	m_trackingMenu = false;
}

void CComboDropList::OnCaptureChanged( CWnd* pWnd )
{
	if ( !m_trackingMenu )			// (!) prevent closing the dropdown list while tracking the context menu
		__super::OnCaptureChanged( pWnd );
}

LRESULT CComboDropList::OnLBFindString( WPARAM wParam, LPARAM lParam )
{
	if ( m_useExactMatch )
		return DefWindowProc( LB_FINDSTRINGEXACT, wParam, lParam );

	return Default();
}
