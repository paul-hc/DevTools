
#include "pch.h"
#include "FileLocatorDialog.h"
#include "IncludeNode.h"
#include "IncludeOptions.h"
#include "SearchPathEngine.h"
#include "ModuleSession.h"
#include "Application.h"
#include "resource.h"
#include "utl/EnumTags.h"
#include "utl/FileSystem.h"
#include "utl/TextClipboard.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/ShellUtilities.h"
#include "utl/UI/WndUtilsEx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/TandemControls.hxx"


namespace reg
{
	static const TCHAR section_dialog[] = _T("FileLocatorDialog");
	static const TCHAR section_list[] = _T("FileLocatorDialog\\list");
}

static struct { UINT m_ckID; inc::SearchFlag m_searchFlag; } s_idToSPFlags[] =
{
	{ IDC_SEARCH_INCLUDE_PATH_CHECK, inc::Flag_StandardPath },
	{ IDC_SEARCH_LOCAL_PATH_CHECK, inc::Flag_LocalPath },
	{ IDC_SEARCH_ADDITIONAL_PATH_CHECK, inc::Flag_AdditionalPath },
	{ IDC_SEARCH_SOURCE_PATH_CHECK, inc::Flag_SourcePath },
	{ IDC_SEARCH_LIBRARY_PATH_CHECK, inc::Flag_LibraryPath },
	{ IDC_SEARCH_BINARY_PATH_CHECK, inc::Flag_BinaryPath }
};


namespace layout
{
	static const CLayoutStyle styles[] =
	{
		{ IDC_INPUT_INCLUDE_TAG_COMBO, SizeX },
		{ IDC_SYSTEM_TAG_RADIO, MoveX },
		{ IDC_LOCAL_TAG_RADIO, MoveX },
		{ IDC_SEARCH_IN_STATIC, MoveX | DoRepaint },
		{ IDC_SEARCH_INCLUDE_PATH_CHECK, MoveX },
		{ IDC_SEARCH_LOCAL_PATH_CHECK, MoveX },
		{ IDC_SEARCH_ADDITIONAL_PATH_CHECK, MoveX },
		{ IDC_SEARCH_SOURCE_PATH_CHECK, MoveX },
		{ IDC_SEARCH_LIBRARY_PATH_CHECK, MoveX },
		{ IDC_SEARCH_BINARY_PATH_CHECK, MoveX },
		{ IDC_LOCAL_DIR_EDIT, SizeX },
		{ IDC_PROJECT_FILE_EDIT, SizeX },
		{ IDC_ADDITIONAL_INC_PATH_EDIT, SizeX },
		{ IDC_FOUND_FILES_LISTCTRL, SizeX | SizeY },
		{ IDOK, Move },
		{ CM_TEXT_VIEW_FILE, Move },
		{ CM_EXPLORE_FILE, Move },
		{ ID_FILE_SAVE, Move },
		{ IDCANCEL, Move }
	};
}


// CFileLocatorDialog implementation

CFileLocatorDialog::CFileLocatorDialog( CWnd* pParent )
	: CLayoutDialog( IDD_FILE_LOCATOR_DIALOG, pParent )
	, m_tagHistoryMaxCount( 15 )
	, m_searchFlags( inc::Mask_AllIncludePaths )
	, m_intrinsic( 0 )
	, m_closedOK( false )
	, m_defaultExt( _T(".h") )
	, m_accelListFocus( IDR_LOCATE_FILE_LISTFOCUS_ACCEL )
	, m_foundFilesListCtrl( IDC_FOUND_FILES_LISTCTRL )
{
	ASSERT_PTR( pParent );

	// init base
	m_regSection = reg::section_dialog;
	RegisterCtrlLayout( ARRAY_PAIR( layout::styles ) );
	LoadDlgIcon( IDD_FILE_LOCATOR_DIALOG );
	m_accelPool.AddAccelTable( new CAccelTable( IDR_LOCATE_FILE_ACCEL ) );

	m_localDirPathEdit.SetContentType( ui::DirPath );
	m_projectFileEdit.SetFileFilter( _T("C++ Project Files (*.dsp)|*.dsp|")
									 _T("All Files (*.*)|*.*||") );

	m_foundFilesListCtrl.SetSection( reg::section_list );
	m_foundFilesListCtrl.SetUseAlternateRowColoring();
	m_foundFilesListCtrl.StoreImageLists( &ft::GetFileTypeImageList() );
	m_foundFilesListCtrl.SetPopupMenu( CReportListControl::OnSelection, NULL );
}

