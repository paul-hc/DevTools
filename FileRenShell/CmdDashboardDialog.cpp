
#include "stdafx.h"
#include "CmdDashboardDialog.h"
#include "FileModel.h"
#include "AppCommands.h"
#include "AppCmdService.h"
#include "Application.h"
#include "OptionsSheet.h"
#include "utl/EnumTags.h"
#include "utl/FmtUtils.h"
#include "utl/StringUtilities.h"
#include "utl/Subject.h"
#include "utl/TimeUtils.h"
#include "utl/UI/ImageStore.h"
#include "utl/UI/UtilitiesEx.h"
#include "utl/UI/ToolStrip.h"
#include "utl/UI/resource.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CCmdItem class

class CCmdItem : public CSubject		// proxy items to be inserted into the list control
{
public:
	CCmdItem( utl::ICommand* pCmd = NULL ) { SetCmd( pCmd ); }

	utl::ICommand* GetCmd( void ) const { return m_pCmd; }
	void SetCmd( utl::ICommand* pCmd );

	template< typename Cmd_T >
	Cmd_T* GetCmdAs( void ) const { return dynamic_cast< Cmd_T* >( m_pCmd ); }

	int GetImageIndex( void ) const { return m_imageIndex; }

	// ISubject interface
	virtual const std::tstring& GetCode( void ) const;

	static CToolStrip& GetCmdTypeStrip( void );
	static int LookupImageIndex( utl::ICommand* pCmd );

	struct ToCmd
	{
		utl::ICommand* operator()( const CCmdItem* pCmdItem ) const
		{
			return pCmdItem->GetCmd();
		}
	};
private:
	utl::ICommand* m_pCmd;
	int m_imageIndex;
	std::tstring m_code;
};


void CCmdItem::SetCmd( utl::ICommand* pCmd )
{
	m_pCmd = pCmd;
	m_imageIndex = LookupImageIndex( m_pCmd );
	m_code.clear();

	if ( m_pCmd != NULL )
	{
		std::vector< std::tstring > fields;
		cmd::QueryCmdFields( fields, m_pCmd );
		if ( !fields.empty() )
			m_code = fields.front();
	}
}

const std::tstring& CCmdItem::GetCode( void ) const
{
	return m_code;
}

CToolStrip& CCmdItem::GetCmdTypeStrip( void )
{
	static CToolStrip s_strip;
	if ( !s_strip.IsValid() )
	{
		s_strip.AddButton( UINT_MAX, ID_EDIT_DETAILS );			// unknown command image
		s_strip.AddButton( cmd::RenameFile, ID_RENAME_ITEM );
		s_strip.AddButton( cmd::TouchFile, ID_TOUCH_FILES );
		s_strip.AddButton( cmd::FindDuplicates, ID_FIND_DUPLICATE_FILES );
		s_strip.AddButton( cmd::DeleteFiles, ID_CMD_DELETE_FILES );
		s_strip.AddButton( cmd::MoveFiles, ID_CMD_MOVE_FILES );
		s_strip.AddButton( cmd::ChangeDestPaths, ID_CMD_CHANGE_DEST_PATHS );
		s_strip.AddButton( cmd::ChangeDestFileStates, ID_CMD_CHANGE_DEST_FILE_STATES );
		s_strip.AddButton( cmd::ResetDestinations, ID_CMD_RESET_DESTINATIONS );
		s_strip.AddButton( cmd::EditOptions, ID_OPTIONS );
	}
	return s_strip;
}

