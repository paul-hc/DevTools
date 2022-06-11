
#include "stdafx.h"
#include "CmdDashboardDialog.h"
#include "FileModel.h"
#include "CommandItem.h"
#include "AppCommands.h"
#include "AppCmdService.h"
#include "Application.h"
#include "OptionsSheet.h"
#include "utl/EnumTags.h"
#include "utl/FmtUtils.h"
#include "utl/StringUtilities.h"
#include "utl/TimeUtils.h"
#include "utl/UI/ImageStore.h"
#include "utl/UI/WndUtilsEx.h"
#include "utl/UI/resource.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/ReportListControl.hxx"


// CCmdDashboardDialog implementation

namespace reg
{
	const TCHAR section[] = _T("CmdDashboardDialog");
}


namespace layout
{
	enum { TopPct = 30, BottomPct = 100 - TopPct };

	static CLayoutStyle styles[] =
	{
		{ IDC_STRIP_BAR_2, MoveX },
		{ IDC_COMMANDS_LIST, SizeX | pctSizeY( TopPct ) },
		{ IDC_CMD_HEADER_STATIC, pctMoveY( TopPct ) },
		{ IDC_CMD_HEADER_EDIT, SizeX | pctMoveY( TopPct ) },
		{ IDC_CMD_DETAILS_STATIC, pctMoveY( TopPct ) },
		{ IDC_CMD_DETAILS_EDIT, pctMoveY( TopPct ) | SizeX | pctSizeY( BottomPct ) },
		{ IDOK, MoveX },
		{ IDCANCEL, MoveX }
	};
}

CCmdDashboardDialog::CCmdDashboardDialog( CFileModel* pFileModel, svc::StackType stackType, CWnd* pParent )
	: CLayoutDialog( IDD_CMD_DASHBOARD_DIALOG, pParent )
	, m_pFileModel( pFileModel )
	, m_pCmdSvc( app::GetCmdSvc() )
	, m_stackType( stackType )
	, m_enableProperties( false )
	, m_actionHistoryStatic( CRegularStatic::Bold )
	, m_commandsList( IDC_COMMANDS_LIST )
{
	m_initCentered = false;
	m_regSection = reg::section;
	RegisterCtrlLayout( ARRAY_PAIR( layout::styles ) );
	LoadDlgIcon( svc::Undo == m_stackType ? ID_EDIT_UNDO : ID_EDIT_REDO );

	m_commandsList.SetSection( m_regSection + _T("\\List") );
	m_commandsList.SetUseAlternateRowColoring();
	m_commandsList.SetTextEffectCallback( this );
	m_commandsList.StoreImageLists( CCommandItem::GetImageList() );

	m_actionToolbar.GetStrip()
		.AddButton( IDC_UNDO_BUTTON, ID_EDIT_UNDO )
		.AddButton( IDC_REDO_BUTTON, ID_EDIT_REDO )
		.AddSeparator()
		.AddButton( ID_OPTIONS );

	m_cmdsToolbar.GetStrip()
		.AddButton( ID_SELECT_TO_TOP )
		.AddButton( ID_EDIT_SELECT_ALL )
		.AddSeparator()
		.AddButton( ID_REMOVE_ITEM );

	m_cmdHeaderEdit.SetImageList( CCommandItem::GetImageList() );
}

CCmdDashboardDialog::~CCmdDashboardDialog()
{
}

CCommandModel* CCmdDashboardDialog::GetCommandModel( void )
{
	return const_cast<CCommandModel*>( app::GetApp().GetCommandModel() );
}

const std::deque< utl::ICommand* >& CCmdDashboardDialog::GetStack( void ) const
{
	return svc::Undo == m_stackType ? GetCommandModel()->GetUndoStack() : GetCommandModel()->GetRedoStack();
}

void CCmdDashboardDialog::BuildCmdItems( void )
{
	const std::deque< utl::ICommand* >& cmdStack = GetStack();

	m_cmdItems.clear();
	m_cmdItems.reserve( cmdStack.size() );

	// go in reverse order since stack top is at the back
	for ( std::deque< utl::ICommand* >::const_reverse_iterator itCmdStack = cmdStack.rbegin(); itCmdStack != cmdStack.rend(); ++itCmdStack )
	{
		m_cmdItems.push_back( CCommandItem() );
		m_cmdItems.back().SetCmd( *itCmdStack );
	}
}

