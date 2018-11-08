
#include "stdafx.h"
#include "ChildFormView.h"
#include "FileItemInfo.h"
#include "utl/ShellContextMenu.h"
#include "utl/ContainerUtilities.h"
#include "utl/Path.h"
#include "utl/FileSystem.h"
#include "utl/MenuUtilities.h"
#include "utl/Utilities.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section_settings[] = _T("ChildFormView");
	static const TCHAR section_fileList[] = _T("ChildFormView\\FileList");
	static const TCHAR entry_listViewMode[] = _T("ListViewMode");
	static const TCHAR entry_useCustomMenu[] = _T("UseCustomMenu");
	static const TCHAR entry_currDirPath[] = _T("CurrDirPath");
}


namespace layout
{
	static CLayoutStyle s_styles[] =
	{
		{ IDC_FILE_LIST, Size },
	};
}


IMPLEMENT_DYNCREATE( CChildFormView, CFormView )

const TCHAR CChildFormView::s_rootPath[] = _T("C:\\");

CChildFormView::CChildFormView( void )
	: CLayoutFormView( IDD_CHILD_VIEW_FORM )
	, m_listViewMode( static_cast< ListViewMode >( AfxGetApp()->GetProfileInt( reg::section_settings, reg::entry_listViewMode, ReportView ) ) )
	, m_useCustomMenu( AfxGetApp()->GetProfileInt( reg::section_settings, reg::entry_useCustomMenu, false ) != FALSE )
	, m_currDirPath( AfxGetApp()->GetProfileString( reg::section_settings, reg::entry_currDirPath, s_rootPath ).GetString() )
	, m_fileListCtrl( IDC_FILE_LIST )
{
	RegisterCtrlLayout( layout::s_styles, COUNT_OF( layout::s_styles ) );
	m_fileListCtrl.SetSection( reg::section_fileList );

	if ( !fs::IsValidDirectory( m_currDirPath.c_str() ) )
		m_currDirPath = s_rootPath;
}

CChildFormView::~CChildFormView()
{
	AfxGetApp()->WriteProfileInt( reg::section_settings, reg::entry_listViewMode, m_listViewMode );
	AfxGetApp()->WriteProfileInt( reg::section_settings, reg::entry_useCustomMenu, m_useCustomMenu );
	AfxGetApp()->WriteProfileString( reg::section_settings, reg::entry_currDirPath, m_currDirPath.c_str() );

	utl::ClearOwningContainer( m_items );
}

void CChildFormView::SetListViewMode( ListViewMode listViewMode )
{
	int viewMode = LV_VIEW_DETAILS;

	m_listViewMode = listViewMode;
	switch ( m_listViewMode )
	{
		case LargeIconView:	ModifyStyle( LVS_TYPEMASK, LVS_ICON ); viewMode = LV_VIEW_ICON; break;
		case SmallIconView:	ModifyStyle( LVS_TYPEMASK, LVS_SMALLICON ); viewMode = LV_VIEW_SMALLICON; break;
		case ListView:		ModifyStyle( LVS_TYPEMASK, LVS_LIST ); viewMode = LV_VIEW_LIST; break;
		default:
			ASSERT( false );
		case ReportView:	ModifyStyle( LVS_TYPEMASK, LVS_REPORT ); viewMode = LV_VIEW_DETAILS; break;
	}

	m_fileListCtrl.SetView( viewMode );
}

void CChildFormView::ReadDirectory( const std::tstring& dirPath )
{
	m_currDirPath = dirPath;

	utl::ClearOwningContainer( m_items );
	QueryDirectoryItems( m_items, m_currDirPath );

	m_fileListCtrl.SetItemCountEx( 0, LVSICF_NOINVALIDATEALL );
	m_fileListCtrl.SetItemCountEx( static_cast< int >( m_items.size() ), LVSICF_NOINVALIDATEALL );
}

