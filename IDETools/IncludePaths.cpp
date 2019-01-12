
#include "stdafx.h"
#include "IncludePaths.h"
#include "IdeUtilities.h"
#include "Application.h"
#include "utl/EnumTags.h"

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


const TCHAR CIncludePaths::s_lineSep[] = _T("\r\n");

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

void CIncludePaths::Load( const fs::CPath& section )
{
	reg::LoadPathGroup( m_standard, section.GetPtr(), reg::entry_Include );
	reg::LoadPathGroup( m_source, section.GetPtr(), reg::entry_Source );
	reg::LoadPathGroup( m_library, section.GetPtr(), reg::entry_Library );
	reg::LoadPathGroup( m_binary, section.GetPtr(), reg::entry_Binary );
}

void CIncludePaths::Save( const fs::CPath& section ) const
{
	reg::SavePathGroup( m_standard, section.GetPtr(), reg::entry_Include );
	reg::SavePathGroup( m_source, section.GetPtr(), reg::entry_Source );
	reg::SavePathGroup( m_library, section.GetPtr(), reg::entry_Library );
	reg::SavePathGroup( m_binary, section.GetPtr(), reg::entry_Binary );
}

std::tstring CIncludePaths::FormatLines( const TCHAR lineSep[] /*= s_lineSep*/ ) const
{
	std::vector< std::tstring > lines; lines.reserve( 4 );
	lines.push_back( m_standard.Format() );
	lines.push_back( m_source.Format() );
	lines.push_back( m_library.Format() );
	lines.push_back( m_binary.Format() );
	return str::Join( lines, lineSep );
}

bool CIncludePaths::ParseLines( const std::tstring& linesSpec, const TCHAR lineSep[] /*= s_lineSep*/ )
{
	std::vector< std::tstring > lines;
	str::Split( lines, linesSpec.c_str(), lineSep );

	return
		4 == lines.size() &&
		m_standard.Parse( lines[ 0 ] ) &&
		m_source.Parse( lines[ 1 ] ) &&
		m_library.Parse( lines[ 2 ] ) &&
		m_binary.Parse( lines[ 3 ] );
}

void CIncludePaths::InitFromIde( void )
{
	switch ( ide::FindIdeType() )
	{
		case ide::VC_60:
			m_standard.Split( ide::GetRegistryPath_VC6( _T("Include Dirs") ).c_str() );
			m_source.Split( ide::GetRegistryPath_VC6( _T("Source Dirs") ).c_str() );
			m_library.Split( ide::GetRegistryPath_VC6( _T("Library Dirs") ).c_str() );
			m_binary.Split( ide::GetRegistryPath_VC6( _T("Path Dirs") ).c_str() );
			break;
		case ide::VC_71to90:
			m_standard.Split( ide::GetRegistryPath_VC71( _T("Include Dirs") ).c_str() );
			m_source.Split( ide::GetRegistryPath_VC71( _T("Source Dirs") ).c_str() );
			m_library.Split( ide::GetRegistryPath_VC71( _T("Library Dirs") ).c_str() );
			m_binary.Split( ide::GetRegistryPath_VC71( _T("Path Dirs") ).c_str() );
			break;
		case ide::VC_110plus:
			break;
	}
}
