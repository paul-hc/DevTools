
#include "pch.h"
#include "WkspSaveDialog.h"
#include "WorkspaceProfile.h"
#include "Application_fwd.h"
#include "resource.h"
#include "utl/Registry.h"
#include "utl/UI/MenuUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace layout
{
	static CLayoutStyle styles[] =
	{
		{ IDC_PROJECT_NAME_COMBO, SizeX },
		{ IDC_FILES_LIST, Size },
		{ IDC_SHOW_FULL_PATH_CHECK, MoveX },
		{ IDC_FULLPATH_STATIC, MoveY },
		{ IDC_FULLPATH_EDIT, MoveY | SizeX },
		{ IDOK, MoveX },
		{ IDC_DELETE_PROJECT_ENTRY, MoveX },
		{ IDC_DELETE_ALL_PROJECTS, MoveX },
		{ IDCANCEL, MoveX },
		{ IDC_SORTBY_FILES, Move }
	};
}


CWkspSaveDialog::CWkspSaveDialog( WorkspaceProfile& rWkspProfile, const CString& section, const CString& currProjectName, CWnd* pParent )
	: CLayoutDialog( IDD_WORKSPACE_SAVE_DIALOG, pParent )
	, m_section( section )
	, m_currProjectName( currProjectName )
	, m_rWkspProfile( rWkspProfile )
	, m_rOptions( m_rWkspProfile.m_options )
	, m_folderItem( _T("") )
	, m_fullPathEdit( ui::FilePath )
{
	m_regSection = WorkspaceProfile::s_sectionWorkspaceDialogs;
	RegisterCtrlLayout( ARRAY_SPAN( layout::styles ) );
	LoadDlgIcon( IDD_WORKSPACE_SAVE_DIALOG );

	if ( m_section.IsEmpty() )
		m_section = WorkspaceProfile::s_defaulWkspSection;
	if ( m_currProjectName.IsEmpty() )
		m_currProjectName = WorkspaceProfile::s_defaulProjectName;

	ui::LoadPopupMenu( m_sortOrderPopup, IDR_CONTEXT_MENU, app::FileSortOrderPopup );

	CWinApp* pApp = AfxGetApp();

	m_rOptions.m_fileSortOrder.SetOrderText(
		pApp->GetProfileString( m_regSection.c_str(), ENTRY_OF( SortOrder ), m_rOptions.m_fileSortOrder.GetOrderTextPtr()->c_str() ).GetString() );
	m_rOptions.m_displayFullPath = pApp->GetProfileInt( m_regSection.c_str(), ENTRY_OF( m_displayFullPath ), m_rOptions.m_displayFullPath ) != FALSE;
}

CWkspSaveDialog::~CWkspSaveDialog()
{
}

void CWkspSaveDialog::setupWindow( void )
{
	ASSERT( IsWindow( m_hWnd ) );

	m_rWkspProfile.AddProjectName( m_currProjectName );
	loadExistingProjects();

	for ( unsigned int i = 0; i != m_rWkspProfile.projectNameArray.size(); ++i )
		m_projectNameCombo.AddString( m_rWkspProfile.projectNameArray[ i ] );

	if ( -1 == m_projectNameCombo.SelectString( -1, m_currProjectName ) )
		m_projectNameCombo.SetCurSel( 0 );

	updateFileContents();
}

void CWkspSaveDialog::cleanupWindow( void )
{
	CWinApp* pApp = AfxGetApp();

	pApp->WriteProfileString( m_regSection.c_str(), ENTRY_OF( SortOrder ), m_rOptions.m_fileSortOrder.GetOrderTextPtr()->c_str() );
	pApp->WriteProfileInt( m_regSection.c_str(), ENTRY_OF( m_displayFullPath ), m_rOptions.m_displayFullPath );
}

bool CWkspSaveDialog::loadExistingProjects( void )
{
	// Enums the already stored projects keys, by ex:
	//	HKEY_CURRENT_USER\Software\RegistryKey\AppName\section\Project1
	reg::CKey projectsKey( AfxGetApp()->GetSectionKey( m_section ) );
	if ( !projectsKey.IsOpen() )
		return false;

	std::vector< std::tstring > subKeyNames;
	projectsKey.QuerySubKeyNames( subKeyNames );

	for ( std::vector< std::tstring >::const_iterator itSubKeyName = subKeyNames.begin(); itSubKeyName != subKeyNames.end(); ++itSubKeyName )
		m_rWkspProfile.AddProjectName( itSubKeyName->c_str() );

	return !subKeyNames.empty();
}

