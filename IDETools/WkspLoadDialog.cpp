
#include "stdafx.h"
#include "WkspLoadDialog.h"
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
		{ IDCANCEL, MoveX },
		{ IDC_BEFORE_OPEN_STATIC, Move },
		{ IDC_CLOSE_ALL_BEFORE_OPEN_CHECK, Move }
	};
}


CWkspLoadDialog::CWkspLoadDialog( WorkspaceProfile& _wkspProfile, const CString& _section, const CString& _currProjectName, CWnd* parent )
	: CLayoutDialog( IDD_WORKSPACE_LOAD_DIALOG, parent )
	, section( _section )
	, currProjectName( _currProjectName )
	, wkspProfile( _wkspProfile )
	, options( NULL )
	, metaFolder( options, _T(""), _T("") )
	, showFullPath( false )
	, m_fullPathEdit( ui::FilePath )
{
	m_regSection = ::sectionWorkspaceDialogs;
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );
	LoadDlgIcon( IDD_WORKSPACE_LOAD_DIALOG );
	m_accelPool.AddAccelTable( new CAccelTable( IDR_WKSPLOAD_ACCEL ) );

	options.m_sortFolders = false;
	options.m_fileSortOrder.Clear();

	if ( section.IsEmpty() )
		section = ::defaulWkspSection;

	CWinApp* pApp = AfxGetApp();
	showFullPath = pApp->GetProfileInt( m_regSection.c_str(), ENTRY_OF( showFullPath ), showFullPath ) != FALSE;
	wkspProfile.m_mustCloseAll = pApp->GetProfileInt( m_regSection.c_str(), ENTRY_OF( mustCloseAll ), wkspProfile.m_mustCloseAll );
}

CWkspLoadDialog::~CWkspLoadDialog()
{
}

void CWkspLoadDialog::setupWindow( void )
{
	ASSERT( IsWindow( m_hWnd ) );

	loadExistingProjects();

	for ( unsigned int i = 0; i != wkspProfile.projectNameArray.size(); ++i )
		projectNameCombo.AddString( wkspProfile.projectNameArray[ i ] );

	if ( projectNameCombo.SelectString( -1, currProjectName ) == -1 )
		projectNameCombo.SetCurSel( 0 );

	updateFileContents();
}

void CWkspLoadDialog::cleanupWindow( void )
{
	CWinApp* pApp = AfxGetApp();

	pApp->WriteProfileInt( m_regSection.c_str(), ENTRY_OF( showFullPath ), showFullPath );
	pApp->WriteProfileInt( m_regSection.c_str(), ENTRY_OF( mustCloseAll ), wkspProfile.m_mustCloseAll );
}

bool CWkspLoadDialog::loadExistingProjects( void )
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

void CWkspLoadDialog::updateFileContents( void )
{
	int i;

	metaFolder.Clear();
	if ( readProjectName() )
	{
		reg::CKey keyWorkspaces( AfxGetApp()->GetSectionKey( section ) );
		if ( keyWorkspaces.IsValid() && keyWorkspaces.HasSubKey( currProjectName ) )
		{
			reg::CKey keyCurrProject = keyWorkspaces.OpenSubKey( currProjectName, false );
			reg::CKey::CInfo keyInfo( keyCurrProject.Get() );
			reg::CValue value;
			TCHAR buffer[ MAX_PATH ];

			value.AttachBuffer( (BYTE*)buffer, sizeof( buffer ) );
			for ( i = 0; i < (int)keyInfo.m_valueCount; ++i )
				if ( keyCurrProject.GetValue( value, wkspProfile.getFileEntryName( i ) ) )
					metaFolder.addFile( value.GetString().c_str() );
		}
	}

	fileList.SetRedraw( FALSE );
	fileList.ResetContent();
	for ( i = 0; i < metaFolder.getFileCount(); ++i )
	{
		CMetaFolder::CFile* file = metaFolder.getFile( i );

		fileList.AddString( showFullPath ? file->m_pathInfo.Get() : file->getLabel() );
		fileList.SetItemDataPtr( i, file );
	}
	fileList.SetSel( -1, TRUE );
	fileList.SetRedraw( TRUE );
	fileList.Invalidate();
	fileList.SetCaretIndex( 0 );
	LBnSelChangeFiles();
}