void CCmdDashboardDialog::SetupCommandList( void )
{
	//lv::TScopedStatus_ByText status( &m_commandsList );

	CScopedLockRedraw freeze( &m_commandsList );
	CScopedInternalChange internalChange( &m_commandsList );

	m_commandsList.DeleteAllItems();
	BuildCmdItems();

	for ( UINT index = 0; index != m_cmdItems.size(); ++index )
	{
		CCommandItem* pCmdItem = &m_cmdItems[ index ];

		m_commandsList.InsertObjectItem( index, pCmdItem, pCmdItem->GetImageIndex() );		// Source

		if ( const cmd::IFileDetailsCmd* pDetailsCmd = pCmdItem->GetCmdAs< cmd::IFileDetailsCmd >() )
			m_commandsList.SetSubItemText( index, FileCount, num::FormatNumber( pDetailsCmd->GetFileCount() ) );

		if ( const cmd::IPersistentCmd* pPersistCmd = pCmdItem->GetCmdAs< cmd::IPersistentCmd >() )
			m_commandsList.SetSubItemText( index, Timestamp, time_utl::FormatTimestamp( pPersistCmd->GetTimestamp() ) );
	}

	m_actionHistoryStatic.SetWindowText( str::Format( _T("%s History (%d actions):"), svc::GetTags_StackType().FormatUi( m_stackType ).c_str(), m_cmdItems.size() ) );
}

const CCommandItem* CCmdDashboardDialog::GetSelCaretCmdItem( void ) const
{
	int selIndex = m_commandsList.GetSelCaretIndex();
	if ( selIndex != -1 )
		return m_commandsList.GetObjectAt< CCommandItem >( selIndex );
	return NULL;
}

void CCmdDashboardDialog::QuerySelectedCmds( std::vector< utl::ICommand* >& rSelCommands ) const
{
	std::vector< CCommandItem* > selItems;
	m_commandsList.QuerySelectionAs( selItems );

	utl::Assign( rSelCommands, selItems, CCommandItem::ToCmd() );
}

bool CCmdDashboardDialog::IsSelContiguousToTop( const std::vector< int >& selIndexes )
{
	ASSERT( !selIndexes.empty() );
	if ( selIndexes.front() != 0 )
		return false;

	for ( size_t i = 0; i != selIndexes.size(); ++i )
		if ( selIndexes[ i ] != static_cast<int>( i ) )
			return false;

	return true;
}

bool CCmdDashboardDialog::SelectCommandList( int selIndex )
{
	selIndex = std::min( selIndex, static_cast<int>( m_cmdItems.size() - 1 ) );

	m_commandsList.SetCurSel( selIndex );
	UpdateSelCommand();
	return selIndex != -1;
}

void CCmdDashboardDialog::UpdateSelCommand( void )
{
	const CCommandItem* pSelCmdItem = GetSelCaretCmdItem();
	const utl::ICommand* pSelectedCmd = pSelCmdItem != NULL ? pSelCmdItem->GetCmd() : NULL;
	std::tstring headerText, detailsText;

	// action info acts on selected caret
	if ( pSelectedCmd != NULL )
	{
		std::vector< std::tstring > fields;
		cmd::QueryCmdFields( fields, pSelectedCmd );
		if ( fields.size() > 1 )
			fields.back() = fmt::FormatBraces( fields.back().c_str(), _T("()") );		// enclose timestamp in parens

		headerText = str::Join( fields, _T(" ") );

		if ( const cmd::IFileDetailsCmd* pDetailsCmd = dynamic_cast<const cmd::IFileDetailsCmd*>( pSelectedCmd ) )
		{
			std::vector< std::tstring > lines;
			pDetailsCmd->QueryDetailLines( lines );
			detailsText = str::JoinLines( lines, _T("\r\n") );
		}
	}

	m_cmdHeaderEdit.SetText( headerText );
	m_cmdHeaderEdit.SetImageIndex( pSelCmdItem != NULL ? pSelCmdItem->GetImageIndex() : -1 );
	m_cmdDetailsEdit.SetText( detailsText );

	static const UINT ctrlIds[] = { IDC_CMD_HEADER_STATIC, IDC_CMD_HEADER_EDIT, IDC_CMD_DETAILS_STATIC, IDC_CMD_DETAILS_EDIT };
	ui::EnableControls( *this, ARRAY_PAIR( ctrlIds ), pSelectedCmd != NULL );

	std::vector< int > selIndexes;
	m_commandsList.GetSelection( selIndexes );
	ui::EnableControl( *this, IDOK, !selIndexes.empty() );		// undo/redo acts strictly on selection
}

void CCmdDashboardDialog::QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const
{
	switch ( cmdId )
	{
		case IDC_UNDO_BUTTON:
		case IDC_REDO_BUTTON:
			rText = str::Format( _T("View %s action history"), svc::GetTags_StackType().FormatUi( m_stackType ).c_str() );
			return;
	}
	__super::QueryTooltipText( rText, cmdId, pTooltip );
}

