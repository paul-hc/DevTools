
#include "stdafx.h"
#include "FileEditorBaseDialog.h"
#include "FileModel.h"
#include "GeneralOptions.h"
#include "OptionsSheet.h"
#include "Application_fwd.h"
#include "resource.h"
#include "utl/ContainerUtilities.h"
#include "utl/EnumTags.h"
#include "utl/UI/ReportListControl.h"
#include "utl/UI/Utilities.h"
#include "utl/UI/resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CFileEditorBaseDialog::CFileEditorBaseDialog( CFileModel* pFileModel, cmd::CommandType nativeCmdType, UINT templateId, CWnd* pParent )
	: CBaseMainDialog( templateId, pParent )
	, m_pFileModel( pFileModel )
	, m_pCmdSvc( app::GetCmdSvc() )
	, m_mode( EditMode )
{
	ASSERT_PTR( m_pCmdSvc );
	m_nativeCmdTypes.push_back( nativeCmdType );

	m_pFileModel->AddObserver( this );
	CGeneralOptions::Instance().AddObserver( this );

	m_toolbar.GetStrip()
		.AddButton( IDC_UNDO_BUTTON, ID_EDIT_UNDO )
		.AddButton( IDC_REDO_BUTTON, ID_EDIT_REDO )
		.AddSeparator()
		.AddButton( ID_OPTIONS );
}

CFileEditorBaseDialog::~CFileEditorBaseDialog()
{
	m_pFileModel->RemoveObserver( this );
	CGeneralOptions::Instance().RemoveObserver( this );
}

CFileModel* CFileEditorBaseDialog::GetFileModel( void ) const
{
	return m_pFileModel;
}

CDialog* CFileEditorBaseDialog::GetDialog( void )
{
	return this;
}

bool CFileEditorBaseDialog::IsRollMode( void ) const
{
	return RollBackMode == m_mode || RollForwardMode == m_mode;
}

void CFileEditorBaseDialog::QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const
{
	switch ( cmdId )
	{
		case IDC_UNDO_BUTTON:
		case IDC_REDO_BUTTON:
		{
			svc::StackType stackType = IDC_UNDO_BUTTON == cmdId ? svc::Undo : svc::Redo;
			if ( utl::ICommand* pTopCmd = m_pCmdSvc->PeekCmd( stackType ) )
			{
				std::tstring cmdInfo = pTopCmd->Format( utl::DetailedLine );
				rText = str::Format( _T("%s: %s"), svc::GetTags_StackType().FormatUi( stackType ).c_str(), cmdInfo.c_str() );
			}
			else
				rText = str::Format( _T("%s the file operation"), svc::GetTags_StackType().FormatUi( stackType ).c_str() );
			break;
		}
		default:
			__super::QueryTooltipText( rText, cmdId, pTooltip );
	}
}

bool CFileEditorBaseDialog::SafeExecuteCmd( utl::ICommand* pCmd )
{
	return m_pFileModel->SafeExecuteCmd( this, pCmd );
}

void CFileEditorBaseDialog::UpdateOkButton( const std::tstring& caption, UINT iconId /*= 0*/ )
{
	if ( NULL == m_okButton.m_hWnd )
		return;				// avoid button update if not yet sub-classed (CFileEditorBaseDialog::DoDataExchange not yet called)

	if ( 0 == iconId && IsRollMode() )
		iconId = ID_COMMIT_MODE;

	m_okButton.SetButtonCaption( caption );
	m_okButton.SetIconId( iconId );
}

