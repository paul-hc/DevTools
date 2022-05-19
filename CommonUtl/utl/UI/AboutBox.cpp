
#include "stdafx.h"
#include "AboutBox.h"
#include "CmdUpdate.h"
#include "ImageStore.h"
#include "LayoutEngine.h"
#include "ReportListControl.h"
#include "ThemeStatic.h"
#include "Utilities.h"
#include "VersionInfo.h"
#include "resource.h"
#include "utl/ContainerUtilities.h"
#include "utl/FileSystem.h"
#include "utl/AppTools.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace hlp
{
	struct CCompilerInfo
	{
		DWORD m_baseCompilerVer;			// ex: _MSC_VER=1400 for Visual Studio 2005
		const TCHAR* m_pVisualStudio;
		const TCHAR* m_pVisualCpp;
	};

	const CCompilerInfo& FindCompilerInfo( DWORD mscVer = _MSC_VER )
	{
		static const CCompilerInfo compilers[] =
		{
			{ 1100, _T("Visual Studio 5"), _T("VC++ 5") },
			{ 1200, _T("Visual Studio 6"), _T("VC++ 6") },
			{ 1300, _T("Visual Studio 2002"), _T("VC++ 7") },
			{ 1310, _T("Visual Studio 2003"), _T("VC++ 7.1") },
			{ 1400, _T("Visual Studio 2005"), _T("VC++ 8") },
			{ 1500, _T("Visual Studio 2008"), _T("VC++ 9") },
			{ 1600, _T("Visual Studio 2010"), _T("VC++ 10") },
			{ 1700, _T("Visual Studio 2012"), _T("VC++ 11") },
			{ 1800, _T("Visual Studio 2013"), _T("VC++ 12") },
			{ 1900, _T("Visual Studio 2015"), _T("VC++ 14") },
			{ 1910, _T("Visual Studio 2017"), _T("VC++ 14.1") }
		};

		for ( unsigned int i = 0, last = COUNT_OF( compilers ) - 1; i != last; ++i )
			if ( mscVer >= compilers[ i ].m_baseCompilerVer && mscVer < compilers[ i + 1 ].m_baseCompilerVer )
				return compilers[ i ];

		return compilers[ COUNT_OF( compilers ) - 1 ];
	}


	inline bool IsDebug( void )
	{
	#ifdef _DEBUG
		return true;
	#else
		return false;
	#endif
	}

	inline bool IsUnicode( void )
	{
	#ifdef _UNICODE
		return true;
	#else
		return false;
	#endif
	}

	inline bool IsMfcDll( void )
	{
	#ifdef _AFXDLL
		return true;
	#else
		return false;
	#endif
	}
}


#define VERSION_MAJOR( x ) ( (x) / 100 )
#define VERSION_MINOR( x ) ( (x) % 100 )

#define VERSION_MAJOR_HEX( x ) ( ( (x) & 0xFFFF00 ) >> 8 )
#define VERSION_MINOR_HEX( x ) ( (x) & 0xFF )


// CAboutBox implementation

namespace layout
{
	enum { CommentsPct = 75, ListPct = 100 - CommentsPct };

	static const CLayoutStyle styles[] =
	{
		{ IDC_ABOUT_NAME_VERSION_STATIC, SizeX },
		{ IDC_ABOUT_COPYRIGHT_STATIC, SizeX },
		{ IDC_ABOUT_WRITTEN_BY_STATIC, SizeX },
		{ IDC_ABOUT_EMAIL_STATIC, SizeX },
		{ IDC_ABOUT_COMMENTS_EDIT, SizeX | pctSizeY( CommentsPct ) },
		{ IDC_ABOUT_BUILD_INFO_LABEL, pctMoveY( CommentsPct ) },
		{ IDC_ABOUT_BUILD_INFO_LIST, pctMoveY( CommentsPct ) | SizeX | pctSizeY( ListPct ) },
		{ IDC_ABOUT_EXPLORE_MODULE, MoveY },
		{ IDOK, Move }
	};
}


UINT CAboutBox::s_appIconId = 0;

