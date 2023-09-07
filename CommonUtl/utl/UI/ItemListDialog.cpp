
#include "pch.h"
#include "ItemListDialog.h"
#include "PathItemBase.h"
#include "EnumTags.h"
#include "StringCompare.h"
#include "TextEdit.h"
#include "WndUtils.h"
#include "resource.h"
#include "utl/ContainerOwnership.h"
#include "utl/TextClipboard.h"
#include <afxpriv.h>		// for WM_IDLEUPDATECMDUI

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section_dialog[] = _T("utl\\ItemListDialog");
	static const TCHAR section_list[] = _T("utl\\ItemListDialog\\list");
}

namespace layout
{
	static CLayoutStyle s_styles[] =
	{
		{ IDC_TOOLBAR_PLACEHOLDER, SizeX },
		{ IDC_ITEMS_SHEET, Size },
		{ IDOK, Move },
		{ IDCANCEL, Move }
	};
}

CItemListDialog::CItemListDialog( CWnd* pParent, const ui::CItemContent& content, const TCHAR* pTitle /*= nullptr*/ )
	: CLayoutDialog( IDD_ITEM_LIST_DIALOG, pParent )
	, m_readOnly( false )
	, m_content( content )
	, m_pTitle( pTitle )
	, m_selItemPos( utl::npos )
{
	m_regSection = m_childSheet.m_regSection = reg::section_dialog;
	RegisterCtrlLayout( ARRAY_SPAN( layout::s_styles ) );
	LoadDlgIcon( ID_EDIT_LIST_ITEMS );
	m_idleUpdateDeep = true;				// for CItemsEditPage::OnSelectedLinesChanged

	m_childSheet.AddPage( new CItemsListPage( this ) );
	m_childSheet.AddPage( new CItemsEditPage( this ) );

	m_toolbar.GetStrip()
		.AddButton( ID_ADD_ITEM )
		.AddButton( ID_REMOVE_ITEM )
		.AddButton( ID_REMOVE_ALL_ITEMS )
		.AddSeparator()
		.AddButton( ID_EDIT_ITEM )
		.AddSeparator()
		.AddButton( ID_MOVE_UP_ITEM )
		.AddButton( ID_MOVE_DOWN_ITEM )
		.AddSeparator()
		.AddButton( ID_EDIT_COPY )
		.AddButton( ID_EDIT_PASTE );
}

void CItemListDialog::SetSelItemPos( size_t selItemPos )
{
	m_selItemPos = std::min( selItemPos, m_items.size() - 1 );		// limit to bounds
}

bool CItemListDialog::InEditMode( void ) const
{
	return m_internal.IsInternalChange() || GetActivePage()->InEditMode();
}

bool CItemListDialog::EditSelItem( void )
{
	if ( !m_readOnly )
		if ( detail::IContentPage* pActivePage = GetActivePage() )
			return pActivePage->EditSelItem();

	return false;
}

bool CItemListDialog::InputItem( size_t itemPos, const std::tstring& newItem )
{
	if ( itemPos >= m_items.size() )
		return false;

	if ( !m_content.IsValidItem( newItem ) )
	{
		ui::ReportError( _T("Input error: the item must be valid.") );
		UpdateData( DialogOutput );					// rollback to old item value
		return false;
	}

	std::vector<std::tstring> items = m_items;
	items[ itemPos ] = newItem;
	m_content.FilterItems( items );					// rely on content-specific validation
	if ( items.size() != m_items.size() )
	{
		ui::ReportError( _T("Input error: the item must be unique.") );
		UpdateData( DialogOutput );					// rollback to old item value
		return false;
	}

	m_items[ itemPos ] = items[ itemPos ];			// do the validated input (trimmed, etc)
	UpdateData( DialogOutput );
	return true;
}

bool CItemListDialog::InputAllItems( const std::vector<std::tstring>& items )
{
	m_items = items;
	m_content.FilterItems( m_items );				// rely on content-specific validation
	SetSelItemPos( m_selItemPos );
	if ( m_items.size() != items.size() )
	{
		ui::ReportError( _T("Input error: items must be unique and not empty.") );
		UpdateData( DialogOutput );					// rollback to old item value
		return false;
	}
	UpdateData( DialogOutput );
	return true;
}

void CItemListDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = nullptr == m_toolbar.m_hWnd;

	m_toolbar.DDX_Placeholder( pDX, IDC_TOOLBAR_PLACEHOLDER, H_AlignLeft | V_AlignCenter );

	if ( firstInit )
	{
		if ( m_pTitle != nullptr )
			SetWindowText( m_pTitle );
		else
			ui::SetWindowText( m_hWnd, str::Format( _T("Edit %s Items"), ui::GetTags_ContentType().FormatUi( m_content.m_type ).c_str() ) );

		ui::EnableControl( m_hWnd, IDOK, !m_readOnly );

		m_selItemPos = !m_items.empty() ? 0 : utl::npos;
	}

	m_childSheet.DDX_DetailSheet( pDX, IDC_ITEMS_SHEET );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
		m_childSheet.OutputPages();
	else
	{
		m_childSheet.SetSheetModified();
		m_childSheet.ApplyChanges();
	}

	CLayoutDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CItemListDialog, CLayoutDialog )
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

void CItemListDialog::OnAddItem( void )
{
	CScopedInternalChange internalChange( &m_internal );
	m_toolbar.UpdateCmdUI();		// update the buttons state since during label edit modal loop there are no idle UI updates

	size_t addingPos = m_selItemPos;

	if ( addingPos != utl::npos )
		++addingPos;
	else
		addingPos = m_items.size();

	m_items.insert( m_items.begin() + addingPos, std::tstring() );
	m_selItemPos = addingPos;
	UpdateData( DialogOutput );

	if ( !EditSelItem() )
	{	// remove added item
		m_items.erase( m_items.begin() + addingPos );
		SetSelItemPos( --addingPos );
		UpdateData( DialogOutput );
	}
}

void CItemListDialog::OnUpdateAddItem( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !m_readOnly && !InEditMode() );
}

void CItemListDialog::OnRemoveItem( void )
{
	m_items.erase( m_items.begin() + m_selItemPos );
	m_selItemPos = std::min( m_selItemPos, m_items.size() - 1 );
	UpdateData( DialogOutput );
}

void CItemListDialog::OnUpdateRemoveItem( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !m_readOnly && m_selItemPos != -1 && !InEditMode() );
}

void CItemListDialog::OnRemoveAll( void )
{
	if ( IDOK == AfxMessageBox( _T("Delete all items in the list?"), MB_OKCANCEL | MB_ICONQUESTION ) )
	{
		m_items.clear();
		m_selItemPos = utl::npos;
		UpdateData( DialogOutput );
	}
}

void CItemListDialog::OnUpdateRemoveAll( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !m_readOnly && !m_items.empty() && !InEditMode() );
}

void CItemListDialog::OnEditItem( void )
{
	ASSERT( m_selItemPos != utl::npos );
	EditSelItem();
}

void CItemListDialog::OnUpdateEditItem( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !m_readOnly && m_selItemPos != utl::npos && !InEditMode() );
}

void CItemListDialog::OnMoveUpItem( void )
{
	REQUIRE( m_selItemPos > 0 && m_selItemPos < m_items.size() );

	std::swap( m_items[ m_selItemPos ], m_items[ m_selItemPos - 1 ] );
	--m_selItemPos;
	UpdateData( DialogOutput );
}

void CItemListDialog::OnUpdateMoveUpItem( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !m_readOnly && m_selItemPos > 0 && m_selItemPos < m_items.size() && !InEditMode() );
}

void CItemListDialog::OnMoveDownItem( void )
{
	REQUIRE( m_selItemPos < m_items.size() - 1 );

	std::swap( m_items[ m_selItemPos ], m_items[ m_selItemPos + 1 ] );
	++m_selItemPos;
	UpdateData( DialogOutput );
}

void CItemListDialog::OnUpdateMoveDownItem( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !m_readOnly && m_selItemPos < ( m_items.size() - 1 ) && !InEditMode() );
}

