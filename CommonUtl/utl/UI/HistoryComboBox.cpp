
#include "pch.h"
#include "HistoryComboBox.h"
#include "ComboBoxEdit.h"
#include "TextEditor.h"
#include "ItemListDialog.h"
#include "MenuUtilities.h"
#include "ShellPidl.h"
#include "WndUtils.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CHistoryComboBox implementation

static ACCEL s_keys[] =
{
	{ FVIRTKEY | FCONTROL, VK_RETURN, ID_ADD_ITEM }		// Ctrl + Plus
};

static ACCEL s_dropDownKeys[] =
{
	{ FVIRTKEY | FCONTROL, VK_BACK, ID_REMOVE_ITEM }	// Ctrl + Backspace
};


CHistoryComboBox::CHistoryComboBox( unsigned int maxCount /*= ui::HistoryMaxSize*/, const TCHAR* pItemSep /*= _T(";")*/, str::CaseType caseType /*= str::Case*/ )
	: CFrameHostCtrl<CComboBox>()
	, m_maxCount( maxCount )
	, m_pItemSep( pItemSep )
	, m_caseType( caseType )
	, m_accel( ARRAY_SPAN( s_keys ) )
	, m_dropDownAccel( ARRAY_SPAN( s_dropDownKeys ) )
	, m_dropSelIndex( CB_ERR )
	, m_pSection( nullptr )
	, m_pEntry( nullptr )
{
	SetFocusMargins( 2, 2 );
	SetShowFocus();
}

CHistoryComboBox::~CHistoryComboBox()
{
}

void CHistoryComboBox::SaveHistory( const TCHAR* pSection, const TCHAR* pEntry )
{
	ui::SaveHistoryCombo( *this, pSection, pEntry, m_pItemSep, m_maxCount, m_caseType );
}

void CHistoryComboBox::LoadHistory( const TCHAR* pSection, const TCHAR* pEntry, const TCHAR* pDefaultText /*= nullptr*/ )
{
	// store registry info for inplace saving
	m_pSection = pSection;
	m_pEntry = pEntry;

	ui::LoadHistoryCombo( *this, m_pSection, m_pEntry, pDefaultText, m_pItemSep );
	UpdateContentStateFrame();
}

std::tstring CHistoryComboBox::GetCurrentText( void ) const
{
	return ui::GetComboSelText( *this );
}

std::pair<bool, ui::ComboField> CHistoryComboBox::SetEditText( const std::tstring& currText )
{
	return ui::SetComboEditText( *this, currText, m_caseType );
}

void CHistoryComboBox::StoreCurrentEditItem( void )
{
	ui::UpdateHistoryCombo( *this, m_maxCount, m_caseType );		// store edit item in the list (with validation)
}

void CHistoryComboBox::SetEdit( CTextEditor* pEdit )
{
	ASSERT_NULL( m_hWnd );				// must be called before creation
	m_pEdit.reset( pEdit );
}

int CHistoryComboBox::GetCmdSelIndex( void ) const
{
	if ( GetDroppedState() )			// allow user to delete selected items when the combo list is dropped down
		return GetCurSel();				// while in LBox it changes dynamically when highligting a new item

	if ( m_dropSelIndex >= 0 && m_dropSelIndex < GetCount() )
		return m_dropSelIndex;
	return CB_ERR;
}