void CChildFormView::QueryDirectoryItems( std::vector< CFileItemInfo* >& rItems, const std::tstring& dirPath )
{
	if ( CFileItemInfo* pParentDirItem = CFileItemInfo::MakeParentDirItem( dirPath ) )		// not a root directory?
		rItems.push_back( pParentDirItem );

	std::tstring dirSpecAll = path::Combine( dirPath.c_str(), _T("*.*") );

	// add sub-directory items first, then append file items
	std::vector< CFileItemInfo* > fileItems;

	CFileFind finder;
	for ( BOOL found = finder.FindFile( dirSpecAll.c_str() ); found;  )
	{
		found = finder.FindNextFile();
		std::tstring filePath = finder.GetFilePath().GetString(); filePath;		// for debugging

		if ( finder.IsDirectory() )
		{
			if ( !finder.IsDots() )						// skip "." and ".." dir entries
				rItems.push_back( CFileItemInfo::MakeItem( finder ) );
		}
		else
			fileItems.push_back( CFileItemInfo::MakeItem( finder ) );
	}

	rItems.insert( rItems.end(), fileItems.begin(), fileItems.end() );			// add file items after sub-directories
}

void CChildFormView::QuerySelectedFilePaths( std::vector< std::tstring >& rSelFilePaths ) const
{
	for ( POSITION pos = m_fileListCtrl.GetFirstSelectedItemPosition(); pos != NULL; )
	{
		int itemIndex = m_fileListCtrl.GetNextSelectedItem( pos );
		rSelFilePaths.push_back( m_items[ itemIndex ]->m_fullPath );
	}
}

int CChildFormView::TrackContextMenu( const std::vector< std::tstring >& selFilePaths, const CPoint& screenPos )
{
	shell::CContextMenu contextMenu( m_hWnd );
	contextMenu.SetFilePaths( selFilePaths );

	if ( m_useCustomMenu )
	{
		enum MenuBar { FilePopup, ViewPopup, AboutPopup };
		CMenu* pViewPopup = GetParent()->GetMenu()->GetSubMenu( ViewPopup );		// 'View' popup from the menu-bar
		CMenu customPopup;

		customPopup.CreatePopupMenu();
		ui::CopyMenuItems( customPopup, 0, *pViewPopup );							// copy View popup items so it won't get deleted

		customPopup.CheckMenuRadioItem( IDM_VIEW_LARGEICONS, IDM_VIEW_REPORT, IDM_VIEW_LARGEICONS + m_listViewMode, MF_BYCOMMAND );
		customPopup.CheckMenuItem( IDM_VIEW_CUSTOMMENU, m_useCustomMenu ? MF_CHECKED : MF_UNCHECKED );

		CMenu* pContextMenu = contextMenu.GetPopupMenu();

		pContextMenu->AppendMenu( MF_BYPOSITION | MF_POPUP, (UINT_PTR)customPopup.Detach(), _T("Custom Menu") );
		pContextMenu->AppendMenu( MF_BYPOSITION, MF_SEPARATOR );
	}

	if ( int cmdId = contextMenu.TrackMenu( screenPos ) )
		GetParent()->SendMessage( WM_COMMAND, cmdId, 0 );

	return 0;
}

void CChildFormView::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_fileListCtrl.m_hWnd;

	DDX_Control( pDX, IDC_FILE_LIST, m_fileListCtrl );

	if ( firstInit )
	{
		::SetWindowTheme( m_fileListCtrl, L"Explorer", NULL );		// enable Explorer theme

		m_fileListCtrl.SetImageList( CFileItemInfo::GetSystemImageList( false ), LVSIL_SMALL );
		m_fileListCtrl.SetImageList( CFileItemInfo::GetSystemImageList( true ), LVSIL_NORMAL );

		ReadDirectory( m_currDirPath );
	}

	__super::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CChildFormView, CLayoutFormView )
	ON_NOTIFY( NM_RCLICK, IDC_FILE_LIST, OnRClickFileList )
	ON_NOTIFY( NM_DBLCLK, IDC_FILE_LIST, OnDblclkFileList )
	ON_NOTIFY( LVN_GETDISPINFO, IDC_FILE_LIST, OnGetDispInfoFileList )
	ON_COMMAND_RANGE( IDM_VIEW_LARGEICONS, IDM_VIEW_REPORT, OnViewMode )
	ON_UPDATE_COMMAND_UI_RANGE( IDM_VIEW_LARGEICONS, IDM_VIEW_REPORT, OnUpdateViewMode )
	ON_COMMAND( IDM_VIEW_CUSTOMMENU, OnUseCustomMenu )
	ON_UPDATE_COMMAND_UI( IDM_VIEW_CUSTOMMENU, OnUpdateUseCustomMenu )
