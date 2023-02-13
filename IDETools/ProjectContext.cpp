
#include "pch.h"
#include "ProjectContext.h"
#include "DspParser.h"
#include "PathInfo.h"
#include "Application.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CProjectContext::CProjectContext( void )
{
}

CProjectContext::~CProjectContext()
{
}

void CProjectContext::SetLocalDirPath( const std::tstring& localDirPath )
{
	m_localDirPath = localDirPath;
	OnLocalDirPathChanged();
}

bool CProjectContext::FindProjectFile( void )
{
	m_associatedProjectFile.clear();
	if ( !m_localDirPath.empty() || !m_localCurrentFile.empty() )
	{
		PathInfoEx searchPathDSP;

		if ( !m_localCurrentFile.empty() )
			searchPathDSP.assign( m_localCurrentFile.c_str() );
		else if ( !m_localDirPath.empty() )
			searchPathDSP.assignDirPath( m_localDirPath.c_str() );
		searchPathDSP.assignNameExt( _T("*.dsp") );
	}
	OnAssociatedProjectFileChanged();			// notify the changes
	return !m_associatedProjectFile.empty();
}

void CProjectContext::OnLocalDirPathChanged( void )
{
	if ( !m_localCurrentFile.empty() )
	{	// correlate local file (reset it)
		PathInfoEx piLocalFile( m_localCurrentFile.c_str() ), piLocalDir;

		piLocalDir.assignDirPath( m_localDirPath.c_str() );
		if ( !path::EquivalentPtr( piLocalFile.getDirPath(), piLocalDir.getDirPath() ) )
		{	// new local directory path is different than local file's directory path -> reset local file
			CScopedInternalChange internalChange( this );
			SetLocalCurrentFile( std::tstring() );
		}
	}
	if ( !IsInternalChange() )
		FindProjectFile();
}

void CProjectContext::OnLocalCurrentFileChanged( void )
{
	if ( !IsInternalChange() )
	{
		PathInfo localPath( m_localCurrentFile.c_str() );

		m_localDirPath = localPath.getDirPath( false );
		FindProjectFile();
	}
}

void CProjectContext::OnAssociatedProjectFileChanged( void )
{
	if ( !m_associatedProjectFile.empty() )
	{
		PathInfoEx projectFullPath( m_associatedProjectFile.c_str() );

		if ( projectFullPath.exist() )
		{
			PathInfoEx piLocalDir;

			piLocalDir.assignDirPath( m_localDirPath.c_str() );
			if ( !path::EquivalentPtr( projectFullPath.getDirPath(), piLocalDir.getDirPath() ) )
			{	// Correlate local dir with the dir path of the DSP file
				CScopedInternalChange internalChange( this );
				SetLocalDirPath( (LPCTSTR)projectFullPath.getDirPath( false ) );
			}
		}
	}
}

void CProjectContext::OnProjectActiveConfigurationChanged( void )
{
}
