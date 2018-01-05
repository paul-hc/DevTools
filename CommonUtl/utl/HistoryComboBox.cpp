
#include "stdafx.h"
#include "HistoryComboBox.h"
#include "TextEditor.h"
#include "ItemListDialog.h"
#include "MenuUtilities.h"
#include "Utilities.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


class CComboDropList : public CWnd		// can't inherit from CListBox
{
public:
	CComboDropList( CComboBox* pParentCombo )
		: m_pParentCombo( pParentCombo )
	{
		ASSERT_PTR( m_pParentCombo->GetSafeHwnd() );
	}
private:
	CComboBox* m_pParentCombo;

	// generated stuff
private:
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint point );

	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP( CComboDropList, CWnd )
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

void CComboDropList::OnContextMenu( CWnd* pWnd, CPoint point )
{
	CWnd::OnContextMenu( pWnd, point );
	m_pParentCombo->SendMessage( WM_CONTEXTMENU, (WPARAM)pWnd->GetSafeHwnd(), MAKELPARAM( point.x, point.y ) );
}


// CHistoryComboBox implementation

static ACCEL keys[] =
{
	{ FVIRTKEY | FCONTROL, VK_RETURN, ID_ADD_ITEM }
};

static ACCEL dropDownKeys[] =
{
	{ FVIRTKEY, VK_DELETE, ID_REMOVE_ITEM }
};


CHistoryComboBox::CHistoryComboBox( unsigned int maxCount /*= ui::HistoryMaxSize*/, const TCHAR* pItemSep /*= _T(";")*/, str::CaseType caseType /*= str::Case*/ )
	: CComboBox()
	, m_maxCount( maxCount )
	, m_pItemSep( pItemSep )
	, m_caseType( caseType )
	, m_frameColor( CLR_NONE )
	, m_accel( keys, COUNT_OF( keys ) )
	, m_dropDownAccel( dropDownKeys, COUNT_OF( dropDownKeys ) )
	, m_dropSelIndex( CB_ERR )
	, m_pSection( NULL )
	, m_pEntry( NULL )
{
}

CHistoryComboBox::~CHistoryComboBox()
{
}

void CHistoryComboBox::SaveHistory( const TCHAR* pSection, const TCHAR* pEntry )
{
	ui::SaveHistoryCombo( *this, pSection, pEntry, m_pItemSep, m_maxCount, m_caseType );
}

void CHistoryComboBox::LoadHistory( const TCHAR* pSection, const TCHAR* pEntry, const TCHAR* pDefaultText /*= NULL*/ )
{
	// store registry info for inplace saving
	m_pSection = pSection;
	m_pEntry = pEntry;

	ui::LoadHistoryCombo( *this, m_pSection, m_pEntry, pDefaultText, m_pItemSep );
}

std::tstring CHistoryComboBox::GetCurrentText( void ) const
{
	return ui::GetComboSelText( *this );
}

bool CHistoryComboBox::SetFrameColor( COLORREF frameColor )
{
	if ( m_frameColor == frameColor )
		return false;

	m_frameColor = frameColor;
	Invalidate();
	return true;
}

int CHistoryComboBox::GetCmdSelIndex( void ) const
{
	if ( GetDroppedState() )			// allow user to delete selected items when the combo list is dropped down
		return GetCurSel();				// while in LBox it changes dynamically when highligting a new item

	if ( m_dropSelIndex >= 0 && m_dropSelIndex < GetCount() )
		return m_dropSelIndex;
	return CB_ERR;
}

void CHistoryComboBox::ValidateContent( void )
{
	ui::SendCommandToParent( m_hWnd, HCN_VALIDATEITEMS );		// give parent a chance to cleanup invalid items

	std::vector< std::tstring > items;
	ui::ReadComboItems( items, *this );

	std::vector< std::tstring > newItems = items;
	GetItemContent().FilterItems( newItems );

	if ( newItems != items )
		ui::WriteComboItems( *this, newItems );
}