END_MESSAGE_MAP()

void CChildFormView::OnRClickFileList( NMHDR* pNMHDR, LRESULT* pResult )
{
	NMITEMACTIVATE* pNmItemActivate = (NMITEMACTIVATE*)pNMHDR;
	*pResult = 0;

	std::vector< std::tstring > selFilePaths;
	QuerySelectedFilePaths( selFilePaths );

	CPoint screenPos( pNmItemActivate->ptAction );
	m_fileListCtrl.ClientToScreen( &screenPos );

	TrackContextMenu( selFilePaths, screenPos );
}

void CChildFormView::OnDblclkFileList( NMHDR* pNMHDR, LRESULT* pResult )
{
	int itemIndex = m_fileListCtrl.GetNextItem( -1, LVNI_FOCUSED );
	if ( itemIndex != -1 )
	{
		const CFileItemInfo* pItem = m_items[ itemIndex ];

		if ( pItem->IsDirectory() )
			ReadDirectory( pItem->m_fullPath );
		else
		{
		}
	}
}

void CChildFormView::OnGetDispInfoFileList( NMHDR* pNMHDR, LRESULT* pResult )
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	LVITEM* pLvItem = &pDispInfo->item;
	*pResult = 0;

	const CFileItemInfo* pItemData = m_items[ pLvItem->iItem ];
	const CFileItemInfo::CDetails& itemDetails = pItemData->GetDetails();

	if ( HasFlag( pLvItem->mask, LVIF_IMAGE ) )
		pLvItem->iImage = itemDetails.m_iconIndex;

	if ( HasFlag( pLvItem->mask, LVIF_TEXT ) )		// test needed
		switch ( pLvItem->iSubItem )
		{
			case Filename:		_tcscpy( pLvItem->pszText, pItemData->m_filename.c_str() ); break;
			case Size:			_tcscpy( pLvItem->pszText, itemDetails.m_fileSizeText.c_str() ); break;
			case Type:			_tcscpy( pLvItem->pszText, itemDetails.m_typeName.c_str() ); break;
			case LastModified:	_tcscpy( pLvItem->pszText, itemDetails.m_modifiedDateText.c_str() ); break;
			case Attributes:	_tcscpy( pLvItem->pszText, itemDetails.m_fileAttributesText.c_str() ); break;
		}
}

void CChildFormView::OnUpdateViewMode( CCmdUI* pCmdUI )
{
	pCmdUI->Enable();

	UINT selCmdId = IDM_VIEW_LARGEICONS + GetViewMode();
	ui::SetRadio( pCmdUI, selCmdId == pCmdUI->m_nID );
}

void CChildFormView::OnViewMode( UINT cmdId )
{
	SetListViewMode( static_cast< ListViewMode >( cmdId - IDM_VIEW_LARGEICONS ) );
}

void CChildFormView::OnUseCustomMenu( void )
{
	SetUseCustomMenu( !m_useCustomMenu );		// toggle
}

void CChildFormView::OnUpdateUseCustomMenu( CCmdUI* pCmdUI )
{
	// TODO: Code für die Befehlsbehandlungsroutine zum Aktualisieren der Benutzeroberfläche hier einfügen
	pCmdUI->Enable();
	pCmdUI->SetCheck( m_useCustomMenu ? MF_CHECKED : MF_UNCHECKED );
}
