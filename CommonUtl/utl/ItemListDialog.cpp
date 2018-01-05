
#include "stdafx.h"
#include "ItemListDialog.h"
#include "Clipboard.h"
#include "TextEdit.h"
#include "Utilities.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section_settings[] = _T("Settings");
	static const TCHAR section_dialog[] = _T("ItemListDialog");
	static const TCHAR section_list[] = _T("ItemListDialog\\list");
	static const TCHAR entry_selectedItem[] = _T("Selected item text");
}

namespace layout
{
	static CLayoutStyle styles[] =
	{
		{ IDC_TOOLBAR_PLACEHOLDER, StretchX },
		{ IDC_ITEMS_LIST, Stretch },
		{ IDOK, OffsetX },
		{ IDCANCEL, OffsetX }
	};
}

CItemListDialog::CItemListDialog( CWnd* pParent, const ui::CItemContent& content, const TCHAR* pTitle /*= NULL*/ )
	: CLayoutDialog( IDD_ITEM_LIST_DIALOG, pParent )
	, m_readOnly( false )
	, m_content( content )
	, m_pTitle( pTitle )
	, m_addingItem( false )
	, m_sepListCtrl( IDC_ITEMS_LIST )
	, m_accel( IDR_ITEM_LIST_ACCEL )
{
	m_regSection = reg::section_dialog;
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );
	LoadDlgIcon( ID_EDIT_LIST_ITEMS );
	m_sepListCtrl.SetSection( reg::section_list );
}

bool CItemListDialog::EditItem( int itemIndex )
{
	if ( m_readOnly )
		return false;

	if ( ui::String == m_content.m_type )
		m_sepListCtrl.EditLabel( itemIndex );
	else
	{
		std::tstring newItem = m_content.EditItem( m_sepListCtrl.GetItemText( itemIndex, Item ), this );
		if ( newItem.empty() )
			return false;

		m_sepListCtrl.SetItemText( itemIndex, Item, newItem.c_str() );
		UpdateData( DialogSaveChanges );
	}
	return true;
}

void CItemListDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_sepListCtrl.m_hWnd;

	DDX_Control( pDX, IDC_ITEMS_LIST, m_sepListCtrl );
	m_toolbar.DDX_Placeholder( pDX, IDC_TOOLBAR_PLACEHOLDER, H_AlignLeft | V_AlignCenter, IDR_ITEM_LIST_STRIP );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		if ( firstInit )
		{
			m_sepListCtrl.SetFont( CTextEdit::GetFixedFont( ui::String == m_content.m_type ? CTextEdit::Large : CTextEdit::Normal ) );
			if ( m_pTitle != NULL )
				SetWindowText( m_pTitle );
			ui::EnableControl( m_hWnd, IDOK, !m_readOnly );
		}

		{
			CScopedInternalChange internalChange( &m_sepListCtrl );
			m_sepListCtrl.DeleteAllItems();
			for ( unsigned int i = 0; i != m_items.size(); ++i )
				m_sepListCtrl.InsertItem( i, m_items[ i ].c_str() );
		}

		if ( !m_items.empty() )
		{
			int selIndex = m_sepListCtrl.FindItemIndex( AfxGetApp()->GetProfileString( reg::section_settings, reg::entry_selectedItem ).GetString() );
			if ( -1 == selIndex )
				selIndex = 0;

			m_sepListCtrl.SetCurSel( selIndex );
		}

		m_toolbar.UpdateCmdUI();
	}
	else
	{
		m_items.clear();

		for ( int i = 0, itemCount = m_sepListCtrl.GetItemCount(); i != itemCount; ++i )
		{
			std::tstring itemText = m_sepListCtrl.GetItemText( i, 0 );
			if ( !itemText.empty() )
				if ( std::find( m_items.begin(), m_items.end(), itemText ) == m_items.end() )
					m_items.push_back( itemText );
		}
	}

	CLayoutDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CItemListDialog, CLayoutDialog )
	ON_WM_DESTROY()
	ON_NOTIFY( LVN_ITEMCHANGED, IDC_ITEMS_LIST, OnLvnItemChanged_Items )
	ON_NOTIFY( LVN_BEGINLABELEDIT, IDC_ITEMS_LIST, OnLvnBeginLabelEdit_Items )
	ON_NOTIFY( LVN_ENDLABELEDIT, IDC_ITEMS_LIST, OnLvnEndLabelEdit_Items )
	ON_COMMAND( ID_ADD_ITEM, OnAddItem )
	ON_UPDATE_COMMAND_UI( ID_ADD_ITEM, OnUpdateAddItem )
	ON_COMMAND( ID_REMOVE_ITEM, OnRemoveItem )
	ON_UPDATE_COMMAND_UI( ID_REMOVE_ITEM, OnUpdateRemoveItem )
	ON_COMMAND( ID_REMOVE_ALL_ITEMS, OnRemoveAll )
	ON_UPDATE_COMMAND_UI( ID_REMOVE_ALL_ITEMS, OnUpdateRemoveAll )
	ON_COMMAND( ID_EDIT_ITEM, OnEditItem )
	ON_UPDATE_COMMAND_UI( ID_EDIT_ITEM, OnUpdateEditItem )
	ON_COMMAND( ID_MOVE_UP_ITEM, OnMoveUpItem )
	ON_UPDATE_COMMAND_UI( ID_MOVE_UP_ITEM, OnUpdateMoveUpItem )
	ON_COMMAND( ID_MOVE_DOWN_ITEM, OnMoveDownItem )
	ON_UPDATE_COMMAND_UI( ID_MOVE_DOWN_ITEM, OnUpdateMoveDownItem )
	ON_COMMAND( ID_EDIT_COPY, OnCopyItems )
	ON_UPDATE_COMMAND_UI( ID_EDIT_COPY, OnUpdateCopyItems )
	ON_COMMAND( ID_EDIT_PASTE, OnPasteItems )
	ON_UPDATE_COMMAND_UI( ID_EDIT_PASTE, OnUpdatePasteItems )