void CItemListDialog::OnCopyItems( void )
{
	std::tstring items = str::Join( m_items, _T("\r\n") );
	CTextClipboard::CopyText( items, m_hWnd );
}

void CItemListDialog::OnUpdateCopyItems( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !m_items.empty() && !InEditMode() );
}

const TCHAR* CItemListDialog::FindSeparatorMostUsed( const std::tstring& text )
{
	static const TCHAR* s_sepArray[] = { _T(";"), _T("|"), _T("\r\n"), _T("\n") };

	return *std::max_element( s_sepArray, END_OF( s_sepArray ), pred::LessSequenceCount<TCHAR>(text.c_str()));
}

void CItemListDialog::OnPasteItems( void )
{
	std::tstring text;
	if ( CTextClipboard::PasteText( text, m_hWnd ) )
	{
		m_content.SplitItems( m_items, text, FindSeparatorMostUsed( text ) );
		m_selItemPos = std::min( m_selItemPos, m_items.size() - 1 );
		UpdateData( DialogOutput );
	}
}

void CItemListDialog::OnUpdatePasteItems( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( CTextClipboard::CanPasteText() && !InEditMode() );
}


// CItemsListPage class

namespace layout
{
	static CLayoutStyle s_listPageStyles[] =
	{
		{ IDC_ITEMS_LIST, Size }
	};
}

CItemsListPage::CItemsListPage( CItemListDialog* pDialog )
	: CLayoutPropertyPage( IDD_ITEMS_LIST_PAGE )
	, m_pDialog( pDialog )
	, m_rContent( m_pDialog->GetContent() )
	, m_listCtrl( IDC_ITEMS_LIST )
	, m_accel( IDR_LIST_EDITOR_ACCEL )
{
	RegisterCtrlLayout( ARRAY_SPAN( layout::s_listPageStyles ) );
	SetUseLazyUpdateData();			// call UpdateData on page activation change

	m_listCtrl.SetSection( reg::section_list );
	m_listCtrl.SetSubjectAdapter( ui::GetFullPathAdapter() );			// display full paths
}

CItemsListPage::~CItemsListPage()
{
	utl::ClearOwningContainer( m_pathItems );
}

bool CItemsListPage::InEditMode( void ) const
{
	return m_listCtrl.GetEditControl() != nullptr;
}

bool CItemsListPage::EditSelItem( void )
{
	int selIndex = m_pDialog->GetSelItemIndex();
	std::tstring newItem;

	if ( ui::String == m_rContent.m_type )
	{
		if ( const CReportListControl::CLabelEdit* pLabelEdit = m_listCtrl.EditLabelModal( selIndex ) )
			newItem = pLabelEdit->m_newLabel;
		else
			return false;
	}
	else
	{
		newItem = m_rContent.EditItem( GetListItemText( selIndex ).c_str(), this, 0 );
		if ( !newItem.empty() )
			m_listCtrl.SetItemText( selIndex, Item, newItem.c_str() );
		else
			return false;			// canceled by user
	}

	return m_pDialog->InputItem( selIndex, newItem );
}

std::tstring CItemsListPage::GetListItemText( int index ) const
{
	if ( ui::String == m_rContent.m_type )
		return m_listCtrl.GetItemText( index, CPathItemListCtrl::Code ).GetString();

	return m_listCtrl.FormatCode( m_listCtrl.GetSubjectAt( index ) );
}

void CItemsListPage::QueryListItems( std::vector<std::tstring>& rItems ) const
{
	unsigned int count = m_listCtrl.GetItemCount();
	rItems.clear();
	rItems.reserve( count );
	for ( unsigned int i = 0; i != count; ++i )
		rItems.push_back( GetListItemText( i ) );
}