void CCmdDashboardDialog::CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const
{
	subItem, pCtrl;

	static const ui::CTextEffect s_editorCmd( ui::Regular, color::Grey40, CLR_NONE );
	const cmd::IPersistentCmd* pPersistCmd = CReportListControl::AsPtr<CCommandItem>( rowKey )->GetCmdAs< cmd::IPersistentCmd >();

	if ( NULL == pPersistCmd )
		rTextEffect.Combine( s_editorCmd );
}

BOOL CCmdDashboardDialog::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	return
		__super::OnCmdMsg( id, code, pExtra, pHandlerInfo ) ||
		m_commandsList.OnCmdMsg( id, code, pExtra, pHandlerInfo );		// allow handling list std commands (Copy, Select All)
}

void CCmdDashboardDialog::DoDataExchange( CDataExchange* pDX )
{
	const bool firstInit = NULL == m_actionHistoryStatic.m_hWnd;

	DDX_Control( pDX, IDC_ACTION_HISTORY_STATIC, m_actionHistoryStatic );
	DDX_Control( pDX, IDC_COMMANDS_LIST, m_commandsList );
	DDX_Control( pDX, IDC_CMD_HEADER_EDIT, m_cmdHeaderEdit );
	DDX_Control( pDX, IDC_CMD_DETAILS_EDIT, m_cmdDetailsEdit );
	ui::DDX_ButtonIcon( pDX, IDOK, svc::Undo == m_stackType ? ID_EDIT_UNDO : ID_EDIT_REDO );
	m_actionToolbar.DDX_Placeholder( pDX, IDC_STRIP_BAR_1, H_AlignLeft | V_AlignBottom );
	m_cmdsToolbar.DDX_Placeholder( pDX, IDC_STRIP_BAR_2, H_AlignRight | V_AlignBottom );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		if ( firstInit )
			m_enableProperties = NULL == ui::FindAncestorAs< COptionsSheet >( this );		// prevent recursion

		SetupCommandList();
		SelectCommandList( 0 );

		ui::SetDlgItemText( this, IDOK, svc::GetTags_StackType().FormatUi( m_stackType ) );
	}

	CLayoutDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CCmdDashboardDialog, CLayoutDialog )
	ON_NOTIFY( LVN_ITEMCHANGED, IDC_COMMANDS_LIST, OnLvnItemChanged_CommandsList )
	ON_COMMAND_RANGE( IDC_UNDO_BUTTON, IDC_REDO_BUTTON, OnStackType )
	ON_UPDATE_COMMAND_UI_RANGE( IDC_UNDO_BUTTON, IDC_REDO_BUTTON, OnUpdateStackType )
	ON_COMMAND( ID_OPTIONS, OnOptions )
	ON_UPDATE_COMMAND_UI( ID_OPTIONS, OnUpdateOptions )
	ON_COMMAND_EX( ID_EDIT_SELECT_ALL, OnCmdList_SelectAll )
	ON_COMMAND( ID_SELECT_TO_TOP, OnCmdList_SelectToTop )
	ON_UPDATE_COMMAND_UI( ID_SELECT_TO_TOP, OnUpdateCmdList_SelectToTop )
	ON_COMMAND( ID_REMOVE_ITEM, OnCmdList_Delete )
	ON_UPDATE_COMMAND_UI( ID_REMOVE_ITEM, OnUpdateCmdList_Delete )
END_MESSAGE_MAP()

BOOL CCmdDashboardDialog::OnInitDialog( void )
{
	__super::OnInitDialog();
	GotoDlgCtrl( &m_commandsList );
	return FALSE;		// skip default dialog focus
}

