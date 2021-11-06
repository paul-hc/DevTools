
#include "stdafx.h"
#include "IncludeDirectories.h"
#include "IncludeOptions.h"
#include "ModuleSession.h"
#include "Application.h"
#include "utl/ContainerUtilities.h"
#include "utl/Registry.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	const fs::CPath section = _T("Settings\\Directories");
	const TCHAR entry_currSetPos[] = _T("currSetPos");
}


const std::tstring CIncludeDirectories::s_defaultSetName = _T("Default Set");
bool CIncludeDirectories::s_created = false;

CIncludeDirectories::CIncludeDirectories( void )
{
	Reset();
	Load();
}

CIncludeDirectories& CIncludeDirectories::Instance( void )
{
	static CIncludeDirectories dirs;
	return dirs;
}

void CIncludeDirectories::Clear( void )
{
	utl::ClearOwningAssocContainerValues( m_includePaths );
	m_currSetPos = utl::npos;
	m_searchSpecs.clear();
}

void CIncludeDirectories::Assign( const CIncludeDirectories& right )
{
	Clear();

	for ( utl::vector_map< std::tstring, CIncludePaths* >::const_iterator itSrc = right.m_includePaths.begin(); itSrc != right.m_includePaths.end(); ++itSrc )
		m_includePaths[ itSrc->first ] = new CIncludePaths( *itSrc->second );

	m_currSetPos = right.m_currSetPos;
}

void CIncludeDirectories::Reset( void )
{
	Clear();

	CIncludePaths* pDefaultIncludePaths = new CIncludePaths;
	pDefaultIncludePaths->InitFromIde();

	m_includePaths[ s_defaultSetName ] = pDefaultIncludePaths;
	m_currSetPos = 0;
}

CIncludePaths* CIncludeDirectories::Add( const std::tstring& setName, bool doSelect /*= true*/ )
{
	ASSERT( !setName.empty() );

	delete utl::FindValue( m_includePaths, setName );		// delete any existing set matching same name

	CIncludePaths* pIncludePaths = new CIncludePaths;
	m_includePaths[ setName ] = pIncludePaths;

	if ( doSelect )
		m_currSetPos = m_includePaths.FindPos( setName );		// also select the added entry

	m_searchSpecs.clear();
	return pIncludePaths;
}

void CIncludeDirectories::RemoveCurrent( void )
{
	const std::tstring& setName = GetCurrentName();
	utl::vector_map< std::tstring, CIncludePaths* >::iterator itFound = m_includePaths.find( setName );
	ASSERT( itFound != m_includePaths.end() );

	delete itFound->second;
	m_includePaths.erase( itFound );

	if ( m_includePaths.empty() )			// last removed?
		Reset();
	else
		m_currSetPos = std::min( m_currSetPos, m_includePaths.size() - 1 );

	m_searchSpecs.clear();
}

void CIncludeDirectories::RenameCurrent( const std::tstring& newSetName )
{
	const std::tstring& setName = GetCurrentName();
	if ( newSetName.empty() || setName == newSetName )
		return;

	size_t foundPos = m_includePaths.FindPos( setName );
	ASSERT( foundPos != utl::npos );

	CIncludePaths* pFound = m_includePaths.at( foundPos ).second;

	m_includePaths.erase( m_includePaths.begin() + foundPos );
	m_includePaths[ newSetName ] = pFound;

	if ( m_currSetPos == foundPos )
		m_currSetPos = m_includePaths.FindPos( newSetName );
}

const CIncludePaths* CIncludeDirectories::GetCurrentPaths( void ) const
{
	ASSERT( m_currSetPos < m_includePaths.size() );
	return m_includePaths.at( m_currSetPos ).second;
}

CIncludePaths* CIncludeDirectories::RefCurrentPaths( void )
{
	m_searchSpecs.clear();
	return m_includePaths.at( m_currSetPos ).second;
}

