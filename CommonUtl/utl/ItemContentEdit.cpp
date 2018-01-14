
#include "stdafx.h"
#include "ItemContentEdit.h"
#include "ItemListDialog.h"
#include "StringUtilities.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


class CListEditDialog : public CLayoutDialog
{
public:
	CListEditDialog( const std::tstring& flatItems, const TCHAR* pSeparator, const ui::CItemContent& content, CWnd* pParent = NULL );

	static CFont* GetGlyphFont( void );
public:
	bool m_readOnly;
	std::tstring m_flatItems;
private:
	const TCHAR* m_pSeparator;
	ui::CItemContent m_content;

	// enum { IDD = IDD_LIST_EDIT_DIALOG };
	CTextEdit m_itemsEdit;

	// generated overrides
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	// generated message map
	virtual void OnOK( void );
	afx_msg void OnEditSelectAll( void );
	afx_msg void OnMoveUpDown( UINT cmdId );
	afx_msg void OnBrowseButton( void );

	DECLARE_MESSAGE_MAP()
};


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
	switch ( m_listEditor )
	{
		case ListDialog:
		{
			CItemListDialog dialog( GetParent(), m_content );
			str::Split( dialog.m_items, ui::GetWindowText( m_hWnd ).c_str(), m_pSeparator );
			if ( !IsWritable() )
				dialog.m_readOnly = true;
			if ( dialog.DoModal() != IDOK )
				return;
			ui::SetWindowText( m_hWnd, str::Join( dialog.m_items, m_pSeparator ) );
			break;
		}
		case ListEditDialog:
		{
			CListEditDialog dialog( ui::GetWindowText( m_hWnd ), m_pSeparator, m_content, GetParent() );
			if ( !IsWritable() )
				dialog.m_readOnly = true;
			if ( dialog.DoModal() != IDOK )
				return;
			ui::SetWindowText( m_hWnd, dialog.m_flatItems );
			break;
		}
		case Custom:
			BaseClass::OnBuddyCommand( cmdId );
			return;
	}

	SetFocus();
	ui::SendCommandToParent( m_hWnd, CN_DETAILSCHANGED );
}

UINT CItemListEdit::GetStockButtonIconId( void ) const
{
	if ( ui::String == m_content.m_type )
		return ID_EDIT_LIST_ITEMS;
	return BaseClass::GetStockButtonIconId();
}


// CListEditDialog implementation


static const TCHAR lineEnd[] = _T("\r\n");


namespace layout
{
	static CLayoutStyle styles[] =
	{
		{ IDC_LEDIT_ITEM_LIST_EDIT, Stretch },
		{ ID_MOVE_DOWN_ITEM, OffsetY },
		{ ID_MOVE_UP_ITEM, OffsetY },
		{ IDOK, Offset },
		{ IDC_LEDIT_BROWSE, Offset },
		{ IDCANCEL, Offset }
	};
}


CListEditDialog::CListEditDialog( const std::tstring& flatItems, const TCHAR* pSeparator, const ui::CItemContent& content, CWnd* pParent /*= NULL*/ )
	: CLayoutDialog( IDD_LIST_EDIT_DIALOG, pParent )
	, m_readOnly( false )
	, m_flatItems( flatItems )
	, m_pSeparator( pSeparator )
	, m_content( content )
{
	m_regSection = _T("utl\\ListEditDialog");
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );
}

CFont* CListEditDialog::GetGlyphFont( void )
{
	static CFont glyphFont;
	if ( NULL == glyphFont.GetSafeHandle() )
		ui::MakeStandardControlFont( glyphFont, ui::CFontInfo( _T("Arial Unicode MS"), true, false, 150 ) );		// 50% bigger
	return &glyphFont;
}

void CListEditDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_itemsEdit.m_hWnd;

	DDX_Control( pDX, IDC_LEDIT_ITEM_LIST_EDIT, m_itemsEdit );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		if ( firstInit )
		{
			CWnd* pMoveDownButton = GetDlgItem( ID_MOVE_DOWN_ITEM );
			CWnd* pMoveUpButton = GetDlgItem( ID_MOVE_UP_ITEM );

			pMoveDownButton->SetFont( GetGlyphFont() );
			pMoveUpButton->SetFont( GetGlyphFont() );

			pMoveDownButton->SetWindowText( L"\x25bc" );		// black down-pointing triangle
			pMoveUpButton->SetWindowText( L"\x25b2" );			// black up-pointing triangle

			ui::ShowControl( m_hWnd, IDC_LEDIT_BROWSE, ui::DirPath == m_content.m_type || ui::FilePath == m_content.m_type );
			ui::EnableControl( m_hWnd, IDOK, !m_readOnly );
		}

		std::tstring multiLineText = m_flatItems;
		str::Replace( multiLineText, m_pSeparator, lineEnd );

		m_itemsEdit.SetLimitText( 100 * _MAX_PATH );
		ui::SetWindowText( m_itemsEdit, multiLineText );
		m_itemsEdit.SetFocus();
		m_itemsEdit.SetSel( 0, 0 );
	}

	CLayoutDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CListEditDialog, CLayoutDialog )
	ON_COMMAND( ID_EDIT_SELECT_ALL, OnEditSelectAll )
	ON_COMMAND_RANGE( ID_MOVE_UP_ITEM, ID_MOVE_DOWN_ITEM, OnMoveUpDown )
	ON_COMMAND( IDC_LEDIT_BROWSE, OnBrowseButton )
END_MESSAGE_MAP()

void CListEditDialog::OnOK( void )
{
	std::vector< std::tstring > items;
	str::Split( items, ui::GetWindowText( m_itemsEdit ).c_str(), lineEnd );
	str::RemoveEmptyItems( items );
	m_flatItems = str::Join( items, m_pSeparator );

	CLayoutDialog::OnOK();
}

void CListEditDialog::OnEditSelectAll( void )
{
	m_itemsEdit.SetSel( 0, -1, TRUE );
}

void CListEditDialog::OnMoveUpDown( UINT cmdId )
{
	int currentLineIndex = m_itemsEdit.LineFromChar();

	std::vector< std::tstring > items;
	str::Split( items, ui::GetWindowText( m_itemsEdit ).c_str(), lineEnd );

	int newLineIndex = currentLineIndex;

	if ( ID_MOVE_DOWN_ITEM == cmdId )
		++newLineIndex;
	else
		--newLineIndex;

	if ( newLineIndex >= 0 && newLineIndex < (int)items.size() )
	{
		std::swap( items[ newLineIndex ], items[ currentLineIndex ] );

		ui::SetWindowText( m_itemsEdit, str::Join( items, lineEnd ) );
		m_itemsEdit.SetFocus();

		int start = m_itemsEdit.LineIndex( newLineIndex );
		TCHAR line[ _MAX_PATH ];
		int lineCharCount = static_cast< int >( m_itemsEdit.SendMessage( EM_GETLINE, newLineIndex, (LPARAM)line ) );

		line[ lineCharCount ] = _T('\0');

		if ( start != -1 )
			m_itemsEdit.SetSel( start, start + (int)str::GetLength( line ) );
	}
}

void CListEditDialog::OnBrowseButton( void )
{
	int currentLineIndex = m_itemsEdit.LineFromChar();

	TCHAR line[ _MAX_PATH ];
	int lineCharCount = m_itemsEdit.GetLine( currentLineIndex, line, COUNT_OF( line ) );
	line[ lineCharCount ] = _T('\0');

	int start = m_itemsEdit.LineIndex();
	if ( start != -1 )
		m_itemsEdit.SetSel( start, start + (int)str::GetLength( line ) );

	std::tstring newItem = m_content.EditItem( line, this );
	if ( !newItem.empty() )
	{
		m_itemsEdit.ReplaceSel( newItem.c_str(), TRUE );
		m_itemsEdit.SetFocus();

		if ( start != -1 )
			m_itemsEdit.SetSel( start, start + (int)newItem.length() );
	}
}