void CItemsListPage::OutputList( void )
{
	std::vector<std::tstring> listItems;
	QueryListItems( listItems );

	CScopedInternalChange internalChange( &m_listCtrl );

	if ( m_pDialog->m_items != listItems )
	{
		m_listCtrl.DeleteAllItems();

		if ( ui::String == m_rContent.m_type )
		{
			for ( unsigned int i = 0; i != m_pDialog->m_items.size(); ++i )
				m_listCtrl.InsertItem( i, m_pDialog->m_items[ i ].c_str() );
		}
		else
		{
			CPathItem::MakePathItems( m_pathItems, m_pDialog->m_items );

			for ( unsigned int i = 0; i != m_pathItems.size(); ++i )
				m_listCtrl.InsertObjectItem( i, m_pathItems[ i ] );
		}
	}

	int selIndex = m_pDialog->GetSelItemIndex();
	if ( selIndex != -1 )
	{
		m_listCtrl.SetCurSel( selIndex );
		m_listCtrl.EnsureVisible( selIndex, false );
	}
}

void CItemsListPage::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = nullptr == m_listCtrl.m_hWnd;

	DDX_Control( pDX, IDC_ITEMS_LIST, m_listCtrl );

	if ( firstInit )
	{
		switch ( m_rContent.m_type )
		{
			case ui::String:
				m_listCtrl.SetFont( CTextEdit::GetFixedFont( CTextEdit::Large ) );
				break;
			case ui::DirPath:
			case ui::FilePath:
			case ui::MixedPath:
				//m_listCtrl.SetCustomFileGlyphDraw();
				break;
		}
	}

	if ( DialogOutput == pDX->m_bSaveAndValidate )
		OutputList();

	CLayoutPropertyPage::DoDataExchange( pDX );
}

BEGIN_MESSAGE_MAP( CItemsListPage, CLayoutPropertyPage )
	ON_NOTIFY( LVN_ITEMCHANGED, IDC_ITEMS_LIST, OnLvnItemChanged_Items )
	ON_NOTIFY( LVN_ENDLABELEDIT, IDC_ITEMS_LIST, OnLvnEndLabelEdit_Items )
END_MESSAGE_MAP()

BOOL CItemsListPage::PreTranslateMessage( MSG* pMsg )
{
	return
		m_accel.Translate( pMsg, m_pDialog->m_hWnd, m_listCtrl ) ||
		CLayoutPropertyPage::PreTranslateMessage( pMsg );
}

void CItemsListPage::OnLvnItemChanged_Items( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLISTVIEW* pNmList = (NMLISTVIEW*)pNmHdr;
	*pResult = 0;

	if ( CReportListControl::IsSelectionChangeNotify( pNmList, LVIS_SELECTED | LVIS_FOCUSED ) )
	{
		int selIndex = m_listCtrl.GetCurSel();
		if ( selIndex != -1 )
			m_pDialog->SetSelItemPos( selIndex );
	}
}

void CItemsListPage::OnLvnEndLabelEdit_Items( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLVDISPINFO* pDispInfo = (NMLVDISPINFO*)pNmHdr;
	*pResult = 0;					// rollback input

	if ( pDispInfo->item.pszText != nullptr )
		if ( m_pDialog->InputItem( pDispInfo->item.iItem, pDispInfo->item.pszText ) )
			*pResult = TRUE;		// signal valid input so it will commit the changes
}


// CItemsEditPage class

namespace layout
{
	static CLayoutStyle s_editPageStyles[] =
	{
		{ IDC_ITEMS_EDIT, Size }
	};
}

const TCHAR CItemsEditPage::s_lineEnd[] = _T("\r\n");

CItemsEditPage::CItemsEditPage( CItemListDialog* pDialog )
	: CLayoutPropertyPage( IDD_ITEMS_EDIT_PAGE )
	, m_pDialog( pDialog )
	, m_rContent( m_pDialog->GetContent() )
	, m_selLineRange( -1 )
	, m_accel( IDD_ITEMS_EDIT_PAGE )
{
	RegisterCtrlLayout( ARRAY_SPAN( layout::s_editPageStyles ) );
	SetUseLazyUpdateData();			// call UpdateData on page activation change
	m_mlEdit.SetKeepSelOnFocus();
}

bool CItemsEditPage::InEditMode( void ) const
{
	return false;
}

