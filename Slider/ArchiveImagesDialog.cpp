
#include "stdafx.h"
#include "ArchiveImagesDialog.h"
#include "AlbumModel.h"
#include "FileAttr.h"
#include "ICatalogStorage.h"
#include "Application_fwd.h"
#include "resource.h"
#include "utl/UI/PasswordDialog.h"
#include "utl/UI/ShellDialogs.h"
#include "utl/UI/UtilitiesEx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section[] = _T("ArchiveImagesDialog");
	static const TCHAR entry_FormatHistory[] = _T("FormatHistory");
	static const TCHAR entry_FileOperation[] = _T("FileOperation");
	static const TCHAR entry_DestType[] = _T("DestinationType");
	static const TCHAR entry_SeqCounter[] = _T("SeqCounter");
}

namespace layout
{
	static const CLayoutStyle styles[] =
	{
		{ IDC_DEST_DIR_PATH_EDIT, SizeX },
		{ IDC_BROWSE_DEST_FOLDER_BUTTON, MoveX },
		{ IDC_CREATE_DEST_FOLDER, MoveX },
		{ ID_EDIT_ARCHIVE_PASSWORD, MoveX },
		{ IDC_FILE_PATHS_LIST, Size },
		{ IDC_TARGET_GROUP, SizeX | DoRepaint },
		{ IDC_TARGET_FILE_COUNT_STATIC, SizeX },
		{ IDOK, MoveX },
		{ IDCANCEL, MoveX }
	};
}


const TCHAR CArchiveImagesDialog::s_archiveFnameSuffix[] = _T("_stg");


CArchiveImagesDialog::CArchiveImagesDialog( const CAlbumModel* pModel, const std::tstring& srcDocPath, CWnd* pParent /*= NULL*/ )
	: CLayoutDialog( IDD_ARCHIVE_IMAGES_DIALOG, pParent )
	, m_pModel( pModel )
	, m_srcDocPath( srcDocPath )
	, m_fileOp( (FileOp)AfxGetApp()->GetProfileInt( reg::section, reg::entry_FileOperation, FOP_FileCopy ) )
	, m_destType( (DestType)AfxGetApp()->GetProfileInt( reg::section, reg::entry_DestType, ToDirectory ) )
	, m_seqCounter( AfxGetApp()->GetProfileInt( reg::section, reg::entry_SeqCounter, 1 ) )
	, m_destOnSelection( false )
	, m_inInit( true )
	, m_dirty( false )
	, m_filesListCtrl( IDC_FILE_PATHS_LIST )
	, m_formatCombo( ui::HistoryMaxSize, PROF_SEP )
{
	// init base
	m_regSection = reg::section;
	RegisterCtrlLayout( ARRAY_PAIR( layout::styles ) );
	m_initCentered = false;

	m_filesListCtrl.SetSection( m_regSection + _T("\\List") );

	ASSERT_PTR( m_pModel );
	ENSURE( FOP_FileCopy == m_fileOp || FOP_FileMove == m_fileOp );
}

CArchiveImagesDialog::~CArchiveImagesDialog()
{
}

bool CArchiveImagesDialog::FetchFileContext( void )
{
	if ( m_destOnSelection )
	{
		m_lvState.FromListCtrl( &m_filesListCtrl );

		std::vector< CFileAttr* > selSrcFileAttrs;
		m_pModel->QueryFileAttrsSequence( selSrcFileAttrs, m_lvState.m_pIndexImpl->m_selItems );
		m_archivingModel.SetupSourcePaths( selSrcFileAttrs );

		return !selSrcFileAttrs.empty();
	}

	m_archivingModel.SetupSourcePaths( m_pModel->GetImagesModel().GetFileAttrs() );
	return m_pModel->AnyFoundFiles();
}

bool CArchiveImagesDialog::SetDefaultDestPath( void )
{
	if ( !m_pModel->AnyFoundFiles() )
	{
		m_destPath.Clear();
		return false;
	}

	std::tstring firstDirPath = m_pModel->GetFileAttr( 0 )->GetPath().GetParentPath().Get();
	switch ( m_destType )
	{
		default: ASSERT( false );
		case ToDirectory:
			m_destPath.Set( firstDirPath );
			break;
		case ToArchiveStg:
		{
			fs::CPathParts parts( firstDirPath );
			parts.m_fname += parts.m_ext + s_archiveFnameSuffix;			// add to fname existing ext (for folders with extension)
			parts.m_ext = CCatalogStorageFactory::GetDefaultExtension();
			m_destPath = parts.MakePath();
			break;
		}
	}
	ui::EnableControl( m_hWnd, ID_EDIT_ARCHIVE_PASSWORD, ToArchiveStg == m_destType );
	return true;
}