int CFileEditorBaseDialog::PopStackRunCrossEditor( svc::StackType stackType )
{
	// end this dialog and spawn the foreign dialog editor
	cmd::CommandType foreignCmdType = static_cast< cmd::CommandType >( m_pCmdSvc->PeekCmd( stackType )->GetTypeID() );
	ASSERT( foreignCmdType != m_nativeCmdTypes.front() );
	ASSERT( m_pCmdSvc->CanUndoRedo( stackType, foreignCmdType ) );

	CWnd* pParent = GetParent();

	std::pair< IFileEditor*, bool > editorPair = m_pFileModel->HandleUndoRedo( stackType, pParent );
	if ( NULL == editorPair.first )				// we've got no editor to undo/redo?
	{
		if ( editorPair.second )				// command handled?
			SwitchMode( EditMode );				// signal the dirty state of this editor
		return 0;
	}

	// we've got a foreign editor to undo/redo
	std::auto_ptr< IFileEditor > pFileEditor( editorPair.first );

	m_pFileModel->RemoveObserver( this );		// reject further updates
	OnCancel();									// end this dialog

	pFileEditor->PopStackTop( stackType );

	m_nModalResult = static_cast< int >( pFileEditor->GetDialog()->DoModal() );		// pass the modal result from the spawned editor dialog
	return m_nModalResult;
}

bool CFileEditorBaseDialog::IsNativeCmd( const utl::ICommand* pCmd ) const
{
	ASSERT_PTR( pCmd );
	ASSERT( !m_nativeCmdTypes.empty() );

	return utl::Contains( m_nativeCmdTypes, pCmd->GetTypeID() );
}

bool CFileEditorBaseDialog::IsForeignCmd( const utl::ICommand* pCmd ) const
{
	ASSERT_PTR( pCmd );
	if ( cmd::IsPersistentCmd( pCmd ) )		// editor-specific?
		return pCmd->GetTypeID() != m_nativeCmdTypes.front();

	return false;
}

utl::ICommand* CFileEditorBaseDialog::PeekCmdForDialog( svc::StackType stackType ) const
{
	if ( utl::ICommand* pTopCmd = m_pCmdSvc->PeekCmd( stackType ) )
		if ( !IsForeignCmd( pTopCmd ) )
			if ( m_pCmdSvc->CanUndoRedo( stackType ) )
				return pTopCmd;

	return NULL;
}

bool CFileEditorBaseDialog::PromptCloseDialog( Prompt prompt /*= PromptNoFileChanges*/ )
{
	static const std::tstring s_noChangesPrefix = _T("There are no file changes to apply.\n\n");
	std::tstring message = _T("Close the dialog?");

	if ( PromptNoFileChanges == prompt )
		message = s_noChangesPrefix + message;

	return IDOK == AfxMessageBox( message.c_str(), MB_OKCANCEL );
}

bool CFileEditorBaseDialog::IsErrorItem( const CPathItemBase* pItem ) const
{
	return utl::Contains( m_errorItems, pItem );
}

void CFileEditorBaseDialog::DoDataExchange( CDataExchange* pDX )
{
	const bool firstInit = NULL == m_okButton.m_hWnd;

	DDX_Control( pDX, IDOK, m_okButton );
	m_toolbar.DDX_Placeholder( pDX, IDC_TOOLBAR_PLACEHOLDER, H_AlignRight | V_AlignCenter );

	if ( firstInit )
		SwitchMode( m_mode );

	__super::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CFileEditorBaseDialog, CBaseMainDialog )
	ON_CONTROL_RANGE( BN_CLICKED, IDC_UNDO_BUTTON, IDC_REDO_BUTTON, OnUndoRedo )
	ON_COMMAND( ID_OPTIONS, OnOptions )
	ON_UPDATE_COMMAND_UI( ID_OPTIONS, OnUpdateOptions )
END_MESSAGE_MAP()

BOOL CFileEditorBaseDialog::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	return
		m_pFileModel->OnCmdMsg( id, code, pExtra, pHandlerInfo ) ||
		__super::OnCmdMsg( id, code, pExtra, pHandlerInfo );
}

void CFileEditorBaseDialog::OnUndoRedo( UINT btnId )
{
	GotoDlgCtrl( GetDlgItem( IDOK ) );
	PopStackTop( IDC_UNDO_BUTTON == btnId ? svc::Undo : svc::Redo );
}

void CFileEditorBaseDialog::OnOptions( void )
{
	COptionsSheet sheet( m_pFileModel, this );
	sheet.DoModal();
}

void CFileEditorBaseDialog::OnUpdateOptions( CCmdUI* pCmdUI )
{
	pCmdUI->Enable();
}
