
#include "StdAfx.h"
#include "MoveFileDialog.h"
#include "resource.h"
#include "utl/FileSystem.h"
#include "utl/ItemContentHistoryCombo.h"
#include "utl/ShellUtilities.h"
#include "utl/Utilities.h"

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
		{ IDC_SOURCE_EDIT, StretchX },
		{ IDC_DEST_FOLDER_COMBO, Stretch },
		{ IDOK, Offset },
		{ IDCANCEL, Offset }
	};
}


CMoveFileDialog::CMoveFileDialog( const std::vector< std::tstring >& filesToMove, CWnd* pParent /*= NULL*/ )
	: CLayoutDialog( IDD_FILE_MOVE_DIALOG, pParent )
	, m_pDestFolderCombo( new CItemContentHistoryCombo( ui::DirPath ) )
{
	m_regSection = reg::section_dialog;
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );
	m_pDestFolderCombo->GetContent().m_itemsFlags = ui::CItemContent::All | ui::CItemContent::EnsurePathExist;

	m_filesToMove.reserve( filesToMove.size() );
	for ( std::vector< std::tstring >::const_iterator itSrc = filesToMove.begin(); itSrc != filesToMove.end(); ++itSrc )
		m_filesToMove.push_back( fs::CPath( *itSrc ) );

	ENSURE( !m_filesToMove.empty() );

}

CMoveFileDialog::~CMoveFileDialog()
{
}

void CMoveFileDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_pDestFolderCombo->m_hWnd;

	DDX_Control( pDX, IDC_DEST_FOLDER_COMBO, *m_pDestFolderCombo );
	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		if ( firstInit )
		{
			m_pDestFolderCombo->LimitText( MAX_PATH );
			m_pDestFolderCombo->LoadHistory( reg::section_dialog, reg::entry_destFolderHistory );
			m_destFolderPath = m_pDestFolderCombo->GetCurrentText();		// store last selected dir path

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
	ON_CBN_EDITCHANGE( IDC_DEST_FOLDER_COMBO, OnCBnEditChangeDestFolder )
	ON_CBN_SELCHANGE( IDC_DEST_FOLDER_COMBO, OnCBnSelChangeDestFolder )
	ON_WM_DROPFILES()
END_MESSAGE_MAP()

BOOL CMoveFileDialog::OnInitDialog( void )
{
	CLayoutDialog::OnInitDialog();

	std::tstring filesToMoveText;

	ASSERT( !m_filesToMove.empty() );
	if ( 1 == m_filesToMove.size() )
		filesToMoveText = m_filesToMove.front().Get();
	else
	{
		fs::CPath refDirPath( m_filesToMove.front().GetDirPath() );

		for ( std::vector< fs::CPath >::const_iterator itSrc = m_filesToMove.begin(); itSrc != m_filesToMove.end(); ++itSrc )
		{
			fs::CPath dirPath( itSrc->GetDirPath() );

			if ( !filesToMoveText.empty() )
				filesToMoveText += _T("; ");

			if ( dirPath == refDirPath )
				filesToMoveText += itSrc->GetNameExt();
			else
				filesToMoveText += itSrc->Get();
		}
		filesToMoveText = refDirPath.Get() + _T(": ") + filesToMoveText;
	}

	ui::SetDlgItemText( m_hWnd, IDC_SOURCE_EDIT, filesToMoveText );
	return TRUE;
}

void CMoveFileDialog::OnDropFiles( HDROP hDropInfo )
{
	UINT fileCount = ::DragQueryFile( hDropInfo, UINT_MAX, NULL, 0 );

	SetForegroundWindow();				// activate us first
	if ( 1 == fileCount )
	{
		TCHAR folderPath[ MAX_PATH ];
		::DragQueryFile( hDropInfo, 0, folderPath, COUNT_OF( folderPath ) );
		if ( fs::IsValidDirectory( folderPath ) )
		{
			m_destFolderPath.Set( folderPath );
			UpdateData( DialogSaveChanges );
		}
		else
			AfxMessageBox( IDS_DROPONLYONEFOLDER );
	}
	else
		AfxMessageBox( IDS_DROPONLYONEFOLDER );

	::DragFinish( hDropInfo );
}

void CMoveFileDialog::OnCBnEditChangeDestFolder( void )
{
	m_destFolderPath.Set( ui::GetComboSelText( *m_pDestFolderCombo, ui::ByEdit ) );
	ui::EnableControl( m_hWnd, IDOK, fs::IsValidDirectory( m_destFolderPath.GetPtr() ) );
}

void CMoveFileDialog::OnCBnSelChangeDestFolder( void )
{
	m_destFolderPath.Set( ui::GetComboSelText( *m_pDestFolderCombo, ui::BySel ) );
	ui::EnableControl( m_hWnd, IDOK, fs::IsValidDirectory( m_destFolderPath.GetPtr() ) );
}