END_MESSAGE_MAP()

BOOL CItemListDialog::PreTranslateMessage( MSG* pMsg )
{
	return
		m_accel.Translate( pMsg, m_hWnd, m_sepListCtrl ) ||
		CLayoutDialog::PreTranslateMessage( pMsg );
}

void CItemListDialog::OnDestroy( void )
{
	std::tstring selItemText;
	int selIndex = m_sepListCtrl.GetCurSel();
	if ( selIndex != -1 )
		selItemText = m_sepListCtrl.GetItemText( selIndex, 0 );

	AfxGetApp()->WriteProfileString( reg::section_settings, reg::entry_selectedItem, selItemText.c_str() );

	CLayoutDialog::OnDestroy();
}

BOOL CItemListDialog::OnCommand( WPARAM wParam, LPARAM lParam )
{
	BOOL outcome = CLayoutDialog::OnCommand( wParam, lParam );
	m_toolbar.UpdateCmdUI();
	return outcome;
}

BOOL CItemListDialog::OnNotify( WPARAM wParam, LPARAM lParam, LRESULT* pResult )
{
	BOOL outcome = CLayoutDialog::OnNotify( wParam, lParam, pResult );
	m_toolbar.UpdateCmdUI();
	return outcome;
}

void CItemListDialog::OnLvnItemChanged_Items( NMHDR* pNmHdr, LRESULT* pResult )
{
	NM_LISTVIEW* pNmListView = (NM_LISTVIEW*)pNmHdr;
	static const UINT selMask = LVIS_SELECTED | LVIS_FOCUSED;

	if ( ( pNmListView->uNewState & selMask ) != ( pNmListView->uOldState & selMask ) )
		m_toolbar.UpdateCmdUI();

	*pResult = 0;
}

void CItemListDialog::OnLvnBeginLabelEdit_Items( NMHDR* pNmHdr, LRESULT* pResult )
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNmHdr;
	pDispInfo;
	*pResult = 0;
}

void CItemListDialog::OnLvnEndLabelEdit_Items( NMHDR* pNmHdr, LRESULT* pResult )
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNmHdr;

	bool fieldEntered = pDispInfo->item.pszText != NULL;
	bool fieldValid = false;
	std::tstring newItemText;
	if ( pDispInfo->item.pszText != NULL )
		newItemText = pDispInfo->item.pszText;

	if ( !newItemText.empty() )
	{
		int foundIndex = m_sepListCtrl.FindItemIndex( newItemText );
		fieldValid = foundIndex == -1 || foundIndex == pDispInfo->item.iItem;
	}

	*pResult = fieldValid;

	if ( fieldEntered && !fieldValid )
	{
		std::tstring message = _T("Error.\n\nItem cannot be empty.");
		if ( !newItemText.empty() )
			message = str::Format( _T("Error.\n\nItem '%s' must be unique."), newItemText.c_str() );
		AfxMessageBox( message.c_str(), MB_ICONERROR | MB_OK );
	}

	if ( !fieldValid && m_addingItem )
	{
		int targetIndex = pDispInfo->item.iItem;
		{
			CScopedInternalChange internalChange( &m_sepListCtrl );
			m_sepListCtrl.DeleteItem( targetIndex );
		}
		--targetIndex;
		targetIndex = std::max( targetIndex, 0 );
		targetIndex = std::min( targetIndex, m_sepListCtrl.GetItemCount() - 1 );
		if ( targetIndex  >= 0 )
			m_sepListCtrl.SetCurSel( targetIndex );
	}

	if ( fieldEntered && fieldValid )
		UpdateData( DialogSaveChanges );

	m_addingItem = false;
}

