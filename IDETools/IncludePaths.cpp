
#include "stdafx.h"
#include "IdeUtilities.h"
#include "IncludePaths.h"
#include "ModuleSession.h"
#include "Application.h"
#include "utl/ContainerUtilities.h"
#include "utl/Path.h"
#include "utl/Registry.h"
#include "utl/StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const TCHAR* CIncludePaths::m_pathSep = _T(";");

CIncludePaths::CIncludePaths( void )
{
	if ( ide::isVC6() )
	{
		AddVC6RegistryPath( m_standard, _T("Include Dirs") );
		AddVC6RegistryPath( m_source, _T("Source Dirs") );
		AddVC6RegistryPath( m_library, _T("Library Dirs") );
		AddVC6RegistryPath( m_binary, _T("Path Dirs") );
	}
	else if ( ide::isVC71() )
	{
		AddVC71RegistryPath( m_standard, _T("Include Dirs") );
		AddVC71RegistryPath( m_source, _T("Source Dirs") );
		AddVC71RegistryPath( m_library, _T("Library Dirs") );
		AddVC71RegistryPath( m_binary, _T("Path Dirs") );
	}

	enum { BufferSize = 8192 };
	std::vector< TCHAR > environVariable( BufferSize );

	if ( ::GetEnvironmentVariable( _T("INCLUDE"), &environVariable.front(), BufferSize ) != 0 )
		AddIncludePath( m_standard, str::ExpandEnvironmentStrings( &environVariable.front() ).c_str() );

	if ( ::GetEnvironmentVariable( _T("LIB"), &environVariable.front(), BufferSize ) != 0 )
		AddIncludePath( m_library, str::ExpandEnvironmentStrings( &environVariable.front() ).c_str() );

	UpdateMoreAdditional();
}

CIncludePaths::~CIncludePaths()
{
}

void CIncludePaths::SetMoreAdditionalIncludePath( const std::tstring& moreAdditionalIncludePath )
{
	app::GetModuleSession().SetAdditionalIncludePath( moreAdditionalIncludePath );
	UpdateMoreAdditional();
}

void CIncludePaths::UpdateMoreAdditional( void )
{
	m_moreAdditional.clear();
	AddIncludePath( m_moreAdditional, app::GetModuleSession().GetExpandedAdditionalIncludePath().c_str() );
}

bool CIncludePaths::AddIncludePath( std::vector< std::tstring >& rPaths, const TCHAR* pIncPaths )
{
	str::Split( rPaths, pIncPaths, m_pathSep );
	utl::RemoveDuplicates< pred::EquivalentPathString >( rPaths );
	return !rPaths.empty();
}

bool CIncludePaths::AddVC6RegistryPath( std::vector< std::tstring >& rPaths, LPCTSTR pEntry )
{
	// Visual C++ 6 'Directories' registry key path
	static const TCHAR* regKeyPathVC6 = _T("HKEY_CURRENT_USER\\Software\\Microsoft\\DevStudio\\6.0\\Build System\\Components\\Platforms\\Win32 (x86)\\Directories");

	reg::CKey regKey( regKeyPathVC6, false );
	if ( regKey.IsValid() )
	{
		std::tstring path = regKey.ReadString( pEntry );
		DEBUG_LOG( _T("VC6 %s: %s\n"), pEntry, path.c_str() );
		return AddIncludePath( rPaths, path.c_str() );
	}

	TRACE( _T("# Error accessing the registry: %s\n"), regKeyPathVC6 );
	return false;
}

bool CIncludePaths::AddVC71RegistryPath( std::vector< std::tstring >& rPaths, LPCTSTR pEntry )
{
	// Visual C++ 7.1 'Directories' registry key path
	static const TCHAR* regKeyPathVC71 = _T("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\7.1\\VC\\VC_OBJECTS_PLATFORM_INFO\\Win32\\Directories");

	reg::CKey regKey( regKeyPathVC71, false );

	if ( regKey.IsValid() )
	{
		std::tstring path = regKey.ReadString( pEntry );
		const std::tstring& vc71InstallDir = GetVC71InstallDir();

		if ( !vc71InstallDir.empty() )
			str::Replace( path, _T("$(VCInstallDir)"), vc71InstallDir.c_str() );

		DEBUG_LOG( _T("VC71 %s: %s\n"), pEntry, path.c_str() );
		return AddIncludePath( rPaths, path.c_str() );
	}

	TRACE( _T("# Error accessing the registry: %s\n"), regKeyPathVC71 );
	return false;
}

const std::tstring& CIncludePaths::GetVC71InstallDir( void )
{
	static std::tstring vc71InstallDir;
	if ( vc71InstallDir.empty() )
	{
		reg::CKey regKey( _T("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\7.1"), false );
		if ( regKey.IsValid() )
		{
			vc71InstallDir = regKey.ReadString( _T("InstallDir") );
			str::Replace( vc71InstallDir, _T("Common7\\IDE\\"), _T("Vc7\\") );
		}
	}

	return vc71InstallDir;
}

const TCHAR* CIncludePaths::GetLocationTag( loc::IncludeLocation location )
{
	static const TCHAR*	locationNames[] =
	{
		_T("Include Path"),
		_T("Local Path"),
		_T("Additional Path"),
		_T("Absolute Path"),
		_T("Source Path"),
		_T("Library Path"),
		_T("Binary Path")
	};

	ASSERT( location >= 0 && COUNT_OF( locationNames ) );
	return locationNames[ location ];
}