void CHistoryComboBox::PreSubclassWindow( void )
{
	CComboBox::PreSubclassWindow();

	COMBOBOXINFO cbInfo = { sizeof( COMBOBOXINFO ) };
	if ( GetComboBoxInfo( &cbInfo ) )
	{
		if ( cbInfo.hwndList != NULL && NULL == m_pEdit.get() )
		{
			m_pEdit.reset( new CTextEditor );
			m_pEdit->SubclassWindow( cbInfo.hwndItem );
		}
		if ( cbInfo.hwndList != NULL && NULL == m_pDropList.get() )
		{
			m_pDropList.reset( new CComboDropList( this ) );
			m_pDropList->SubclassWindow( cbInfo.hwndList );
		}
	}
}

BOOL CHistoryComboBox::PreTranslateMessage( MSG* pMsg )
{
	if ( CAccelTable::IsKeyMessage( pMsg ) )
	{
		if ( GetDroppedState() )			// allow user to delete selected items when the combo list is dropped down
		{
			if ( GetCurSel() != CB_ERR && m_dropDownAccel.Translate( pMsg, m_hWnd ) )
				return true;
		}
		if ( m_accel.Translate( pMsg, m_hWnd ) )
			return true;
	}

	return CComboBox::PreTranslateMessage( pMsg );
}


// message handlers

BEGIN_MESSAGE_MAP( CHistoryComboBox, CComboBox )
	ON_WM_CONTEXTMENU()
	ON_WM_PAINT()
	ON_COMMAND( ID_ADD_ITEM, OnStoreEditItem )
	ON_COMMAND( ID_REMOVE_ITEM, OnDeleteListItem )
	ON_UPDATE_COMMAND_UI( ID_REMOVE_ITEM, OnUpdateSelectedListItem )
	ON_COMMAND( ID_REMOVE_ALL_ITEMS, OnDeleteAllListItems )
	ON_UPDATE_COMMAND_UI( ID_REMOVE_ALL_ITEMS, OnUpdateDeleteAllListItems )
	ON_COMMAND( ID_EDIT_LIST_ITEMS, OnEditListItems )
	ON_COMMAND( ID_FILE_SAVE, OnSaveHistory )
	ON_COMMAND( CHistoryComboBox::Cmd_ResetDropSelIndex, OnResetDropSelIndex )
END_MESSAGE_MAP()

void CHistoryComboBox::OnContextMenu( CWnd* pWnd, CPoint point )
{
	CMenu contextMenu;

	m_dropSelIndex = CB_ERR;
	if ( GetDroppedState() )
	{
		COMBOBOXINFO cbInfo = { sizeof( COMBOBOXINFO ) };
		if ( GetComboBoxInfo( &cbInfo ) )
			if ( pWnd == m_pDropList.get() )
				ui::LoadPopupMenu( contextMenu, IDR_STD_CONTEXT_MENU, ui::HistoryComboPopup );
	}
	else if ( pWnd == this )
		ui::LoadPopupMenu( contextMenu, IDR_STD_CONTEXT_MENU, ui::HistoryComboPopup );

	if ( contextMenu.GetSafeHmenu() != NULL )
	{
		m_dropSelIndex = GetCurSel();
		ui::TrackPopupMenu( contextMenu, this, point, TPM_RIGHTBUTTON );
		// delayed reset of m_dropSelIndex, after the actual command is processed
		PostMessage( WM_COMMAND, MAKEWPARAM( Cmd_ResetDropSelIndex, BN_CLICKED ), 0 );
		return;
	}

	CComboBox::OnContextMenu( pWnd, point );
}

void CHistoryComboBox::OnPaint( void )
{
	CComboBox::OnPaint();

	if ( m_frameColor != CLR_NONE )
	{
		CClientDC dc( this );
		CBrush borderBrush( m_frameColor );
		CRect rect;
		GetClientRect( &rect );
		dc.FrameRect( &rect, &borderBrush );
	}
}

void CHistoryComboBox::OnUpdateSelectedListItem( CCmdUI* pCmdUI )
{
	TRACE( _T("* GetCurSel()=%d dropSelIndex=%d\n"), GetCurSel(), GetCmdSelIndex() );

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
	ui::UpdateHistoryCombo( *this, m_maxCount, m_caseType );		// store edit item in the list (with validation)
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
