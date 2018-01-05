
#include "stdafx.h"
#include "DspProject.h"
#include "DspParser.h"
#include "Application.h"
#include <set>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// DspProject implementation

IMPLEMENT_DYNCREATE(DspProject, CCmdTarget)

DspProject::DspProject( void )
{
	EnableAutomation();

	// To keep the application running as long as an OLE automation
	//	object is active, the constructor calls AfxOleLockApp.
	AfxOleLockApp();
}

DspProject::~DspProject()
{
	// To terminate the application when all objects created with
	// 	with OLE automation, the destructor calls AfxOleUnlockApp.
	AfxOleUnlockApp();
}

void DspProject::OnFinalRelease( void )
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.
	CCmdTarget::OnFinalRelease();
}

void DspProject::clear( void )
{
	m_parserPtr = std::auto_ptr< DspParser >();

	m_projectFiles.clear();
	m_diskSourceFiles.clear();
	m_filesToAdd.clear();
	m_filesToRemove.clear();
}

void DspProject::lookupSourceFiles( void )
{
	filterProjectFiles();

	m_filesToAdd.clear();
	m_filesToRemove.clear();

	std::set< CString > sourceFolders;
	std::vector< PathInfoEx >::const_iterator itFile;

	// find the source folders based on the existing DSP source folders
	for ( itFile = m_projectFiles.begin(); itFile != m_projectFiles.end(); ++itFile )
		sourceFolders.insert( ( *itFile ).getDirPath() );

	CFileFind fileFinder;
	std::set< CString >::const_iterator itFolder;

	static std::vector< PathInfoEx > defaultFileFilters( 1, PathInfoEx( _T("*.*") ) );
	const std::vector< PathInfoEx >* pFileFilters = !m_sourceFileFilters.empty() ? &m_sourceFileFilters : &defaultFileFilters;

	// Find all the source files
	for ( itFolder = sourceFolders.begin(); itFolder != sourceFolders.end(); ++itFolder )
		for ( std::vector< PathInfoEx >::const_iterator itFilter = pFileFilters->begin();
			  itFilter != pFileFilters->end(); ++itFilter )
		{
			CString fileFilter = *itFolder + ( *itFilter ).fullPath;

			for ( BOOL isHit = fileFinder.FindFile( fileFilter ); isHit; )
			{
				isHit = fileFinder.FindNextFile();
				if ( !fileFinder.IsDirectory() && !fileFinder.IsDots() )
				{
					PathInfoEx foundFile( fileFinder.GetFilePath() );

					if ( DspParser::isSourceFile( foundFile ) )
						m_diskSourceFiles.push_back( foundFile );
				}
			}
		}

	// find the new source files
	for ( itFile = m_diskSourceFiles.begin(); itFile != m_diskSourceFiles.end(); ++itFile )
		if ( std::find( m_projectFiles.begin(), m_projectFiles.end(), *itFile ) == m_projectFiles.end() )
			m_filesToAdd.push_back( *itFile );

	// find the removed sources that still exist in the DSP project
	for ( itFile = m_projectFiles.begin(); itFile != m_projectFiles.end(); ++itFile )
		if ( std::find( m_diskSourceFiles.begin(), m_diskSourceFiles.end(), *itFile ) == m_diskSourceFiles.end() )
			m_filesToRemove.push_back( *itFile );
}

size_t DspProject::filterProjectFiles( void )
{
	size_t orgFileCount = m_projectFiles.size();

	std::vector< PathInfoEx >::iterator fileIt = m_projectFiles.begin();

	while ( fileIt != m_projectFiles.end() )
		if ( isSourceFileMatch( *fileIt ) )
			++fileIt;
		else
			fileIt = m_projectFiles.erase( fileIt );

	return orgFileCount - m_projectFiles.size();
}

// Compares only the extension
bool DspProject::isSourceFileMatch( const PathInfoEx& filePath ) const
{
	for ( std::vector< PathInfoEx >::const_iterator filterIt = m_sourceFileFilters.begin(); filterIt != m_sourceFileFilters.end(); ++filterIt )
		if ( path::EquivalentPtr( ( *filterIt ).name, _T("*") ) || path::EquivalentPtr( ( *filterIt ).name, filePath.name ) )
			if ( path::EquivalentPtr( ( *filterIt ).ext, _T("*") ) || path::EquivalentPtr( ( *filterIt ).ext, filePath.ext ) )
				return true;

	return false;
}

