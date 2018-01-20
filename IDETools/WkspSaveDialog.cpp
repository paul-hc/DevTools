
#include "stdafx.h"
#include "WkspSaveDialog.h"
#include "WorkspaceProfile.h"
#include "Application_fwd.h"
#include "resource.h"
#include "utl/MenuUtilities.h"
#include "utl/Registry.h"

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


CWkspSaveDialog::CWkspSaveDialog( WorkspaceProfile& _wkspProfile, const CString& _section, const CString& _currProjectName, CWnd* parent )
	: CLayoutDialog( IDD_WORKSPACE_SAVE_DIALOG, parent )
	, section( _section )
	, currProjectName( _currProjectName )
	, wkspProfile( _wkspProfile )
	, m_rOptions( wkspProfile.m_options )
	, metaFolder( m_rOptions, _T(""), _T("") )
	, showFullPath( false )
	, m_fullPathEdit( ui::FilePath )
{
	m_regSection = ::sectionWorkspaceDialogs;
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );
	LoadDlgIcon( IDD_WORKSPACE_SAVE_DIALOG );

	if ( section.IsEmpty() )
		section = ::defaulWkspSection;
	if ( currProjectName.IsEmpty() )
		currProjectName = ::defaulProjectName;

	ui::LoadPopupMenu( m_sortOrderPopup, IDR_CONTEXT_MENU, app_popup::SortFiles );

	CWinApp* pApp = AfxGetApp();

	m_rOptions.m_fileSortOrder.SetFromString(
		(LPCTSTR)pApp->GetProfileString( m_regSection.c_str(), ENTRY_OF( fileSortOrder ), m_rOptions.m_fileSortOrder.GetAsString().c_str() ) );
	showFullPath = pApp->GetProfileInt( m_regSection.c_str(), ENTRY_OF( showFullPath ), showFullPath ) != FALSE;

}

CWkspSaveDialog::~CWkspSaveDialog()
{
}

void CWkspSaveDialog::setupWindow( void )
{
	ASSERT( IsWindow( m_hWnd ) );

	wkspProfile.AddProjectName( currProjectName );
	loadExistingProjects();

	for ( unsigned int i = 0; i != wkspProfile.projectNameArray.size(); ++i )
		projectNameCombo.AddString( wkspProfile.projectNameArray[ i ] );

	if ( projectNameCombo.SelectString( -1, currProjectName ) == -1 )
		projectNameCombo.SetCurSel( 0 );

	updateFileContents();
}

void CWkspSaveDialog::cleanupWindow( void )
{
	CWinApp* pApp = AfxGetApp();

	pApp->WriteProfileString( m_regSection.c_str(), ENTRY_OF( fileSortOrder ), m_rOptions.m_fileSortOrder.GetAsString().c_str() );

	pApp->WriteProfileInt( m_regSection.c_str(), ENTRY_OF( showFullPath ), showFullPath );
}

bool CWkspSaveDialog::loadExistingProjects( void )
{
	// Enums the already stored projects keys, by ex:
	//	HKEY_CURRENT_USER\Software\RegistryKey\AppName\section\Project1
	reg::CKey projectsKey( AfxGetApp()->GetSectionKey( section ) );
	if ( !projectsKey.IsValid() )
		return false;

	reg::CKeyIterator itProj( &projectsKey );

	for ( ; itProj.IsValid(); ++itProj )
		wkspProfile.AddProjectName( itProj.GetName() );

	return itProj.GetCount() > 0;
}

void CWkspSaveDialog::updateFileContents( void )
{
	int selIndex = fileList.GetCurSel();
	CString orgSelString;

	if ( selIndex != -1 )
		fileList.GetText( selIndex, orgSelString );

	metaFolder.Clear();
	for ( unsigned int i = 0; i != wkspProfile.fileArray.size(); ++i )
		metaFolder.addFile( wkspProfile.fileArray[ i ] );

	fileList.SetRedraw( FALSE );
	fileList.ResetContent();
	for ( unsigned int i = 0; i < metaFolder.getFileCount(); ++i )
	{
		CMetaFolder::CFile* file = metaFolder.getFile( i );

		fileList.AddString( showFullPath ? file->m_pathInfo.Get() : file->getLabel() );
		fileList.SetItemDataPtr( i, file );
	}

	if ( !orgSelString.IsEmpty() )
		VERIFY( fileList.SelectString( -1, orgSelString ) != -1 );
	else
		fileList.SetCurSel( 0 );

	fileList.SetRedraw( TRUE );
	fileList.Invalidate();
	LBnSelChangeFiles();
}

bool CWkspSaveDialog::saveFiles( void )
{
	if ( !readProjectName() )
		return false;

	reg::CKey keyWorkspaces( AfxGetApp()->GetSectionKey( section ) );

	if ( !keyWorkspaces.IsValid() )
		return false;

	int listCount = fileList.GetCount();

	if ( listCount == 0 && keyWorkspaces.HasSubKey( currProjectName ) )
	{	// Nothing to save but there is an existing current project key -> query for remove:
		CString message;

		message.Format( IDS_QUERY_DELETE_PROJECT_MESSAGE, (LPCTSTR)currProjectName );
		if ( AfxMessageBox( message, MB_YESNO ) == IDYES )
		{
			keyWorkspaces.RemoveSubKey( currProjectName );
			return true;
		}
	}

	reg::CKey keyCurrProject = keyWorkspaces.OpenSubKey( currProjectName, listCount > 0 );

	if ( keyCurrProject.IsValid() )
	{
		keyCurrProject.RemoveAll();
		for ( int i = 0; i < listCount; ++i )
		{
			reg::CValue entryValue;
			entryValue.SetString( getListFile( i )->m_pathInfo.Get() );
			VERIFY( keyCurrProject.SetValue( wkspProfile.getFileEntryName( i ), entryValue ) );
		}
	}
	else
		ASSERT( listCount == 0 );

	return true;
}