CFileLocatorDialog::~CFileLocatorDialog()
{
}

std::tstring CFileLocatorDialog::GetAdditionalIncludePaths( void )
{
	return ( CIncludeOptions::Instance().m_additionalIncludePath + app::GetModuleSession().m_moreAdditionalIncludePath ).Join();
}

void CFileLocatorDialog::readProfile( void )
{
	CWinApp* pApp = AfxGetApp();
	BOOL isLocalTag = pApp->GetProfileInt( m_regSection.c_str(), ENTRY_OF( isLocalTag ), TRUE ) != FALSE;
	CheckRadioButton( IDC_SYSTEM_TAG_RADIO, IDC_LOCAL_TAG_RADIO, IDC_SYSTEM_TAG_RADIO + isLocalTag );

	std::vector< std::tstring > tagHistoryArray;
	str::Split( tagHistoryArray, (LPCTSTR)pApp->GetProfileString( m_regSection.c_str(), ENTRY_OF( tagHistory ), _T("") ), _T(";") );
	ui::WriteComboItems( m_includeTagCombo, tagHistoryArray );

	m_searchFlags = pApp->GetProfileInt( m_regSection.c_str(), ENTRY_OF( m_searchFlags ), m_searchFlags );
	m_defaultExt = pApp->GetProfileString( m_regSection.c_str(), ENTRY_OF( m_defaultExt ), m_defaultExt.c_str() );
}

void CFileLocatorDialog::saveProfile( void )
{
	CWinApp* pApp = AfxGetApp();

	if ( m_closedOK )
	{
		SaveHistory();
		pApp->WriteProfileInt( m_regSection.c_str(), ENTRY_OF( isLocalTag ), !IsDlgButtonChecked( IDC_SYSTEM_TAG_RADIO ) );
		pApp->WriteProfileString( m_regSection.c_str(), ENTRY_OF( m_defaultExt ), m_defaultExt.c_str() );
	}
	pApp->WriteProfileInt( m_regSection.c_str(), ENTRY_OF( m_searchFlags ), m_searchFlags );

	// avoid saving empty location entries since these are used only as initial states for the two input dialogs
	if ( !m_localDirPath.empty() )
		pApp->WriteProfileString( m_regSection.c_str(), ENTRY_OF( localDirPath ), m_localDirPath.c_str() );
	if ( !m_associatedProjectFile.empty() )
		pApp->WriteProfileString( m_regSection.c_str(), ENTRY_OF( associatedProjectFile ), m_associatedProjectFile.c_str() );
}

void CFileLocatorDialog::SaveHistory( void )
{
	std::vector< std::tstring > items;
	ui::ReadComboItems( items, m_includeTagCombo );
	str::RemoveEmptyItems( items );

	AfxGetApp()->WriteProfileString( m_regSection.c_str(), ENTRY_OF( tagHistory ), str::Join( items, _T(";") ).c_str() );
}

void CFileLocatorDialog::UpdateHistory( const std::tstring& selFile, bool doSave /*= true*/ )
{
	CIncludeTag selectedTag( ui::GetComboSelText( m_includeTagCombo ).c_str() );
	fs::CPathParts parts( selFile );

	if ( parts.m_ext.length() <= 4 )
		str::ToLower( parts.m_ext );

	m_defaultExt = parts.m_ext;
	fs::CPath path = parts.MakePath();
	if ( path::IsRelative( path.GetPtr() ) )
	{	// build a new include tag based on the selected file name+ext
		selectedTag.Setup( parts.GetFilename().c_str(), FALSE == IsDlgButtonChecked( IDC_SYSTEM_TAG_RADIO ) );
		SetComboTag( selectedTag.GetTag() );
	}

	saveCurrentTagToHistory();
	if ( doSave )
		SaveHistory();
}