int CCmdItem::LookupImageIndex( utl::ICommand* pCmd )
{
	const CToolStrip& strip = GetCmdTypeStrip();
	size_t imagePos = strip.FindButtonPos( pCmd != NULL ? pCmd->GetTypeID() : UINT_MAX );

	if ( utl::npos == imagePos )
		imagePos = strip.FindButtonPos( UINT_MAX );

	ENSURE( imagePos < strip.m_buttonIds.size() );
	return static_cast< int >( imagePos );
}


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
		{ IDOK, Move },
		{ IDCANCEL, Move }
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
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );
	LoadDlgIcon( svc::Undo == m_stackType ? ID_EDIT_UNDO : ID_EDIT_REDO );

	m_commandsList.SetSection( m_regSection + _T("\\List") );
	m_commandsList.SetUseAlternateRowColoring();
	m_commandsList.SetTextEffectCallback( this );
	m_commandsList.StoreImageLists( CCmdItem::GetCmdTypeStrip().m_pImageList.get() );

	m_actionToolbar.GetStrip()
		.AddButton( IDC_UNDO_BUTTON, ID_EDIT_UNDO )
		.AddButton( IDC_REDO_BUTTON, ID_EDIT_REDO )
		.AddSeparator()
		.AddButton( ID_OPTIONS );

	m_cmdsToolbar.GetStrip()
		.AddButton( ID_EDIT_SELECT_ALL )
		.AddSeparator()
		.AddButton( ID_REMOVE_ITEM );
}

CCmdDashboardDialog::~CCmdDashboardDialog()
{
}

CCommandModel* CCmdDashboardDialog::GetCommandModel( void )
{
	return const_cast< CCommandModel* >( app::GetApp().GetCommandModel() );
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
		m_cmdItems.push_back( CCmdItem() );
		m_cmdItems.back().SetCmd( *itCmdStack );
	}
}

void CCmdDashboardDialog::SetupCommandList( void )
{
	//CScopedListTextSelection sel( &m_commandsList );

	CScopedLockRedraw freeze( &m_commandsList );
	CScopedInternalChange internalChange( &m_commandsList );

	m_commandsList.DeleteAllItems();
	BuildCmdItems();

	for ( UINT index = 0; index != m_cmdItems.size(); ++index )
	{
		CCmdItem* pCmdItem = &m_cmdItems[ index ];

		m_commandsList.InsertObjectItem( index, pCmdItem, pCmdItem->GetImageIndex() );		// Source

		if ( const cmd::IFileDetailsCmd* pDetailsCmd = pCmdItem->GetCmdAs< cmd::IFileDetailsCmd >() )
			m_commandsList.SetSubItemText( index, FileCount, num::FormatNumber( pDetailsCmd->GetFileCount() ) );

		if ( const cmd::IPersistentCmd* pPersistCmd = pCmdItem->GetCmdAs< cmd::IPersistentCmd >() )
			m_commandsList.SetSubItemText( index, Timestamp, time_utl::FormatTimestamp( pPersistCmd->GetTimestamp() ) );
	}

	m_actionHistoryStatic.SetWindowText( str::Format( _T("%s History (%d actions):"), svc::GetTags_StackType().FormatUi( m_stackType ).c_str(), m_cmdItems.size() ) );
}

utl::ICommand* CCmdDashboardDialog::GetSelectedCmd( void ) const
{
	int selIndex = m_commandsList.GetSelCaretIndex();
	if ( selIndex != -1 )
		return m_commandsList.GetObjectAt< CCmdItem >( selIndex )->GetCmd();
	return NULL;
}

void CCmdDashboardDialog::QuerySelectedCmds( std::vector< utl::ICommand* >& rSelCommands ) const
{
	std::vector< CCmdItem* > selItems;
	m_commandsList.QuerySelectionAs( selItems );

	utl::Assign( rSelCommands, selItems, CCmdItem::ToCmd() );
}

bool CCmdDashboardDialog::SelectCommandList( int selIndex )
{
	selIndex = std::min( selIndex, static_cast< int >( m_cmdItems.size() - 1 ) );

	if ( -1 == selIndex )
		return false;

	m_commandsList.SetCurSel( selIndex );
	UpdateSelCommand();
	return true;
}