BEGIN_MESSAGE_MAP(DspProject, CCmdTarget)
	//{{AFX_MSG_MAP(DspProject)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(DspProject, CCmdTarget)
	//{{AFX_DISPATCH_MAP(DspProject)
	DISP_PROPERTY_EX(DspProject, "DspProjectFilePath", GetDspProjectFilePath, SetDspProjectFilePath, VT_BSTR)
	DISP_FUNCTION(DspProject, "GetFileFilterCount", GetFileFilterCount, VT_I4, VTS_NONE)
	DISP_FUNCTION(DspProject, "GetFileFilterAt", GetFileFilterAt, VT_BSTR, VTS_I4)
	DISP_FUNCTION(DspProject, "AddFileFilter", AddFileFilter, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION(DspProject, "ClearAllFileFilters", ClearAllFileFilters, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(DspProject, "GetProjectFileCount", GetProjectFileCount, VT_I4, VTS_NONE)
	DISP_FUNCTION(DspProject, "GetProjectFileAt", GetProjectFileAt, VT_BSTR, VTS_I4)
	DISP_FUNCTION(DspProject, "ContainsSourceFile", ContainsSourceFile, VT_BOOL, VTS_BSTR)
	DISP_FUNCTION(DspProject, "GetFilesToAddCount", GetFilesToAddCount, VT_I4, VTS_NONE)
	DISP_FUNCTION(DspProject, "GetFileToAddAt", GetFileToAddAt, VT_BSTR, VTS_I4)
	DISP_FUNCTION(DspProject, "GetFilesToRemoveCount", GetFilesToRemoveCount, VT_I4, VTS_NONE)
	DISP_FUNCTION(DspProject, "GetFileToRemoveAt", GetFileToRemoveAt, VT_BSTR, VTS_I4)
	DISP_FUNCTION(DspProject, "GetAdditionalIncludePath", GetAdditionalIncludePath, VT_BSTR, VTS_BSTR)
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IDspProject to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the
//  dispinterface in the .ODL file.

// {9C88D6AC-E3AE-43D6-AE96-500CB8B5E1A9}
static const IID IID_IDspProject =
{ 0x9c88d6ac, 0xe3ae, 0x43d6, { 0xae, 0x96, 0x50, 0xc, 0xb8, 0xb5, 0xe1, 0xa9 } };

BEGIN_INTERFACE_MAP(DspProject, CCmdTarget)
	INTERFACE_PART(DspProject, IID_IDspProject, Dispatch)
END_INTERFACE_MAP()

// {C60E380C-3DE3-4D69-9120-D76A23ECDC8D}
IMPLEMENT_OLECREATE(DspProject, "IDETools.DspProject", 0xc60e380c, 0x3de3, 0x4d69, 0x91, 0x20, 0xd7, 0x6a, 0x23, 0xec, 0xdc, 0x8d)

/////////////////////////////////////////////////////////////////////////////
// DspProject message handlers
/////////////////////////////////////////////////////////////////////////////

BSTR DspProject::GetDspProjectFilePath( void )
{
	CString dspProjectFilePath;

	if ( m_parserPtr.get() != NULL )
		dspProjectFilePath = m_parserPtr->m_dspFilePath;

	return dspProjectFilePath.AllocSysString();
}

void DspProject::SetDspProjectFilePath( LPCTSTR dspProjectFilePath )
{
	clear();

	ASSERT( !AfxGetApp()->GetProfileInt( _T("Settings\\DebugBreak"), _T("DspProject::SetDspProjectFilePath"), FALSE ) );

	try
	{
		m_parserPtr = std::auto_ptr< DspParser >( new DspParser( dspProjectFilePath ) );
		m_parserPtr->querySourceFiles( m_projectFiles );

		lookupSourceFiles();
	}
	catch ( CException* exc )
	{
		app::TraceException( *exc );
		exc->Delete();
	}
}

long DspProject::GetFileFilterCount( void )
{
	return (long)m_sourceFileFilters.size();
}

// A file filter looks like "*.*", "res\*.ico"
BSTR DspProject::GetFileFilterAt( long index )
{
	CString fileFilter;

	if ( index >= 0 && index < (long)m_sourceFileFilters.size() )
		fileFilter = m_sourceFileFilters[ index ].fullPath;

	return fileFilter.AllocSysString();
}

void DspProject::AddFileFilter( LPCTSTR sourceFileFilter )
{
	m_sourceFileFilters.push_back( PathInfoEx( sourceFileFilter ) );
}

void DspProject::ClearAllFileFilters()
{
	m_sourceFileFilters.clear();
}

long DspProject::GetProjectFileCount()
{
	return (long)m_projectFiles.size();
}

BSTR DspProject::GetProjectFileAt( long index )
{
	CString sourceFilePath;

	if ( index >= 0 && index < (long)m_projectFiles.size() )
		sourceFilePath = m_projectFiles[ index ].fullPath;

	return sourceFilePath.AllocSysString();
}

BOOL DspProject::ContainsSourceFile( LPCTSTR sourceFilePath )
{
	return std::find( m_projectFiles.begin(), m_projectFiles.end(), PathInfoEx( sourceFilePath ) ) != m_projectFiles.end();
}

BSTR DspProject::GetAdditionalIncludePath( LPCTSTR configurationName /*= _T("")*/ )
{
	CString additionalIncludePath;

	if ( m_parserPtr.get() != NULL )
		additionalIncludePath = m_parserPtr->GetAdditionalIncludePath( configurationName );
	return additionalIncludePath.AllocSysString();
}

long DspProject::GetFilesToAddCount( void )
{
	return (long)m_filesToAdd.size();
}

BSTR DspProject::GetFileToAddAt( long index )
{
	CString toAddFilePath;

	if ( index >= 0 && index < (long)m_filesToAdd.size() )
		toAddFilePath = m_filesToAdd[ index ].fullPath;

	return toAddFilePath.AllocSysString();
}

long DspProject::GetFilesToRemoveCount( void )
{
	return (long)m_filesToRemove.size();
}

BSTR DspProject::GetFileToRemoveAt( long index )
{
	CString toRemoveFilePath;

	if ( index >= 0 && index < (long)m_filesToRemove.size() )
		toRemoveFilePath = m_filesToRemove[ index ].fullPath;

	return toRemoveFilePath.AllocSysString();
}