bool CFileLocatorDialog::saveCurrentTagToHistory( void )
{
	std::tstring tagString = ui::GetComboSelText( m_includeTagCombo );
	int selIndex = m_includeTagCombo.GetCurSel();

	if ( CB_ERR == selIndex )
	{
		CIncludeTag tag( CIncludeTag::GetStrippedTag( tagString.c_str() ).c_str() );
		if ( tag.IsEmpty() )
			return false;

		// check for some kind of duplicate (e.g. different case duplicate)
		selIndex = m_includeTagCombo.FindString( -1, tag.GetTag().c_str() );
		if ( selIndex != CB_ERR )
			m_includeTagCombo.DeleteString( selIndex );

		m_includeTagCombo.InsertString( 0, tag.GetTag().c_str() );
		m_includeTagCombo.SetCurSel( 0 );

		// delete extra tags from history
		int historyCount = m_includeTagCombo.GetCount();

		while ( historyCount > m_tagHistoryMaxCount )
			m_includeTagCombo.DeleteString( --historyCount );
	}
	else if ( selIndex > 0 )
	{	// selected item is not on history top -> move it on top
		CIncludeTag tag( tagString.c_str() );
		if ( tag.IsEmpty() )
			return false;

		m_includeTagCombo.DeleteString( selIndex );
		m_includeTagCombo.InsertString( 0, tag.GetTag().c_str() );
		m_includeTagCombo.SetCurSel( 0 );
	}
	return true;
}

int CFileLocatorDialog::SearchForTag( const std::tstring& includeTag )
{
	CIncludeTag tag( includeTag.c_str() );

	if ( tag.IsEven() )
		CheckRadioButton( IDC_SYSTEM_TAG_RADIO, IDC_LOCAL_TAG_RADIO, IDC_SYSTEM_TAG_RADIO + tag.IsLocalInclude() );
	else
	{
		CheckDlgButton( IDC_SYSTEM_TAG_RADIO, FALSE );
		CheckDlgButton( IDC_LOCAL_TAG_RADIO, FALSE );
	}

	// set the appropriate label based on tag file extension
	fs::CPathParts parts( tag.GetFilePath().Get() );
	const TCHAR* pTagStaticText = _T("");

	switch ( ft::FindTypeOfExtension( parts.m_ext.c_str() ) )
	{
		case ft::Extless:			// It may may be a STL header file
		case ft::H:
		case ft::CPP:
		case ft::C:
		case ft::HXX:
		case ft::CXX:
		case ft::RC:
			pTagStaticText = _T("#&include");
			break;
		case ft::IDL:
			pTagStaticText = _T("&import");
			break;
		case ft::TLB:
			pTagStaticText = _T("#&import");
			break;
		case ft::Unknown:
			if ( path::EqualsPtr( parts.m_ext.c_str(), _T(".dll") ) )
				pTagStaticText = _T("#&import");
			break;
	}
	ui::SetDlgItemText( this, IDC_TAG_STATIC, pTagStaticText );

	CWaitCursor searchInProgress;

	m_foundFiles.clear();
	if ( !tag.IsEmpty() )
	{
		inc::CFoundPaths foundResults;
		inc::CSearchPathEngine searchEngine( m_localDirPath, m_searchFlags );
		searchEngine.QueryIncludeFiles( foundResults, tag );
		foundResults.Swap( m_foundFiles );		// store the found files
	}

	{
		CScopedLockRedraw freeze( &m_foundFilesListCtrl );
		CScopedInternalChange internalChange( &m_foundFilesListCtrl );

		m_foundFilesListCtrl.DeleteAllItems();

		for ( int i = 0; i < (int)m_foundFiles.size(); ++i )
		{
			parts.SplitPath( m_foundFiles[ i ].first.Get() );

			m_foundFilesListCtrl.InsertItem( i, parts.GetFilename().c_str(), ft::FindTypeOfExtension( parts.m_ext.c_str() ) );
			m_foundFilesListCtrl.SetSubItemText( i, Directory, parts.GetDirPath().GetPtr() );
			m_foundFilesListCtrl.SetSubItemText( i, Location, inc::GetTags_Location().FormatUi( m_foundFiles[ i ].second ) );
		}
	}

	if ( !m_foundFiles.empty() )
		m_foundFilesListCtrl.SetCurSel( 0 );		// caret and selection

	EnableCommandButtons();

	ui::SetDlgItemText( this, IDC_FOUND_FILES_STATIC, str::Format( m_foundFilesFormat.c_str(), m_foundFiles.size() ) );
	return 0;
}