void CCmdDashboardDialog::UpdateSelCommand( void )
{
	utl::ICommand* pSelectedCmd = GetSelectedCmd();
	std::tstring headerText, detailsText;

	if ( pSelectedCmd != NULL )
	{
		std::vector< std::tstring > fields;
		cmd::QueryCmdFields( fields, pSelectedCmd );
		if ( fields.size() > 1 )
			fields.back() = fmt::FormatBraces( fields.back().c_str(), _T("()") );		// enclose timestamp in parens

		headerText = str::Join( fields, _T(" ") );

		if ( cmd::IFileDetailsCmd* pDetailsCmd = dynamic_cast< cmd::IFileDetailsCmd* >( pSelectedCmd ) )
		{
			std::vector< std::tstring > lines;
			pDetailsCmd->QueryDetailLines( lines );
			detailsText = str::JoinLines( lines, _T("\r\n") );
		}
	}

	m_cmdHeaderEdit.SetText( headerText );
	m_cmdDetailsEdit.SetText( detailsText );

	static const UINT ctrlIds[] = { IDC_CMD_HEADER_STATIC, IDC_CMD_HEADER_EDIT, IDC_CMD_DETAILS_STATIC, IDC_CMD_DETAILS_EDIT, IDOK };
	ui::EnableControls( *this, ARRAY_PAIR( ctrlIds ), pSelectedCmd != NULL );
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
	const cmd::IPersistentCmd* pPersistCmd = CReportListControl::AsPtr< CCmdItem >( rowKey )->GetCmdAs< cmd::IPersistentCmd >();

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
	ON_COMMAND_EX( ID_EDIT_SELECT_ALL, OnSelCmds_SelectAll )
	ON_COMMAND( ID_REMOVE_ITEM, OnSelCmds_Delete )
	ON_UPDATE_COMMAND_UI( ID_REMOVE_ITEM, OnUpdateSelCmds_Delete )
	ON_COMMAND( ID_OPTIONS, OnOptions )
	ON_UPDATE_COMMAND_UI( ID_OPTIONS, OnUpdateOptions )
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
	bool succeeded = false;

	int selIndex = m_commandsList.GetSelCaretIndex();
	if ( 0 == selIndex )
		succeeded = m_pCmdSvc->UndoRedo( m_stackType );
	else
	{
		std::tstring message = str::Format( _T("Are you sure you want to %s the command not at the stack top?\n\nThis could have unforseen side effects."),
			svc::GetTags_StackType().FormatUi( m_stackType ).c_str() );

		if ( IDYES == AfxMessageBox( message.c_str(), MB_YESNO | MB_DEFBUTTON2 ) )
		{
			CAppCmdService* m_pAppCmdSvc = checked_static_cast< CAppCmdService* >( m_pCmdSvc );
			succeeded = m_pAppCmdSvc->UndoRedoAt( m_stackType, selIndex );
		}
	}
	UpdateData( DialogOutput );		// refresh the remaining commands on the stack

	if ( succeeded && !keepRunning )
		__super::OnOK();
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

void CCmdDashboardDialog::OnLvnItemChanged_CommandsList( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLISTVIEW* pNmList = (NMLISTVIEW*)pNmHdr;
	*pResult = 0;

	if ( CReportListControl::IsSelectionChangeNotify( pNmList, LVIS_SELECTED | LVIS_FOCUSED ) )
		UpdateSelCommand();
}

BOOL CCmdDashboardDialog::OnSelCmds_SelectAll( UINT cmdId )
{
	cmdId;
	GotoDlgCtrl( &m_commandsList );
	return FALSE;		// continue routing (to m_commandsList)
}

void CCmdDashboardDialog::OnSelCmds_Delete( void )
{
	int minSelIndex;

	if ( m_commandsList.GetSelIndexBounds( &minSelIndex, NULL ) )
	{
		std::vector< utl::ICommand* > selCommands;
		QuerySelectedCmds( selCommands );

		GotoDlgCtrl( &m_commandsList );
		if ( IDYES == AfxMessageBox( str::Format( _T("Are you sure you want to delete %d commands?"), selCommands.size() ).c_str(), MB_YESNO ) )
		{
			GetCommandModel()->RemoveCommandsThat( pred::ContainsAny< std::vector< utl::ICommand* > >( selCommands ) );

			SetupCommandList();
			SelectCommandList( minSelIndex );
		}
	}
}

void CCmdDashboardDialog::OnUpdateSelCmds_Delete( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_commandsList.AnySelected() );
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