void CHistoryComboBox::QueryTooltipText( OUT std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const implement
{
	cmdId, pTooltip;

	if ( GetItemContent().IsPathContent() )		// virtual call (for sub-classes that manage their own ui::CItemContent object)
	{
		fs::CPath filePath( ui::GetComboSelText( *this ) );
		filePath.Expand();

		if ( filePath.IsGuidPath() )
		{
			std::tstring wildPattern;
			fs::CSearchPatternParts::SplitPattern( &filePath, &wildPattern, filePath );		// split into folder path and wildcard pattern

			shell::CPidlAbsolute itemPidl( filePath.GetPtr() );
			if ( !itemPidl.IsNull() )
			{	// display the friendly editing name
				filePath = itemPidl.GetEditingName();
				filePath /= wildPattern;
				rText = filePath.Get();
			}
		}
		else
			rText = filePath.Get();
	}
}

void CHistoryComboBox::ValidateContent( void )
{
	ui::SendCommandToParent( m_hWnd, HCN_VALIDATEITEMS );		// give parent a chance to cleanup invalid items

	std::vector<std::tstring> items;
	ui::ReadComboItems( items, *this );

	std::vector<std::tstring> newItems = items;
	GetItemContent().FilterItems( newItems );

	if ( newItems != items )
		ui::WriteComboItems( *this, newItems );
}

bool CHistoryComboBox::UpdateContentStateFrame( void )
{
	bool validContent = true;

	if ( GetItemContent().IsPathContent() )		// virtual call (for sub-classes that manage their own ui::CItemContent object)
	{
		fs::CPath filePath( ui::GetComboSelText( *this ) );

		if ( !filePath.IsEmpty() )		// content initialized?
		{
			validContent = filePath.FileMatchExist();
			SetFrameColor( validContent ? CLR_NONE : color::ScarletRed );
		}
	}

	return validContent;
}

void CHistoryComboBox::PreSubclassWindow( void )
{
	__super::PreSubclassWindow();

	//ModifyStyle( 0, CBS_AUTOHSCROLL );		// doesn't work after creation, must set in the .rc file!

	COMBOBOXINFO cbInfo = { sizeof( COMBOBOXINFO ) };
	if ( GetComboBoxInfo( &cbInfo ) )
	{
		if ( cbInfo.hwndItem != nullptr )
		{
			if ( nullptr == m_pEdit.get() )
				m_pEdit.reset( new CTextEditor() );

			m_pEdit->SubclassWindow( cbInfo.hwndItem );
			//m_pEdit->ModifyStyle( 0, ES_AUTOHSCROLL );		// doesn't work after creation, must set in the .rc file!
		}

		if ( cbInfo.hwndList != nullptr && nullptr == m_pDropList.get() )
		{
			m_pDropList.reset( new CComboDropList( this, false ) );
			m_pDropList->SubclassWindow( cbInfo.hwndList );
		}
	}
}

BOOL CHistoryComboBox::PreTranslateMessage( MSG* pMsg )
{
	if ( CAccelTable::IsKeyMessage( pMsg ) )
	{
		if ( GetDroppedState() )			// allow user to delete selected items when the combo list is dropped down
			if ( GetCurSel() != CB_ERR && m_dropDownAccel.Translate( pMsg, m_hWnd ) )
				return true;

		if ( m_accel.Translate( pMsg, m_hWnd ) )
			return true;
	}

	return __super::PreTranslateMessage( pMsg );
}


// message handlers

BEGIN_MESSAGE_MAP( CHistoryComboBox, TBaseClass )
	ON_WM_CONTEXTMENU()
	ON_CONTROL_REFLECT_EX( CBN_EDITCHANGE, OnChanged_Reflect )
	ON_CONTROL_REFLECT_EX( CBN_SELCHANGE, OnChanged_Reflect )
	ON_COMMAND( ID_ADD_ITEM, OnStoreEditItem )
	ON_UPDATE_COMMAND_UI( ID_ADD_ITEM, OnUpdateSelectedListItem )
	ON_COMMAND( ID_REMOVE_ITEM, OnDeleteListItem )
	ON_UPDATE_COMMAND_UI( ID_REMOVE_ITEM, OnUpdateSelectedListItem )
	ON_COMMAND( ID_REMOVE_ALL_ITEMS, OnDeleteAllListItems )
	ON_UPDATE_COMMAND_UI( ID_REMOVE_ALL_ITEMS, OnUpdateDeleteAllListItems )
	ON_COMMAND( ID_EDIT_LIST_ITEMS, OnEditListItems )
	ON_COMMAND( ID_FILE_SAVE, OnSaveHistory )
	ON_COMMAND( CHistoryComboBox::Cmd_ResetDropSelIndex, OnResetDropSelIndex )
END_MESSAGE_MAP()

void CHistoryComboBox::OnContextMenu( CWnd* pWnd, CPoint screenPos )
{
	CMenu contextMenu;

	m_dropSelIndex = CB_ERR;
	if ( GetDroppedState() )
	{
		COMBOBOXINFO cbInfo = { sizeof( COMBOBOXINFO ) };
		if ( GetComboBoxInfo( &cbInfo ) )
			if ( pWnd == m_pDropList.get() )
				ui::LoadPopupMenu( &contextMenu, IDR_STD_CONTEXT_MENU, ui::HistoryComboPopup );
	}
	else if ( pWnd == this )
		ui::LoadPopupMenu( &contextMenu, IDR_STD_CONTEXT_MENU, ui::HistoryComboPopup );

	if ( contextMenu.GetSafeHmenu() != nullptr )
	{
		m_dropSelIndex = GetCurSel();
		ui::TrackPopupMenu( contextMenu, this, screenPos, TPM_RIGHTBUTTON );
		// delayed reset of m_dropSelIndex, after the actual command is processed
		PostMessage( WM_COMMAND, MAKEWPARAM( Cmd_ResetDropSelIndex, BN_CLICKED ), 0 );
		return;
	}

	__super::OnContextMenu( pWnd, screenPos );
}

BOOL CHistoryComboBox::OnChanged_Reflect( void )
{
	UpdateContentStateFrame();
	return FALSE;					// continue routing
}

void CHistoryComboBox::OnUpdateSelectedListItem( CCmdUI* pCmdUI )
{
	TRACE_FL( _T("\n* GetCurSel()=%d dropSelIndex=%d\n"), GetCurSel(), GetCmdSelIndex() );

	pCmdUI->Enable( GetCmdSelIndex() != CB_ERR );
}

void CHistoryComboBox::OnDeleteListItem( void )
{
	int selIndex = GetCmdSelIndex();
	if ( selIndex != CB_ERR )
		DeleteString( selIndex );									// delete selected item (when combo list is dropped-down)
}

void CHistoryComboBox::OnStoreEditItem( void )
{
	StoreCurrentEditItem();
}

void CHistoryComboBox::OnDeleteAllListItems( void )
{
	ResetContent();
}

void CHistoryComboBox::OnUpdateDeleteAllListItems( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( GetCount() != 0 );
}

void CHistoryComboBox::OnEditListItems( void )
{
	CItemListDialog dlg( this, GetItemContent() );
	ui::ReadComboItems( dlg.m_items, *this );

	if ( IDOK == dlg.DoModal() )
	{
		ui::WriteComboItems( *this, dlg.m_items );
		ShowDropDown();
	}
}

void CHistoryComboBox::OnSaveHistory( void )
{
	if ( !str::IsEmpty( m_pSection ) && !str::IsEmpty( m_pEntry ) )
		SaveHistory( m_pSection, m_pEntry );
}

void CHistoryComboBox::OnResetDropSelIndex( void )
{
	TRACE( _T("* ResetDropSelIndex: m_dropSelIndex=%d\n"), m_dropSelIndex );
	m_dropSelIndex = CB_ERR;
}