bool CFileLocatorDialog::EnableCommandButtons( void )
{
	std::vector< int > selFiles;
	bool anySelected;

	getSelectedFoundFiles( selFiles );
	anySelected = selFiles.size() > 0;

	static const UINT ctrlIds[] = { IDOK, CM_TEXT_VIEW_FILE, ID_FILE_SAVE };
	ui::EnableControls( m_hWnd, ctrlIds, COUNT_OF( ctrlIds ), anySelected );
	ui::EnableControl( m_hWnd, CM_EXPLORE_FILE, 1 == selFiles.size() );
	return anySelected;
}

void CFileLocatorDialog::SetComboTag( const std::tstring& includeTag )
{
	int selIndex = m_includeTagCombo.FindString( -1, includeTag.c_str() );

	if ( selIndex != CB_ERR )
		m_includeTagCombo.SetCurSel( selIndex );
	else
	{
		if ( m_includeTagCombo.GetCurSel() != CB_ERR )
			m_includeTagCombo.SetCurSel( CB_ERR );			// clear the selection
		ui::SetWindowText( m_includeTagCombo, includeTag );
	}
}

void CFileLocatorDialog::ModifyTagType( bool localInclude )
{
	std::tstring text = ui::GetComboSelText( m_includeTagCombo );
	str::Trim( text, _T("<>\"") );
	CIncludeTag tag( text, localInclude );

	SetComboTag( tag.GetTag() );
	GotoDlgCtrl( &m_includeTagCombo );
	m_includeTagCombo.SetEditSel( 1, 1 );
	SearchForTag( ui::GetComboSelText( m_includeTagCombo ) );
}

bool CFileLocatorDialog::isValidLocalDirPath( bool allowEmpty /*= true*/ )
{
	if ( m_localDirPath.empty() )
		return allowEmpty;
	return fs::IsValidDirectory( m_localDirPath.c_str() );
}

int CFileLocatorDialog::getCurrentFoundFile( void ) const
{
	int selIndex = m_foundFilesListCtrl.GetNextItem( -1, LVNI_ALL | LVNI_SELECTED | LVNI_FOCUSED );			// sel & caret prefered

	if ( selIndex == -1 )
		selIndex = m_foundFilesListCtrl.GetNextItem( -1, LVNI_ALL | LVNI_SELECTED );
	return selIndex;
}

// Returns the index of the first selected item (if any), that is usually the caret item.
int CFileLocatorDialog::getSelectedFoundFiles( std::vector< int >& selFiles )
{
	POSITION pos = m_foundFilesListCtrl.GetFirstSelectedItemPosition();
	int selIndex = -1;
	std::vector< inc::TPathLocPair >::const_iterator itFoundBase = m_foundFiles.begin();

	while ( pos != NULL )
	{
		selIndex = m_foundFilesListCtrl.GetNextSelectedItem( pos );
		selFiles.push_back( selIndex );
	}
	// No selection at all, try to add the caret item
	if ( selIndex == -1 && ( selIndex = getCurrentFoundFile() ) != -1 )
		selFiles.push_back( selIndex );
	return selIndex;
}

