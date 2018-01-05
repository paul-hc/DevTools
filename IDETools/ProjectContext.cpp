
#include "stdafx.h"
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

		CFileFind finder;
		BOOL isDspFile = finder.FindFile( searchPathDSP.Get() );
		bool foundAssoc = false;
		ULONGLONG maxDspFileSize = 0;
		std::tstring targetDspFilePath;

		while ( isDspFile )
		{
			isDspFile = finder.FindNextFile();
			// If local file is specified, search for a DSP referencing it
			if ( !m_localCurrentFile.empty() )
			{
				DspParser dspParser( finder.GetFilePath() );

				if ( dspParser.findFile( m_localCurrentFile.c_str() ) )
				{	// The project contains a reference to the local file ->
					std::tstring additionalIncludePath = ExtractAdditionalIncludePath( finder.GetFilePath(), m_projectActiveConfiguration.c_str() );

					if ( !additionalIncludePath.empty() )
					{
						m_associatedProjectFile = finder.GetFilePath();
						foundAssoc = true;
						break;
					}
				}
			}

			ULONGLONG dspFileSize = finder.GetLength();

			if ( dspFileSize > maxDspFileSize )
			{
				maxDspFileSize = dspFileSize;
				targetDspFilePath = finder.GetFilePath();
			}
		}

		if ( !foundAssoc && !targetDspFilePath.empty() )
			m_associatedProjectFile = targetDspFilePath;		// heuristic criteria: choose the biggest DSP file :o)
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

std::tstring CProjectContext::ExtractAdditionalIncludePath( const TCHAR* dspFullPath, const TCHAR* activeConfiguration /*= NULL*/ )
{
	try
	{
		DspParser dspParser( dspFullPath );
		return (LPCTSTR)dspParser.GetAdditionalIncludePath( activeConfiguration );
	}
	catch ( CException* exc )
	{
		app::TraceException( *exc );
		exc->Delete();
	}
	return std::tstring();
}

void CProjectContext::OnAssociatedProjectFileChanged( void )
{
	m_projectAdditionalIncludePath.clear();
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
			m_projectAdditionalIncludePath = ExtractAdditionalIncludePath( m_associatedProjectFile.c_str(), m_projectActiveConfiguration.c_str() );
		}
	}
	OnProjectAdditionalIncludePathChanged();
}

void CProjectContext::OnProjectActiveConfigurationChanged( void )
{
}

void CProjectContext::OnProjectAdditionalIncludePathChanged( void )
{
}
