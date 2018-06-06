
#include "stdafx.h"
#include "FileEditorBaseDialog.h"
#include "FileModel.h"
#include "resource.h"
#include "utl/EnumTags.h"
#include "utl/Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CFileEditorBaseDialog::CFileEditorBaseDialog( CFileModel* pFileModel, cmd::CommandType nativeCmdType, UINT templateId, CWnd* pParent )
	: CBaseMainDialog( templateId, pParent )
	, m_nativeCmdType( nativeCmdType )
	, m_pFileModel( pFileModel )
{
	m_pFileModel->AddObserver( this );
}

CFileEditorBaseDialog::~CFileEditorBaseDialog()
{
	m_pFileModel->RemoveObserver( this );
}

CFileModel* CFileEditorBaseDialog::GetFileModel( void ) const
{
	return m_pFileModel;
}

CDialog* CFileEditorBaseDialog::GetDialog( void )
{
	return this;
}

void CFileEditorBaseDialog::QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const
{
	switch ( cmdId )
	{
		case IDC_UNDO_BUTTON:
		case IDC_REDO_BUTTON:
		{
			cmd::UndoRedo undoRedo = IDC_UNDO_BUTTON == cmdId ? cmd::Undo : cmd::Redo;
			if ( utl::ICommand* pTopCmd = m_pFileModel->PeekCmdAs< utl::ICommand >( undoRedo ) )
				rText = str::Format( _T("%s: %s"), cmd::GetTags_UndoRedo().FormatUi( undoRedo ).c_str(), pTopCmd->Format( true ).c_str() );
			else
				rText = str::Format( _T("%s the file operation"), cmd::GetTags_UndoRedo().FormatUi( undoRedo ).c_str() );
			break;
		}
		default:
			__super::QueryTooltipText( rText, cmdId, pTooltip );
	}
}

int CFileEditorBaseDialog::PopStackRunCrossEditor( cmd::UndoRedo undoRedo )
{
	// end this dialog and spawn the foreign dialog editor
	cmd::CommandType foreignCmdType = static_cast< cmd::CommandType >( m_pFileModel->PeekCmdAs< utl::ICommand >( undoRedo )->GetTypeID() );
	ASSERT( foreignCmdType != m_nativeCmdType );
	ASSERT( m_pFileModel->CanUndoRedo( undoRedo, foreignCmdType ) );

	m_pFileModel->RemoveObserver( this );		// reject further updates

	CWnd* pParent = GetParent();
	OnCancel();									// end this dialog

	std::auto_ptr< IFileEditor > pFileEditor( m_pFileModel->MakeFileEditor( foreignCmdType, pParent ) );
	pFileEditor->PopUndoRedoTop( undoRedo );

	m_nModalResult = static_cast< int >( pFileEditor->GetDialog()->DoModal() );		// pass the modal result from the spawned editor dialog
	return m_nModalResult;
}

void CFileEditorBaseDialog::DoDataExchange( CDataExchange* pDX )
{
	ui::DDX_ButtonIcon( pDX, IDC_UNDO_BUTTON, ID_EDIT_UNDO, false );
	ui::DDX_ButtonIcon( pDX, IDC_REDO_BUTTON, ID_EDIT_REDO, false );

	__super::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CFileEditorBaseDialog, CBaseMainDialog )
	ON_CONTROL_RANGE( BN_CLICKED, IDC_UNDO_BUTTON, IDC_REDO_BUTTON, OnBnClicked_UndoRedo )
END_MESSAGE_MAP()

void CFileEditorBaseDialog::OnBnClicked_UndoRedo( UINT btnId )
{
	GotoDlgCtrl( GetDlgItem( IDOK ) );
	PopUndoRedoTop( IDC_UNDO_BUTTON == btnId ? cmd::Undo : cmd::Redo );
}
