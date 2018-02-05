
#include "stdafx.h"
#include "IncludePaths.h"
#include "IdeUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	const TCHAR entry_Include[] = _T("Include");
	const TCHAR entry_Source[] = _T("Source");
	const TCHAR entry_Library[] = _T("Library");
	const TCHAR entry_Binary[] = _T("Binary");
}


CIncludePaths::CIncludePaths( void )
	: m_standard( inc::StandardPath )
	, m_source( inc::SourcePath )
	, m_library( inc::LibraryPath )
	, m_binary( inc::BinaryPath )
{
}

const inc::CDirPathGroup& CIncludePaths::Get_INCLUDE( void )
{
	static const inc::CDirPathGroup include( _T("INCLUDE"), inc::StandardPath );
	return include;
}

const inc::CDirPathGroup& CIncludePaths::Get_LIB( void )
{
	static const inc::CDirPathGroup include( _T("LIB"), inc::LibraryPath );
	return include;
}

const inc::CDirPathGroup& CIncludePaths::Get_PATH( void )
{
	static const inc::CDirPathGroup include( _T("PATH"), inc::BinaryPath );
	return include;
}

void CIncludePaths::Load( const TCHAR section[] )
{
	m_standard.Load( section, reg::entry_Include );
	m_source.Load( section, reg::entry_Source );
	m_library.Load( section, reg::entry_Library );
	m_binary.Load( section, reg::entry_Binary );
}

void CIncludePaths::Save( const TCHAR section[] ) const
{
	m_standard.Save( section, reg::entry_Include );
	m_source.Save( section, reg::entry_Source );
	m_library.Save( section, reg::entry_Library );
	m_binary.Save( section, reg::entry_Binary );
}

void CIncludePaths::InitFromIde( void )
{
	switch ( ide::FindIdeType() )
	{
		case ide::VC_60:
			m_standard.Store( ide::GetRegistryPath_VC6( _T("Include Dirs") ).c_str() );
			m_source.Store( ide::GetRegistryPath_VC6( _T("Source Dirs") ).c_str() );
			m_library.Store( ide::GetRegistryPath_VC6( _T("Library Dirs") ).c_str() );
			m_binary.Store( ide::GetRegistryPath_VC6( _T("Path Dirs") ).c_str() );
			break;
		case ide::VC_71to90:
			m_standard.Store( ide::GetRegistryPath_VC71( _T("Include Dirs") ).c_str() );
			m_source.Store( ide::GetRegistryPath_VC71( _T("Source Dirs") ).c_str() );
			m_library.Store( ide::GetRegistryPath_VC71( _T("Library Dirs") ).c_str() );
			m_binary.Store( ide::GetRegistryPath_VC71( _T("Path Dirs") ).c_str() );
			break;
		case ide::VC_110plus:
			break;
	}
}
