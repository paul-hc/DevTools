
#include "stdafx.h"
#include "ListCtrlEditorFrame.h"
#include "ReportListControl.h"
#include "Utilities.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/ReportListControl.hxx"


CListCtrlEditorFrame::CListCtrlEditorFrame( CReportListControl* pListCtrl, CToolBar* pToolbar )
	: CCmdTarget()
	, CTooltipsHook()
	, m_pListCtrl( pListCtrl )
	, m_pToolbar( pToolbar )
	, m_pParentWnd( m_pListCtrl->GetParent() )
	, m_listAccel( IDR_LIST_EDITOR_ACCEL )
{
	ASSERT_PTR( m_pListCtrl->GetSafeHwnd() );		// must instantiate this after subclassing of the list

	// Special CommandFrame mode of the list - read the notes in CReportListControl::OnCmdMsg():
	//	- the list receives the commands from the context menu & toolbar, but routes them to parent dialog if handlers are defined
	//	- this allows proper command routing for multiple list frames in the same dialog.
	m_pListCtrl->SetFrameEditor( this );

	if ( m_pToolbar != NULL )
	{
		m_pToolbar->SetOwner( m_pListCtrl );		// important: route commands directly to the list (for the case of multiple frames in parent dialog)
		HookWindow( m_pListCtrl->GetSafeHwnd() );	// needed for displaying tooltips in the toolbar
	}
}

CListCtrlEditorFrame::~CListCtrlEditorFrame()
{
}

bool CListCtrlEditorFrame::InInlineEditingMode( void ) const
{
	return m_pListCtrl->IsInternalChange() || m_pListCtrl->GetEditControl() != NULL;
}


CCmdTarget* CListCtrlEditorFrame::GetCmdTarget( void )
{
	return this;
}

bool CListCtrlEditorFrame::HandleTranslateMessage( MSG* pMsg )
{
	if ( CAccelTable::IsKeyMessage( pMsg ) )
		if ( m_listAccel.TranslateIfOwnsFocus( pMsg, m_pListCtrl->m_hWnd, m_pListCtrl->m_hWnd ) )
			return true;

	return m_pListCtrl->PreTranslateMessage( pMsg ) != FALSE;
}

bool CListCtrlEditorFrame::HandleCtrlCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	if ( m_pParentWnd->OnCmdMsg( id, code, pExtra, pHandlerInfo ) )
		return true;			// handled by dialog custom handler, which take precedence over internal handler

	if ( __super::OnCmdMsg( id, code, pExtra, pHandlerInfo ) )		// non-virtual call
		return true;			// handled by THIS frame

	return false;
}


BOOL CListCtrlEditorFrame::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	if ( __super::OnCmdMsg( id, code, pExtra, pHandlerInfo ) )
		return TRUE;			// handled by this frame

	if ( m_pListCtrl->OnCmdMsg( id, code, pExtra, pHandlerInfo ) )
		return TRUE;			// handled by the list

	return FALSE;
}


// command handlers

BEGIN_MESSAGE_MAP( CListCtrlEditorFrame, CCmdTarget )
	ON_UPDATE_COMMAND_UI( ID_ADD_ITEM, OnUpdate_NotEditing )
	ON_COMMAND( ID_REMOVE_ITEM, OnRemoveItem )
	ON_UPDATE_COMMAND_UI( ID_REMOVE_ITEM, OnUpdate_AnySelected )
	ON_COMMAND( ID_REMOVE_ALL_ITEMS, OnRemoveAll )
	ON_UPDATE_COMMAND_UI( ID_REMOVE_ALL_ITEMS, OnUpdate_RemoveAll )
	ON_UPDATE_COMMAND_UI( ID_EDIT_ITEM, OnUpdate_SingleSelected )
END_MESSAGE_MAP()

void CListCtrlEditorFrame::OnUpdate_NotEditing( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !InInlineEditingMode() );
}

void CListCtrlEditorFrame::OnUpdate_AnySelected( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !InInlineEditingMode() && m_pListCtrl->AnySelected() );
}

void CListCtrlEditorFrame::OnUpdate_SingleSelected( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !InInlineEditingMode() && m_pListCtrl->SingleSelected() );
}

void CListCtrlEditorFrame::OnRemoveItem( void )
{
	std::vector< int > selIndexes;
	if ( !m_pListCtrl->GetSelection( selIndexes ) )
		return;				// command not disabled?

	lv::CNmItemsRemoved info( m_pListCtrl, selIndexes.front() );

	m_pListCtrl->QueryObjectsByIndex( info.m_removedObjects, selIndexes );

	{
		CScopedInternalChange scopedChange( m_pListCtrl );

		for ( size_t i = selIndexes.size(); i-- != 0; )
			m_pListCtrl->DeleteItem( selIndexes[ i ] );
	}

	info.m_minSelIndex = std::min( info.m_minSelIndex, m_pListCtrl->GetItemCount() - 1 );

	if ( info.m_minSelIndex != -1 )
		m_pListCtrl->SetCaretIndex( info.m_minSelIndex );

	info.m_nmHdr.NotifyParent();				// lv::LVN_ItemsRemoved -> notify parent to delete owned objects
}

void CListCtrlEditorFrame::OnRemoveAll( void )
{
	lv::CNmItemsRemoved info( m_pListCtrl );
	m_pListCtrl->QueryObjectsSequence( info.m_removedObjects );

	{
		CScopedInternalChange scopedChange( m_pListCtrl );

		m_pListCtrl->DeleteAllItems();
	}

	info.m_nmHdr.NotifyParent();				// lv::LVN_ItemsRemoved -> notify parent to delete owned objects
}

void CListCtrlEditorFrame::OnUpdate_RemoveAll( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !InInlineEditingMode() && m_pListCtrl->GetItemCount() != 0 );
}