void CCmdDashboardDialog::OnOK( void )
{
	bool keepRunning = !ui::IsKeyPressed( VK_SHIFT );

	std::vector< int > selIndexes;
	m_commandsList.GetSelection( selIndexes );		// indexes sorted ascending
	if ( selIndexes.empty() )
		return;

	bool succeeded = false;

	if ( 1 == selIndexes.size() && 0 == selIndexes.front() )	// single command at top?
	{
		succeeded = m_pCmdSvc->UndoRedo( m_stackType );
		UpdateData( DialogOutput );		// refresh the remaining commands on the stack
	}
	else
	{
		if ( !IsSelContiguousToTop( selIndexes ) )
			if ( ui::MessageBox( str::Format( _T("Are you sure you want to %s the action not in sequence from the top?\n\nThis could have unforseen side effects."),
								 svc::GetTags_StackType().FormatKey( m_stackType ).c_str() ), MB_YESNO | MB_DEFBUTTON2 ) != IDYES )
				return;

		CAppCmdService* m_pAppCmdSvc = checked_static_cast<CAppCmdService*>( m_pCmdSvc );
		bool done = false;

		std::vector< CCommandItem* > selCmdItems;
		m_commandsList.QueryObjectsByIndex( selCmdItems, selIndexes );

		for ( size_t pos = 0; !done && pos != selCmdItems.size(); ++pos )
		{
			utl::ICommand* pCmd = selCmdItems[ pos ]->GetCmd();
			size_t stackPos = m_pCmdSvc->FindCmdTopPos( m_stackType, pCmd );
			if ( stackPos != utl::npos )
				if ( m_pAppCmdSvc->UndoRedoAt( m_stackType, stackPos ) )
					succeeded |= true;
				else
					done = true;

			int itemIndex = m_commandsList.FindItemIndex( selCmdItems[ pos ] );
			if ( itemIndex != -1 )
				m_commandsList.DeleteItem( itemIndex );
		}

		ui::SetDlgItemText( this, IDOK, svc::GetTags_StackType().FormatUi( m_stackType ) );
	}

	if ( succeeded && !keepRunning )
		__super::OnOK();
}

void CCmdDashboardDialog::OnLvnItemChanged_CommandsList( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLISTVIEW* pNmList = (NMLISTVIEW*)pNmHdr;
	*pResult = 0;

	if ( CReportListControl::IsSelectionChangeNotify( pNmList, LVIS_SELECTED | LVIS_FOCUSED ) )
		UpdateSelCommand();
}

void CCmdDashboardDialog::OnStackType( UINT cmdId )
{
	m_stackType = IDC_UNDO_BUTTON == cmdId ? svc::Undo : svc::Redo;
	LoadDlgIcon( svc::Undo == m_stackType ? ID_EDIT_UNDO : ID_EDIT_REDO );
	SetupDlgIcons();

	UpdateData( DialogOutput );		// refresh the remaining commands on the stack
}

void CCmdDashboardDialog::OnUpdateStackType( CCmdUI* pCmdUI )
{
	pCmdUI->SetRadio( ( svc::Undo == m_stackType ) == ( IDC_UNDO_BUTTON == pCmdUI->m_nID ) );
}

void CCmdDashboardDialog::OnOptions( void )
{
	COptionsSheet sheet( m_pFileModel, this );
	sheet.DoModal();
}

void CCmdDashboardDialog::OnUpdateOptions( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_enableProperties );
}

BOOL CCmdDashboardDialog::OnCmdList_SelectAll( UINT cmdId )
{
	cmdId;
	GotoDlgCtrl( &m_commandsList );
	return FALSE;		// continue routing (to m_commandsList)
}

void CCmdDashboardDialog::OnCmdList_SelectToTop( void )
{
	GotoDlgCtrl( &m_commandsList );

	int maxSelIndex;
	if ( m_commandsList.GetSelIndexBounds( NULL, &maxSelIndex ) )
	{
		std::vector< int > selIndexes( maxSelIndex + 1 );
		for ( UINT i = 0; i != selIndexes.size(); ++i )
			selIndexes[ i ] = i;

		m_commandsList.SetSelection( selIndexes );
	}
}

void CCmdDashboardDialog::OnUpdateCmdList_SelectToTop( CCmdUI* pCmdUI )
{
	bool enable = false;
	std::vector< int > selIndexes;
	if ( m_commandsList.GetSelection( selIndexes ) )
		enable = !IsSelContiguousToTop( selIndexes );

	pCmdUI->Enable( enable );
}

void CCmdDashboardDialog::OnCmdList_Delete( void )
{
	int minSelIndex;

	if ( m_commandsList.GetSelIndexBounds( &minSelIndex, NULL ) )
	{
		std::vector< utl::ICommand* > selCommands;
		QuerySelectedCmds( selCommands );

		GotoDlgCtrl( &m_commandsList );
		if ( IDYES == ui::MessageBox( str::Format( _T("Are you sure you want to delete %d actions?"), selCommands.size() ), MB_YESNO ) )
		{
			GetCommandModel()->RemoveCommandsThat( pred::ContainsAny< std::vector< utl::ICommand* > >( selCommands ) );

			SetupCommandList();
			SelectCommandList( minSelIndex );
		}
	}
}

void CCmdDashboardDialog::OnUpdateCmdList_Delete( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_commandsList.AnySelected() );
}
