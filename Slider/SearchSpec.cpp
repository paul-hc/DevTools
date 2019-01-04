
#include "stdafx.h"
#include "SearchSpec.h"
#include "ImageArchiveStg.h"
#include "Application.h"
#include "resource.h"
#include "utl/Serialization.h"
#include "utl/SerializeStdTypes.h"
#include "utl/StringUtilities.h"
#include "utl/UI/ShellDialogs.h"
#include "utl/UI/WicImage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


void CSearchSpec::Stream( CArchive& archive )
{
	if ( archive.IsStoring() )
	{
		archive << m_searchPath;
		archive << m_searchFilters;
		archive << m_type;
		archive << m_options;
	}
	else
	{
		archive >> m_searchPath;
		archive >> m_searchFilters;
		archive >> (int&)m_type;
		archive >> (int&)m_options;
	}
}

bool CSearchSpec::Setup( const fs::CPath& searchPath, const std::tstring& searchFilters /*= std::tstring()*/, Options options /*= RecurseSubDirs*/ )
{
	m_searchPath = searchPath;

	m_type = Invalid;
	if ( m_searchPath.FileExist() )
		if ( fs::IsValidDirectory( m_searchPath.GetPtr() ) )
			m_type = DirPath;
		else
			m_type = CImageArchiveStg::HasImageArchiveExt( m_searchPath.GetPtr() ) ? ArchiveStgFile : ExplicitFile;

	switch ( m_type )
	{
		case DirPath:
			m_searchFilters = searchFilters;
			m_options = options;
			break;
		case ArchiveStgFile:
		case ExplicitFile:
			m_searchFilters.clear();
			m_options = Normal;
			break;
	}
	return m_type != Invalid;
}

const std::tstring& CSearchSpec::GetImageSearchFilters( void ) const
{
	ASSERT( IsDirPath() ); return !m_searchFilters.empty() ? m_searchFilters : app::GetAllSourcesSpecs();
}

bool CSearchSpec::BrowseFilePath( PathType pathType /*= PT_AsIs*/, CWnd* pParentWnd /*= NULL*/, DWORD extraFlags /*= OFN_FILEMUSTEXIST*/ )
{
	std::tstring filePath = m_searchPath.Get();

	if ( PT_AsIs == pathType )
		if ( DirPath == m_type )
			pathType = PT_DirPath;
		else		// CompoundFile, ExplicitFile
			pathType = PT_FilePath;

	bool picked = false;

	if ( PT_DirPath == pathType )
		picked = shell::BrowseForFolder( filePath, pParentWnd, NULL, shell::BF_FileSystem, _T("Select the image searching folder path:") );
	else
		if ( ArchiveStgFile == m_type )
			picked = app::BrowseArchiveStgFile( filePath, pParentWnd, shell::FileOpen, extraFlags );
		else	// ExplicitFile, Invalid
			picked = shell::BrowseImageFile( filePath, shell::FileOpen, extraFlags, pParentWnd );

	return picked && Setup( filePath, m_searchFilters, m_options );
}

void CSearchSpec::EnumImageFiles( fs::IEnumerator* pEnumerator ) const
{
	if ( !IsValidPath() )
		AfxThrowFileException( CFileException::fileNotFound, -1, m_searchPath.GetPtr() );		// folder path doesn't exist

	switch ( m_type )
	{
		case DirPath:
			fs::EnumFiles( pEnumerator, m_searchPath, GetImageSearchFilters().c_str(), RecurseSubDirs == m_options ? Deep : Shallow );
			break;
		case ArchiveStgFile:
		case ExplicitFile:
			pEnumerator->AddFoundFile( m_searchPath.GetPtr() );
			break;
	}
}

bool CSearchSpec::IsNumFileName( const TCHAR* pFullPath )
{
	std::tstring fname( path::FindFilename( pFullPath ), path::FindExt( pFullPath ) );
	Range< size_t > numSeq = num::FindNumericSequence( fname );
	return numSeq.IsNonEmpty();
}

int CSearchSpec::ParseNumFileNameNumber( const TCHAR* pFullPath )
{
	std::tstring fname( path::FindFilename( pFullPath ), path::FindExt( pFullPath ) );
	Range< size_t > numSeq = num::FindNumericSequence( fname );

	int number = -1;
	if ( numSeq.IsNonEmpty() )
		num::ParseNumber( number, fname.c_str() + numSeq.m_start );
	return number;
}

std::tstring CSearchSpec::FormatNumericFilePath( const TCHAR* pFullPath, int numSeq )
{
	fs::CPathParts parts( pFullPath );
	parts.m_fname = str::Format( _T("%04d"), numSeq );
	return parts.MakePath().Get();
}