void CItemListDialog::OnAddItem( void )
{
	int insertIndex = m_sepListCtrl.GetCurSel();

	if ( insertIndex != -1 )
		++insertIndex;
	else
		insertIndex = m_sepListCtrl.GetItemCount();

	m_addingItem = true;
	m_sepListCtrl.InsertItem( insertIndex, _T("") );
	m_sepListCtrl.SetCurSel( insertIndex );
	if ( !EditItem( insertIndex ) )
		ui::SendCommand( m_hWnd, ID_REMOVE_ITEM );
}

void CItemListDialog::OnUpdateAddItem( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !m_readOnly && NULL == m_sepListCtrl.GetEditControl() );
}

void CItemListDialog::OnRemoveItem( void )
{
	int delIndex = m_sepListCtrl.GetCurSel();
	ASSERT( delIndex != -1 );

	CScopedInternalChange internalChange( &m_sepListCtrl );
	m_sepListCtrl.DeleteItem( delIndex );
	delIndex = std::min( delIndex, m_sepListCtrl.GetItemCount() - 1 );
	m_sepListCtrl.SetCurSel( delIndex );

	UpdateData( DialogSaveChanges );
}

void CItemListDialog::OnUpdateRemoveItem( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !m_readOnly && m_sepListCtrl.GetCurSel() != -1 );
}

void CItemListDialog::OnRemoveAll( void )
{
	if ( IDOK == AfxMessageBox( _T("Delete all items in the list?"), MB_OKCANCEL | MB_ICONQUESTION ) )
	{
		m_items.clear();
		UpdateData( DialogOutput );
	}
}

void CItemListDialog::OnUpdateRemoveAll( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !m_readOnly && m_sepListCtrl.GetItemCount() != 0 );
}

void CItemListDialog::OnEditItem( void )
{
	int selIndex = m_sepListCtrl.GetCurSel();
	if ( selIndex != -1 )
		EditItem( selIndex );
}

void CItemListDialog::OnUpdateEditItem( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !m_readOnly && m_sepListCtrl.GetCurSel() != -1 && NULL == m_sepListCtrl.GetEditControl() );
}

void CItemListDialog::OnMoveUpItem( void )
{
	int selIndex = m_sepListCtrl.GetCurSel();
	ASSERT( selIndex != -1 );

	CScopedInternalChange internalChange( &m_sepListCtrl );

	CString itemText = m_sepListCtrl.GetItemText( selIndex, Item );
	m_sepListCtrl.DeleteItem( selIndex );
	m_sepListCtrl.InsertItem( --selIndex, itemText );
	m_sepListCtrl.SetCurSel( selIndex );
}

void CItemListDialog::OnUpdateMoveUpItem( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !m_readOnly && m_sepListCtrl.GetCurSel() > 0 );
}

void CItemListDialog::OnMoveDownItem( void )
{
	int selIndex = m_sepListCtrl.GetCurSel();
	ASSERT( selIndex != -1 );

	CScopedInternalChange internalChange( &m_sepListCtrl );

	CString itemText = m_sepListCtrl.GetItemText( selIndex, Item );
	m_sepListCtrl.DeleteItem( selIndex );
	m_sepListCtrl.InsertItem( ++selIndex, itemText );
	m_sepListCtrl.SetCurSel( selIndex );
}

void CItemListDialog::OnUpdateMoveDownItem( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !m_readOnly && m_sepListCtrl.GetCurSel() < static_cast<int>( m_items.size() - 1 ) );
}

void CItemListDialog::OnCopyItems( void )
{
	std::tstring items = str::Unsplit( m_items, _T("\r\n") );
	CClipboard::CopyText( items, this );
}

void CItemListDialog::OnUpdateCopyItems( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !m_items.empty() );
}

void CItemListDialog::OnPasteItems( void )
{
	std::tstring text;
	if ( CClipboard::PasteText( text, this ) )
	{
		m_content.SplitItems( m_items, text, _T("\r\n") );
		UpdateData( DialogOutput );
	}
}

void CItemListDialog::OnUpdatePasteItems( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( CClipboard::CanPasteText() );
}