bool CWkspLoadDialog::transferFiles( void )
{
	if ( !readProjectName() )
		return false;

	int selCount = fileList.GetSelCount();

	wkspProfile.fileArray.clear();
	if ( selCount > 0 )
	{
		int* selection = new int[ selCount ];

		VERIFY( fileList.GetSelItems( selCount, selection ) != LB_ERR );
		for ( int i = 0; i < selCount; ++i )
			wkspProfile.fileArray.push_back( getListFile( selection[ i ] )->m_pathInfo.Get() );

		delete selection;
	}
	return true;
}

bool CWkspLoadDialog::readProjectName( void )
{
	int selIndex = projectNameCombo.GetCurSel();

	currProjectName.Empty();

	if ( selIndex != CB_ERR )
		projectNameCombo.GetLBText( selIndex, currProjectName );

	if ( !currProjectName.IsEmpty() )
		return true;

	return false;
}

void CWkspLoadDialog::handleSelection( Ternary operation )
{
	if ( operation != Toggle )
		fileList.SetSel( -1, operation );
	else
	{	// Toggle
		for ( int i = 0, count = fileList.GetCount(); i < count; ++i )
			fileList.SetSel( i, !fileList.GetSel( i ) );
	}
}

CMetaFolder::CFile* CWkspLoadDialog::getListFile( int listIndex ) const
{
	return (CMetaFolder::CFile*)fileList.GetItemDataPtr( listIndex );
}

void CWkspLoadDialog::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_PROJECT_NAME_COMBO, projectNameCombo );
	DDX_Control( pDX, IDC_FILES_LIST, fileList );
	DDX_Control( pDX, IDC_FULLPATH_EDIT, m_fullPathEdit );

	CLayoutDialog::DoDataExchange( pDX );
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
	CLayoutDialog::OnInitDialog();

	CheckDlgButton( IDC_SHOW_FULL_PATH_CHECK, showFullPath );
	CheckDlgButton( IDC_CLOSE_ALL_BEFORE_OPEN_CHECK, wkspProfile.m_mustCloseAll );

	setupWindow();
	return TRUE;
}

void CWkspLoadDialog::OnDestroy( void )
{
	cleanupWindow();
	CLayoutDialog::OnDestroy();
}

void CWkspLoadDialog::OnOK( void )
{
	transferFiles();
	CLayoutDialog::OnOK();
}

void CWkspLoadDialog::OnContextMenu( CWnd* pWnd, CPoint point )
{
	if ( pWnd == &fileList )
	{
		CMenu contextMenu;
		ui::LoadPopupMenu( contextMenu, IDR_CONTEXT_MENU, app_popup::ProfileListContext );

		if ( point.x == -1 || point.y == -1 )
			::GetCursorPos( &point );

		contextMenu.TrackPopupMenu( TPM_RIGHTBUTTON, point.x, point.y, this );
	}
}

void CWkspLoadDialog::CkShowFullPath( void )
{
	showFullPath = IsDlgButtonChecked( IDC_SHOW_FULL_PATH_CHECK ) != FALSE;
	fileList.SetCurSel( -1 );
	updateFileContents();
}

void CWkspLoadDialog::CkCloseAllBeforeOpen( void )
{
	wkspProfile.m_mustCloseAll = IsDlgButtonChecked( IDC_CLOSE_ALL_BEFORE_OPEN_CHECK );
}

void CWkspLoadDialog::CBnSelChangeProjectName( void )
{
	updateFileContents();
}

void CWkspLoadDialog::LBnSelChangeFiles( void )
{
	int caretIndex = fileList.GetCaretIndex();
	CString fullPath;

	if ( caretIndex != LB_ERR && caretIndex < fileList.GetCount() )
		fullPath = metaFolder.getFile( caretIndex )->m_pathInfo.Get();

	SetDlgItemText( IDC_FULLPATH_EDIT, fullPath );
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