std::tstring CFileLocatorDialog::getSelectedFoundFilesFlat( const std::vector< int >& selFiles, const TCHAR* sep /*= _T(";")*/ ) const
{
	std::tstring selFilesFlat;

	for ( size_t i = 0; i != selFiles.size(); ++i )
	{
		if ( i != 0 )
			selFilesFlat += sep;
		selFilesFlat += m_foundFiles[ selFiles[ i ] ].first.Get();
	}
	return selFilesFlat;
}

int CFileLocatorDialog::storeSelection( void )
{
	std::vector< int > selFiles;

	getSelectedFoundFiles( selFiles );
	m_selectedFiles.clear();
	for ( size_t i = 0; i != selFiles.size(); ++i )
		m_selectedFiles.push_back( m_foundFiles[ selFiles[ i ] ] );
	m_selectedFilePath = getSelectedFoundFilesFlat( selFiles );
	return (int)m_selectedFiles.size();
}

void CFileLocatorDialog::OnLocalDirPathChanged( void )
{
	CProjectContext::OnLocalDirPathChanged();

	if ( 0 == m_intrinsic && m_hWnd != NULL )
		ui::SetWindowText( m_localDirPathEdit, m_localDirPath );
}

void CFileLocatorDialog::OnAssociatedProjectFileChanged( void )
{
	CProjectContext::OnAssociatedProjectFileChanged();
	if ( 0 == m_intrinsic && m_hWnd != NULL )
		ui::SetWindowText( m_projectFileEdit, m_associatedProjectFile );
}

void CFileLocatorDialog::OnProjectActiveConfigurationChanged( void )
{
	CProjectContext::OnProjectActiveConfigurationChanged();
}

void CFileLocatorDialog::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_INPUT_INCLUDE_TAG_COMBO, m_includeTagCombo );
	DDX_Control( pDX, IDC_LOCAL_DIR_EDIT, m_localDirPathEdit );
	DDX_Control( pDX, IDC_PROJECT_FILE_EDIT, m_projectFileEdit );
	DDX_Control( pDX, IDC_FOUND_FILES_LISTCTRL, m_foundFilesListCtrl );
	ui::DDX_ButtonIcon( pDX, IDOK, ID_FILE_OPEN );
	ui::DDX_ButtonIcon( pDX, ID_FILE_SAVE );

	CLayoutDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CFileLocatorDialog, CLayoutDialog )
	ON_CBN_EDITCHANGE( IDC_INPUT_INCLUDE_TAG_COMBO, CBnEditChangeInputIncludeTag )
	ON_CBN_SELCHANGE( IDC_INPUT_INCLUDE_TAG_COMBO, CBnSelChangeInputIncludeTag )
	ON_BN_CLICKED( IDC_SYSTEM_TAG_RADIO, CkSystemTagRadio )
	ON_BN_CLICKED( IDC_LOCAL_TAG_RADIO, CkLocalTagRadio )
	ON_CN_EDITDETAILS( IDC_LOCAL_DIR_EDIT, OnEnEditItems_LocalPath )
	ON_CN_EDITDETAILS( IDC_PROJECT_FILE_EDIT, OnEnEditItems_ProjectFile )
	ON_WM_CTLCOLOR()
	ON_NOTIFY( NM_DBLCLK, IDC_FOUND_FILES_LISTCTRL, LVnDblclkFoundFiles )
	ON_NOTIFY( LVN_BEGINLABELEDIT, IDC_FOUND_FILES_LISTCTRL, LVnBeginLabelEditFoundFiles )
	ON_NOTIFY( LVN_ITEMCHANGED, IDC_FOUND_FILES_LISTCTRL, LVnItemChangedFoundFiles )
	ON_WM_DESTROY()
	ON_WM_CONTEXTMENU()
	ON_COMMAND( ID_FILE_OPEN, OnOK )
	ON_COMMAND( CM_EXPLORE_FILE, CmExploreFile )
	ON_COMMAND( ID_FILE_SAVE, OnStoreFullPath )
	ON_COMMAND_RANGE( CM_TEXT_VIEW_FILE, CM_VIEW_FILE, CmViewFile )
	ON_COMMAND_RANGE( IDC_SEARCH_INCLUDE_PATH_CHECK, IDC_SEARCH_BINARY_PATH_CHECK, CkSearchPath )