bool CItemsEditPage::EditSelItem( void )
{
	std::tstring currLine = m_mlEdit.GetLineTextAt();
	int startPos = m_mlEdit.LineIndex();
	if ( startPos != -1 )
		m_mlEdit.SetSel( startPos, startPos + static_cast<int>( currLine.length() ) );

	int selIndex = m_pDialog->GetSelItemIndex();
	std::tstring newItem;

	if ( ui::String == m_rContent.m_type )
		return true;

	newItem = m_rContent.EditItem( currLine.c_str(), this, 0 );
	if ( newItem.empty() )
		return false;			// canceled by user

	m_mlEdit.ReplaceSel( newItem.c_str(), TRUE );
	m_mlEdit.SetFocus();

	if ( startPos != -1 )
		m_mlEdit.SetSel( startPos, startPos + static_cast<int>( newItem.length() ) );

	return m_pDialog->InputItem( selIndex, newItem );
}

Range<int> CItemsEditPage::GetLineRange( int linePos ) const
{
	Range<int> lineRange( m_mlEdit.LineIndex( linePos ) );
	lineRange.m_end += m_mlEdit.LineLength( lineRange.m_start );
	return lineRange;
}

Range<int> CItemsEditPage::SelectLine( int linePos )
{
	Range<int> lineRange = GetLineRange( linePos );
	if ( lineRange.m_start != -1 )
		m_mlEdit.SetSel( lineRange.m_start, lineRange.m_end );
	return lineRange;
}

void CItemsEditPage::QueryEditItems( std::vector<std::tstring>& rItems ) const
{
	str::Split( rItems, m_mlEdit.GetText().c_str(), s_lineEnd );
}

void CItemsEditPage::OutputEdit( void )
{
	std::vector<std::tstring> editItems;
	QueryEditItems( editItems );

	CScopedInternalChange internalChange( &m_mlEdit );

	if ( m_pDialog->m_items != editItems )
		m_mlEdit.SetText( str::Join( m_pDialog->m_items, s_lineEnd ) );

	int selIndex = m_pDialog->GetSelItemIndex();
	if ( selIndex != -1 )
	{
		SelectLine( selIndex );
	}
}

void CItemsEditPage::OnSelectedLinesChanged( void )
{
	int selIndex = m_mlEdit.LineFromChar();
	if ( selIndex != -1 )
		m_pDialog->SetSelItemPos( selIndex );

	TRACE( _T(" - ml-edit sel lines[%d]: {%d, %d}\n"), selIndex, m_selLineRange.m_start, m_selLineRange.m_end );
}

void CItemsEditPage::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = nullptr == m_mlEdit.m_hWnd;

	DDX_Control( pDX, IDC_ITEMS_EDIT, m_mlEdit );
	if ( firstInit )
		m_mlEdit.SetLimitText( 100 * MAX_PATH );		// 100 lines

	if ( DialogSaveChanges == pDX->m_bSaveAndValidate )
	{
		std::vector<std::tstring> items;
		QueryEditItems( items );
		m_pDialog->InputAllItems( items );
	}
	else
		OutputEdit();			// always output, even on dialog input, since invalid items may get removed

	CLayoutPropertyPage::DoDataExchange( pDX );
}

BEGIN_MESSAGE_MAP( CItemsEditPage, CLayoutPropertyPage )
	ON_MESSAGE( WM_IDLEUPDATECMDUI, OnIdleUpdateCmdUI )
	ON_COMMAND( ID_EDIT_SELECT_ALL, OnEditSelectAll )
END_MESSAGE_MAP()

BOOL CItemsEditPage::PreTranslateMessage( MSG* pMsg )
{
	return
		m_accel.Translate( pMsg, m_pDialog->m_hWnd, m_mlEdit ) ||
		CLayoutPropertyPage::PreTranslateMessage( pMsg );
}

LRESULT CItemsEditPage::OnIdleUpdateCmdUI( WPARAM wParam, LPARAM lParam )
{
	wParam, lParam;
	Range<int> selLineRange = m_mlEdit.GetLineRangeAt();
	if ( selLineRange != m_selLineRange )
	{
		m_selLineRange = selLineRange;
		OnSelectedLinesChanged();
	}
	return 0L;
}

void CItemsEditPage::OnEditSelectAll( void )
{
	m_mlEdit.SetSel( 0, -1, TRUE );
}
