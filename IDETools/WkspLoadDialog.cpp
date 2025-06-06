
#include "pch.h"
#include "WkspLoadDialog.h"
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
		{ IDCANCEL, MoveX },
		{ IDC_BEFORE_OPEN_STATIC, Move },
		{ IDC_CLOSE_ALL_BEFORE_OPEN_CHECK, Move }
	};
}


CWkspLoadDialog::CWkspLoadDialog( WorkspaceProfile& rWkspProfile, const CString& section, const CString& currProjectName, CWnd* pParent )
	: CLayoutDialog( IDD_WORKSPACE_LOAD_DIALOG, pParent )
	, m_section( section )
	, m_currProjectName( currProjectName )
	, m_rWkspProfile( rWkspProfile )
	, m_options( nullptr )
	, m_folderItem( _T("") )
	, m_fullPathEdit( ui::FilePath )
{
	m_regSection = WorkspaceProfile::s_sectionWorkspaceDialogs;
	RegisterCtrlLayout( ARRAY_SPAN( layout::styles ) );
	LoadDlgIcon( IDD_WORKSPACE_LOAD_DIALOG );
	m_accelPool.AddAccelTable( new CAccelTable( IDR_WKSPLOAD_ACCEL ) );

	CWinApp* pApp = AfxGetApp();

	m_options.m_displayFullPath = pApp->GetProfileInt( m_regSection.c_str(), ENTRY_MEMBER( m_displayFullPath ), m_options.m_displayFullPath ) != FALSE;
	m_options.m_sortFolders = false;
	m_options.m_fileSortOrder.Clear();

	if ( m_section.IsEmpty() )
		m_section = WorkspaceProfile::s_defaulWkspSection;

	m_rWkspProfile.m_mustCloseAll = pApp->GetProfileInt( m_regSection.c_str(), ENTRY_OF( mustCloseAll ), m_rWkspProfile.m_mustCloseAll );
}

CWkspLoadDialog::~CWkspLoadDialog()
{
}

void CWkspLoadDialog::setupWindow( void )
{
	ASSERT( IsWindow( m_hWnd ) );

	loadExistingProjects();

	for ( unsigned int i = 0; i != m_rWkspProfile.projectNameArray.size(); ++i )
		m_projectNameCombo.AddString( m_rWkspProfile.projectNameArray[ i ] );

	if ( m_projectNameCombo.SelectString( -1, m_currProjectName ) == -1 )
		m_projectNameCombo.SetCurSel( 0 );

	updateFileContents();
}

void CWkspLoadDialog::cleanupWindow( void )
{
	CWinApp* pApp = AfxGetApp();

	pApp->WriteProfileInt( m_regSection.c_str(), ENTRY_MEMBER( m_displayFullPath ), m_options.m_displayFullPath );
	pApp->WriteProfileInt( m_regSection.c_str(), ENTRY_OF( mustCloseAll ), m_rWkspProfile.m_mustCloseAll );
}

bool CWkspLoadDialog::loadExistingProjects( void )
{
	// Enums the already stored projects keys, by ex:
	//	HKEY_CURRENT_USER\Software\RegistryKey\AppName\section\Project1
	reg::CKey projectsKey( AfxGetApp()->GetSectionKey( m_section ) );

	if ( !projectsKey.IsOpen() )
		return false;

	std::vector<std::tstring> projects;
	projectsKey.QuerySubKeyNames( projects );

	for ( std::vector<std::tstring>::const_iterator itProject = projects.begin(); itProject != projects.end(); ++itProject )
		m_rWkspProfile.AddProjectName( itProject->c_str() );

	return !projects.empty();
}

void CWkspLoadDialog::updateFileContents( void )
{
	m_folderItem.Clear();
	if ( readProjectName() )
	{
		reg::CKey workspacesKey( AfxGetApp()->GetSectionKey( m_section ) );

		if ( workspacesKey.IsOpen() )
		{
			reg::CKey projectKey;
			if ( projectKey.Open( workspacesKey.Get(), m_currProjectName.GetString(), KEY_READ ) )
			{
				std::vector<std::tstring> valueNames;
				projectKey.QueryValueNames( valueNames );

				for ( std::vector<std::tstring>::const_iterator itValueName = valueNames.begin(); itValueName != valueNames.end(); ++itValueName )
					m_folderItem.AddFileItem( nullptr, projectKey.ReadStringValue( itValueName->c_str() ) );		// was m_rWkspProfile.getFileEntryName( i )
			}
		}
	}

	m_fileList.SetRedraw( FALSE );
	m_fileList.ResetContent();

	const std::vector<CFileItem*>& fileItems = m_folderItem.GetFileItems();
	for ( unsigned int index = 0; index != fileItems.size(); ++index )
	{
		CFileItem* pFileItem = fileItems[ index ];

		m_fileList.AddString( pFileItem->FormatLabel( &m_options ).c_str() );
		m_fileList.SetItemDataPtr( index, pFileItem );
	}

	m_fileList.SetSel( -1, TRUE );
	m_fileList.SetRedraw( TRUE );
	m_fileList.Invalidate();
	m_fileList.SetCaretIndex( 0 );
	LBnSelChangeFiles();
}