bool CWkspSaveDialog::readProjectName( void )
{
	projectNameCombo.GetWindowText( currProjectName );
	if ( !currProjectName.IsEmpty() )
		return true;
	AfxMessageBox( IDS_EMPTY_CURR_PROJECT_MESSAGE, MB_OK | MB_ICONHAND );
	return false;
}

CMetaFolder::CFile* CWkspSaveDialog::getListFile( int listIndex ) const
{
	return (CMetaFolder::CFile*)fileList.GetItemDataPtr( listIndex );
}

bool CWkspSaveDialog::dragListNotify( UINT listID, DRAGLISTINFO& dragInfo )
{
	if ( listID == IDC_FILES_LIST )
		if ( dragInfo.uNotification == DL_DROPPED )
		{
			int srcIndex = fileList.GetCurSel();
			int destIndex = fileList.ItemFromPt( dragInfo.ptCursor );

			if ( destIndex == -1 )
			{
				CString message;
				CString selFilePath;

				if ( srcIndex != -1 )
					selFilePath = getListFile( srcIndex )->m_pathInfo.Get();

				message.Format( IDC_REMOVE_SELECTED_FILE, (LPCTSTR)selFilePath );
				if ( srcIndex != -1 && AfxMessageBox( message, MB_OKCANCEL | MB_ICONQUESTION ) == IDOK )
				{
					int profIndex = wkspProfile.findFile( selFilePath );

					// remove from source index
					if ( profIndex != -1 )
						wkspProfile.fileArray.erase( wkspProfile.fileArray.begin() + profIndex );

					fileList.DeleteString( srcIndex );
					if ( srcIndex >= fileList.GetCount() )
						--srcIndex;
					fileList.SetCurSel( srcIndex );
					return false;
				}
			}
		}
	return true;
}

void CWkspSaveDialog::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_PROJECT_NAME_COMBO, projectNameCombo );
	DDX_Control( pDX, IDC_FILES_LIST, fileList );
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

BOOL CWkspSaveDialog::OnInitDialog( void )
{
	CLayoutDialog::OnInitDialog();
	DragAcceptFiles( TRUE );

	CheckDlgButton( IDC_SHOW_FULL_PATH_CHECK, showFullPath );

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
	UINT fileCount =::DragQueryFile( hDropInfo, UINT( -1 ), NULL, 0 );
	TCHAR fullPath[ MAX_PATH ];

	for ( UINT index = 0; index < fileCount; index++ )
	{
		::DragQueryFile( hDropInfo, index, fullPath, COUNT_OF( fullPath ) );
		wkspProfile.AddFile( fullPath );
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

		message.Format( IDC_DELETE_PROJECT_ENTRY, (LPCTSTR)currProjectName );
		if ( AfxMessageBox( message, MB_OKCANCEL | MB_ICONEXCLAMATION ) == IDOK )
		{
			reg::CKey keyWorkspaces( AfxGetApp()->GetSectionKey( section ) );

			if ( keyWorkspaces.IsValid() && keyWorkspaces.HasSubKey( currProjectName ) )
				keyWorkspaces.RemoveSubKey( currProjectName );
		}
	}
}

void CWkspSaveDialog::CmDeleteAllProjects( void )
{
	if ( readProjectName() )
		if ( AfxMessageBox( IDC_DELETE_ALL_PROJECTS, MB_OKCANCEL | MB_ICONEXCLAMATION ) == IDOK )
		{
			reg::CKey keyWorkspaces( AfxGetApp()->GetSectionKey( section ) );

			if ( keyWorkspaces.IsValid() )
				keyWorkspaces.RemoveAllSubKeys();
		}
}

void CWkspSaveDialog::CkShowFullPath( void )
{
	showFullPath = IsDlgButtonChecked( IDC_SHOW_FULL_PATH_CHECK ) != FALSE;
	fileList.SetCurSel( -1 );
	updateFileContents();
}

void CWkspSaveDialog::CmSortByFiles( void )
{
	CRect buttonRect;

	GetDlgItem( IDC_SORTBY_FILES )->GetWindowRect( buttonRect );

	m_rOptions.updateSortOrderMenu( m_sortOrderPopup );
	UINT cmdId = m_sortOrderPopup.TrackPopupMenu( TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
												  buttonRect.right, buttonRect.top, this );

	if ( cmdId != 0 )
		if ( m_rOptions.OnMenuCommand( cmdId ) )
			updateFileContents();
}

void CWkspSaveDialog::LBnSelChangeFiles( void )
{
	int selIndex = fileList.GetCurSel();
	CString fullPath;

	if ( selIndex != -1 )
		fullPath = metaFolder.getFile( selIndex )->m_pathInfo.Get();

	SetDlgItemText( IDC_FULLPATH_EDIT, fullPath );
}