void CIncludeDirectories::SetCurrentPos( size_t currSetPos )
{
	ASSERT( currSetPos < m_includePaths.size() );
	m_currSetPos = currSetPos;

	m_searchSpecs.clear();
}

const std::vector< inc::TDirSearchPair >& CIncludeDirectories::GetSearchSpecs( void ) const
{
	const CIncludePaths* pIncludePaths = GetCurrentPaths();
	ASSERT_PTR( pIncludePaths );

	if ( m_searchSpecs.empty() )
	{
		m_searchSpecs.push_back( inc::TDirSearchPair( &pIncludePaths->m_standard, inc::Flag_StandardPath ) );
		m_searchSpecs.push_back( inc::TDirSearchPair( &CIncludePaths::Get_INCLUDE(), inc::Flag_StandardPath ) );
		m_searchSpecs.push_back( inc::TDirSearchPair( &CIncludeOptions::Instance().m_additionalIncludePath, inc::Flag_AdditionalPath ) );
		m_searchSpecs.push_back( inc::TDirSearchPair( &app::GetModuleSession().m_moreAdditionalIncludePath, inc::Flag_AdditionalPath ) );
		m_searchSpecs.push_back( inc::TDirSearchPair( &pIncludePaths->m_source, inc::Flag_SourcePath ) );
		m_searchSpecs.push_back( inc::TDirSearchPair( &pIncludePaths->m_library, inc::Flag_LibraryPath ) );
		m_searchSpecs.push_back( inc::TDirSearchPair( &CIncludePaths::Get_LIB(), inc::Flag_LibraryPath ) );
		m_searchSpecs.push_back( inc::TDirSearchPair( &pIncludePaths->m_binary, inc::Flag_BinaryPath ) );
		m_searchSpecs.push_back( inc::TDirSearchPair( &CIncludePaths::Get_PATH(), inc::Flag_BinaryPath ) );
	}
	return m_searchSpecs;
}


void CIncludeDirectories::Load( void )
{
	CWinApp* pApp = AfxGetApp();

	reg::CKey sectionKey( pApp->GetSectionKey( reg::section.GetPtr() ) );

	std::vector< std::tstring > subKeyNames;
	sectionKey.QuerySubKeyNames( subKeyNames );

	if ( !subKeyNames.empty() )
	{
		Clear();

		for ( std::vector< std::tstring >::const_iterator itSubKeyName = subKeyNames.begin(); itSubKeyName != subKeyNames.end(); ++itSubKeyName )
			Add( *itSubKeyName )->Load( reg::section / *itSubKeyName );
	}

	m_currSetPos = pApp->GetProfileInt( reg::section.GetPtr(), reg::entry_currSetPos, static_cast<int>( m_currSetPos ) );
	m_currSetPos = std::min( m_currSetPos, m_includePaths.size() - 1 );

	m_searchSpecs.clear();
}

void CIncludeDirectories::Save( void ) const
{
	CWinApp* pApp = AfxGetApp();

	// delete keys no longer used
	reg::CKey key( pApp->GetSectionKey( reg::section.GetPtr() ) );

	std::vector< std::tstring > subKeyNames;
	key.QuerySubKeyNames( subKeyNames );

	for ( std::vector< std::tstring >::const_iterator itSubKeyName = subKeyNames.begin(); itSubKeyName != subKeyNames.end(); ++itSubKeyName )
		if ( NULL == utl::FindValue( m_includePaths, *itSubKeyName ) )
			key.DeleteSubKey( itSubKeyName->c_str() );

	for ( utl::vector_map< std::tstring, CIncludePaths* >::const_iterator itIncludePath = m_includePaths.begin(); itIncludePath != m_includePaths.end(); ++itIncludePath )
		itIncludePath->second->Save( reg::section / itIncludePath->first );

	pApp->WriteProfileInt( reg::section.GetPtr(), reg::entry_currSetPos, static_cast<int>( m_currSetPos ) );
}
