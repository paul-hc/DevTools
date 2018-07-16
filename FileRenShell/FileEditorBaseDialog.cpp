
#include "stdafx.h"
#include "FileEditorBaseDialog.h"
#include "FileModel.h"
#include "GeneralOptions.h"
#include "OptionsSheet.h"
#include "resource.h"
#include "utl/ContainerUtilities.h"
#include "utl/EnumTags.h"
#include "utl/Utilities.h"
#include "utl/resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CFileEditorBaseDialog::CFileEditorBaseDialog( CFileModel* pFileModel, cmd::CommandType nativeCmdType, UINT templateId, CWnd* pParent )
	: CBaseMainDialog( templateId, pParent )
	, m_pFileModel( pFileModel )
	, m_mode( EditMode )
{
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
			cmd::StackType stackType = IDC_UNDO_BUTTON == cmdId ? cmd::Undo : cmd::Redo;
			if ( utl::ICommand* pTopCmd = m_pFileModel->PeekCmdAs< utl::ICommand >( stackType ) )
			{
				std::tstring cmdInfo = pTopCmd->Format( utl::DetailedLine );
				rText = str::Format( _T("%s: %s"), cmd::GetTags_StackType().FormatUi( stackType ).c_str(), cmdInfo.c_str() );
			}
			else
				rText = str::Format( _T("%s the file operation"), cmd::GetTags_StackType().FormatUi( stackType ).c_str() );
			break;
		}
		default:
			__super::QueryTooltipText( rText, cmdId, pTooltip );
	}
}

int CFileEditorBaseDialog::PopStackRunCrossEditor( cmd::StackType stackType )
{
	// end this dialog and spawn the foreign dialog editor
	cmd::CommandType foreignCmdType = static_cast< cmd::CommandType >( m_pFileModel->PeekCmdAs< utl::ICommand >( stackType )->GetTypeID() );
	ASSERT( foreignCmdType != m_nativeCmdTypes.front() );
	ASSERT( m_pFileModel->CanUndoRedo( stackType, foreignCmdType ) );

	m_pFileModel->RemoveObserver( this );		// reject further updates

	CWnd* pParent = GetParent();
	OnCancel();									// end this dialog

	std::auto_ptr< IFileEditor > pFileEditor( m_pFileModel->MakeFileEditor( foreignCmdType, pParent ) );
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

utl::ICommand* CFileEditorBaseDialog::PeekCmdForDialog( cmd::StackType stackType ) const
{
	if ( utl::ICommand* pTopCmd = m_pFileModel->PeekCmdAs< utl::ICommand >( stackType ) )
		if ( !IsForeignCmd( pTopCmd ) )
			if ( m_pFileModel->CanUndoRedo( stackType ) )
				return pTopCmd;

	return NULL;
}

int CFileEditorBaseDialog::EnsureVisibleFirstError( CReportListControl* pFileListCtrl ) const
{
	ASSERT_PTR( pFileListCtrl->GetSafeHwnd() );

	int firstErrorIndex = -1;

	if ( !m_errorItems.empty() )
		firstErrorIndex = pFileListCtrl->FindItemIndex( (LPARAM)m_errorItems.front() );

	if ( firstErrorIndex != -1 )
		pFileListCtrl->EnsureVisible( firstErrorIndex, FALSE );		// visible first error

	pFileListCtrl->Invalidate();				// trigger some highlighting
	return firstErrorIndex;
}

bool CFileEditorBaseDialog::PromptCloseDialog( void )
{
	return IDOK == AfxMessageBox( _T("There are no file changes to apply.\n\nClose the dialog?"), MB_OKCANCEL );
}

void CFileEditorBaseDialog::DoDataExchange( CDataExchange* pDX )
{
	m_toolbar.DDX_Placeholder( pDX, IDC_TOOLBAR_PLACEHOLDER, H_AlignRight | V_AlignCenter );

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
	PopStackTop( IDC_UNDO_BUTTON == btnId ? cmd::Undo : cmd::Redo );
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