void CWkspSaveDialog::updateFileContents( void )
{
	int selIndex = m_fileList.GetCurSel();
	CString orgSelString;

	if ( selIndex != -1 )
		m_fileList.GetText( selIndex, orgSelString );

	m_folderItem.Clear();
	for ( unsigned int i = 0; i != m_rWkspProfile.fileArray.size(); ++i )
		m_folderItem.AddFileItem( nullptr, fs::CPath( m_rWkspProfile.fileArray[ i ].GetString() ) );

	m_fileList.SetRedraw( FALSE );
	m_fileList.ResetContent();

	const std::vector< CFileItem* >& fileItems = m_folderItem.GetFileItems();
	for ( unsigned int index = 0; index != fileItems.size(); ++index )
	{
		CFileItem* pFileItem = fileItems[ index ];

		m_fileList.AddString( pFileItem->FormatLabel( &m_rOptions ).c_str() );
		m_fileList.SetItemDataPtr( index, pFileItem );
	}

	if ( !orgSelString.IsEmpty() )
		VERIFY( m_fileList.SelectString( -1, orgSelString ) != -1 );
	else
		m_fileList.SetCurSel( 0 );

	m_fileList.SetRedraw( TRUE );
	m_fileList.Invalidate();
	LBnSelChangeFiles();
}

bool CWkspSaveDialog::saveFiles( void )
{
	if ( !readProjectName() )
		return false;

	reg::CKey workspacesKey( AfxGetApp()->GetSectionKey( m_section ) );

	if ( !workspacesKey.IsOpen() )
		return false;

	UINT listCount = m_fileList.GetCount();

	if ( 0 == listCount && workspacesKey.HasSubKey( m_currProjectName ) )
	{	// nothing to save but there is an existing current project key -> prompt to delete:
		if ( IDYES == AfxMessageBox( str::Format( _T("There are no files avaiable for '%s' project !\n\nDo you want to remove it's entry?"), m_currProjectName.GetString() ).c_str(), MB_YESNO ) )
		{
			workspacesKey.DeleteSubKey( m_currProjectName );
			return true;
		}
	}

	reg::CKey projectKey;

	if ( listCount != 0 )
		projectKey.Create( workspacesKey.Get(), m_currProjectName.GetString() );
	else
		projectKey.Open( workspacesKey.Get(), m_currProjectName.GetString() );

	if ( projectKey.IsOpen() )
	{
		projectKey.DeleteAll();

		for ( UINT i = 0; i != listCount; ++i )
			projectKey.WriteStringValue( m_rWkspProfile.getFileEntryName( i ), getListFile( i )->GetFilePath().Get() );
	}
	else
		ASSERT( 0 == listCount );

	return true;
}

bool CWkspSaveDialog::readProjectName( void )
{
	m_projectNameCombo.GetWindowText( m_currProjectName );
	if ( !m_currProjectName.IsEmpty() )
		return true;

	AfxMessageBox( IDS_EMPTY_CURR_PROJECT_MESSAGE, MB_OK | MB_ICONHAND );
	return false;
}

CFileItem* CWkspSaveDialog::getListFile( int listIndex ) const
{
	return (CFileItem*)m_fileList.GetItemDataPtr( listIndex );
}

bool CWkspSaveDialog::dragListNotify( UINT listID, DRAGLISTINFO& dragInfo )
{
	if ( listID == IDC_FILES_LIST )
		if ( dragInfo.uNotification == DL_DROPPED )
		{
			int srcIndex = m_fileList.GetCurSel();
			int destIndex = m_fileList.ItemFromPt( dragInfo.ptCursor );

			if ( destIndex == -1 )
			{
				CString message;
				fs::CPath selFilePath;

				if ( srcIndex != -1 )
					selFilePath = getListFile( srcIndex )->GetFilePath();

				message.Format( IDC_REMOVE_SELECTED_FILE, selFilePath.GetPtr() );
				if ( srcIndex != -1 && IDOK == AfxMessageBox( message, MB_OKCANCEL | MB_ICONQUESTION ) )
				{
					int profIndex = m_rWkspProfile.findFile( selFilePath.GetPtr() );

					// remove from source index
					if ( profIndex != -1 )
						m_rWkspProfile.fileArray.erase( m_rWkspProfile.fileArray.begin() + profIndex );

					m_fileList.DeleteString( srcIndex );
					if ( srcIndex >= m_fileList.GetCount() )
						--srcIndex;
					m_fileList.SetCurSel( srcIndex );
					return false;
				}
			}
		}
	return true;
}

void CWkspSaveDialog::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_PROJECT_NAME_COMBO, m_projectNameCombo );
	DDX_Control( pDX, IDC_FILES_LIST, m_fileList );
	DDX_Control( pDX, IDC_FULLPATH_EDIT, m_fullPathEdit );

	CLayoutDialog::DoDataExchange( pDX );
}


UINT WM_DragListNotify = ::RegisterWindowMessage( DRAGLISTMSGSTRING );