END_MESSAGE_MAP()

BOOL CFileLocatorDialog::PreTranslateMessage( MSG* pMsg )
{
	return
		m_accelListFocus.Translate( pMsg, m_hWnd, m_foundFilesListCtrl ) ||
		CLayoutDialog::PreTranslateMessage( pMsg );
}

BOOL CFileLocatorDialog::OnInitDialog( void )
{
	CLayoutDialog::OnInitDialog();

	++m_intrinsic;

	// setup
	readProfile();
	for ( size_t i = 0; i != COUNT_OF( s_idToSPFlags ); ++i )
		CheckDlgButton( s_idToSPFlags[ i ].m_ckID, HasFlag( m_searchFlags, s_idToSPFlags[ i ].m_searchFlag ) );

	m_foundFilesFormat = ui::GetDlgItemText( this, IDC_FOUND_FILES_STATIC );

	ui::SetWindowText( m_localDirPathEdit, m_localDirPath );
	ui::SetDlgItemText( this, IDC_ADDITIONAL_INC_PATH_EDIT, GetAdditionalIncludePaths() );
	ui::SetWindowText( m_projectFileEdit, m_associatedProjectFile );

	CIncludeTag intialTag( m_defaultExt, !IsDlgButtonChecked( IDC_SYSTEM_TAG_RADIO ) );

	SetComboTag( intialTag.GetTag() );
	m_includeTagCombo.SetEditSel( 1, 1 );
	GotoDlgCtrl( &m_includeTagCombo );
	m_includeTagCombo.SetEditSel( 1, 1 );

	SearchForTag( ui::GetComboSelText( m_includeTagCombo ) );

	--m_intrinsic;

	return FALSE;
}

void CFileLocatorDialog::OnDestroy( void )
{
	saveProfile();
	CLayoutDialog::OnDestroy();
}

void CFileLocatorDialog::OnOK( void )
{
	if ( storeSelection() > 0 )
	{
		m_closedOK = true;
		UpdateHistory( m_selectedFiles[ 0 ].first.Get() );
		CLayoutDialog::OnOK();
	}
}

void CFileLocatorDialog::OnEnEditItems_LocalPath( void )
{
	SetLocalDirPath( ui::GetWindowText( m_localDirPathEdit ) );
}

void CFileLocatorDialog::OnEnEditItems_ProjectFile( void )
{
	SetAssociatedProjectFile( ui::GetWindowText( m_projectFileEdit ) );
}

void CFileLocatorDialog::CBnEditChangeInputIncludeTag( void )
{
	std::tstring includeTag = ui::GetDlgItemText( this, IDC_INPUT_INCLUDE_TAG_COMBO );
	SearchForTag( includeTag );
}

void CFileLocatorDialog::CBnSelChangeInputIncludeTag( void )
{
	int selIndex = m_includeTagCombo.GetCurSel();
	if ( selIndex != CB_ERR )
		SearchForTag( ui::GetComboItemText( m_includeTagCombo, selIndex ) );
}

void CFileLocatorDialog::CkSystemTagRadio( void )
{
	ModifyTagType( false );
}

void CFileLocatorDialog::CkLocalTagRadio( void )
{
	ModifyTagType( true );
}

HBRUSH CFileLocatorDialog::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor )
{
	HBRUSH hBrush = CLayoutDialog::OnCtlColor( pDC, pWnd, nCtlColor );

	if ( nCtlColor == CTLCOLOR_STATIC && pWnd != NULL )
		switch ( pWnd->GetDlgCtrlID() )
		{
			case IDC_LOCAL_DIR_EDIT:
			case IDC_PROJECT_FILE_EDIT:
			case IDC_ADDITIONAL_INC_PATH_EDIT:
				pDC->SetTextColor( RGB( 0, 0, 0x80 ) );
				break;
		}
	return hBrush;
}

void CFileLocatorDialog::LVnDblclkFoundFiles( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMITEMACTIVATE* pNmItemActivate = (NMITEMACTIVATE*)pNmHdr;

	if ( 0 == pNmItemActivate->iSubItem && getCurrentFoundFile() != -1 )
		SendMessage( WM_COMMAND, IDOK );
	*pResult = 0;
}