void CArchiveImagesDialog::SetupFilesView( bool firstTimeInit /*= false*/ )
{
	CScopedInternalChange internalChange( &m_filesListCtrl );
	{
		CScopedLockRedraw freeze( &m_filesListCtrl );

		if ( firstTimeInit )
			m_filesListCtrl.DeleteAllItems();
		else
			m_lvState.FromListCtrl( &m_filesListCtrl );

		if ( firstTimeInit )
			for ( UINT i = 0; i != m_pModel->GetFileAttrCount(); ++i )
			{
				const fs::CFlexPath& srcPath = m_pModel->GetFileAttr( i )->GetPath();

				m_filesListCtrl.InsertItem( LVIF_TEXT, i, (LPTSTR)srcPath.GetOriginParentPath().GetPtr(), 0, 0, 0, 0 );
				m_filesListCtrl.SetSubItemText( i, SrcFilename, srcPath.GetFilename() );
				m_filesListCtrl.SetSubItemText( i, DestFilename, _T("") );		// clear the dest fname-ext field
			}

		// setup the destination for selected items
		const std::vector< TTransferPathPair >& xferPairs = m_archivingModel.GetPathPairs();
		if ( m_destOnSelection )
		{
			for ( size_t i = 0; i != xferPairs.size(); ++i )
			{
				const fs::CFlexPath& srcPath = xferPairs[ i ].second;
				m_filesListCtrl.SetSubItemText( m_lvState.m_pIndexImpl->m_selItems[ i ], DestFilename, srcPath.GetFilename() );
			}
		}
		else
		{
			for ( UINT i = 0; i != xferPairs.size(); ++i )
			{
				const fs::CFlexPath& srcPath = xferPairs[ i ].second;
				m_filesListCtrl.SetSubItemText( i, DestFilename, srcPath.GetFilename() );
			}
		}
	}

	// restore the selection (if any)
	if ( m_destOnSelection )
		m_lvState.ToListCtrl( &m_filesListCtrl );
	else
		m_filesListCtrl.ClearSelection();
}

void CArchiveImagesDialog::UpdateDirty( void )
{
	ui::SetDlgItemText( m_hWnd, IDOK, m_dirty ? _T("&Make Files") : ( FOP_FileCopy == m_fileOp ? _T("COPY Files") : _T("MOVE Files") ) );
	ui::EnableControl( m_hWnd, IDC_CREATE_DEST_FOLDER, m_destType == ToDirectory );

	UpdateTargetFileCountStatic();
}

void CArchiveImagesDialog::SetDirty( bool isDirty /*= true*/ )
{
	if ( isDirty && !m_dirty )
	{
		m_archivingModel.ResetDestPaths();
		SetupFilesView();
		SetDlgItemInt( IDC_COUNTER_EDIT, m_seqCounter );
	}
	m_dirty = isDirty;
	UpdateDirty();
}

bool CArchiveImagesDialog::CheckDestFolder( void )
{
	if ( m_destType == ToArchiveStg || m_destPath.FileExist() )
		return true;

	if ( IDCANCEL == AfxMessageBox( str::Format( IDS_PROMPT_INVALID_FOLDER, m_destPath.GetPtr() ).c_str(), MB_OKCANCEL | MB_ICONQUESTION ) )
		return false;

	try
	{
		fs::thr::CreateDirPath( m_destPath.GetPtr(), fs::MfcExc );
		return true;
	}
	catch ( CException* pExc )
	{
		app::HandleReportException( pExc );
		return false;
	}
}