BEGIN_MESSAGE_MAP(CWkspSaveDialog, CLayoutDialog)
	ON_WM_DESTROY()
	ON_WM_DROPFILES()
	ON_BN_CLICKED( IDC_DELETE_PROJECT_ENTRY, CmDeleteProjectEntry )
	ON_BN_CLICKED( IDC_DELETE_ALL_PROJECTS, CmDeleteAllProjects )
	ON_BN_CLICKED( IDC_SHOW_FULL_PATH_CHECK, CkShowFullPath )
	ON_BN_CLICKED( IDC_SORTBY_FILES, CmSortByFiles )
	ON_LBN_SELCHANGE( IDC_FILES_LIST, LBnSelChangeFiles )
END_MESSAGE_MAP()

LRESULT CWkspSaveDialog::WindowProc( UINT message, WPARAM wParam, LPARAM lParam )
{
	if ( message == WM_DragListNotify )
		if ( !dragListNotify( wParam, *(DRAGLISTINFO*)lParam ) )
			return FALSE;

	return CLayoutDialog::WindowProc( message, wParam, lParam );
}

BOOL CWkspSaveDialog::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	if ( m_rOptions.OnCmdMsg( id, code, pExtra, pHandlerInfo ) )
		return true;

	return __super::OnCmdMsg( id, code, pExtra, pHandlerInfo );
}

BOOL CWkspSaveDialog::OnInitDialog( void )
{
	CLayoutDialog::OnInitDialog();
	DragAcceptFiles( TRUE );

	CheckDlgButton( IDC_SHOW_FULL_PATH_CHECK, m_rOptions.m_displayFullPath );

	setupWindow();
	return TRUE;
}

void CWkspSaveDialog::OnDestroy( void )
{
	cleanupWindow();
	CLayoutDialog::OnDestroy();
}

void CWkspSaveDialog::OnOK( void )
{
	if ( saveFiles() )
		CLayoutDialog::OnOK();
}

void CWkspSaveDialog::OnDropFiles( HDROP hDropInfo )
{
	UINT fileCount =::DragQueryFile( hDropInfo, UINT( -1 ), nullptr, 0 );
	TCHAR fullPath[ MAX_PATH ];

	for ( UINT index = 0; index < fileCount; ++index )
	{
		::DragQueryFile( hDropInfo, index, fullPath, COUNT_OF( fullPath ) );
		m_rWkspProfile.AddFile( fullPath );
	}

	::DragFinish( hDropInfo );
	if ( fileCount > 0 )
		updateFileContents();
}

void CWkspSaveDialog::CmDeleteProjectEntry( void )
{
	if ( readProjectName() )
	{
		CString message;

		message.Format( IDC_DELETE_PROJECT_ENTRY, (LPCTSTR)m_currProjectName );
		if ( IDOK == AfxMessageBox( message, MB_OKCANCEL | MB_ICONEXCLAMATION ) )
		{
			reg::CKey workspacesKey( AfxGetApp()->GetSectionKey( m_section ) );

			if ( workspacesKey.IsOpen() )
				workspacesKey.DeleteSubKey( m_currProjectName );
		}
	}
}

void CWkspSaveDialog::CmDeleteAllProjects( void )
{
	if ( readProjectName() )
		if ( AfxMessageBox( IDC_DELETE_ALL_PROJECTS, MB_OKCANCEL | MB_ICONEXCLAMATION ) == IDOK )
		{
			reg::CKey workspacesKey( AfxGetApp()->GetSectionKey( m_section ) );

			if ( workspacesKey.IsOpen() )
				workspacesKey.DeleteAllSubKeys();
		}
}

void CWkspSaveDialog::CkShowFullPath( void )
{
	m_rOptions.m_displayFullPath = IsDlgButtonChecked( IDC_SHOW_FULL_PATH_CHECK ) != FALSE;
	m_fileList.SetCurSel( -1 );
	updateFileContents();
}

void CWkspSaveDialog::CmSortByFiles( void )
{
ASSERT( false );

/*	CRect buttonRect;
	GetDlgItem( IDC_SORTBY_FILES )->GetWindowRect( &buttonRect );

	m_rOptions.updateSortOrderMenu( m_sortOrderPopup );
	UINT cmdId = m_sortOrderPopup.TrackPopupMenu( TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
												  buttonRect.right, buttonRect.top, this );

	if ( cmdId != 0 )
		if ( m_rOptions.OnMenuCommand( cmdId ) )
			updateFileContents();*/
}

void CWkspSaveDialog::LBnSelChangeFiles( void )
{
	int selIndex = m_fileList.GetCurSel();
	fs::CPath fullPath;

	if ( selIndex != -1 )
		fullPath = m_folderItem.GetFileItems()[ selIndex ]->GetFilePath();

	SetDlgItemText( IDC_FULLPATH_EDIT, fullPath.GetPtr() );
}