void CFileLocatorDialog::LVnBeginLabelEditFoundFiles( NMHDR* pNmHdr, LRESULT* pResult )
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNmHdr;
	pDispInfo;

	if ( CEdit* pLabelEdit = m_foundFilesListCtrl.GetEditControl() )
		pLabelEdit->SetReadOnly( TRUE );
	*pResult = 0;
}

void CFileLocatorDialog::LVnItemChangedFoundFiles( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLISTVIEW* pNmListView = (NMLISTVIEW*)pNmHdr;
	UINT selMask = LVIS_SELECTED | LVIS_FOCUSED;

	if ( 0 == m_intrinsic )
		if ( ( pNmListView->uNewState & selMask ) != ( pNmListView->uOldState & selMask ) )
			EnableCommandButtons();
	*pResult = 0;
}

void CFileLocatorDialog::CkSearchPath( UINT ckID )
{
	for ( size_t i = 0; i != COUNT_OF( s_idToSPFlags ); ++i )
		if ( ckID == s_idToSPFlags[ i ].m_ckID )
		{
			SetFlag( m_searchFlags, s_idToSPFlags[ i ].m_searchFlag, IsDlgButtonChecked( ckID ) != FALSE );
			break;
		}

	SearchForTag( ui::GetComboSelText( m_includeTagCombo ) );
}

void CFileLocatorDialog::OnContextMenu( CWnd* pWnd, CPoint screenPos )
{
	if ( pWnd == &m_foundFilesListCtrl )
	{
		std::vector< int > selFiles;
		getSelectedFoundFiles( selFiles );
		if ( !selFiles.empty() )
		{
			CMenu contextMenu;
			ui::LoadPopupMenu( contextMenu, IDR_CONTEXT_MENU, app::FoundListContextPopup );
			contextMenu.SetDefaultItem( ID_FILE_OPEN );
			contextMenu.EnableMenuItem( CM_EXPLORE_FILE, MF_BYCOMMAND | ( 1 == selFiles.size() ? MF_ENABLED : MF_GRAYED ) );

			ui::TrackPopupMenu( contextMenu, this, screenPos );
		}
	}
}

void CFileLocatorDialog::CmExploreFile( void )
{
	int selIndex = getCurrentFoundFile();

	if ( selIndex != -1 )
	{
		UpdateHistory( m_foundFiles[ selIndex ].first.Get() );
		shell::ExploreAndSelectFile( m_foundFiles[ selIndex ].first.GetPtr() );
	}
}

void CFileLocatorDialog::CmViewFile( UINT cmdId )
{
	std::vector< int > selFiles;

	getSelectedFoundFiles( selFiles );
	for ( size_t i = 0; i != selFiles.size(); ++i )
	{
		std::tstring fileFullPath = m_foundFiles[ selFiles[ i ] ].first.Get();

		if ( 0 == i )
			UpdateHistory( fileFullPath );

		// Use text key (.txt) for text view, or the default for run
		LPCTSTR useExtType = cmdId == CM_TEXT_VIEW_FILE ? _T(".txt") : NULL;

		shell::Execute( this, fileFullPath.c_str(), NULL, SEE_MASK_FLAG_DDEWAIT, NULL, NULL, useExtType );
	}
}

void CFileLocatorDialog::OnStoreFullPath( void )
{
	std::vector< int > selFiles;

	getSelectedFoundFiles( selFiles );
	if ( selFiles.size() > 0 )
	{
		static const TCHAR sep[] = _T("\r\n");
		std::tstring selFilesFlat = getSelectedFoundFilesFlat( selFiles, sep );

		if ( selFiles.size() > 1 )
			selFilesFlat += sep;
		UpdateHistory( m_foundFiles[ selFiles[ 0 ] ].first.Get() );

		CTextClipboard::CopyText( selFilesFlat, AfxGetMainWnd()->GetSafeHwnd() );
	}
}
