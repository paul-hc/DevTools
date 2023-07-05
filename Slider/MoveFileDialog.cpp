
#include "stdafx.h"
#include "MoveFileDialog.h"
#include "resource.h"
#include "utl/FileSystem.h"
#include "utl/UI/ItemContentHistoryCombo.h"
#include "utl/UI/ShellUtilities.h"
#include "utl/UI/WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	const TCHAR section_dialog[] = _T("MoveFileDialog");
	const TCHAR entry_destFolderHistory[] = _T("DestFolderHistory");
}


namespace layout
{
	static const CLayoutStyle styles[] =
	{
		{ IDC_SOURCE_EDIT, Size },
		{ IDOK, Move },
		{ IDCANCEL, Move }
	};
}


CMoveFileDialog::CMoveFileDialog( const std::vector<fs::CPath>& filesToMove, CWnd* pParent /*= NULL*/ )
	: CLayoutDialog( IDD_FILE_MOVE_DIALOG, pParent )
	, m_filesToMove( filesToMove )
	, m_pDestFolderCombo( new CItemContentHistoryCombo( ui::DirPath ) )
{
	m_regSection = reg::section_dialog;
	RegisterCtrlLayout( ARRAY_SPAN( layout::styles ) );
	m_pDestFolderCombo->RefContent().m_itemsFlags = ui::CItemContent::All | ui::CItemContent::EnsurePathExist;

	m_srcFilesEdit.SetUseFixedFont( false );
	m_srcFilesEdit.SetKeepSelOnFocus();

	ENSURE( !m_filesToMove.empty() );
}

CMoveFileDialog::~CMoveFileDialog()
{
}

void CMoveFileDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_pDestFolderCombo->m_hWnd;

	DDX_Control( pDX, IDC_DEST_FOLDER_COMBO, *m_pDestFolderCombo );
	DDX_Control( pDX, IDC_SOURCE_EDIT, m_srcFilesEdit );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		if ( firstInit )
		{
			m_pDestFolderCombo->LimitText( MAX_PATH );
			m_pDestFolderCombo->LoadHistory( reg::section_dialog, reg::entry_destFolderHistory );
			m_destFolderPath = m_pDestFolderCombo->GetCurrentText();		// store last selected dir path

			m_srcFilesEdit.SetText( str::Join( m_filesToMove, CTextEdit::s_lineEnd ) );

			DragAcceptFiles( TRUE );
		}
	}

	ui::DDX_Path( pDX, IDC_DEST_FOLDER_COMBO, m_destFolderPath );
	ui::EnableControl( m_hWnd, IDOK, fs::IsValidDirectory( m_destFolderPath.GetPtr() ) );

	if ( DialogSaveChanges == pDX->m_bSaveAndValidate )
		m_pDestFolderCombo->SaveHistory( reg::section_dialog, reg::entry_destFolderHistory );

	CLayoutDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CMoveFileDialog, CLayoutDialog )
	ON_WM_DROPFILES()
	ON_CBN_EDITCHANGE( IDC_DEST_FOLDER_COMBO, OnCBnEditChange_DestFolder )
	ON_CBN_SELCHANGE( IDC_DEST_FOLDER_COMBO, OnCBnSelChange_DestFolder )
	ON_CN_DETAILSCHANGED( IDC_DEST_FOLDER_COMBO, OnCnDetailsChanged_DestFolder )
END_MESSAGE_MAP()

void CMoveFileDialog::OnDropFiles( HDROP hDropInfo )
{
	SetForegroundWindow();				// activate us first

	std::vector<fs::CPath> dirPaths;
	shell::QueryDroppedFiles( dirPaths, hDropInfo );

	if ( 1 == dirPaths.size() )
		if ( fs::IsValidDirectory( dirPaths.front().GetPtr() ) )
		{
			m_pDestFolderCombo->SetEditText( dirPaths.front().Get() );
			UpdateData( DialogSaveChanges );
		}
		else
			AfxMessageBox( IDS_DROPONLYONEFOLDER );
	else
		AfxMessageBox( IDS_DROPONLYONEFOLDER );
}

void CMoveFileDialog::OnCBnEditChange_DestFolder( void )
{
	m_destFolderPath.Set( ui::GetComboSelText( *m_pDestFolderCombo, ui::ByEdit ) );
	ui::EnableControl( m_hWnd, IDOK, fs::IsValidDirectory( m_destFolderPath.GetPtr() ) );
}

void CMoveFileDialog::OnCBnSelChange_DestFolder( void )
{
	m_destFolderPath.Set( ui::GetComboSelText( *m_pDestFolderCombo, ui::BySel ) );
	ui::EnableControl( m_hWnd, IDOK, fs::IsValidDirectory( m_destFolderPath.GetPtr() ) );
}

void CMoveFileDialog::OnCnDetailsChanged_DestFolder( void )
{
	UpdateData( DialogSaveChanges );
}
