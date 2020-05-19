
#include "stdafx.h"
#include "ListCtrlEditorHost.h"
#include "ReportListControl.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CListCtrlEditorHost::CListCtrlEditorHost( CReportListControl* pListCtrl )
	: CCmdTarget()
	, m_pListCtrl( pListCtrl )
	, m_listAccel( IDR_LIST_EDITOR_ACCEL )
{
	ASSERT_PTR( m_pListCtrl );
}

CListCtrlEditorHost::~CListCtrlEditorHost()
{
}

bool CListCtrlEditorHost::HandleTranslateMessage( MSG* pMsg )
{
	if ( CAccelTable::IsKeyMessage( pMsg ) )
		if ( m_listAccel.TranslateIfOwnsFocus( pMsg, m_pListCtrl->m_hWnd, m_pListCtrl->m_hWnd ) )
			return true;

	return m_pListCtrl->PreTranslateMessage( pMsg ) != FALSE;
}


// command handlers

BEGIN_MESSAGE_MAP( CListCtrlEditorHost, CCmdTarget )
END_MESSAGE_MAP()
