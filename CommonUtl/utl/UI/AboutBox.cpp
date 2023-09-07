
#include "pch.h"
#include "AboutBox.h"
#include "CmdUpdate.h"
#include "ImageStore.h"
#include "LayoutEngine.h"
#include "ReportListControl.h"
#include "ThemeStatic.h"
#include "ToolbarImagesDialog.h"
#include "WndUtils.h"
#include "VersionInfo.h"
#include "resource.h"
#include "utl/FileSystem.h"
#include "utl/AppTools.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace hlp
{
	struct CCompilerInfo
	{
		WORD m_baseCompilerVer;			// ex: _MSC_VER=1400 for Visual Studio 2005
		const TCHAR* m_pVisualStudio;
		const TCHAR* m_pVisualCpp;
	};

	const CCompilerInfo& FormatCompilerInfo( WORD mscVer = _MSC_VER )
	{
		static const CCompilerInfo s_compilers[] =
		{
			{ 1100, _T("Visual Studio 5"),    _T("VC++ 5") },
			{ 1200, _T("Visual Studio 6"),    _T("VC++ 6") },
			{ 1300, _T("Visual Studio 2002"), _T("VC++ 7") },
			{ 1310, _T("Visual Studio 2003"), _T("VC++ 7.1") },
			{ 1400, _T("Visual Studio 2005"), _T("VC++ 8") },
			{ 1500, _T("Visual Studio 2008"), _T("VC++ 9") },
			{ 1600, _T("Visual Studio 2010"), _T("VC++ 10") },
			{ 1700, _T("Visual Studio 2012"), _T("VC++ 11") },
			{ 1800, _T("Visual Studio 2013"), _T("VC++ 12") },
			{ 1900, _T("Visual Studio 2015"), _T("VC++ 14") },
			{ 1910, _T("Visual Studio 2017"), _T("VC++ 15") },
			{ 1911, _T("Visual Studio 2017"), _T("VC++ 15.3") },
			{ 1912, _T("Visual Studio 2017"), _T("VC++ 15.5") },
			{ 1913, _T("Visual Studio 2017"), _T("VC++ 15.6") },
			{ 1914, _T("Visual Studio 2017"), _T("VC++ 15.7") },
			{ 1915, _T("Visual Studio 2017"), _T("VC++ 15.8") },
			{ 1916, _T("Visual Studio 2017"), _T("VC++ 15.9") },
			{ 1920, _T("Visual Studio 2019"), _T("VC++ 16.0") },
			{ 1921, _T("Visual Studio 2019"), _T("VC++ 16.1") },
			{ 1922, _T("Visual Studio 2019"), _T("VC++ 16.2") },
			{ 1923, _T("Visual Studio 2019"), _T("VC++ 16.3") },
			{ 1924, _T("Visual Studio 2019"), _T("VC++ 16.4") },
			{ 1925, _T("Visual Studio 2019"), _T("VC++ 16.5") },
			{ 1926, _T("Visual Studio 2019"), _T("VC++ 16.6") },
			{ 1927, _T("Visual Studio 2019"), _T("VC++ 16.7") },
			{ 1928, _T("Visual Studio 2019"), _T("VC++ 16.9") },
			{ 1929, _T("Visual Studio 2019"), _T("VC++ 16.11") },
			{ 1930, _T("Visual Studio 2022"), _T("VC++ 17.0") },
			{ 1931, _T("Visual Studio 2022"), _T("VC++ 17.1") },
			{ 1932, _T("Visual Studio 2022"), _T("VC++ 17.2") },
			{ 1933, _T("Visual Studio 2022"), _T("VC++ 17.3") },
			{ 1934, _T("Visual Studio 2022"), _T("VC++ 17.4") },
			{ 1935, _T("Visual Studio 2022"), _T("VC++ 17.5") },

			{ 9999, _T("Visual Studio ?? TODO..."), _T("VC++ ??") }
		};

		for ( unsigned int i = 0, last = COUNT_OF( s_compilers ) - 1; i != last; ++i )
			if ( mscVer >= s_compilers[ i ].m_baseCompilerVer && mscVer < s_compilers[ i + 1 ].m_baseCompilerVer )
				return s_compilers[ i ];

		//static CCompilerInfo s_unknown;
		return s_compilers[ COUNT_OF( s_compilers ) - 1 ];
	}

	std::tstring FormatCompilerVersion( WORD mscVer = _MSC_VER )
	{
	#define MS_COMPILER_VER_MAJOR( x ) ( (x) / 100 )
	#define MS_COMPILER_VER_MINOR( x ) ( (x) % 100 )

		const hlp::CCompilerInfo& compilerInfo = hlp::FormatCompilerInfo( mscVer );

		return str::Format( _T("%s, %s, MSC %d.%d"), compilerInfo.m_pVisualStudio, compilerInfo.m_pVisualCpp, MS_COMPILER_VER_MAJOR( mscVer ), MS_COMPILER_VER_MINOR( mscVer ) );
	}


	std::tstring FormatMFCVersion( WORD mfcVer = _MFC_VER )
	{
	#define MFC_VER_MAJOR( x ) ( ( (x) & 0xFFFF00 ) >> 8 )
	#define MFC_VER_MINOR( x ) ( (x) & 0xFF )

	#ifdef _AFXDLL
		bool isMfcDll = true;
	#else
		bool isMfcDll = false;
	#endif

		return str::Format( _T("MFC %d.%d %c %s"),
							MFC_VER_MAJOR( mfcVer ),
							MFC_VER_MINOR( mfcVer ),
							str::Conditional( L'\x25cf', '-' ),
							isMfcDll ? _T("Dynamic Library") : _T("Static Library") );
	}

	const TCHAR* FormatWindowsVersion( WORD winVer = WINVER )		// _WIN32_WINNT_* macro (_WIN32_WINNT_WIN7) <=> high-word of NTDDI_* macro (e.g. NTDDI_WIN7)
	{
		switch ( winVer )
		{
			case 0x0400:	return _T("Windows NT 4");			// _WIN32_WINNT_NT4
			case 0x0500:	return _T("Windows 2000");			// _WIN32_WINNT_WIN2K
			case 0x0501:	return _T("Windows XP");			// _WIN32_WINNT_WINXP
			case 0x0502:	return _T("Windows Server 2003");	// _WIN32_WINNT_WS03
			case 0x0600:	return _T("Windows Vista");			// _WIN32_WINNT_VISTA
			case 0x0601:	return _T("Windows 7");				// _WIN32_WINNT_WIN7
			case 0x0602:	return _T("Windows 8");				// _WIN32_WINNT_WIN8
			case 0x0603:	return _T("Windows 8.1");			// _WIN32_WINNT_WINBLUE
			case 0x0A00:	return _T("Windows 10");			// _WIN32_WINNT_WIN10
			case 0x0B00:	return _T("Windows 11");			// _WIN32_WINNT_WIN11
			case 0x0C00:	return _T("Windows 12");			// _WIN32_WINNT_WIN12
		}

		static std::tstring s_text;
		s_text = str::Format( _T("(Windows %d.%d)"), HIBYTE( winVer ), LOBYTE( winVer ) );
		return s_text.c_str();
	}

	const TCHAR* FormatInternetExplorerVersion( WORD ieVer = _WIN32_IE )
	{
		switch ( ieVer )
		{
			case 0x0200:	return _T("Internet Explorer 2");		// _WIN32_IE_IE20
			case 0x0300:	return _T("Internet Explorer 3");		// _WIN32_IE_IE30
			case 0x0302:	return _T("Internet Explorer 3.02");	// _WIN32_IE_IE302
			case 0x0400:	return _T("Internet Explorer 4");		// _WIN32_IE_IE40
			case 0x0401:	return _T("Internet Explorer 4.01");	// _WIN32_IE_IE401
			case 0x0500:	return _T("Internet Explorer 5");		// _WIN32_IE_IE50
			case 0x0501:	return _T("Internet Explorer 5.01");	// _WIN32_IE_IE501
			case 0x0550:	return _T("Internet Explorer 5.5");		// _WIN32_IE_IE55
			case 0x0600:	return _T("Internet Explorer 6");		// _WIN32_IE_IE60
			case 0x0601:	return _T("Internet Explorer 6 SP1");	// _WIN32_IE_IE60SP1
			case 0x0603:	return _T("Internet Explorer 6 SP2");	// _WIN32_IE_IE60SP2
			case 0x0700:	return _T("Internet Explorer 7");		// _WIN32_IE_IE70
			case 0x0800:	return _T("Internet Explorer 8");		// _WIN32_IE_IE80
			case 0x0900:	return _T("Internet Explorer 9");		// _WIN32_IE_IE90
			case 0x0A00:	return _T("Internet Explorer 10");		// _WIN32_IE_IE100
		}

		static std::tstring s_text;
		s_text = str::Format( _T("(Internet Explorer %d.%d)"), HIBYTE( ieVer ), LOBYTE( ieVer ) );
		return s_text.c_str();
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
}


// CAboutBox implementation

namespace layout
{
	enum { CommentsPct = 60, ListPct = 100 - CommentsPct };

	static const CLayoutStyle s_styles[] =
	{
		{ IDC_ABOUT_NAME_VERSION_STATIC, SizeX },
		{ IDC_ABOUT_COPYRIGHT_STATIC, SizeX },
		{ IDC_ABOUT_WRITTEN_BY_STATIC, SizeX },
		{ IDC_ABOUT_EMAIL_STATIC, SizeX },
		{ IDC_ABOUT_COMMENTS_EDIT, SizeX | pctSizeY( CommentsPct ) },
		{ IDC_ABOUT_BUILD_INFO_LABEL, pctMoveY( CommentsPct ) },
		{ IDC_ABOUT_BUILD_INFO_LIST, pctMoveY( CommentsPct ) | SizeX | pctSizeY( ListPct ) },
		{ IDC_ABOUT_EXPLORE_MODULE, MoveY },
		{ IDD_TOOLBAR_IMAGES_DIALOG, MoveY },
		{ IDOK, Move }
	};
}


UINT CAboutBox::s_appIconId = 0;

CAboutBox::CAboutBox( CWnd* pParent )
	: CLayoutDialog( IDD_ABOUT_BOX, pParent )
	, m_modulePath( app::GetModulePath() )
	, m_exePath( fs::GetModuleFilePath( nullptr ) )
	, m_pEmailStatic( new CLinkStatic( _T("mailto:") ) )
	, m_pBuildInfoList( new CReportListControl() )
{
	m_regSection = _T("utl\\About");
	RegisterCtrlLayout( ARRAY_SPAN( layout::s_styles ) );
	GetLayoutEngine().MaxClientSize() = CSize( 600, 700 );

	std::vector<std::tstring> columnSpecs;
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
		return nullptr;

	return m_pBuildInfoList->GetPtrAt<fs::CPath>( caretIndex );
}

void CAboutBox::SetupBuildInfoList( void )
{
	CScopedInternalChange internalChange( m_pBuildInfoList.get() );
	m_pBuildInfoList->DeleteAllItems();

	int pos = 0;

	AddBuildInfoPair( pos++, _T("Platform"), str::Format( _T("%d-bit"), utl::GetPlatformBits() ) );

	AddBuildInfoPair( pos++, _T("Build"), hlp::IsDebug() ? _T("DEBUG Build") : _T("RELEASE Build") );
	m_pBuildInfoList->MarkCellAt( pos - 1, Value, ui::CTextEffect( ui::Bold, hlp::IsDebug() ? color::Red : color::BlueWindows10 ) );

	AddBuildInfoPair( pos++, _T("Character Set"), hlp::IsUnicode() ? _T("Unicode") : _T("ANSI") );
	AddBuildInfoPair( pos++, _T("Compiler"), hlp::FormatCompilerVersion( _MSC_VER ) );		// e.g. _MSC_VER 1500 for VC9 (VS-2008)
	AddBuildInfoPair( pos++, _T("MFC"), hlp::FormatMFCVersion( _MFC_VER ) );				// e.g. _MFC_VER 0x0900

	if ( m_modulePath != m_exePath )
		AddBuildInfoPair( pos++, _T("Module"), m_modulePath.Get(), &m_modulePath );

	AddBuildInfoPair( pos++, _T("Executable"), m_exePath.Get(), &m_exePath );

	// note: _WIN32_WINNT is a synonim of WINVER
	AddBuildInfoPair( pos++, _T("WINVER"), str::Format( _T("0x%04X  %s"), WINVER, hlp::FormatWindowsVersion( WINVER ) ) );						// e.g. WINVER 0x0601
	AddBuildInfoPair( pos++, _T("_WIN32_IE"), str::Format( _T("0x%04X  %s"), _WIN32_IE, hlp::FormatInternetExplorerVersion( _WIN32_IE ) ) );	// e.g. _WIN32_IE 0x0800

	// ignore old Win95 macro: https://devblogs.microsoft.com/oldnewthing/20070411-00/?p=27283
//#ifdef _WIN32_WINDOWS
//	AddBuildInfoPair( pos++, _T("_WIN32_WINDOWS"), str::Format( _T("0x%04X  %s"), _WIN32_WINDOWS, hlp::FormatWindowsVersion( _WIN32_WINDOWS ) ) );	// e.g. _WIN32_WINDOWS 0x0410
//#endif
}

void CAboutBox::AddBuildInfoPair( int pos, const TCHAR* pProperty, const std::tstring& value, const void* pItemData /*= nullptr*/ )
{
	m_pBuildInfoList->InsertItem( LVIF_TEXT | ( pItemData != nullptr ? LVIF_PARAM : 0 ), pos, pProperty, 0, 0, 0, (LPARAM)pItemData );
	m_pBuildInfoList->SetItemText( pos, Value, value.c_str() );
}

void CAboutBox::QueryTooltipText( OUT std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const
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
	ON_BN_CLICKED( IDD_TOOLBAR_IMAGES_DIALOG, OnViewToolBars )
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
	if ( nullptr == pItemPath )
		pItemPath = &m_modulePath;

	std::tstring parameters = _T("/select,") + pItemPath->Get();
	::ShellExecute( m_hWnd, nullptr, _T("explorer.exe"), parameters.c_str(), nullptr, SW_SHOWNORMAL );
}

void CAboutBox::OnUpdateExploreModule( CCmdUI* pCmdUI )
{
	const fs::CPath* pItemPath = GetSelPath();

	pCmdUI->Enable( nullptr == pItemPath || pItemPath->FileExist() );
}

void CAboutBox::OnViewToolBars( void )
{
	CToolbarImagesDialog dlg( this );

	dlg.DoModal();
}
