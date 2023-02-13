
#include "pch.h"
#include "IncludeOptions.h"
#include "Application.h"
#include "utl/Path.h"
#include "utl/StringUtilities.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section[] = _T("Settings\\IncludeOptions");
}


CIncludeOptions::CIncludeOptions( void )
	: m_viewMode( vmRelPathFileName )
	, m_ordering( ordAlphaText )
	, m_maxNestingLevel( 5 )
	, m_maxParseLines( 5000 )
	, m_noDuplicates( false )
	, m_openBlownUp( false )
	, m_selRecover( true )
	, m_lazyParsing( true )
	, m_additionalIncludePath( inc::AdditionalPath )
{
	Load();
}

CIncludeOptions::~CIncludeOptions()
{
}

CIncludeOptions& CIncludeOptions::Instance( void )
{
	static CIncludeOptions options;
	return options;
}

void CIncludeOptions::Load( void )
{
	CWinApp* pApp = AfxGetApp();

	m_viewMode = static_cast<ViewMode>( pApp->GetProfileInt( reg::section, ENTRY_MEMBER( m_viewMode ), m_viewMode ) );
	m_ordering = static_cast<Ordering>( pApp->GetProfileInt( reg::section, ENTRY_MEMBER( m_ordering ), m_ordering ) );
	m_maxNestingLevel = pApp->GetProfileInt( reg::section, ENTRY_MEMBER( m_maxNestingLevel ), m_maxNestingLevel );
	m_maxParseLines = pApp->GetProfileInt( reg::section, ENTRY_MEMBER( m_maxParseLines ), m_maxParseLines );
	m_noDuplicates = pApp->GetProfileInt( reg::section, ENTRY_MEMBER( m_noDuplicates ), m_noDuplicates ) != FALSE;
	m_openBlownUp = pApp->GetProfileInt( reg::section, ENTRY_MEMBER( m_openBlownUp ), m_openBlownUp ) != FALSE;
	m_selRecover = pApp->GetProfileInt( reg::section, ENTRY_MEMBER( m_selRecover ), m_selRecover ) != FALSE;
	m_lazyParsing = pApp->GetProfileInt( reg::section, ENTRY_MEMBER( m_lazyParsing ), m_lazyParsing ) != FALSE;
	m_lastBrowsedFile.Set( pApp->GetProfileString( reg::section, ENTRY_MEMBER( m_lastBrowsedFile ), m_lastBrowsedFile.GetPtr() ).GetString() );

	reg::LoadPathGroup( m_fnIgnored, reg::section, ENTRY_MEMBER( m_fnIgnored ) );
	reg::LoadPathGroup( m_fnAdded, reg::section, ENTRY_MEMBER( m_fnAdded ) );
	reg::LoadPathGroup( m_additionalIncludePath, reg::section, ENTRY_MEMBER( m_additionalIncludePath ) );
}

void CIncludeOptions::Save( void ) const
{
	CWinApp* pApp = AfxGetApp();

	pApp->WriteProfileInt( reg::section, ENTRY_MEMBER( m_viewMode ), m_viewMode );
	pApp->WriteProfileInt( reg::section, ENTRY_MEMBER( m_ordering ), m_ordering );
	pApp->WriteProfileInt( reg::section, ENTRY_MEMBER( m_maxNestingLevel ), m_maxNestingLevel );
	pApp->WriteProfileInt( reg::section, ENTRY_MEMBER( m_maxParseLines ), m_maxParseLines );
	pApp->WriteProfileInt( reg::section, ENTRY_MEMBER( m_noDuplicates ), m_noDuplicates );
	pApp->WriteProfileInt( reg::section, ENTRY_MEMBER( m_openBlownUp ), m_openBlownUp );
	pApp->WriteProfileInt( reg::section, ENTRY_MEMBER( m_selRecover ), m_selRecover );
	pApp->WriteProfileInt( reg::section, ENTRY_MEMBER( m_lazyParsing ), m_lazyParsing );
	pApp->WriteProfileString( reg::section, ENTRY_MEMBER( m_lastBrowsedFile ), m_lastBrowsedFile.GetPtr() );

	reg::SavePathGroup( m_fnIgnored, reg::section, ENTRY_MEMBER( m_fnIgnored ) );
	reg::SavePathGroup( m_fnAdded, reg::section, ENTRY_MEMBER( m_fnAdded ) );
	reg::SavePathGroup( m_additionalIncludePath, reg::section, ENTRY_MEMBER( m_additionalIncludePath ) );
}

const CEnumTags& CIncludeOptions::GetTags_DepthLevel( void )
{
	static const CEnumTags tags( str::Load( IDC_DEPTH_LEVEL_COMBO ) );
	return tags;
}

const CEnumTags& CIncludeOptions::GetTags_ViewMode( void )
{
	static const CEnumTags tags( str::Load( IDC_VIEW_MODE_COMBO ) );
	return tags;
}

const CEnumTags& CIncludeOptions::GetTags_Ordering( void )
{
	static const CEnumTags tags( str::Load( IDC_ORDER_COMBO ) );
	return tags;
}