bool CArchiveImagesDialog::GenerateDestFiles( void )
{
	std::tstring format = m_formatCombo.GetCurrentText();
	bool validFormat = CArchivingModel::IsValidFormat( format );
	if ( !validFormat )
	{
		m_formatCombo.SetFrameColor( color::Error );
		AfxMessageBox( str::Format( IDS_INVALID_FORMAT_FMT, format.c_str() ).c_str(), MB_OK | MB_ICONEXCLAMATION );
		return false;
	}
	else if ( !CheckDestFolder() || !FetchFileContext() )
		return false;

	UINT prevCounter = GetDlgItemInt( IDC_COUNTER_EDIT ), newCounter = prevCounter;

	// In dialog generation mode, we need to force shallow stream names (root-only) since we don't save an "_Album.sld",
	// so we can't persist CAlbumModel::UseDeepStreamPaths flag for consistency on load.
	//
	if ( m_archivingModel.GenerateDestPaths( m_destPath, format, &newCounter, true ) )
	{
		m_formatCombo.SetFrameColor( color::Null );
		if ( newCounter != prevCounter )
			SetDlgItemInt( IDC_COUNTER_EDIT, newCounter );

		SetupFilesView();	// fill in and select the found files list
		m_dirty = false;
		UpdateDirty();
		return true;
	}
	else
	{
		m_formatCombo.SetFrameColor( color::Error );
		SetDirty();
		return false;
	}
}

bool CArchiveImagesDialog::CommitFileOperation( void )
{
	std::tstring message = str::Format( IDS_PROMPT_FILE_OP,
		m_fileOp == FOP_FileCopy ? _T("copy") : _T("move"),
		m_archivingModel.GetPathPairs().size(),
		m_destPath.GetPtr() );

	if ( AfxMessageBox( message.c_str(), MB_OKCANCEL | MB_ICONQUESTION ) != IDOK )
		return false;

	switch ( m_destType )
	{
		default: ASSERT( false );
		case ToDirectory:
			return m_archivingModel.CommitOperations( m_fileOp );
		case ToArchiveStg:
			if ( m_destPath.FileExist() )
				if ( app::GetUserReport().MessageBox( str::Format( IDS_OVERWRITE_PROMPT, m_destPath.GetPtr() ), MB_YESNO | MB_ICONQUESTION ) != IDYES )
					return false;

			return m_archivingModel.BuildArchiveStorageFile( m_destPath, m_fileOp, this );
	}
}

void CArchiveImagesDialog::UpdateTargetFileCountStatic( void )
{
	size_t fileCount = m_destOnSelection ? m_lvState.GetSelCount() : m_pModel->GetFileAttrCount();
	ui::SetWindowText( m_targetFileCountStatic, str::Format( _T("%d files"), fileCount ) );
}

void CArchiveImagesDialog::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_formatCombo.m_hWnd;

	DDX_Control( pDX, IDC_TARGET_FILE_COUNT_STATIC, m_targetFileCountStatic );
	DDX_Control( pDX, IDC_FILE_PATHS_LIST, m_filesListCtrl );
	DDX_Control( pDX, IDC_FORMAT_COMBO, m_formatCombo );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		if ( firstInit )
		{
			m_formatCombo.LimitText( _MAX_PATH );
			m_formatCombo.LoadHistory( m_regSection.c_str(), reg::entry_FormatHistory, _T("*.*") );
		}

		LOGFONT logFont;
		m_targetFileCountStatic.GetFont()->GetLogFont( &logFont );
		_tcscpy( logFont.lfFaceName, _T("Arial") );
		logFont.lfWeight = FW_HEAVY;
		logFont.lfHeight = -30;
		m_largeBoldFont.CreateFontIndirect( &logFont );
		m_targetFileCountStatic.SetFont( &m_largeBoldFont );
	}

	CLayoutDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CArchiveImagesDialog, CLayoutDialog )
	ON_WM_DESTROY()
	ON_WM_CTLCOLOR()
	ON_CBN_EDITCHANGE( IDC_FORMAT_COMBO, OnCBnChange_Format )
	ON_CBN_SELCHANGE( IDC_FORMAT_COMBO, OnCBnChange_Format )
	ON_EN_CHANGE( IDC_COUNTER_EDIT, OnEnChangeCounter )
	ON_BN_CLICKED( IDC_BROWSE_DEST_FOLDER_BUTTON, CmBrowseDestPath )
	ON_BN_CLICKED( IDC_COPY_FILES_RADIO, OnToggleCopyFilesRadio )
	ON_BN_CLICKED( IDC_RENAME_FILES_RADIO, OnToggleRenameFilesRadio )
	ON_EN_CHANGE( IDC_DEST_DIR_PATH_EDIT, OnEnChangeDestFolder )
	ON_BN_CLICKED( IDC_CREATE_DEST_FOLDER, OnBnClicked_CreateDestFolder )
	ON_BN_CLICKED( ID_EDIT_ARCHIVE_PASSWORD, OnEditArchivePassword )
	ON_BN_CLICKED( IDC_TO_FOLDER_RADIO, OnToggleToFolderRadio )
	ON_BN_CLICKED( IDC_TO_COMPOUND_FILE_RADIO, OnToggleToCompoundFileRadio )
	ON_BN_CLICKED( IDC_SELECTED_FILES_RADIO, OnToggleTargetSelectedFilesRadio )
	ON_BN_CLICKED( IDC_ALL_FILES_RADIO, OnToggleTargetAllFilesRadio )
	ON_NOTIFY( LVN_ITEMCHANGED, IDC_FILE_PATHS_LIST, LVnItemChangedFilePathsList )
	ON_BN_CLICKED( IDC_RESETNUMCOUNTER_BUTTON, CmResetNumCounter )
	ON_BN_DOUBLECLICKED( IDC_RESETNUMCOUNTER_BUTTON, CmResetNumCounter )