CAboutBox::CAboutBox( CWnd* pParent )
	: CLayoutDialog( IDD_ABOUT_BOX, pParent )
	, m_modulePath( app::GetModulePath() )
	, m_exePath( fs::GetModuleFilePath( NULL ) )
	, m_pEmailStatic( new CLinkStatic( _T("mailto:") ) )
	, m_pBuildInfoList( new CReportListControl() )
{
	m_regSection = _T("utl\\About");
	RegisterCtrlLayout( ARRAY_PAIR( layout::styles ) );
	GetLayoutEngine().MaxClientSize() = CSize( 600, 700 );

	std::vector< std::tstring > columnSpecs;
	str::Split( columnSpecs, _T("Property=85|Value=-1"), _T("|") );
	m_pBuildInfoList->SetLayoutInfo( columnSpecs );
	m_pBuildInfoList->SetTabularTextSep( _T("\t") );
}

CAboutBox::~CAboutBox()
{
}

const fs::CPath* CAboutBox::GetSelPath( void ) const
{
	int caretIndex = m_pBuildInfoList->GetCaretIndex();
	if ( -1 == caretIndex )
		return NULL;

	return m_pBuildInfoList->GetPtrAt< fs::CPath >( caretIndex );
}

void CAboutBox::SetupBuildInfoList( void )
{
	DWORD mscVer = _MSC_VER;

	CScopedInternalChange internalChange( m_pBuildInfoList.get() );
	m_pBuildInfoList->DeleteAllItems();

	enum Column { Property, Value };

	int pos = 0;

	m_pBuildInfoList->InsertItem( pos, _T("Platform") );
	m_pBuildInfoList->SetItemText( pos, Value, str::Format( _T("%d bit"), utl::GetPlatformBits() ).c_str() );

	m_pBuildInfoList->InsertItem( ++pos, _T("Build") );
	m_pBuildInfoList->SetItemText( pos, Value, hlp::IsDebug() ? _T("DEBUG") : _T("RELEASE") );
	if ( hlp::IsDebug() )
	{
		static const ui::CTextEffect s_debugEffect( ui::Bold, color::Red );
		m_pBuildInfoList->MarkCellAt( pos, Value, s_debugEffect );
	}

	m_pBuildInfoList->InsertItem( ++pos, _T("Character Set") );
	m_pBuildInfoList->SetItemText( pos, Value, hlp::IsUnicode() ? _T("Unicode") : _T("ANSI") );

	const hlp::CCompilerInfo& compilerInfo = hlp::FindCompilerInfo( mscVer );
	m_pBuildInfoList->InsertItem( ++pos, _T("Compiler") );
	m_pBuildInfoList->SetItemText( pos, Value,
		str::Format( _T("%s, %s, MSC %d.%d"), compilerInfo.m_pVisualStudio, compilerInfo.m_pVisualCpp, VERSION_MAJOR( mscVer ), VERSION_MINOR( mscVer ) ).c_str() );

	// ex: _MFC_VER 0x0700
	m_pBuildInfoList->InsertItem( ++pos, _T("MFC") );
	m_pBuildInfoList->SetItemText( pos, Value,
		str::Format( _T("MFC %d.%d %c %s"),
			VERSION_MAJOR_HEX( _MFC_VER ),
			VERSION_MINOR_HEX( _MFC_VER ),
			str::Conditional( L'\x25cf', '-' ),
			hlp::IsMfcDll() ? _T("Dynamic Library") : _T("Static Library") ).c_str() );

	if ( m_modulePath != m_exePath )
	{
		m_pBuildInfoList->InsertItem( LVIF_TEXT | LVIF_PARAM, ++pos, _T("Module"), 0, 0, 0, (LPARAM)&m_modulePath );
		m_pBuildInfoList->SetItemText( pos, Value, m_modulePath.GetPtr() );
	}

	m_pBuildInfoList->InsertItem( LVIF_TEXT | LVIF_PARAM, ++pos, _T("Executable"), 0, 0, 0, (LPARAM)&m_exePath );
	m_pBuildInfoList->SetItemText( pos, Value, m_exePath.GetPtr() );
}

