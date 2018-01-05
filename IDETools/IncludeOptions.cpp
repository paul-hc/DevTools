
#include "stdafx.h"
#include "IncludeOptions.h"
#include "utl/ContainerUtilities.h"
#include "utl/Path.h"
#include "utl/StringUtilities.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const TCHAR CIncludeOptions::m_sectionName[] = _T("Settings\\IncludeOptions");

CIncludeOptions::CIncludeOptions( void )
	: m_viewMode( vmRelPathFileName )
	, m_ordering( ordAlphaText )
	, m_maxNestingLevel( 5 )
	, m_maxParseLines( 5000 )
	, m_noDuplicates( false )
	, m_openBlownUp( false )
	, m_selRecover( true )
	, m_lazyParsing( true )
{
}

CIncludeOptions::~CIncludeOptions()
{
}

void CIncludeOptions::ReadProfile( void )
{
	CWinApp* pApp = AfxGetApp();

	m_viewMode = (ViewMode)pApp->GetProfileInt( m_sectionName, ENTRY_MEMBER( m_viewMode ), m_viewMode );
	m_ordering = (Ordering)pApp->GetProfileInt( m_sectionName, ENTRY_MEMBER( m_ordering ), m_ordering );
	m_maxNestingLevel = pApp->GetProfileInt( m_sectionName, ENTRY_MEMBER( m_maxNestingLevel ), m_maxNestingLevel );
	m_maxParseLines = pApp->GetProfileInt( m_sectionName, ENTRY_MEMBER( m_maxParseLines ), m_maxParseLines );
	m_noDuplicates = pApp->GetProfileInt( m_sectionName, ENTRY_MEMBER( m_noDuplicates ), m_noDuplicates ) != FALSE;
	m_openBlownUp = pApp->GetProfileInt( m_sectionName, ENTRY_MEMBER( m_openBlownUp ), m_openBlownUp ) != FALSE;
	m_selRecover = pApp->GetProfileInt( m_sectionName, ENTRY_MEMBER( m_selRecover ), m_selRecover ) != FALSE;
	m_lazyParsing = pApp->GetProfileInt( m_sectionName, ENTRY_MEMBER( m_lazyParsing ), m_lazyParsing ) != FALSE;
	m_lastBrowsedFile = (LPCTSTR)pApp->GetProfileString( m_sectionName, ENTRY_MEMBER( m_lastBrowsedFile ), m_lastBrowsedFile.c_str() );

	SetupArrayExclude( (LPCTSTR)pApp->GetProfileString( m_sectionName, ENTRY_MEMBER( m_fnExclude ), m_fnExclude.c_str() ), PROF_SEP );
	SetupArrayMore( (LPCTSTR)pApp->GetProfileString( m_sectionName, ENTRY_MEMBER( m_fnMore ), m_fnMore.c_str() ), PROF_SEP );
}

void CIncludeOptions::WriteProfile( void ) const
{
	CWinApp* pApp = AfxGetApp();

	pApp->WriteProfileInt( m_sectionName, ENTRY_MEMBER( m_viewMode ), m_viewMode );
	pApp->WriteProfileInt( m_sectionName, ENTRY_MEMBER( m_ordering ), m_ordering );
	pApp->WriteProfileInt( m_sectionName, ENTRY_MEMBER( m_maxNestingLevel ), m_maxNestingLevel );
	pApp->WriteProfileInt( m_sectionName, ENTRY_MEMBER( m_maxParseLines ), m_maxParseLines );
	pApp->WriteProfileInt( m_sectionName, ENTRY_MEMBER( m_noDuplicates ), m_noDuplicates );
	pApp->WriteProfileInt( m_sectionName, ENTRY_MEMBER( m_openBlownUp ), m_openBlownUp );
	pApp->WriteProfileInt( m_sectionName, ENTRY_MEMBER( m_selRecover ), m_selRecover );
	pApp->WriteProfileInt( m_sectionName, ENTRY_MEMBER( m_lazyParsing ), m_lazyParsing );
	pApp->WriteProfileString( m_sectionName, ENTRY_MEMBER( m_lastBrowsedFile ), m_lastBrowsedFile.c_str() );

	pApp->WriteProfileString( m_sectionName, ENTRY_MEMBER( m_fnExclude ), m_fnExclude.c_str() );
	pApp->WriteProfileString( m_sectionName, ENTRY_MEMBER( m_fnMore ), m_fnMore.c_str() );
}

void CIncludeOptions::SetupArrayExclude( const TCHAR* pFnExclude, const TCHAR* pSep )
{
	m_fnExclude = pFnExclude;
	str::Tokenize( m_fnArrayExclude, m_fnExclude.c_str(), pSep );			// discard empty entries
	std::for_each( m_fnArrayExclude.begin(), m_fnArrayExclude.end(), path::Normalize );
	utl::RemoveDuplicates< pred::EquivalentPathString >( m_fnArrayExclude );
}

void CIncludeOptions::SetupArrayMore( const TCHAR* pFnMore, const TCHAR* pSep )
{
	m_fnMore = pFnMore;
	str::Tokenize( m_fnArrayMore, m_fnMore.c_str(), pSep );					// discard empty entries
}

const CEnumTags& CIncludeOptions::GetTags_DepthLevel( void )
{
	static const CEnumTags& tags( str::Load( IDC_DEPTH_LEVEL_COMBO ) );
	return tags;
}

const CEnumTags& CIncludeOptions::GetTags_ViewMode( void )
{
	static const CEnumTags& tags( str::Load( IDC_VIEW_MODE_COMBO ) );
	return tags;
}

const CEnumTags& CIncludeOptions::GetTags_Ordering( void )
{
	static const CEnumTags& tags( str::Load( IDC_ORDER_COMBO ) );
	return tags;
}