END_MESSAGE_MAP()

BOOL CArchiveImagesDialog::OnInitDialog( void )
{
	CLayoutDialog::OnInitDialog();

	SetDlgItemInt( IDC_COUNTER_EDIT, m_seqCounter );

	CheckRadioButton( IDC_COPY_FILES_RADIO, IDC_RENAME_FILES_RADIO, IDC_COPY_FILES_RADIO + ( m_fileOp == FOP_FileMove ) );
	CheckRadioButton( IDC_TO_FOLDER_RADIO, IDC_TO_COMPOUND_FILE_RADIO, IDC_TO_FOLDER_RADIO + m_destType );

	// by default set the destination folder to the folder of the first file (if any)
	SetDefaultDestPath();
	ui::SetDlgItemText( m_hWnd, IDC_DEST_DIR_PATH_EDIT, m_destPath.Get() );

	m_destOnSelection = m_lvState.m_pIndexImpl->m_selItems.size() > 1;
	CheckRadioButton( IDC_SELECTED_FILES_RADIO, IDC_ALL_FILES_RADIO, m_destOnSelection ? IDC_SELECTED_FILES_RADIO : IDC_ALL_FILES_RADIO );
	SetupFilesView( true );	// fill in and select the found files list

	m_inInit = false;
	m_dirty = true;

	UpdateDirty();
	return TRUE;
}

void CArchiveImagesDialog::OnDestroy( void )
{
	AfxGetApp()->WriteProfileInt( m_regSection.c_str(), reg::entry_FileOperation, m_fileOp );
	AfxGetApp()->WriteProfileInt( m_regSection.c_str(), reg::entry_DestType, m_destType );

	CLayoutDialog::OnDestroy();
}

void CArchiveImagesDialog::OnOK( void )
{
	if ( m_dirty )
		GenerateDestFiles();
	else if ( CheckDestFolder() )
	{
		if ( CommitFileOperation() )
		{
			m_formatCombo.SaveHistory( m_regSection.c_str(), reg::entry_FormatHistory );
			AfxGetApp()->WriteProfileInt( m_regSection.c_str(), reg::entry_SeqCounter, GetDlgItemInt( IDC_COUNTER_EDIT ) );
			CLayoutDialog::OnOK();
		}
	}
	else
		SetDirty();
}

void CArchiveImagesDialog::OnEnChangeCounter( void )
{
	if ( !m_inInit )
	{
		if ( GetFocus() == GetDlgItem( IDC_COUNTER_EDIT ) )
			m_seqCounter = GetDlgItemInt( IDC_COUNTER_EDIT );
		m_inInit = true;
		SetDirty();
		m_inInit = false;
	}
}

void CArchiveImagesDialog::OnToggleCopyFilesRadio( void )
{
	m_fileOp = FOP_FileCopy;
	SetDirty();
}

void CArchiveImagesDialog::OnToggleRenameFilesRadio( void )
{
	m_fileOp = FOP_FileMove;
	SetDirty();
}

void CArchiveImagesDialog::OnToggleToFolderRadio( void )
{
	m_destType = ToDirectory;
	SetDefaultDestPath();
	ui::SetDlgItemText( m_hWnd, IDC_DEST_DIR_PATH_EDIT, m_destPath.Get() );
	SetDirty();
}