bool CWkspLoadDialog::transferFiles( void )
{
	if ( !readProjectName() )
		return false;

	int selCount = m_fileList.GetSelCount();

	m_rWkspProfile.fileArray.clear();
	if ( selCount != 0 )
	{
		std::vector<int> selection( selCount );

		VERIFY( m_fileList.GetSelItems( selCount, &selection.front() ) != LB_ERR );
		for ( int i = 0; i < selCount; ++i )
			m_rWkspProfile.fileArray.push_back( GetListFileItem( selection[ i ] )->GetFilePath().GetPtr() );
	}
	return true;
}

bool CWkspLoadDialog::readProjectName( void )
{
	int selIndex = m_projectNameCombo.GetCurSel();

	m_currProjectName.Empty();

	if ( selIndex != CB_ERR )
		m_projectNameCombo.GetLBText( selIndex, m_currProjectName );

	if ( !m_currProjectName.IsEmpty() )
		return true;

	return false;
}

void CWkspLoadDialog::handleSelection( TTernary operation )
{
	if ( operation != Toggle )
		m_fileList.SetSel( -1, operation );
	else
	{	// Toggle
		for ( int i = 0, count = m_fileList.GetCount(); i < count; ++i )
			m_fileList.SetSel( i, !m_fileList.GetSel( i ) );
	}
}

void CWkspLoadDialog::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_PROJECT_NAME_COMBO, m_projectNameCombo );
	DDX_Control( pDX, IDC_FILES_LIST, m_fileList );
	DDX_Control( pDX, IDC_FULLPATH_EDIT, m_fullPathEdit );

	__super::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CWkspLoadDialog, CLayoutDialog )
	ON_WM_DESTROY()
	ON_WM_CONTEXTMENU()
	ON_BN_CLICKED( IDC_SHOW_FULL_PATH_CHECK, CkShowFullPath )
	ON_BN_CLICKED( IDC_CLOSE_ALL_BEFORE_OPEN_CHECK, CkCloseAllBeforeOpen )
	ON_CBN_SELCHANGE( IDC_PROJECT_NAME_COMBO, CBnSelChangeProjectName )
	ON_LBN_SELCHANGE( IDC_FILES_LIST, LBnSelChangeFiles )
	ON_COMMAND_RANGE( CM_MULTISEL_SELECT_ALL, CM_MULTISEL_TOGGLE_SEL, CmMultiSelection )
END_MESSAGE_MAP()

BOOL CWkspLoadDialog::OnInitDialog( void )
{
	__super::OnInitDialog();

	CheckDlgButton( IDC_SHOW_FULL_PATH_CHECK, m_options.m_displayFullPath );
	CheckDlgButton( IDC_CLOSE_ALL_BEFORE_OPEN_CHECK, m_rWkspProfile.m_mustCloseAll );

	setupWindow();
	return TRUE;
}

void CWkspLoadDialog::OnDestroy( void )
{
	cleanupWindow();
	__super::OnDestroy();
}

void CWkspLoadDialog::OnOK( void )
{
	transferFiles();
	__super::OnOK();
}

void CWkspLoadDialog::OnContextMenu( CWnd* pWnd, CPoint screenPos )
{
	if ( pWnd == &m_fileList )
		ui::TrackContextMenu( IDR_CONTEXT_MENU, app::ProfileListContextPopup, this, screenPos );
	else
		__super::OnContextMenu( pWnd, screenPos );
}

void CWkspLoadDialog::CkShowFullPath( void )
{
	m_options.m_displayFullPath = IsDlgButtonChecked( IDC_SHOW_FULL_PATH_CHECK ) != FALSE;
	m_fileList.SetCurSel( -1 );
	updateFileContents();
}

void CWkspLoadDialog::CkCloseAllBeforeOpen( void )
{
	m_rWkspProfile.m_mustCloseAll = IsDlgButtonChecked( IDC_CLOSE_ALL_BEFORE_OPEN_CHECK );
}

void CWkspLoadDialog::CBnSelChangeProjectName( void )
{
	updateFileContents();
}

void CWkspLoadDialog::LBnSelChangeFiles( void )
{
	int caretIndex = m_fileList.GetCaretIndex();
	fs::CPath fullPath;

	if ( caretIndex != LB_ERR && caretIndex < m_fileList.GetCount() )
		fullPath = m_folderItem.GetFileItems()[ caretIndex ]->GetFilePath();

	SetDlgItemText( IDC_FULLPATH_EDIT, fullPath.GetPtr() );
}

void CWkspLoadDialog::CmMultiSelection( UINT cmdId )
{
	switch ( cmdId )
	{
		case CM_MULTISEL_SELECT_ALL:
			handleSelection( True );
			break;
		case CM_MULTISEL_CLEAR_ALL:
			handleSelection( False );
			break;
		case CM_MULTISEL_TOGGLE_SEL:
			handleSelection( Toggle );
			break;
	}
}