void CAboutBox::QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const
{
	switch ( cmdId )
	{
		case IDC_ABOUT_BUILD_DATE_STATIC:
			rText = m_buildTime;
			break;
	}

	if ( rText.empty() )
		__super::QueryTooltipText( rText, cmdId, pTooltip );
}

void CAboutBox::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_ABOUT_ICON_STATIC, m_appIconStatic );
	DDX_Control( pDX, IDC_ABOUT_EMAIL_STATIC, *m_pEmailStatic );
	DDX_Control( pDX, IDC_ABOUT_BUILD_INFO_LIST, *m_pBuildInfoList );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		if ( s_appIconId != 0 )
			if ( const CIcon* pAppIcon = ui::GetImageStoresSvc()->RetrieveLargestIcon( s_appIconId ) )
				m_appIconStatic.SetIcon( pAppIcon->GetHandle() );

		SetupBuildInfoList();

		CVersionInfo version;

		m_buildTime = version.FormatBuildTime();
		if ( !m_buildTime.empty() )
			m_buildTime.insert( 0, _T("at ") );

		ui::SetWindowText( m_hWnd, version.ExpandValues( ui::GetWindowText( m_hWnd ).c_str() ) );		// expand dialog title
		ui::ShowControl( m_hWnd, IDC_ABOUT_BUILD_DATE_STATIC, !version.GetBuildTimestamp().empty() );

		// expand static fields
		static const UINT fieldIds[] = { IDC_ABOUT_NAME_VERSION_STATIC, IDC_ABOUT_BUILD_DATE_STATIC, IDC_ABOUT_COPYRIGHT_STATIC, IDC_ABOUT_WRITTEN_BY_STATIC, IDC_ABOUT_EMAIL_STATIC };
		for ( unsigned int i = 0; i != COUNT_OF( fieldIds ); ++i )
			ui::SetDlgItemText( this, fieldIds[ i ], version.ExpandValues( ui::GetDlgItemText( this, fieldIds[ i ] ).c_str() ) );

		ui::SetDlgItemText( this, IDC_ABOUT_COMMENTS_EDIT, version.ExpandValues( _T("[Comments]") ) );
		ui::UpdateControlUI( ::GetDlgItem( m_hWnd, IDC_ABOUT_EXPLORE_MODULE ) );
	}

	CLayoutDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CAboutBox, CLayoutDialog )
	ON_NOTIFY( LVN_ITEMCHANGED, IDC_ABOUT_BUILD_INFO_LIST, OnLvnItemChanged_ListItems )
	ON_BN_CLICKED( IDC_ABOUT_EXPLORE_MODULE, OnExploreModule )
	ON_UPDATE_COMMAND_UI( IDC_ABOUT_EXPLORE_MODULE, OnUpdateExploreModule )
END_MESSAGE_MAP()

void CAboutBox::OnLvnItemChanged_ListItems( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLISTVIEW* pNmList = (NMLISTVIEW*)pNmHdr;
	*pResult = 0;

	if ( CReportListControl::IsSelectionChangeNotify( pNmList, LVIS_SELECTED | LVIS_FOCUSED ) )
		ui::UpdateControlUI( ::GetDlgItem( m_hWnd, IDC_ABOUT_EXPLORE_MODULE ) );
}

void CAboutBox::OnExploreModule( void )
{
	const fs::CPath* pItemPath = GetSelPath();
	if ( NULL == pItemPath )
		pItemPath = &m_modulePath;

	std::tstring parameters = _T("/select,") + pItemPath->Get();
	::ShellExecute( m_hWnd, NULL, _T("explorer.exe"), parameters.c_str(), NULL, SW_SHOWNORMAL );
}

void CAboutBox::OnUpdateExploreModule( CCmdUI* pCmdUI )
{
	const fs::CPath* pItemPath = GetSelPath();

	pCmdUI->Enable( NULL == pItemPath || pItemPath->FileExist() );
}
