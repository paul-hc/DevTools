
#include "stdafx.h"
#include "AboutBox.h"
#include "ImageStore.h"
#include "LayoutEngine.h"
#include "ReportListControl.h"
#include "ThemeStatic.h"
#include "Utilities.h"
#include "VersionInfo.h"
#include "resource.h"

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
			{ 1800, _T("Visual Studio 2013"), _T("VC++ 12") }
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
	static const CLayoutStyle styles[] =
	{
		{ IDC_ABOUT_NAME_VERSION_STATIC, StretchX },
		{ IDC_ABOUT_COPYRIGHT_STATIC, StretchX },
		{ IDC_ABOUT_WRITTEN_BY_STATIC, StretchX },
		{ IDC_ABOUT_EMAIL_STATIC, StretchX },
		{ IDC_ABOUT_BUILD_INFO_LIST, Stretch },
		{ IDC_ABOUT_EXPLORE_EXECUTABLE, OffsetY },
		{ IDOK, Offset }
	};
}


UINT CAboutBox::m_appIconId = 0;

CAboutBox::CAboutBox( CWnd* pParent )
	: CLayoutDialog( IDD_ABOUT_BOX, pParent )
	, m_executablePath( ui::GetModuleFileName() )
	, m_pEmailStatic( new CLinkStatic( _T("mailto:") ) )
	, m_pBuildInfoList( new CReportListControl() )
{
	m_regSection = _T("utl\\About");
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );
	GetLayoutEngine().MaxClientSize() = CSize( 600, 360 );

	std::vector< std::tstring > columnSpecs;
	str::Split( columnSpecs, _T("Property=85|Value=-1"), _T("|") );
	m_pBuildInfoList->SetLayoutInfo( columnSpecs );
}

CAboutBox::~CAboutBox()
{
}

void CAboutBox::SetupBuildInfoList( void )
{
	DWORD mscVer = _MSC_VER;
	DWORD platBits = sizeof( void* ) * 8;

	CScopedInternalChange internalChange( m_pBuildInfoList.get() );
	m_pBuildInfoList->DeleteAllItems();

	enum Column { Property, Value };

	int pos = -1;

	m_pBuildInfoList->InsertItem( ++pos, _T("Platform") );
	m_pBuildInfoList->SetItemText( pos, Value, str::Format( _T("%d bit"), platBits ).c_str() );

	m_pBuildInfoList->InsertItem( ++pos, _T("Build") );
	m_pBuildInfoList->SetItemText( pos, Value, hlp::IsDebug() ? _T("debug") : _T("RELEASE") );

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

	m_pBuildInfoList->InsertItem( ++pos, _T("Executable") );
	m_pBuildInfoList->SetItemText( pos, Value, m_executablePath.c_str() );
}

void CAboutBox::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_ABOUT_ICON_STATIC, m_appIconStatic );
	DDX_Control( pDX, IDC_ABOUT_EMAIL_STATIC, *m_pEmailStatic );
	DDX_Control( pDX, IDC_ABOUT_BUILD_INFO_LIST, *m_pBuildInfoList );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		if ( m_appIconId != 0 && CImageStore::HasSharedStore() )
			if ( const CIcon* pAppIcon = CImageStore::GetSharedStore()->RetrieveLargestIcon( m_appIconId ) )
				m_appIconStatic.SetIcon( pAppIcon->GetHandle() );

		SetupBuildInfoList();

		CVersionInfo version;
		ui::SetWindowText( m_hWnd, version.ExpandValues( ui::GetWindowText( m_hWnd ).c_str() ) );		// expand dialog title

		// expand static fields
		static const UINT fieldIds[] = { IDC_ABOUT_NAME_VERSION_STATIC, IDC_ABOUT_COPYRIGHT_STATIC, IDC_ABOUT_WRITTEN_BY_STATIC, IDC_ABOUT_EMAIL_STATIC };
		for ( unsigned int i = 0; i != COUNT_OF( fieldIds ); ++i )
			ui::SetDlgItemText( this, fieldIds[ i ], version.ExpandValues( ui::GetDlgItemText( this, fieldIds[ i ] ).c_str() ) );
	}

	CLayoutDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CAboutBox, CLayoutDialog )
	ON_BN_CLICKED( IDC_ABOUT_EXPLORE_EXECUTABLE, OnExploreExecutable )
END_MESSAGE_MAP()

void CAboutBox::OnExploreExecutable( void )
{
	std::tstring parameters = _T("/select,") + m_executablePath;
	::ShellExecute( m_hWnd, NULL, _T("explorer.exe"), parameters.c_str(), NULL, SW_SHOWNORMAL );
}