void CArchiveImagesDialog::OnToggleToCompoundFileRadio( void )
{
	m_destType = ToArchiveStg;
	SetDefaultDestPath();
	ui::SetDlgItemText( m_hWnd, IDC_DEST_DIR_PATH_EDIT, m_destPath.Get() );
	SetDirty();
}

void CArchiveImagesDialog::OnToggleTargetSelectedFilesRadio( void )
{
	m_destOnSelection = true;
	m_lvState.ToListCtrl( &m_filesListCtrl );		// restore the last known selection
	SetDirty();
}

void CArchiveImagesDialog::OnToggleTargetAllFilesRadio( void )
{
	m_destOnSelection = false;
	m_lvState.FromListCtrl( &m_filesListCtrl );		// backup the current selection state
	m_filesListCtrl.ClearSelection();
	SetDirty();
}

void CArchiveImagesDialog::OnCBnChange_Format( void )
{
	if ( !m_inInit )
		SetDirty();

	m_formatCombo.SetFrameColor( CArchivingModel::IsValidFormat( m_formatCombo.GetCurrentText() ) ? color::Null : color::Error );
}

void CArchiveImagesDialog::OnEnChangeDestFolder( void )
{
	if ( !m_inInit )
	{
		m_destPath.Set( ui::GetDlgItemText( m_hWnd, IDC_DEST_DIR_PATH_EDIT ) );
		SetDirty();
	}

	ui::EnableControl( m_hWnd, IDC_CREATE_DEST_FOLDER,
		!m_destPath.IsEmpty() &&
		!m_destPath.FileExist() &&
		!app::IsCatalogFile( m_destPath.GetPtr() ) );
}

void CArchiveImagesDialog::CmBrowseDestPath( void )
{
	if ( ToDirectory == m_destType )
	{
		if ( !shell::BrowseForFolder( m_destPath, this, NULL, shell::BF_FileSystem, _T("Destination folder"), false ) )
			return;
	}
	else
	{
		if ( !app::BrowseCatalogFile( m_destPath, this, shell::FileSaveAs ) )
			return;
	}

	ui::SetDlgItemText( m_hWnd, IDC_DEST_DIR_PATH_EDIT, m_destPath.Get() );
	CheckDestFolder();
}

void CArchiveImagesDialog::OnBnClicked_CreateDestFolder( void )
{
	bool success = fs::CreateDirPath( m_destPath.GetPtr() );
	AfxMessageBox( str::Format( success ? IDS_PROMPT_CREATED_DEST_FOLDER : IDS_PROMPT_CANT_CREATE_DEST_FOLDER, m_destPath.GetPtr() ).c_str(), MB_ICONINFORMATION );
}

void CArchiveImagesDialog::OnEditArchivePassword( void )
{
	CPasswordDialog dlg( this, &m_destPath );
	dlg.SetPassword( m_archivingModel.GetPassword() );

	if ( IDOK == dlg.DoModal() )
		m_archivingModel.StorePassword( dlg.GetPassword() );
}

void CArchiveImagesDialog::LVnItemChangedFilePathsList( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLISTVIEW* pNmListView = (NMLISTVIEW*)pNmHdr; pNmListView;

	// store new selection only if not empty
	if ( m_filesListCtrl.GetSelectedCount() != 0 )
	{
		m_lvState.FromListCtrl( &m_filesListCtrl );
		CheckRadioButton( IDC_SELECTED_FILES_RADIO, IDC_ALL_FILES_RADIO, IDC_SELECTED_FILES_RADIO );
		OnToggleTargetSelectedFilesRadio();
	}
	if ( m_destOnSelection )
		SetDirty();
	*pResult = 0;
}

void CArchiveImagesDialog::CmResetNumCounter( void )
{
	SetDlgItemInt( IDC_COUNTER_EDIT, 1 );
	GotoDlgCtrl( GetDlgItem( IDC_COUNTER_EDIT ) );
}

HBRUSH CArchiveImagesDialog::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor )
{
	HBRUSH hBkBrush = CLayoutDialog::OnCtlColor( pDC, pWnd, nCtlColor );

	if ( nCtlColor == CTLCOLOR_STATIC && pWnd == &m_targetFileCountStatic )
		pDC->SetTextColor( 0xFF0000 );

	return hBkBrush;
}
