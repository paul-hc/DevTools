
#include "stdafx.h"
#include "ItemContentEdit.h"
#include "ItemListDialog.h"
#include "StringUtilities.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CItemContentEdit implementation

CItemContentEdit::CItemContentEdit( ui::ContentType type /*= ui::String*/, const TCHAR* pFileFilter /*= NULL*/ )
	: CBaseItemContentCtrl< CTextEdit >( type, pFileFilter )
{
	SetUseFixedFont( false );
	SetContentType( type );			// also switch the icon
}

CItemContentEdit::~CItemContentEdit()
{
}

void CItemContentEdit::OnBuddyCommand( UINT cmdId )
{
	if ( ui::String == m_content.m_type )					// not very useful
	{
		BaseClass::OnBuddyCommand( cmdId );
		return;
	}
	else
	{
		std::tstring newItem = m_content.EditItem( ui::GetWindowText( *this ).c_str(), GetParent() );
		if ( newItem.empty() )
			return;					// cancelled by user

		if ( IsWritable() )
			ui::SetWindowText( *this, newItem );
	}

	SetFocus();
	SelectAll();
	ui::SendCommandToParent( m_hWnd, CN_DETAILSCHANGED );
}


// CItemListEdit implementation

CItemListEdit::CItemListEdit( const TCHAR* pSeparator /*= _T(";")*/ )
	: CBaseItemContentCtrl< CTextEdit >()
	, m_pSeparator( pSeparator )
	, m_listEditor( ListDialog )
{
	SetContentType( ui::String );			// also switch the icon
}

CItemListEdit::~CItemListEdit()
{
}

void CItemListEdit::DDX_Items( CDataExchange* pDX, std::tstring& rFlatItems, int ctrlId /*= 0*/ )
{
	if ( NULL == m_hWnd && ctrlId != 0 )
		DDX_Control( pDX, ctrlId, *this );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
		SetText( rFlatItems );
	else
	{
		std::vector< std::tstring > items;
		m_content.SplitItems( items, GetText().c_str(), m_pSeparator );		// trim, remove empty, ensure unique
		rFlatItems = str::Join( items, m_pSeparator );
	}
}

void CItemListEdit::DDX_Items( CDataExchange* pDX, std::vector< std::tstring >& rItems, int ctrlId /*= 0*/ )
{
	if ( NULL == m_hWnd && ctrlId != 0 )
		DDX_Control( pDX, ctrlId, *this );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
		SetText( str::Join( rItems, m_pSeparator ) );
	else
		m_content.SplitItems( rItems, GetText().c_str(), m_pSeparator );
}

void CItemListEdit::DDX_ItemsUiEscapeSeqs( CDataExchange* pDX, std::tstring& rFlatItems, int ctrlId /*= 0*/ )
{
	if ( NULL == m_hWnd && ctrlId != 0 )
		DDX_Control( pDX, ctrlId, *this );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
		SetText( ui::FormatEscapeSeq( rFlatItems ) );
	else
	{
		std::vector< std::tstring > items;
		m_content.SplitItems( items, GetText().c_str(), m_pSeparator );		// trim, remove empty, ensure unique
		rFlatItems = ui::ParseEscapeSeqs( str::Join( items, m_pSeparator ) );
	}
}

void CItemListEdit::DDX_ItemsUiEscapeSeqs( CDataExchange* pDX, std::vector< std::tstring >& rItems, int ctrlId /*= 0*/ )
{
	if ( NULL == m_hWnd && ctrlId != 0 )
		DDX_Control( pDX, ctrlId, *this );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
		SetText( ui::FormatEscapeSeq( str::Join( rItems, m_pSeparator ) ) );
	else
		m_content.SplitItems( rItems, ui::ParseEscapeSeqs( GetText() ).c_str(), m_pSeparator );
}

void CItemListEdit::OnBuddyCommand( UINT cmdId )
{
	if ( Custom == m_listEditor )
	{
		BaseClass::OnBuddyCommand( cmdId );
		return;
	}

	CItemListDialog dialog( GetParent(), m_content );
	str::Split( dialog.m_items, ui::GetWindowText( m_hWnd ).c_str(), m_pSeparator );
	if ( !IsWritable() )
		dialog.m_readOnly = true;
	if ( dialog.DoModal() != IDOK )
		return;
	ui::SetWindowText( m_hWnd, str::Join( dialog.m_items, m_pSeparator ) );

	SetFocus();
	ui::SendCommandToParent( m_hWnd, CN_DETAILSCHANGED );
}

UINT CItemListEdit::GetStockButtonIconId( void ) const
{
	if ( ui::String == m_content.m_type )
		return ID_EDIT_LIST_ITEMS;
	return BaseClass::GetStockButtonIconId();
}